/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include <vector>

#include "co_routine.h"
#include "phxcoroutine.h"
#include "phxthread.h"
#include "phxsqlproxyconfig.h"
#include "connection_dispatcher.h"
#include "io_routine.h"
#include "group_status_cache.h"
#include "monitor_routine.h"

namespace phxsqlproxy {

class PHXSqlProxyConfig;
typedef struct tagWorkerConfig WorkerConfig_t;
class IORoutine;

class WorkerThread : public PhxThread {

 public:
    WorkerThread(PHXSqlProxyConfig * config, WorkerConfig_t * worker_config);

    ~WorkerThread();

    int AssignFD(int fd);

    template<class T>
    void InitRoutines(GroupStatusCache * group_status_cache) {
        io_routine_mgr_ = new IORoutineMgr(config_, worker_config_);

        connection_dispatcher_ = new ConnectionDispatcher(io_routine_mgr_);

        int routine_count = worker_config_->io_routine_count_;
        for (int i = 0; i < routine_count; ++i) {
            T * io_routine = new T(io_routine_mgr_, group_status_cache);
            io_routines_.push_back(io_routine);
        }

        monitor_routine_ = new MonitorRoutine(config_, io_routine_mgr_);
    }

    void run();

 private:
    PHXSqlProxyConfig * config_;

    WorkerConfig_t * worker_config_;

    ConnectionDispatcher * connection_dispatcher_;

    MonitorRoutine * monitor_routine_;

    IORoutineMgr * io_routine_mgr_;
    std::vector<IORoutine *> io_routines_;

};

}
