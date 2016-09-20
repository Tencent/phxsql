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
#include "master_cache.h"
#include "membership_cache.h"
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
IORoutine::IORoutine(IORoutineMgr * routine_mgr, MasterCache * master_cache, MembershipCache * membership_cache) :
        io_routine_mgr_(routine_mgr),
        master_cache_(master_cache),
        membership_cache_(membership_cache),
        client_fd_(-1),
        sqlsvr_fd_(-1) {
    config_ = routine_mgr->GetConfig();
    ClearVariablesAndStatus();
}

IORoutine::~IORoutine() {
    ClearAll();
}

MasterCache * IORoutine::GetMasterCache() {
    return master_cache_;
}

MembershipCache * IORoutine::GetMembershipCache() {
    return membership_cache_;
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

/**
 * @function : connect to destination
*/
int IORoutine::ConnectDest() {
    std::string dest_ip = "";
    int dest_port = 0;

    if (!config_->GetOnlyProxy()) { // obtain only_proxy by GetOnlyProxy(), and its default value is 0
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

    int ret = RoutineConnectWithTimeout(fd, (struct sockaddr*) &addr, sizeof(addr), 200);
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
        written_once = RoutineWriteWithTimeout(dest_fd, buf + total_written, write_size - total_written, 1000);
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
    int port = 0;
    int ret = GetPeerName(fd, client_ip_, port);
    if (ret == 0) {
        LogVerbose("requniqid %llu receive connect from [%s:%d]", req_uniq_id_, client_ip_.c_str(), port);
    }
}

int IORoutine::FakeClientIPInAuthBuf(char * buf, size_t buf_len) {
    if (buf_len < 36) {
        LogError("requniqid %llu %s failed in %d", req_uniq_id_, __func__, __LINE__);
        return -__LINE__;
    }
    char * reserverd_begin = buf + 14;
    char ip_buf[INET6_ADDRSTRLEN] = { 0 };

    uint32_t ip = 0;
    memcpy(&ip, reserverd_begin, sizeof(uint32_t));
    if (ip) {
        if (inet_ntop(AF_INET, reserverd_begin, ip_buf, INET6_ADDRSTRLEN) != NULL) {
            const std::vector<std::string> & ip_list = GetMembershipCache()->GetMembership();
            bool found = false;
            for (auto itr : ip_list) {
                if (itr == client_ip_) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                LogError("requniqid %llu receive fake ip package from [%s], fake ip [%s]", req_uniq_id_,
                         client_ip_.c_str(), ip_buf);
                return -__LINE__;
            }
        } else
            return -__LINE__;
    } else {
        int ret = inet_pton(AF_INET, client_ip_.c_str(), reserverd_begin);
        if (ret <= 0) {
            LogError("%s:%d requniqid %llu ret %d errno (%d:%s)", __func__, __LINE__, req_uniq_id_, ret, errno,
                     strerror(errno));
            return -__LINE__;
        }
    }
    return 0;
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

                        if (FakeClientIPInAuthBuf(buf, read_once) != 0) {
                            LogError("requniqid %llu FakeClientIPInAuthBuf failed", req_uniq_id_);
                            return -1;
                        }
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

        while (true) {
            //first connect to master mysql to finish auth
            if (sqlsvr_fd_ == -1) {
                int fd = ConnectDest();
                if (fd <= 0) {
                    ClearAll();
                    break;
                }
                sqlsvr_fd_ = fd;
                SetNoDelay(client_fd_);
                SetNoDelay(sqlsvr_fd_);
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
                ClearAll();
                break;
            }

            uint64_t cost_in_epoll = GetTimestampMS() - begin;
            if (cost_in_epoll < 1000 && cost_in_epoll > 30) {
                LogVerbose("%s:%d requniqid %llu epollcost2 %llu", __func__, __LINE__, req_uniq_id_, cost_in_epoll);
            }

            int byte_size = TransMsgDirect(client_fd_, sqlsvr_fd_, pf, 2);
            if (byte_size < 0) {
                if (byte_size == -364) {
                    LogError("%s:%d %llu svr fd closed", __func__, __LINE__, req_uniq_id_);
                }
                ClearAll();
                break;
            }

            ByteFromMysqlClient(byte_size);
            int byte_size2 = TransMsgDirect(sqlsvr_fd_, client_fd_, pf, 2);
            if (byte_size2 < 0) {
                if (byte_size2 == -364) {
                    LogError("%s:%d %llu svr fd closed", __func__, __LINE__, req_uniq_id_);
                }
                ClearAll();
                break;
            }

            ByteFromConnectDestSvr(byte_size2);
            if (byte_size || byte_size2) {
                TimeCostMonitor(GetTimestampMS() - begin);
            }
        }
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

MasterIORoutine::MasterIORoutine(IORoutineMgr * routine_mgr, MasterCache * master_cache,
                                 MembershipCache * membership_cache) :
        IORoutine(routine_mgr, master_cache, membership_cache) {
}

MasterIORoutine::~MasterIORoutine() {
}

/**
 * @function : GetDestEndpoint
 * @params : dest_ip            IP address of destination
 * @params : dest_port          port number of destination
*/
int MasterIORoutine::GetDestEndpoint(std::string & dest_ip, int & dest_port) {
    int ret = 0;
    dest_ip = "";
    std::string master_ip = "";

    uint64_t expired_time = 0;
    ret = GetMasterCache()->GetMaster(master_ip, expired_time);                 // get master ip
    if (ret != 0) {
        LogError("%s:%d requniqid %llu getmaster failed, ret %d", __func__, __LINE__, req_uniq_id_, ret);
        return -__LINE__;
    }

    bool is_master = (master_ip == string(GetWorkerConfig()->listen_ip_));      // compare master_ip with listen_ip_
    if (is_master) {                                                            // if master_ip is same with listen_ip_
        dest_ip = "127.0.0.1";                                                  
        dest_port = config_->GetMysqlPort();                                    // assign 11111 to dest_port
    } else {                                                                    // if not
        dest_ip = master_ip;                                                    // assign master_ip to dest_ip
        dest_port = GetWorkerConfig()->port_;                                   // assign 54321 to dest_port
    }

    LogVerbose("%s:%d requniqid %llu ret ip [%s] port [%d]", __func__, __LINE__, req_uniq_id_, dest_ip.c_str(),
               dest_port);
    if (dest_ip == "")
        return -__LINE__;
    return 0;
}

bool MasterIORoutine::CanExecute(const char * buf, int size) {
    string master_ip;
    uint64_t expired_time = 0;
    int ret = GetMasterCache()->GetMaster(master_ip, expired_time);
    if (ret != 0) {
        LogError("%s:%d requniqid %llu getmaster failed, ret %d", __func__, __LINE__, req_uniq_id_, ret);
        return false;
    }

    bool can_execute = false;
    if (connect_dest_ == "127.0.0.1") {
        if (master_ip == PhxBaseConfig::GetInnerIP()) {
            can_execute = true;
        }
    } else {
        if (master_ip == connect_dest_) {
            //allowed only if my connect destination is master
            can_execute = true;
        }
    }

    if (can_execute && connect_port_ == config_->GetMysqlPort()) {
        can_execute = RequestFilterPluginEntry::GetDefault()->GetRequestFilterPlugin()->CanExecute(true, db_name_, buf,
                                                                                                   size);
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

SlaveIORoutine::SlaveIORoutine(IORoutineMgr * routine_mgr, MasterCache * master_cache,
                               MembershipCache * membership_cache) :
        IORoutine(routine_mgr, master_cache, membership_cache) {
}

SlaveIORoutine::~SlaveIORoutine() {
}

int SlaveIORoutine::GetDestEndpoint(std::string & dest_ip, int & dest_port) {
    int ret = 0;
    dest_ip = "";
    std::string master_ip = "";

    uint64_t expired_time = 0;
    ret = GetMasterCache()->GetMaster(master_ip, expired_time);
    if (ret != 0) {
        LogError("%s:%d requniqid %llu getmaster failed, ret %d", __func__, __LINE__, req_uniq_id_, ret);
        return -__LINE__;
    }

    if (master_ip == std::string(GetWorkerConfig()->listen_ip_)) {
        if (!config_->MasterEnableReadPort()) {
            const std::vector<std::string> & ip_list = GetMembershipCache()->GetMembership();
            for (size_t i = 0; i < ip_list.size(); ++i) {
                if (ip_list[i] == std::string(GetWorkerConfig()->listen_ip_)) {
                    dest_ip = ip_list[(i + 1) % ip_list.size()];
                    dest_port = GetWorkerConfig()->port_;
                    break;
                }
            }
        } else
            dest_port = config_->GetMysqlPort();
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
    uint64_t expired_time = 0;
    int ret = GetMasterCache()->GetMaster(master_ip, expired_time);
    if (ret != 0) {
        LogError("%s:%d requniqid %llu getmaster failed, ret %d", __func__, __LINE__, req_uniq_id_, ret);
        return false;
    }

    if (!config_->MasterEnableReadPort()) {
        if (master_ip == connect_dest_) {
            //if I connected to a master now, just stop query
            return false;
        }
    }

    bool can_execute = true;
    if (connect_port_ == config_->GetMysqlPort()) {
        can_execute = RequestFilterPluginEntry::GetDefault()->GetRequestFilterPlugin()->CanExecute(false, db_name_, buf,
                                                                                                   size);
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

