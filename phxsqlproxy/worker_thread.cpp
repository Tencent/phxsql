/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "worker_thread.h"

#include "phxcomm/phx_log.h"
#include "co_routine.h"
#include "phxcoroutine.h"

void co_init_curr_thread_env();

namespace phxsqlproxy {

WorkerThread::WorkerThread(PHXSqlProxyConfig * config, WorkerConfig_t * worker_config) :
        config_(config),
        worker_config_(worker_config),
        connection_dispatcher_(NULL),
        monitor_routine_(NULL),
        io_routine_mgr_(NULL) {
}

WorkerThread::~WorkerThread() {
    delete connection_dispatcher_, connection_dispatcher_ = NULL;
    delete io_routine_mgr_, io_routine_mgr_ = NULL;
    delete monitor_routine_, monitor_routine_ = NULL;
    for (size_t i = 0; i < io_routines_.size(); ++i) {
        delete io_routines_[i], io_routines_[i] = NULL;
    }
}
static int TickFunc(void* arg) {
//	phoenix::LogErr("%s:%d Tick", __func__, __LINE__);
    return 0;
}

void WorkerThread::run() {
    co_init_curr_thread_env();
    co_enable_hook_sys();
    monitor_routine_->start();
    for (size_t i = 0; i < io_routines_.size(); ++i) {
        io_routines_[i]->start();
    }
    connection_dispatcher_->start();
    co_eventloop(co_get_epoll_ct(), TickFunc, 0);
}

int WorkerThread::AssignFD(int fd) {
    return connection_dispatcher_->AssignFD(fd);
}

}
