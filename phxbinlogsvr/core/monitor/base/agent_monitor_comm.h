/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include <atomic> 
#include <pthread.h>
#include <string>

#include "phxcomm/thread_base.h"

namespace phxbinlog {

class EventAgent;
class EventManager;
class AgentMonitorComm {
 public:

    static AgentMonitorComm *GetGlobalAgentMonitorComm();

    void MonitorStart();
    void ResetMaster();
    int GetStatus();
    void SetStatus(const int status);

    bool IsMasterChanging();

    pthread_mutex_t *GetLock();
    void TimeWait(const struct timespec &outtime);

    enum {
        NOTHING = 0,
        WAITING = 1,
        MASTER_CHANGING = 2,
        MASTER_CHANGING_INIT = 3,
        RUNNING = 4,
    };

 private:
    AgentMonitorComm();
    ~AgentMonitorComm();
 private:
    pthread_rwlock_t rwmutex_;
    pthread_mutex_t mutex_;
    pthread_cond_t cond_;
    int status_;
    int ready_num_;
};

class AgentExternalMonitor {
 public:
    static void SetHeartBeat(const uint32_t &flag);
    static bool IsHealthy();
    static void CheckTimeOut();
 private:
    static int flag_;
    static int full_flag_;
    static int last_update_time_;
    static pthread_rwlock_t rwmutex_;
};

}
