/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include <stack>

#include "phxcoroutine.h"

#include "phxsqlproxyconfig.h"
#include "io_router.h"
#include "io_channel.h"
#include "proxy_protocol_handler.h"

namespace phxsqlproxy {

#define MAX_ACTIVE_FD_PER_ROUTINE 1024

class IORoutineMgr;

class IORoutine : public Coroutine {
 protected:
    IORoutine(IORoutineMgr * routine_mgr);

 public:
    virtual ~IORoutine();

    void SetClientFD(int fd);

 private:
    int run();

    void ClearAll();

    void ReleaseFD(int & fd);

 protected:
    uint64_t req_uniq_id_;

    IORoutineMgr         * io_routine_mgr_;
    IORouter             * io_router_;
    IOChannel            * io_channel_;
    ProxyProtocolHandler * proxy_protocol_handler_;

    int client_fd_;
    int sqlsvr_fd_;
};

class MasterIORoutine : public IORoutine {
 public:
    MasterIORoutine(IORoutineMgr * routine_mgr, GroupStatusCache * group_status_cache);

    ~MasterIORoutine() override;
};

class SlaveIORoutine : public IORoutine {
 public:
    SlaveIORoutine(IORoutineMgr * routine_mgr, GroupStatusCache * group_status_cache);

    ~SlaveIORoutine() override;
};

class IORoutineMgr {

 public:
    IORoutineMgr(PHXSqlProxyConfig * config, WorkerConfig_t * worker_config);

    PHXSqlProxyConfig * GetConfig();

    virtual ~IORoutineMgr();

    int AddIORoutine(IORoutine * io_routine_ptr);

    int GetIORoutine(IORoutine ** io_routine_pointer);

    int GetUsedIORoutine() const;

    bool IsAllIORoutineUsed();

    WorkerConfig_t * GetWorkerConfig();

 private:
    std::stack<IORoutine *> io_routine_stack_;

    PHXSqlProxyConfig * config_;

    WorkerConfig_t * worker_config_;

    uint32_t max_size_;
};

}
