/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include "phxthread.h"

#include <vector>
#include <string>
#include <unordered_map>

namespace phxsqlproxy {

class PHXSqlProxyConfig;
typedef struct tagWorkerConfig WorkerConfig_t;

typedef struct tagMasterStatus {
    std::string master_ip_ = "";
    uint32_t expired_time_ = 0;
    uint32_t version_ = 0;
} MasterStatus_t;

class GroupStatusCache : public PhxThread {
 public:
    GroupStatusCache(PHXSqlProxyConfig * config, WorkerConfig_t * worker_config);

    virtual ~GroupStatusCache();

    void run();

    int GetMaster(std::string & master_ip);

    bool IsMember(const std::string & ip);

    // use by MasterEnableReadPort=0

    int GetSlave(const std::string & master_ip, std::string & slave_ip);

    void MarkFailure(const std::string & ip);

 private:
    int UpdateMasterStatus();

    int UpdateMembership();

    bool IsMasterValid(const std::string & master_ip, uint32_t expired_time);

 private:
    PHXSqlProxyConfig * config_;
    WorkerConfig_t * worker_config_;

    MasterStatus_t master_status_;
    pthread_rwlock_t master_mutex_;

    std::vector<std::string> membership_;
    std::unordered_map<std::string, uint64_t> last_failure_; // use by MasterEnableReadPort=0
    pthread_rwlock_t membership_mutex_;
};

}
