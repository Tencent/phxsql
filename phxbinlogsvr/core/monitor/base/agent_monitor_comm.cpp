/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "agent_monitor_comm.h"

#include "phxcomm/lock_manager.h"
#include "phxcomm/phx_log.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/define/errordef.h"

#include <unistd.h>
#include <sys/time.h>

using phxsql::RWLockManager;
using phxsql::LockManager;
using phxsql::LogVerbose;

namespace phxbinlog {

AgentMonitorComm::AgentMonitorComm() {
    status_ = NOTHING;

    pthread_mutex_init(&mutex_, NULL);
    pthread_cond_init(&cond_, NULL);
    pthread_rwlock_init(&rwmutex_, NULL);
    ready_num_ = 0;
}

AgentMonitorComm::~AgentMonitorComm() {
    pthread_cond_destroy(&cond_);
    pthread_mutex_destroy(&mutex_);
    pthread_rwlock_destroy(&rwmutex_);
}

AgentMonitorComm *AgentMonitorComm::GetGlobalAgentMonitorComm() {
    static AgentMonitorComm agentmonitorcomm;
    return &agentmonitorcomm;
}

void AgentMonitorComm::MonitorStart() {
    RWLockManager lock(&rwmutex_, RWLockManager::WRITE);
    if (status_ == NOTHING)
        status_ = WAITING;
}

void AgentMonitorComm::ResetMaster() {
    LockManager lock(&mutex_);
    ready_num_++;
    pthread_cond_signal(&cond_);
}

int AgentMonitorComm::GetStatus() {
    LockManager lock(&mutex_);
    RWLockManager rwlock(&rwmutex_, RWLockManager::WRITE);
    if (ready_num_) {
        status_ = MASTER_CHANGING;
        ready_num_ = 0;
    }
    return status_;
}

void AgentMonitorComm::SetStatus(const int status) {
    RWLockManager lock(&rwmutex_, RWLockManager::WRITE);
    status_ = status;
}

void AgentMonitorComm::TimeWait(const struct timespec &outtime) {
    LockManager lock(&mutex_);
    pthread_cond_timedwait(&cond_, &mutex_, &outtime);
}

pthread_mutex_t *AgentMonitorComm::GetLock() {
    return &mutex_;
}

bool AgentMonitorComm::IsMasterChanging() {
    return GetStatus() == AgentMonitorComm::MASTER_CHANGING;
}

enum {
    PROXY_MONITOR_FLAG = 1,
};

pthread_rwlock_t AgentExternalMonitor::rwmutex_;
int AgentExternalMonitor::flag_;
int AgentExternalMonitor::full_flag_ = PROXY_MONITOR_FLAG;
int AgentExternalMonitor::last_update_time_;

void AgentExternalMonitor::SetHeartBeat(const uint32_t &flag) {
    CheckTimeOut();
    RWLockManager lock(&rwmutex_, RWLockManager::WRITE);
    flag_ |= flag;
    LogVerbose("%s set heartbeat flag %d", __func__, flag);
}

bool AgentExternalMonitor::IsHealthy() {
    CheckTimeOut();
    RWLockManager lock(&rwmutex_, RWLockManager::READ);
    LogVerbose("%s full flag %d current flag %u", __func__, full_flag_, flag_);
    return full_flag_ == flag_;
}

void AgentExternalMonitor::CheckTimeOut() {
    RWLockManager lock(&rwmutex_, RWLockManager::WRITE);
    uint32_t now_time = time(NULL);
    if (now_time - last_update_time_ > 5) {
        last_update_time_ = now_time;
        flag_ = 0;
        LogVerbose("%s clear heartbeat", __func__);
    }
}

}

