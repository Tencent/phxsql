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
#include <unistd.h>
#include <string.h>

#include "io_router.h"

#include "phxcomm/phx_log.h"
#include "phxsqlproxyutil.h"
#include "routineutil.h"
#include "monitor_plugin.h"

namespace phxsqlproxy {

#define PRETTY_LOG(log, fmt, ...) \
    log("%s:%d requniqid %llu " fmt, __func__, __LINE__, req_uniq_id_, ## __VA_ARGS__)

#define LOG_ERR(...)   PRETTY_LOG(phxsql::LogError,   __VA_ARGS__)
#define LOG_WARN(...)  PRETTY_LOG(phxsql::LogWarning, __VA_ARGS__)
#define LOG_DEBUG(...) PRETTY_LOG(phxsql::LogVerbose, __VA_ARGS__)

IORouter::IORouter(PHXSqlProxyConfig * config,
                   WorkerConfig_t    * worker_config,
                   GroupStatusCache  * group_status_cache) :
        config_(config),
        worker_config_(worker_config),
        group_status_cache_(group_status_cache) {
    Clear();
}

IORouter::~IORouter() {
}

void IORouter::SetReqUniqID(uint64_t req_uniq_id) {
    req_uniq_id_ = req_uniq_id;
}

void IORouter::Clear() {
    req_uniq_id_ = 0;
    connect_dest_ = "";
    connect_port_ = 0;
}

int IORouter::ConnectDest() {
    std::string dest_ip = "";
    int dest_port = 0;

    if (!config_->GetOnlyProxy()) {
        if (GetDestEndpoint(dest_ip, dest_port) != 0) {
            LOG_ERR("get mysql ip port failed");
            return -__LINE__;
        }
    } else {
        dest_ip = "127.0.0.1";
        dest_port = config_->GetMysqlPort();
    }

    MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->ConnectDest();

    int fd = socket(PF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    if (SetAddr(dest_ip.c_str(), dest_port, addr) != 0) {
        return -__LINE__;
    }
    uint64_t begin = GetTimestampMS();

    SetNonBlock(fd);

    int ret = RoutineConnectWithTimeout(fd, (struct sockaddr*) &addr, sizeof(addr), config_->ConnectTimeoutMs());
    if (ret != 0) {
        LOG_ERR("connect [%s:%d] ret %d, errno (%d:%s)", dest_ip.c_str(), dest_port, ret, errno, strerror(errno));
        MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->ConnectDestFail();
        close(fd);
        fd =  -__LINE__;
    } else {
        LOG_DEBUG("connect [%s:%d] ret fd %d", dest_ip.c_str(), dest_port, fd);
        connect_dest_ = dest_ip;
        connect_port_ = dest_port;
    }

    if (dest_port == config_->GetMysqlPort()) {
        uint64_t cost = GetTimestampMS() - begin;
        LOG_DEBUG("connect [%s:%d] ret fd %d cost %llu", dest_ip.c_str(), dest_port, fd, cost);
        MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->ConnectMysqlSvr(fd);
        MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->ConnectMysqlSvrRunTime(cost);
    }

    return fd;
}

void IORouter::MarkFailure() {
}

MasterIORouter::MasterIORouter(PHXSqlProxyConfig * config,
                               WorkerConfig_t    * worker_config,
                               GroupStatusCache  * group_status_cache) :
        IORouter(config, worker_config, group_status_cache) {
}

MasterIORouter::~MasterIORouter() {
}

int MasterIORouter::GetDestEndpoint(std::string & dest_ip, int & dest_port) {
    int ret = 0;
    dest_ip = "";
    std::string master_ip = "";

    ret = group_status_cache_->GetMaster(master_ip);
    if (ret != 0) {
        LOG_ERR("getmaster failed, ret %d", ret);
        return -__LINE__;
    }

    bool is_master = (master_ip == worker_config_->listen_ip_);
    if (is_master) {
        dest_ip = "127.0.0.1";
        dest_port = config_->GetMysqlPort();
    } else {
        dest_ip = master_ip;
        dest_port = config_->ProxyProtocol() ? worker_config_->proxy_port_ : worker_config_->port_;
    }

    LOG_DEBUG("dest ip [%s] port [%d]", dest_ip.c_str(), dest_port);
    return 0;
}

SlaveIORouter::SlaveIORouter(PHXSqlProxyConfig * config,
                             WorkerConfig_t    * worker_config,
                             GroupStatusCache  * group_status_cache) :
        IORouter(config, worker_config, group_status_cache) {
}

SlaveIORouter::~SlaveIORouter() {
}

void SlaveIORouter::MarkFailure() {
    if (connect_dest_ != "" && connect_dest_ != "127.0.0.1") {
        LOG_DEBUG("mark %s failure", connect_dest_.c_str());
        group_status_cache_->MarkFailure(connect_dest_);
    }
}

int SlaveIORouter::GetDestEndpoint(std::string & dest_ip, int & dest_port) {
    int ret = 0;
    dest_ip = "";
    std::string master_ip = "";

    ret = group_status_cache_->GetMaster(master_ip);
    if (ret != 0) {
        LOG_ERR("getmaster failed, ret %d", ret);
        return -__LINE__;
    }

    bool is_master = (master_ip == worker_config_->listen_ip_);
    if (is_master && !config_->MasterEnableReadPort()) {
        int ret = group_status_cache_->GetSlave(master_ip, dest_ip);
        if (ret != 0) {
            LOG_ERR("getslave failed, master %s", master_ip.c_str());
            return -__LINE__;
        }
        dest_port = config_->ProxyProtocol() ? worker_config_->proxy_port_ : worker_config_->port_;
    } else {
        dest_ip = "127.0.0.1";
        dest_port = config_->GetMysqlPort();
    }

    LOG_DEBUG("dest ip [%s] port [%d]", dest_ip.c_str(), dest_port);
    return 0;
}

}
