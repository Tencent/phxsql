/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <errno.h>
#include <stack>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <signal.h>

#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxcomm/phx_log.h"

#include "co_routine.h"
#include "heartbeat_thread.h"
#include "accept_thread.h"
#include "worker_thread.h"
#include "monitor_routine.h"
#include "group_status_cache.h"
#include "io_routine.h"
#include "phxcoroutine.h"
#include "phxsqlproxyconfig.h"
#include "phxsqlproxyutil.h"
#include "monitor_plugin.h"
#include "requestfilter_plugin.h"
#include "freqctrlconfig.h"
#include "freqfilter_plugin.h"

using namespace phxsqlproxy;
using namespace std;
using namespace phxbinlog;
using namespace phxsql;

void co_init_curr_thread_env();

void SignalStop(int signal_value) {
    // LogError("receive signal %d", signal_value);
}

void SignalHandler(void) {
    assert(signal(SIGPIPE, SIG_IGN) != SIG_ERR);
    assert(signal(SIGALRM, SIG_IGN) != SIG_ERR);
    assert(signal(SIGCHLD, SIG_IGN) != SIG_ERR);
    assert(sigset(SIGINT, SignalStop) != SIG_ERR);
    assert(sigset(SIGTERM, SignalStop) != SIG_ERR);
    assert(sigset(SIGHUP, SignalStop) != SIG_ERR);
    assert(sigset(SIGUSR1, SignalStop) != SIG_ERR);
    return;
}

int TickFunc(void* args) {
//	LogError("%s:%d TickFunc", __func__, __LINE__);
    return 0;
}

template<class T>
int StartWorker(PHXSqlProxyConfig * config, WorkerConfig_t * worker_config) {
    const char * listen_ip = worker_config->listen_ip_;
    int fork_proc_count = worker_config->fork_proc_count_;
    int worker_thread_count = worker_config->worker_thread_count_;

    int listen_fd[2];
    int port[2] = {worker_config->port_, worker_config->proxy_port_};
    for (int i = 0; i < 2; ++i) {
        listen_fd[i] = CreateTcpSocket(port[i], listen_ip, true);
        if (listen_fd[i] <= 0) {
            LogError("creat tcp socket failed ret %d, errno (%d:%s)", listen_fd[i], errno, strerror(errno));
            printf("creat tcp socket failed ret %d, errno (%d:%s)", listen_fd[i], errno, strerror(errno));
            return -__LINE__;
        }

        int ret = listen(listen_fd[i], 1024);
        if (ret < 0) {
            LogError("listen in [%s:%d] fd %d ret %d errno (%d:%s)", listen_ip, port[i], listen_fd[i], ret,
                     errno, strerror(errno));
            printf("creat tcp socket failed ret %d, errno (%d:%s)", listen_fd[i], errno, strerror(errno));
            return -__LINE__;
        }
    }

    //SetNonBlock( listen_fd );
    for (int i = 0; i < fork_proc_count; ++i) {
        pid_t pid = fork();
        if (pid > 0) {
            continue;
        } else if (pid < 0) {
            break;
        }
        //co_init_curr_thread_env();
        co_enable_hook_sys();

        if (config->Sleep()) {
            ::sleep(config->Sleep());
        }

        MonitorPluginEntry * monitor_plugin = MonitorPluginEntry::GetDefault();
        monitor_plugin->SetConfig(config, worker_config);

        RequestFilterPluginEntry * requestfilter_plugin = RequestFilterPluginEntry::GetDefault();
        requestfilter_plugin->SetConfig(config, worker_config);

        //if is master, send heart beat to phxbinlogsvr
        if (worker_config->is_master_port_) {
            HeartBeatThread * heartbeat_thread = new HeartBeatThread(config, worker_config);
            heartbeat_thread->start();
        }

        GroupStatusCache * group_status_cache = new GroupStatusCache(config, worker_config);
        group_status_cache->start();

        vector<WorkerThread *> worker_threads;
        for (int j = 0; j < worker_thread_count; ++j) {
            WorkerThread * worker_thread = new WorkerThread(config, worker_config);
            worker_thread->InitRoutines<T>(group_status_cache);
            worker_threads.push_back(worker_thread);
            worker_thread->start();
        }

        for (int i = 0; i < 2; ++i) {
            AcceptThread * accept_thread = new AcceptThread(config, worker_threads, listen_fd[i]);
            accept_thread->start();
        }

        co_eventloop(co_get_epoll_ct(), TickFunc, 0);
        exit(0);
    }
    return 0;
}

int phxsqlproxymain(int argc, char *argv[], PHXSqlProxyConfig * config) {
    if (argc < 2) {
        printf("Usage %s <config>\n", argv[0]);
        exit(-1);
    }

    SignalHandler();

    if (argc == 3 && strcmp(argv[2], "daemon") == 0) {
        pid_t pid = fork();
        if (pid > 0) {
            exit(0);
        } else if (pid < 0) {
            LogError("fork fail");
            exit(-1);
        }
    }

    int ret = 0;
    if ((ret = StartWorker<MasterIORoutine>(config, config->GetMasterWorkerConfig())) != 0) {
        LogError("Start master worker failed, ret %d", ret);
        printf("Start master worker failed, ret %d", ret);
        exit(-1);
    }
    printf("start master worker finished ...\n");

    if ((ret = StartWorker<SlaveIORoutine>(config, config->GetSlaveWorkerConfig())) != 0) {
        LogError("Start slave worker failed, ret %d", ret);
        printf("Start slave worker failed, ret %d", ret);
        exit(-1);
    }
    printf("start slave worker finished ...\n");
    return 0;
}
