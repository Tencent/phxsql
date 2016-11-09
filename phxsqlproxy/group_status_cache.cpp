/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "group_status_cache.h"

#include "phxbinlogsvr/client/phxbinlog_client_platform_info.h"
#include "phxcomm/phx_log.h"
#include "phxcomm/lock_manager.h"
#include "phxsqlproxyconfig.h"
#include "monitor_plugin.h"
#include "phxsqlproxyutil.h"

#include <poll.h>
#include <pthread.h>

#include <memory>
#include <vector>
#include <string>

using phxbinlogsvr::PhxBinlogClientPlatformInfo;
using phxbinlogsvr::PhxBinlogClient;
using phxsql::LogError;
using phxsql::LogInfo;
using phxsql::LogVerbose;
using phxsql::RWLockManager;

namespace phxsqlproxy {

GroupStatusCache::GroupStatusCache(PHXSqlProxyConfig * config, WorkerConfig_t * worker_config) :
        config_(config), worker_config_(worker_config) {
    pthread_rwlock_init(&master_mutex_, NULL);
    pthread_rwlock_init(&membership_mutex_, NULL);
}

GroupStatusCache::~GroupStatusCache() {
    pthread_rwlock_destroy(&master_mutex_);
    pthread_rwlock_destroy(&membership_mutex_);
}

void GroupStatusCache::run() {
    while (true) {
        int sleep_ms = 300;

        int ret = UpdateMasterStatus();
        if (ret != 0) {
            sleep_ms = 100;
            LogError("UpdateMasterStatus ret %d", ret);
        }
        LogVerbose("UpdateMasterStatus ret %d", ret);

        ret = UpdateMembership();
        if (ret != 0) {
            sleep_ms = 100;
            LogError("UpdateMembership ret %d", ret);
        }
        LogVerbose("UpdateMembership ret %d", ret);

        poll(0, 0, sleep_ms);
    }
}

int GroupStatusCache::UpdateMasterStatus() {
    if (std::string(config_->GetSpecifiedMasterIP()) != "") {
        return 0;
    }

    std::string master_ip = "";
    uint32_t expired_time = 0;
    uint32_t version = 0;
    std::shared_ptr<PhxBinlogClient> client = PhxBinlogClientPlatformInfo::GetDefault()->GetClientFactory()
                                            ->CreateClient();
    int ret = client->GetMaster(&master_ip, &expired_time, &version);
    if (ret != 0) {
        MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->GetMasterInBinLogFail();
        LogError("%s:%d GetMaster failed ret %d", __func__, __LINE__, ret);

        if (!config_->TryBestIfBinlogsvrDead() || membership_.size() == 0) {
            return -__LINE__;
        }

        // try to get master_ip from other binlogsvr
        ret = client->GetGlobalMaster(membership_, &master_ip, &expired_time, &version, /*require_majority*/false);
        if (ret != 0) {
            LogError("%s:%d GetGlobalMaster failed ret %d", __func__, __LINE__, ret);
            return -__LINE__;
        }

        if (master_ip == worker_config_->listen_ip_) {
            // can not believe I'm a master_ip if binlogsvr is dead
            LogError("%s:%d GetGlobalMaster succ but get %s", __func__, __LINE__, master_ip.c_str());
            return -__LINE__;
        }
    }

    if (!IsMasterValid(master_ip, expired_time)) {
        LogError("%s:%d GetMaster success but (%s:%u) expired", __func__, __LINE__, master_ip.c_str(), expired_time);
        return -__LINE__;
    }

    if (version < master_status_.version_) {
        LogError("%s:%d GetMaster success but (%s:%u) version smaller than (%s:%u)", __func__, __LINE__,
                 master_ip.c_str(), version, master_status_.master_ip_.c_str(), master_status_.version_);
        return -__LINE__;
    }

    // version >= master_status_.version_
    if (version > master_status_.version_ || expired_time < master_status_.expired_time_) {
        RWLockManager lock(&master_mutex_, RWLockManager::WRITE);
        master_status_.master_ip_ = master_ip;
        master_status_.expired_time_ = expired_time;
        master_status_.version_ = version;
    }

    return 0;
}

int GroupStatusCache::UpdateMembership() {
    std::shared_ptr<PhxBinlogClient> client = PhxBinlogClientPlatformInfo::GetDefault()->GetClientFactory()
                                            ->CreateClient();
    std::vector<std::string> membership;
    uint32_t port = 0;
    int ret = client->GetMemberList(&membership, &port);

    if (ret == 0 && membership.size() == 0) {
        LogError("%s:%d ret 0 but no member inside", __func__, __LINE__);
        ret = -__LINE__;
    }

    if (ret == 0) {
        LogVerbose("%s:%d get membership ret size %zu", __func__, __LINE__, membership_.size());
        if (membership != membership_) {
            RWLockManager lock(&membership_mutex_, RWLockManager::WRITE);
            membership_ = std::move(membership);
        }
    }

    return ret;
}

bool GroupStatusCache::IsMasterValid(const std::string & master_ip, uint32_t expired_time) {
    if (master_ip == "") {
        return false;
    }
    if (expired_time < (uint32_t)time(NULL)) {
        return false;
    }
    return true;
}

int GroupStatusCache::GetMaster(std::string & master_ip) {
    if (std::string(config_->GetSpecifiedMasterIP()) != "") {
        master_ip = config_->GetSpecifiedMasterIP();
        LogVerbose("master is specified to [%s]", master_ip.c_str());
        return 0;
    }

    RWLockManager lock(&master_mutex_, RWLockManager::READ);

    if (IsMasterValid(master_status_.master_ip_, master_status_.expired_time_)) {
        master_ip = master_status_.master_ip_;
        return 0;
    }

    MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->CheckMasterInvalid();
    LogError("get master [%s] expiretime [%u] current [%u]", master_status_.master_ip_.c_str(),
             master_status_.expired_time_, (uint32_t)time(NULL));
    return -__LINE__;
}

bool GroupStatusCache::IsMember(const std::string & ip) {
    RWLockManager lock(&membership_mutex_, RWLockManager::READ);
    for (auto & itr : membership_) {
        if (itr == ip) {
            return true;
        }
    }
    return false;
}

int GroupStatusCache::GetSlave(const std::string & master_ip, std::string & slave_ip) {
    RWLockManager lock(&membership_mutex_, RWLockManager::READ);
    slave_ip = "";
    uint64_t t_last_failure = 0;
    for (auto & itr : membership_) {
        if (itr != master_ip) {
            if (slave_ip == "" || last_failure_[itr] < t_last_failure) {
                slave_ip = itr;
                t_last_failure = last_failure_[itr];
            }
        }
    }
    return slave_ip != "" ? 0 : -1;
}

void GroupStatusCache::MarkFailure(const std::string & ip) {
    RWLockManager lock(&membership_mutex_, RWLockManager::WRITE);
    last_failure_[ip] = GetTimestampMS();
}

}
