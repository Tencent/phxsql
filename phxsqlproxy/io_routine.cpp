/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>
#include <algorithm>
#include <string>
#include <mysql.h>
#include <cstring>
#include <cstdlib>
#include <random>

#include "co_routine.h"
#include "phxcoroutine.h"
#include "io_routine.h"

#include "phxcomm/base_config.h"
#include "phxcomm/phx_log.h"
#include "phxsqlproxyutil.h"
#include "group_status_cache.h"
#include "monitor_plugin.h"
#include "requestfilter_plugin.h"
#include "phxsqlproxyconfig.h"

using namespace std;
using phxsql::PhxBaseConfig;
using phxsql::LogError;
using phxsql::LogWarning;
using phxsql::LogVerbose;

namespace phxsqlproxy {

static std::default_random_engine random_engine(time(0));

//IORoutineMgr begin
IORoutineMgr::IORoutineMgr(PHXSqlProxyConfig * config, WorkerConfig_t * worker_config) {
    config_ = config;
    max_size_ = worker_config->io_routine_count_;
}

IORoutineMgr::~IORoutineMgr() {
}

int IORoutineMgr::GetUsedIORoutine() const {
    return max_size_ - io_routine_stack_.size();
}

int IORoutineMgr::AddIORoutine(IORoutine * io_routine_ptr) {
    MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->RecycleRoutine();
    io_routine_stack_.push(io_routine_ptr);
    return 0;
}

PHXSqlProxyConfig * IORoutineMgr::GetConfig() {
    return config_;
}

bool IORoutineMgr::IsAllIORoutineUsed() {
    return io_routine_stack_.empty();
}

int IORoutineMgr::GetIORoutine(IORoutine ** io_routine_pointer) {
    if (io_routine_stack_.empty()) {
        LogError("stack empty!");
        return -__LINE__;
    }
    MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->AllocRoutine();
    *io_routine_pointer = io_routine_stack_.top();
    io_routine_stack_.pop();
    return 0;
}

WorkerConfig_t * IORoutineMgr::GetWorkerConfig() {
    return worker_config_;
}

//IORoutineMgr end

//IORoutine begin
IORoutine::IORoutine(IORoutineMgr * routine_mgr, GroupStatusCache * group_status_cache) :
        io_routine_mgr_(routine_mgr),
        group_status_cache_(group_status_cache),
        client_fd_(-1),
        sqlsvr_fd_(-1) {
    config_ = routine_mgr->GetConfig();
    ClearVariablesAndStatus();
}

IORoutine::~IORoutine() {
    ClearAll();
}

GroupStatusCache * IORoutine::GetGroupStatusCache() {
    return group_status_cache_;
}

void IORoutine::ReleaseFD(int & fd) {
    if (fd != -1) {
        shutdown(fd, SHUT_RDWR);
        close(fd);
        fd = -1;
    }
}

void IORoutine::ClearAll() {
    if (client_fd_ != -1 || sqlsvr_fd_ != -1) {
        LogVerbose("requniqid %llu release clientfd %d svrfd %d", req_uniq_id_, client_fd_, sqlsvr_fd_);
    }

    ReleaseFD(client_fd_);
    ReleaseFD(sqlsvr_fd_);
    ClearVariablesAndStatus();
}

void IORoutine::ClearVariablesAndStatus() {
    connect_dest_ = "";
    connect_port_ = 0;
    client_ip_ = "";
    req_uniq_id_ = 0;
    last_sent_request_timestamp_ = 0;
    last_received_request_timestamp_ = 0;
    last_read_fd_ = -1;
    is_authed_ = false;
    db_name_ = "";
}

int IORoutine::ConnectDest() {
    std::string dest_ip = "";
    int dest_port = 0;

    if (!config_->GetOnlyProxy()) {
        if (GetDestEndpoint(dest_ip, dest_port) != 0) {
            LogError("requniqid %llu get mysql ip port failed", req_uniq_id_);
            return -__LINE__;
        }
    } else {
        dest_ip = "127.0.0.1";
        dest_port = config_->GetMysqlPort();
    }

    if (sqlsvr_fd_ != -1 && dest_ip == connect_dest_) {
        return sqlsvr_fd_;
    }

    MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->ConnectDest();

    int fd = socket(PF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    SetAddr(dest_ip.c_str(), dest_port, addr);
    uint64_t begin = GetTimestampMS();

    SetNonBlock(fd);

    int ret = RoutineConnectWithTimeout(fd, (struct sockaddr*) &addr, sizeof(addr), config_->ConnectTimeoutMs());
    if (ret != 0) {
        LogError("%s:%d requniqid %llu connect [%s:%d] ret %d, errno (%d:%s)", __func__, __LINE__, req_uniq_id_,
                 dest_ip.c_str(), dest_port, ret, errno, strerror(errno));

        MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->ConnectDestFail();
        close(fd);
        fd = -__LINE__;
    } else {
        LogVerbose("%s:%d requniqid %llu connect [%s:%d] ret fd %d", __func__, __LINE__, req_uniq_id_, dest_ip.c_str(),
                   dest_port, fd);
        ReleaseFD(sqlsvr_fd_);
        connect_dest_ = dest_ip;
        connect_port_ = dest_port;
    }

    if (dest_port == config_->GetMysqlPort()) {
        uint64_t cost = GetTimestampMS() - begin;
        LogVerbose("%s:%d requniqid %llu connect [%s:%d] ret fd %d cost %llu", __func__, __LINE__, req_uniq_id_,
                   dest_ip.c_str(), dest_port, fd, cost);
        MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->ConnectMysqlSvr(fd);
        MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->ConnectMysqlSvrRunTime(cost);
    }

    return fd;
//client_fd_;
}

int IORoutine::WriteToDest(int dest_fd, const char * buf, int write_size) {
    if (dest_fd == sqlsvr_fd_) {
        last_sent_request_timestamp_ = GetTimestampMS();
    }

    MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->WriteNetwork(write_size);
    //for debug
    //
    if (config_->OpenDebugMode()) {
        std::string debug_str;
        GetMysqlBufDebugString(buf, write_size, debug_str);
        LogVerbose("requniqid %llu dest %d command [%s]", req_uniq_id_, dest_fd, debug_str.c_str());
    }
    //for debug end

    int written_once = 0;
    int total_written = 0;
    while (total_written < write_size) {
        written_once = RoutineWriteWithTimeout(dest_fd, buf + total_written, write_size - total_written,
                                               config_->WriteTimeoutMs());
        if (written_once < 0) {
            LogError("requniqid %llu write to %d buf size [%d] failed, ret %d, errno (%d:%s)", req_uniq_id_, dest_fd,
                     write_size, written_once, errno, strerror(errno));
            MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->WriteNetworkFail(write_size);
            return -__LINE__;
        }
        total_written += written_once;
    }
    if (dest_fd == client_fd_) {
        uint64_t cost = GetTimestampMS() - last_received_request_timestamp_;
        MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->RequestExecuteCost(cost);
    }
    return total_written;
}

void IORoutine::SetClientFD(int fd) {

    ClearAll();
    req_uniq_id_ = ((uint64_t)(random_engine()) << 32) | random_engine();
    client_fd_ = fd;
    int ret1 = GetSockName(fd, listen_ip_, listen_port_);
    int ret2 = GetPeerName(fd, client_ip_, client_port_);
    if (ret1 == 0 && ret2 == 0) {
        LogVerbose("requniqid %llu receive connect from [%s:%d]",
                   req_uniq_id_, client_ip_.c_str(), client_port_);
    }
}

void IORoutine::GetDBNameFromAuthBuf(const char * buf, int buf_size) {
    if (buf_size < 36) {
        return;
    }

    uint32_t client_property = 0;
    memcpy(&client_property, buf + 4, 4);

    if (client_property & CLIENT_CONNECT_WITH_DB) {
        const char * offset = buf + 36;
        //skip username
        offset += strlen(offset) + 1;

        //skip authorize data
        int len_field_size = 0;
        uint64_t len = DecodedLengthBinary(offset, buf_size - (offset - buf), len_field_size);
        if (len == (uint64_t)(-1LL))
            return;
        offset += len_field_size + len;

        //this is the db's name
        if (strlen(offset)) {
            db_name_ = string(offset, strlen(offset));
        }
        LogVerbose("%s:%d requniqid %llu client property %u CLIENT_CONNECT_WITH_DB %d ret db [%s]", __func__, __LINE__,
                   req_uniq_id_, client_property, (int) CLIENT_CONNECT_WITH_DB, db_name_.c_str());
    }
}

void IORoutine::GetDBNameFromReqBuf(const char * buf, int buf_size) {
    if (buf_size > 5) {
        char cmd = buf[4];
        if (cmd == COM_INIT_DB) {
            int buf_len = 0;
            memcpy(&buf_len, buf, 3);
            db_name_ = string(buf + 5, buf_len - 1);
            LogVerbose("%s:%d requniqid %llu bufsize %d buflen %d ret [%s]", __func__, __LINE__, req_uniq_id_, buf_size,
                       buf_len, db_name_.c_str());
        }
    }
}

int IORoutine::TransMsgDirect(int source_fd, int dest_fd, struct pollfd pf[], int nfds) {
    int byte_size = 0;
    int read_once = 0;
    int written = 0;

    for (int i = 0; i < nfds; ++i) {
        char buf[1024 * 16];
        if (pf[i].fd == source_fd) {
            if (pf[i].revents & POLLIN) {
                if ((read_once = read(source_fd, buf, sizeof(buf))) <= 0) {
                    if (read_once < 0) {
                        MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->ReadNetworkFail();
                    }
                    LogError(
                            "%s:%d requniqid %llu clientfd %d svrfd %d source %d dest %d read failed, ret %d, errno (%d:%s)",
                            __func__, __LINE__, req_uniq_id_, client_fd_, sqlsvr_fd_, source_fd, dest_fd, read_once,
                            errno, strerror(errno));
                    return -1;
                }

                if (source_fd == client_fd_) {
                    if (!is_authed_) {
                        GetDBNameFromAuthBuf(buf, read_once);
                        is_authed_ = true;
                    } else {
                        GetDBNameFromReqBuf(buf, read_once);
                    }

                    if (!CanExecute(buf, read_once)) {
                        LogError("requniqid %llu request [cmd:%d] can't execute here", req_uniq_id_, buf[4]);
                        return -1;
                    }
                }

                written = WriteToDest(dest_fd, buf, read_once);
                if (written < 0) {
                    return -__LINE__;
                }

                byte_size += written;
                LogVerbose("requniqid %llu last read clientfd %d svrfd %d source %d dest %d ret %d read ret %d",
                           req_uniq_id_, client_fd_, sqlsvr_fd_, source_fd, dest_fd, read_once, byte_size);
            } else if (pf[i].revents & POLLHUP) {
                LogVerbose("requniqid %llu source %d dest %d read events POLLHUP", req_uniq_id_, source_fd, dest_fd);
            } else if (pf[i].revents & POLLERR) {
                return -__LINE__;
            }
        }
    }
    return byte_size;
}

int IORoutine::run() {
    co_enable_hook_sys();

    while (true) {
        if (client_fd_ == -1) {
            io_routine_mgr_->AddIORoutine(this);
            yield();
            continue;
        }

        int byte_size_tot = 0;
        while (true) {
            //first connect to master mysql to finish auth
            if (sqlsvr_fd_ == -1) {
                int fd = ConnectDest();
                if (fd <= 0) {
                    break;
                }
                sqlsvr_fd_ = fd;
                SetNoDelay(client_fd_);
                SetNoDelay(sqlsvr_fd_);

                if (listen_port_ == GetWorkerConfig()->port_) {
                    string proxy_line = "PROXY TCP4 " + client_ip_ + " " + listen_ip_ + " " +
                                        UIntToStr(client_port_) + " " + UIntToStr(listen_port_) + "\r\n";
                    int ret = WriteToDest(sqlsvr_fd_, proxy_line.c_str(), proxy_line.size());
                    if (ret < 0) {
                        LogError("send proxy line error: %s", proxy_line.c_str());
                        break;
                    }
                }
            }

            struct pollfd pf[2];
            int nfds = 0;
            memset(pf, 0, sizeof(pf));
            pf[0].fd = client_fd_;
            pf[0].events = (POLLIN | POLLERR | POLLHUP);
            nfds++;

            if (sqlsvr_fd_ != -1 && sqlsvr_fd_ != client_fd_) {
                pf[1].fd = sqlsvr_fd_;
                pf[1].events = (POLLIN | POLLERR | POLLHUP);
                nfds++;
            }

            uint64_t begin = GetTimestampMS();

            int return_fd_count = co_poll(co_get_epoll_ct(), pf, nfds, 1000);
            if (return_fd_count < 0) {
                LogError("requniqid %llu poll fd client_fd_ %d sqlsvr_fd_ %d failed,ret %d", req_uniq_id_, client_fd_,
                         sqlsvr_fd_, return_fd_count);
                break;
            }

            uint64_t cost_in_epoll = GetTimestampMS() - begin;
            if (cost_in_epoll < 1000 && cost_in_epoll > 30) {
                LogVerbose("%s:%d requniqid %llu epollcost2 %llu", __func__, __LINE__, req_uniq_id_, cost_in_epoll);
            }

            int byte_size = TransMsgDirect(client_fd_, sqlsvr_fd_, pf, 2);
            if (byte_size < 0) {
                break;
            }
            byte_size_tot += byte_size;

            ByteFromMysqlClient(byte_size);
            int byte_size2 = TransMsgDirect(sqlsvr_fd_, client_fd_, pf, 2);
            if (byte_size2 < 0) {
                break;
            }
            byte_size_tot += byte_size2;

            ByteFromConnectDestSvr(byte_size2);
            if (byte_size || byte_size2) {
                TimeCostMonitor(GetTimestampMS() - begin);
            }
        }

        if (byte_size_tot == 0) {
            // MasterEnableReadPort=0
            if (connect_port_ == GetWorkerConfig()->proxy_port_ && !GetWorkerConfig()->is_master_port_) {
                LogVerbose("%s:%d requniqid %llu mark %s failure", __func__, __LINE__, req_uniq_id_,
                           connect_dest_.c_str());
                GetGroupStatusCache()->MarkFailure(connect_dest_);
            }
        }

        ClearAll();
    }
    return 0;
}

void IORoutine::TimeCostMonitor(uint64_t cost) {
    MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->Epocost(cost);
}

void IORoutine::ByteFromConnectDestSvr(uint32_t byte_size) {
    if (byte_size != 0) {
        MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->ReceiveByteFromConnectDestSvr(byte_size);

        //mysql client stat
        if (connect_port_ == config_->GetMysqlPort()) {
            MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->ReceiveByteFromMysql(byte_size);
            if (last_read_fd_ != sqlsvr_fd_) {
                if (last_sent_request_timestamp_) {
                    uint64_t cost = GetTimestampMS() - last_sent_request_timestamp_;
                    LogVerbose("%s:%d requniqid %llu recieve mysql resp cost %llu", __func__, __LINE__, req_uniq_id_,
                               cost);
                    MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->MysqlQueryCost(cost);
                }
            }
        }

        if (last_read_fd_ != sqlsvr_fd_) {
            MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->DestSvrRespPacket();
            last_read_fd_ = sqlsvr_fd_;
        }

    }
}

void IORoutine::ByteFromMysqlClient(uint32_t byte_size) {
    if (byte_size != 0) {
        MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->ReceiveByteFromMysqlClient(byte_size);

        //mysql client stat
        if (connect_port_ == config_->GetMysqlPort()) {
            MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->MysqlClientConnect(byte_size);
        }

        if (last_read_fd_ != client_fd_) {
            MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->ClientReqPacket();
            last_read_fd_ = client_fd_;
        }
    }
}

bool IORoutine::CanExecute(const char * buf, int size) {
    return true;
}

//IORoutine end

MasterIORoutine::MasterIORoutine(IORoutineMgr * routine_mgr, GroupStatusCache * group_status_cache) :
        IORoutine(routine_mgr, group_status_cache) {
}

MasterIORoutine::~MasterIORoutine() {
}

int MasterIORoutine::GetDestEndpoint(std::string & dest_ip, int & dest_port) {
    int ret = 0;
    dest_ip = "";
    std::string master_ip = "";

    ret = GetGroupStatusCache()->GetMaster(master_ip);
    if (ret != 0) {
        LogError("%s:%d requniqid %llu getmaster failed, ret %d", __func__, __LINE__, req_uniq_id_, ret);
        return -__LINE__;
    }

    bool is_master = (master_ip == GetWorkerConfig()->listen_ip_);
    if (is_master) {
        dest_ip = "127.0.0.1";
        dest_port = config_->GetMysqlPort();
    } else {
        dest_ip = master_ip;
        dest_port = GetWorkerConfig()->proxy_port_;
    }

    LogVerbose("%s:%d requniqid %llu ret ip [%s] port [%d]", __func__, __LINE__, req_uniq_id_, dest_ip.c_str(),
               dest_port);
    if (dest_ip == "")
        return -__LINE__;
    return 0;
}

bool MasterIORoutine::CanExecute(const char * buf, int size) {
    string master_ip;
    int ret = GetGroupStatusCache()->GetMaster(master_ip);
    if (ret != 0) {
        LogError("%s:%d requniqid %llu getmaster failed, ret %d", __func__, __LINE__, req_uniq_id_, ret);
        return false;
    }

    bool can_execute = false;
    if (connect_dest_ == "127.0.0.1") {
        if (master_ip == GetWorkerConfig()->listen_ip_) {
            can_execute = true;
        }
    } else {
        if (master_ip == connect_dest_) {
            //allowed only if my connect destination is master
            can_execute = true;
        }
    }

    if (can_execute && connect_port_ == config_->GetMysqlPort()) {
        can_execute = RequestFilterPluginEntry::GetDefault()->GetRequestFilterPlugin()
                    ->CanExecute(true, db_name_, buf, size);
    }

    if (can_execute == false) {
        LogError("requniqid %llu find sql [%s] can't execute in this connection, port [%d]", req_uniq_id_,
                 string(buf + 5, size - 5).c_str(), GetWorkerConfig()->port_);
    }
    return can_execute;
}

WorkerConfig_t * MasterIORoutine::GetWorkerConfig() {
    return config_->GetMasterWorkerConfig();
}

SlaveIORoutine::SlaveIORoutine(IORoutineMgr * routine_mgr, GroupStatusCache * group_status_cache) :
        IORoutine(routine_mgr, group_status_cache) {
}

SlaveIORoutine::~SlaveIORoutine() {
}

int SlaveIORoutine::GetDestEndpoint(std::string & dest_ip, int & dest_port) {
    int ret = 0;
    dest_ip = "";
    std::string master_ip = "";

    ret = GetGroupStatusCache()->GetMaster(master_ip);
    if (ret != 0) {
        LogError("%s:%d requniqid %llu getmaster failed, ret %d", __func__, __LINE__, req_uniq_id_, ret);
        return -__LINE__;
    }

    if (master_ip == GetWorkerConfig()->listen_ip_) {
        if (!config_->MasterEnableReadPort()) {
            int ret = GetGroupStatusCache()->GetSlave(master_ip, dest_ip);
            if (ret != 0) {
                LogError("%s:%d requniqid %llu getslave failed, master %s", __func__, __LINE__,
                            req_uniq_id_, master_ip.c_str());
                return -__LINE__;
            }
            dest_port = GetWorkerConfig()->proxy_port_;
        } else {
            dest_ip = "127.0.0.1";
            dest_port = config_->GetMysqlPort();
        }
    } else {
        dest_ip = "127.0.0.1";
        dest_port = config_->GetMysqlPort();
    }
    LogVerbose("%s:%d requniqid %llu ret ip [%s] port [%d]", __func__, __LINE__, req_uniq_id_, dest_ip.c_str(),
               dest_port);
    return 0;
}

bool SlaveIORoutine::CanExecute(const char * buf, int size) {
    string master_ip;
    int ret = GetGroupStatusCache()->GetMaster(master_ip);
    if (ret != 0) {
        LogError("%s:%d requniqid %llu getmaster failed, ret %d", __func__, __LINE__, req_uniq_id_, ret);
        return false;
    }

    if (!config_->MasterEnableReadPort()) {
        if (master_ip == connect_dest_) {
            LogError("%s:%d requniqid %llu connected to master %s, stop query", __func__, __LINE__,
                        req_uniq_id_, master_ip.c_str());
            return false;
        }
    }

    bool can_execute = true;
    if (connect_port_ == config_->GetMysqlPort()) {
        can_execute = RequestFilterPluginEntry::GetDefault()->GetRequestFilterPlugin()
                    ->CanExecute(false, db_name_, buf, size);
    }

    if (can_execute == false) {
        LogError("requniqid %llu find sql [%s] can't execute in this connection, port [%d]", req_uniq_id_,
                 string(buf + 5, size - 5).c_str(), GetWorkerConfig()->port_);
    }
    return can_execute;
}

WorkerConfig_t * SlaveIORoutine::GetWorkerConfig() {
    return config_->GetSlaveWorkerConfig();
}

}
