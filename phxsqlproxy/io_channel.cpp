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
#include <sys/poll.h>
#include <unistd.h>
#include <string.h>
#include <mysql.h>

#include "io_channel.h"

#include "phxcomm/phx_log.h"
#include "phxsqlproxyutil.h"
#include "co_routine.h"
#include "routineutil.h"
#include "monitor_plugin.h"
#include "requestfilter_plugin.h"

namespace phxsqlproxy {

#define PRETTY_LOG(log, fmt, ...) \
    log("%s:%d requniqid %llu " fmt, __func__, __LINE__, req_uniq_id_, ## __VA_ARGS__)

#define LOG_ERR(...)   PRETTY_LOG(phxsql::LogError,   __VA_ARGS__)
#define LOG_WARN(...)  PRETTY_LOG(phxsql::LogWarning, __VA_ARGS__)
#define LOG_DEBUG(...) PRETTY_LOG(phxsql::LogVerbose, __VA_ARGS__)

IOChannel::IOChannel(PHXSqlProxyConfig * config,
                     WorkerConfig_t    * worker_config,
                     GroupStatusCache  * group_status_cache) :
        config_(config),
        worker_config_(worker_config),
        group_status_cache_(group_status_cache) {
    Clear();
}

IOChannel::~IOChannel() {
}

void IOChannel::SetReqUniqID(uint64_t req_uniq_id) {
    req_uniq_id_ = req_uniq_id;
}

void IOChannel::Clear() {
    req_uniq_id_ = 0;
    client_fd_ = -1;
    sqlsvr_fd_ = -1;
    connect_dest_ = "";
    connect_port_ = 0;
    is_authed_ = false;
    db_name_ = "";
    last_read_fd_ = -1;
    last_received_request_timestamp_  = 0;
    client_ip_ = "";
}

int IOChannel::TransMsg(int client_fd, int sqlsvr_fd) {
    client_fd_ = client_fd;
    sqlsvr_fd_ = sqlsvr_fd;

    int ret = GetPeerName(sqlsvr_fd, connect_dest_, connect_port_);
    if (ret != 0) {
        LOG_ERR("get dest ip port failed, ret %d sqlsvr_fd %d", ret, sqlsvr_fd);
        return 0;
    }

    // get client_ip_ for FakeClientIPInAuthBuf, forward compatibility
    int t_port;
    ret = GetPeerName(client_fd, client_ip_, t_port);
    if (ret != 0) {
        LOG_ERR("get client ip failed, ret %d client_fd %d", ret, client_fd);
        return 0;
    }

    int byte_size_tot = 0;
    for (;;) {
        struct pollfd pf[2];
        memset(pf, 0, sizeof(pf));
        pf[0].fd = client_fd;
        pf[0].events = (POLLIN | POLLERR | POLLHUP);
        pf[1].fd = sqlsvr_fd;
        pf[1].events = (POLLIN | POLLERR | POLLHUP);

        uint64_t begin = GetTimestampMS();

        int return_fd_count = co_poll(co_get_epoll_ct(), pf, 2, 1000);
        if (return_fd_count < 0) {
            LOG_ERR("poll fd client_fd %d sqlsvr_fd %d failed,ret %d", client_fd, sqlsvr_fd, return_fd_count);
            break;
        }

        uint64_t cost_in_epoll = GetTimestampMS() - begin;
        if (cost_in_epoll < 1000 && cost_in_epoll > 30) {
            LOG_DEBUG("epollcost2 %llu", cost_in_epoll);
        }

        int byte_size = TransMsgDirect(client_fd, sqlsvr_fd, pf, 2);
        if (byte_size < 0) {
            break;
        }
        byte_size_tot += byte_size;
        ByteFromMysqlClient(byte_size);

        int byte_size2 = TransMsgDirect(sqlsvr_fd, client_fd, pf, 2);
        if (byte_size2 < 0) {
            break;
        }
        byte_size_tot += byte_size2;
        ByteFromConnectDestSvr(byte_size2);

        if (byte_size || byte_size2) {
            MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->Epocost(GetTimestampMS() - begin);
        }
    }
    return byte_size_tot;
}

int IOChannel::WriteToDest(int dest_fd, const char * buf, int write_size) {
    MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->WriteNetwork(write_size);
    //for debug
    //
    if (config_->OpenDebugMode()) {
        std::string debug_str;
        GetMysqlBufDebugString(buf, write_size, debug_str);
        LOG_DEBUG("dest %d command [%s]", dest_fd, debug_str.c_str());
    }
    //for debug end

    int written_once = 0;
    int total_written = 0;
    while (total_written < write_size) {
        written_once = RoutineWriteWithTimeout(dest_fd, buf + total_written, write_size - total_written,
                                               config_->WriteTimeoutMs());
        if (written_once < 0) {
            LOG_ERR("write to %d buf size [%d] failed, ret %d, errno (%d:%s)",
                    dest_fd, write_size, written_once, errno, strerror(errno));
            MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->WriteNetworkFail(write_size);
            return -__LINE__;
        }
        total_written += written_once;
    }
    return total_written;
}

int IOChannel::TransMsgDirect(int source_fd, int dest_fd, struct pollfd pf[], int nfds) {
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
                    LOG_ERR("clientfd %d svrfd %d source %d dest %d read failed, ret %d, errno (%d:%s)",
                            client_fd_, sqlsvr_fd_, source_fd, dest_fd, read_once, errno, strerror(errno));
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
                        LOG_ERR("request [cmd:%d] can't execute here", (int) buf[4]);
                        return -1;
                    }
                }

