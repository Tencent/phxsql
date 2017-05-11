/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "master_storage.h"

#include "phxcomm/lock_manager.h"
#include "phxcomm/phx_utils.h"
#include "phxcomm/phx_log.h"
#include "phxbinlogsvr/statistics/phxbinlog_stat.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/define/errordef.h"

using std::string;
using std::vector;
using phxsql::RWLockManager;
using phxsql::Utils;
using phxsql::LogVerbose;
using phxsql::ColorLogInfo;
using phxsql::ColorLogWarning;
using phxsql::ColorLogError;

namespace phxbinlog {

MasterStorage::MasterStorage(const Option *option) {

    option_ = option;
    pthread_rwlock_init(&mutex_, NULL);
}

MasterStorage::~MasterStorage() {
    pthread_rwlock_destroy(&mutex_);
}

int MasterStorage::LoadFromCheckPoint(const MasterInfo &info) {
    if (info.version() > 0) {
        master_ = info;
        master_.set_expire_time(
                time(NULL) + option_->GetBinLogSvrConfig()->GetMasterLeaseTime()
                        + option_->GetBinLogSvrConfig()->GetMasterExtLeaseTime());
    }
    return OK;
}

std::string MasterStorage::GetAdminUserName() {
    RWLockManager lock(&mutex_, RWLockManager::READ);
    return admin_username_;
}

std::string MasterStorage::GetAdminPwd() {
    RWLockManager lock(&mutex_, RWLockManager::READ);
    return admin_pwd_;
}

std::string MasterStorage::GetReplicaUserName() {
    RWLockManager lock(&mutex_, RWLockManager::READ);
    return replica_username_;
}

std::string MasterStorage::GetReplicaPwd() {
    RWLockManager lock(&mutex_, RWLockManager::READ);
    return replica_pwd_;
}

bool MasterStorage::HasAdminUserName() {
    RWLockManager lock(&mutex_, RWLockManager::READ);
    return master_.admin_userinfo_size();
}

bool MasterStorage::HasReplicaUserName() {
    RWLockManager lock(&mutex_, RWLockManager::READ);
    return master_.replica_userinfo_size();
}

int MasterStorage::SetMySqlAdminInfo(const MasterInfo &master_info, const std::string &username,
                                     const std::string &pwd) {
    RWLockManager lock(&mutex_, RWLockManager::WRITE);
    if (master_info.version() != master_.version()) {
        ColorLogError("%s set new username version conflit old %u new %u", __func__, master_.version(),
                      master_info.version());
        return MASTER_VERSION_CONFLICT;
    }
    ColorLogInfo("%s set new username %s, pwd size %zu", __func__, username.c_str(),pwd.size());
    admin_username_ = username;
    admin_pwd_ = pwd;
    return OK;
}

int MasterStorage::SetMySqlReplicaInfo(const MasterInfo &master_info, const std::string &username,
                                       const std::string &pwd) {
    RWLockManager lock(&mutex_, RWLockManager::WRITE);
    if (master_info.version() != master_.version()) {
        ColorLogError("%s set new username version conflit old %u new %u", __func__, master_.version(),
                      master_info.version());
        return MASTER_VERSION_CONFLICT;
    }
    ColorLogInfo("%s set new username %s, pwd size %zu", __func__, username.c_str(),pwd.size());
    replica_username_ = username;
    replica_pwd_ = pwd;
    return OK;
}

bool operator >(const MasterInfo &info1, const MasterInfo &info2) {
    return info1.instance_id() > info2.instance_id() && info1.version() > info2.version();
}

void MasterStorage::GetMaster(MasterInfo *info) {
    RWLockManager lock(&mutex_, RWLockManager::READ);
    *info = master_;
}

int MasterStorage::SetMaster(const MasterInfo &info) {
    RWLockManager lock(&mutex_, RWLockManager::WRITE);

    LogVerbose("%s set info ip %s(export ip %s:%u)old svr id %u expire time %u "
               "version %llu self version %llu instance id %llu, (master,replica) username (%s,%s) ",
               __func__, Utils::GetIP(info.svr_id()).c_str(), info.export_ip().c_str(), info.export_port(),
               master_.svr_id(), info.expire_time(), info.version(), master_.version(), info.instance_id(),
               admin_username_.c_str(), replica_username_.c_str());

    if (master_.svr_id() == 0 || (info.instance_id() > master_.instance_id() && info.version() >= master_.version())) {

        master_ = info;
        master_.set_version(master_.version() + 1);

        ColorLogInfo("%s set info ip %s svr id %u version %llu", __func__, Utils::GetIP(info.svr_id()).c_str(),
                     master_.svr_id(), master_.version());
        return OK;
    } else {
        ColorLogWarning("%s set master fail", __func__);
        STATISTICS(MasterChangeConflict());
        return MASTER_VERSION_CONFLICT;
    }
}

bool MasterStorage::CheckMasterBySvrID(const uint32_t &svrid, const bool check_timeout) {
    RWLockManager lock(&mutex_, RWLockManager::READ);
    LogVerbose("%s check svr id %u(%s) now svr id %u(%s) check time out %d", __func__, svrid,
               Utils::GetIP(svrid).c_str(), master_.svr_id(), Utils::GetIP(master_.svr_id()).c_str(), check_timeout);
    if (master_.svr_id() > 0)
        return master_.svr_id() == svrid && (!check_timeout || (check_timeout && !IsTimeOut(master_)));
    return false;
}

bool MasterStorage::IsTimeOut(const MasterInfo &info) {
    return info.expire_time() < time(NULL);
}

uint64_t MasterStorage::GetLastInstanceID() {
    return master_.instance_id();
}

}

