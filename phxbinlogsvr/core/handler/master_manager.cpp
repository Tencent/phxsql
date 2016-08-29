/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "master_manager.h"

#include "agent_monitor_comm.h"
#include "master_monitor.h"
#include "master_storage.h"
#include "storage_manager.h"
#include "mysql_manager.h"

#include "phxcomm/phx_utils.h"
#include "phxcomm/phx_log.h"
#include "phxbinlogsvr/statistics/phxbinlog_stat.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/define/errordef.h"

using std::string;
using std::vector;
using phxsql::LogVerbose;
using phxsql::Utils;
using phxsql::ColorLogInfo;

namespace phxbinlog {

MasterManager *MasterManager::GetGlobalMasterManager(const Option *option) {
    static MasterManager manager(option);
    return &manager;
}

MasterManager::MasterManager(const Option *option) {
    option_ = option;
    storage_ = StorageManager::GetGlobalStorageManager(option)->GetMasterStorage();
    agent_monitor_comm_ = AgentMonitorComm::GetGlobalAgentMonitorComm();
    mysql_manager_ = MySqlManager::GetGlobalMySqlManager(option);
}

MasterManager::~MasterManager() {
}

void MasterManager::MonitorStart() {
    agent_monitor_comm_->MonitorStart();
}

int MasterManager::GetMasterInfo(MasterInfo *info) {
    storage_->GetMaster(info);
    LogVerbose("%s get master admin username %u replica username %u", __func__, info->admin_userinfo_size(),
               info->replica_userinfo_size());
    return OK;
}

int MasterManager::SetMasterInfo(const MasterInfo &master_info) {
    bool old_master = false;
    if (storage_->CheckMasterBySvrID(master_info.svr_id())) {
        old_master = true;
    }
    int ret = storage_->SetMaster(master_info);
    if (ret == OK) {
        if (old_master) {
            LogVerbose("%s master heart beat", __func__);
            //from master to make heartbeat
            STATISTICS(MasterHeartBeat());
        } else {
            LogVerbose("%s master upate", __func__);
            //notify monitor to check agent status
            agent_monitor_comm_->ResetMaster();
            STATISTICS(MasterChange());
        }
    }
    return ret;
}

int MasterManager::GenMasterInfoByLocal(const string &master_ip, MasterInfo *info) {
    int ret = GetMasterInfo(info);
    if (ret)
        return ret;

    info->set_port(option_->GetMySqlConfig()->GetMySQLPort());
    info->set_svr_id(Utils::GetSvrID(master_ip));

    return OK;
}

bool MasterManager::IsMaster(const MasterInfo &info) {
    return info.svr_id() == option_->GetBinLogSvrConfig()->GetEngineSvrID() && !IsTimeOut(info);
}

bool MasterManager::IsMaster() {
    return storage_->CheckMasterBySvrID(option_->GetBinLogSvrConfig()->GetEngineSvrID());
}

bool MasterManager::IsTimeOut(const MasterInfo &info) {
    return storage_->IsTimeOut(info);
}

bool MasterManager::CheckMasterBySvrID(const uint32_t svrid, const bool check_timeout) {
    return storage_->CheckMasterBySvrID(svrid, check_timeout);
}

int MasterManager::GetMasterInfo(string *master_ip, uint32_t *update_time, uint32_t *expire_time, uint32_t *version) {
    if (CheckMasterBySvrID(option_->GetBinLogSvrConfig()->GetEngineSvrID())
            && agent_monitor_comm_->IsMasterChanging()) {
        *master_ip = "";
        return MASTER_PREPARING;
    } else {
        MasterInfo master_info;
        if (GetMasterInfo(&master_info)) {
            return SVR_FAIL;
        }

        *master_ip = Utils::GetIP(master_info.svr_id());
        *update_time = master_info.update_time();
        *expire_time = master_info.expire_time();
        *version = master_info.version();
    }
    return OK;
}

uint64_t MasterManager::GetMasterInstanceID() {
    return storage_->GetLastInstanceID();
}

void MasterManager::SetHeartBeat(const uint32_t &flag) {
    AgentExternalMonitor::SetHeartBeat(flag);
}

void MasterManager::SetMemberList(const vector<string> &member_list) {
    member_list_ = member_list;
}

void MasterManager::GetMemberIPList(vector<string> *member_list) {
    *member_list = member_list_;
}

int MasterManager::SetMySqlAdminInfo(const string &now_admin_username, const string &now_admin_pwd,
                                     const string &new_username, const string &new_pwd, MasterInfo *info) {

    if (!IsMaster(*info)) {
        return MASTER_CONFLICT;
    }

    vector < string > ip_list;
    GetMemberIPList (&ip_list);

    int ret = mysql_manager_->CreateMySqlAdminInfo(now_admin_username, now_admin_pwd, new_username, new_pwd, ip_list);
    if (ret == OK) {
        UserInfo user_info;
        user_info.set_username(new_username);
        user_info.set_pwd(new_pwd);
        *(info->add_admin_userinfo()) = user_info;
    }
    return ret;
}

int MasterManager::SetMySqlReplicaInfo(const std::string &now_admin_username, const std::string &now_admin_pwd,
                                       const std::string &new_username, const std::string &new_pwd, MasterInfo *info) {

    if (!IsMaster(*info)) {
        return MASTER_CONFLICT;
    }

    int ret = mysql_manager_->CreateMySqlReplicaInfo(now_admin_username, now_admin_pwd, new_username, new_pwd);
    if (ret == OK) {
        UserInfo user_info;
        user_info.set_username(new_username);
        user_info.set_pwd(new_pwd);
        *(info->add_replica_userinfo()) = user_info;
    }
    return ret;
}

int MasterManager::AddMemberAdminPermission(const string &member_ip) {
    return mysql_manager_->AddMemberAdminPermission(member_ip);
}

int MasterManager::RemoveMemberAdminPermission(const string &member_ip) {
    return mysql_manager_->RemoveMemberAdminPermission(member_ip);
}

}