                written = WriteToDest(dest_fd, buf, read_once);
                if (written < 0) {
                    return -__LINE__;
                }

                byte_size += written;
                //LOG_DEBUG("last read clientfd %d svrfd %d source %d dest %d ret %d read ret %d",
                //          client_fd_, sqlsvr_fd_, source_fd, dest_fd, read_once, byte_size);
            } else if (pf[i].revents & POLLHUP) {
                LOG_DEBUG("source %d dest %d read events POLLHUP", source_fd, dest_fd);
            } else if (pf[i].revents & POLLERR) {
                return -__LINE__;
            }
        }
    }
    return byte_size;
}

int IOChannel::FakeClientIPInAuthBuf(char * buf, size_t buf_len) {
    if (buf_len < 36) {
        LOG_ERR("buf len less than 36");
        return -__LINE__;
    }
    char * reserverd_begin = buf + 14;
    char ip_buf[INET6_ADDRSTRLEN] = { 0 };

    uint32_t ip = 0;
    memcpy(&ip, reserverd_begin, sizeof(uint32_t));
    if (ip) {
        if (inet_ntop(AF_INET, reserverd_begin, ip_buf, INET6_ADDRSTRLEN) != NULL) {
            if (!group_status_cache_->IsMember(client_ip_)) {
                LOG_ERR("receive fake ip package from [%s], fake ip [%s]", client_ip_.c_str(), ip_buf);
                return -__LINE__;
            }
        } else
            return -__LINE__;
    } else {
        int ret = inet_pton(AF_INET, client_ip_.c_str(), reserverd_begin);
        if (ret <= 0) {
            LOG_ERR("ret %d errno (%d:%s)", ret, errno, strerror(errno));
            return -__LINE__;
        }
    }
    return 0;
}

void IOChannel::GetDBNameFromAuthBuf(const char * buf, int buf_size) {
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
            db_name_ = std::string(offset, strlen(offset));
        }
        LOG_DEBUG("client property %u CLIENT_CONNECT_WITH_DB %d ret db [%s]",
                  client_property, (int) CLIENT_CONNECT_WITH_DB, db_name_.c_str());
    }
}

