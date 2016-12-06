/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include <unistd.h>
#include <random>

#include "co_routine.h"
#include "io_routine.h"

#include "phxcomm/phx_log.h"
#include "phxsqlproxyutil.h"
#include "monitor_plugin.h"

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
        phxsql::LogError("io routine stack empty!");
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

#define PRETTY_LOG(log, fmt, ...) \
    log("%s:%d requniqid %llu " fmt, __func__, __LINE__, req_uniq_id_, ## __VA_ARGS__)

#define LOG_ERR(...)   PRETTY_LOG(phxsql::LogError,   __VA_ARGS__)
#define LOG_WARN(...)  PRETTY_LOG(phxsql::LogWarning, __VA_ARGS__)
#define LOG_DEBUG(...) PRETTY_LOG(phxsql::LogVerbose, __VA_ARGS__)

//IORoutine begin
IORoutine::IORoutine(IORoutineMgr * routine_mgr) :
        req_uniq_id_(0),
        io_routine_mgr_(routine_mgr),
        io_router_(nullptr),
        io_channel_(nullptr),
        proxy_protocol_handler_(nullptr),
        client_fd_(-1),
        sqlsvr_fd_(-1) {
}

IORoutine::~IORoutine() {
    ReleaseFD(client_fd_);
    ReleaseFD(sqlsvr_fd_);
    delete io_router_;
    delete io_channel_;
    delete proxy_protocol_handler_;
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
        LOG_DEBUG("release clientfd %d svrfd %d", client_fd_, sqlsvr_fd_);
    }

    ReleaseFD(client_fd_);
    ReleaseFD(sqlsvr_fd_);
    io_router_->Clear();
    io_channel_->Clear();
    proxy_protocol_handler_->Clear();
}

void IORoutine::SetClientFD(int fd) {
    ClearAll();
    client_fd_ = fd;
    req_uniq_id_ = ((uint64_t)(random_engine()) << 32) | random_engine();
    io_router_->SetReqUniqID(req_uniq_id_);
    io_channel_->SetReqUniqID(req_uniq_id_);
    proxy_protocol_handler_->SetReqUniqID(req_uniq_id_);
    LOG_DEBUG("receive connect, client_fd [%d]", fd);
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
        do {
            SetNoDelay(client_fd_);
            int read_proxy_hdr = proxy_protocol_handler_->ProcessProxyHeader(client_fd_);
            if (read_proxy_hdr < 0) {
                LOG_ERR("process proxy protocol failed, ret %d", read_proxy_hdr);
                break;
            }
            LOG_DEBUG("process proxy protocol succ, header size %d", read_proxy_hdr);

            int fd = io_router_->ConnectDest();
            if (fd < 0) {
                LOG_ERR("connect dest failed, ret %d", fd);
                break;
            }
            LOG_DEBUG("connect dest succ, fd %d", fd);
            sqlsvr_fd_ = fd;
            SetNoDelay(sqlsvr_fd_);

            int send_proxy_hdr = proxy_protocol_handler_->SendProxyHeader(sqlsvr_fd_);
            if (send_proxy_hdr < 0) {
                LOG_ERR("send proxy protocol failed, ret %d", send_proxy_hdr);
                break;
            }
            LOG_DEBUG("send proxy protocol succ, header size %d", send_proxy_hdr);

            byte_size_tot = io_channel_->TransMsg(client_fd_, sqlsvr_fd_);
            LOG_DEBUG("trans %d bytes msg in total", byte_size_tot);
        } while (0);

        if (byte_size_tot == 0) {
            io_router_->MarkFailure();
        }

        ClearAll();
    }
    return 0;
}

//IORoutine end

MasterIORoutine::MasterIORoutine(IORoutineMgr * routine_mgr, GroupStatusCache * group_status_cache) :
        IORoutine(routine_mgr) {
    PHXSqlProxyConfig * config = routine_mgr->GetConfig();
    WorkerConfig_t * worker_config = config->GetMasterWorkerConfig();
    io_router_ = new MasterIORouter(config, worker_config, group_status_cache);
    io_channel_ = new MasterIOChannel(config, worker_config, group_status_cache);
    proxy_protocol_handler_ = new ProxyProtocolHandler(config, worker_config, group_status_cache);
}

MasterIORoutine::~MasterIORoutine() {
}

SlaveIORoutine::SlaveIORoutine(IORoutineMgr * routine_mgr, GroupStatusCache * group_status_cache) :
        IORoutine(routine_mgr) {
    PHXSqlProxyConfig * config = routine_mgr->GetConfig();
    WorkerConfig_t * worker_config = config->GetSlaveWorkerConfig();
    io_router_ = new SlaveIORouter(config, worker_config, group_status_cache);
    io_channel_ = new SlaveIOChannel(config, worker_config, group_status_cache);
    proxy_protocol_handler_ = new ProxyProtocolHandler(config, worker_config, group_status_cache);
}

SlaveIORoutine::~SlaveIORoutine() {
}

}
