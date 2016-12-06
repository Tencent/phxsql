/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <stack>
#include <random>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "co_routine.h"
#include "phxcoroutine.h"
#include "io_routine.h"
#include "accept_thread.h"

#include "phxcomm/phx_log.h"
#include "phxsqlproxyutil.h"
#include "phxsqlproxyconfig.h"
#include "worker_thread.h"
#include "monitor_plugin.h"

using namespace std;
using phxsql::LogError;

void co_init_curr_thread_env();

namespace phxsqlproxy {

AcceptThread::AcceptThread(PHXSqlProxyConfig * config, const std::vector<WorkerThread *> & worker_threads,
                           int listen_fd) {
    for (size_t i = 0; i < worker_threads.size(); ++i) {
        worker_threads_.push_back(worker_threads[i]);
    }
    config_ = config;
    listen_fd_ = listen_fd;
}

AcceptThread::~AcceptThread() {
}

void AcceptThread::run() {
    co_init_curr_thread_env();
    co_enable_hook_sys();

    static std::default_random_engine random_engine(time (NULL));
    static std::uniform_int_distribution<unsigned> random_range(0, worker_threads_.size() - 1);

    for (;;) {
        struct sockaddr_in addr;  //maybe sockaddr_un;
        memset(&addr, 0, sizeof(addr));
        socklen_t len = sizeof(addr);

        int fd = accept(listen_fd_, (struct sockaddr *) &addr, &len);
        if (fd < 0) {
            if (errno != EAGAIN) {
                MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->AcceptFail();
                LogError("accept failed, ret fd %d, errno (%d:%s)", fd, errno, strerror(errno));
            }
            continue;
        }

        MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->AcceptSuccess();
        SetNonBlock(fd);

        int rand_idx = random_range(random_engine);
        if (worker_threads_[rand_idx]->AssignFD(fd) > 0) {
            MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->ResumeRoutine();
        } else {
            MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->OutOfConn();
            LogError("%s:%d receive connection but no useful io routine", __func__, __LINE__);
            close(fd);
        }
    }
    co_eventloop(co_get_epoll_ct(), 0, 0);
}

}