void IOChannel::GetDBNameFromReqBuf(const char * buf, int buf_size) {
    if (buf_size > 5) {
        char cmd = buf[4];
        if (cmd == COM_INIT_DB) {
            int buf_len = 0;
            memcpy(&buf_len, buf, 3);
            db_name_ = std::string(buf + 5, buf_len - 1);
            LOG_DEBUG("bufsize %d buflen %d ret [%s]", buf_size, buf_len, db_name_.c_str());
        }
    }
}

void IOChannel::ByteFromConnectDestSvr(uint32_t byte_size) {
    if (byte_size != 0) {
        MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->ReceiveByteFromConnectDestSvr(byte_size);

        //mysql client stat
        if (connect_port_ == config_->GetMysqlPort()) {
            MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->ReceiveByteFromMysql(byte_size);
        }

        if (last_read_fd_ != sqlsvr_fd_) {
            MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->DestSvrRespPacket();
            last_read_fd_ = sqlsvr_fd_;

            if (last_received_request_timestamp_) {
                uint64_t cost = GetTimestampMS() - last_received_request_timestamp_;
                if (connect_port_ == config_->GetMysqlPort()) {
                    //LOG_DEBUG("recieve mysql resp cost %llu", cost);
                    MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->MysqlQueryCost(cost);
                }
                //LOG_DEBUG("request exec cost %llu", cost);
                MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->RequestExecuteCost(cost);
            }
        }
    }
}

void IOChannel::ByteFromMysqlClient(uint32_t byte_size) {
    if (byte_size != 0) {
        MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->ReceiveByteFromMysqlClient(byte_size);

        //mysql client stat
        if (connect_port_ == config_->GetMysqlPort()) {
            MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->MysqlClientConnect(byte_size);
        }

        if (last_read_fd_ != client_fd_) {
            MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->ClientReqPacket();
            last_read_fd_ = client_fd_;
            last_received_request_timestamp_ = GetTimestampMS();
        }
    }
}

MasterIOChannel::MasterIOChannel(PHXSqlProxyConfig * config,
                                 WorkerConfig_t    * worker_config,
                                 GroupStatusCache  * group_status_cache) :
        IOChannel(config, worker_config, group_status_cache) {
}

MasterIOChannel::~MasterIOChannel() {
}

bool MasterIOChannel::CanExecute(const char * buf, int size) {
    std::string master_ip;
    int ret = group_status_cache_->GetMaster(master_ip);
    if (ret != 0) {
        LOG_ERR("getmaster failed, ret %d", ret);
        return false;
    }

    bool can_execute = false;
    if (connect_dest_ == "127.0.0.1") {
        if (master_ip == worker_config_->listen_ip_) {
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
        LOG_ERR("find sql [%s] can't execute in this connection, port [%d]",
                std::string(buf + 5, size - 5).c_str(), worker_config_->port_);
    }
    return can_execute;
}

SlaveIOChannel::SlaveIOChannel(PHXSqlProxyConfig * config,
                               WorkerConfig_t    * worker_config,
                               GroupStatusCache  * group_status_cache) :
        IOChannel(config, worker_config, group_status_cache) {
}

SlaveIOChannel::~SlaveIOChannel() {
}

bool SlaveIOChannel::CanExecute(const char * buf, int size) {
    std::string master_ip;
    int ret = group_status_cache_->GetMaster(master_ip);
    if (ret != 0) {
        LOG_ERR("getmaster failed, ret %d", ret);
        return false;
    }

    if (!config_->MasterEnableReadPort()) {
        if (master_ip == connect_dest_) {
            LOG_ERR("connected to master %s, stop query", master_ip.c_str());
            return false;
        }
    }

    bool can_execute = true;
    if (connect_port_ == config_->GetMysqlPort()) {
        can_execute = RequestFilterPluginEntry::GetDefault()->GetRequestFilterPlugin()
                    ->CanExecute(false, db_name_, buf, size);
    }

    if (can_execute == false) {
        LOG_ERR("find sql [%s] can't execute in this connection, port [%d]",
                std::string(buf + 5, size - 5).c_str(), worker_config_->port_);
    }
    return can_execute;
}

}
