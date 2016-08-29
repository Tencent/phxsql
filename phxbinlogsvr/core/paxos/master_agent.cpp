/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "master_agent.h"

#include "phxpaxos/node.h"

#include "master_manager.h"
#include "master_monitor.h"
#include "event_agent.h"
#include "master_executer.h"
#include "paxos_agent.h"

#include <string>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include "phxcomm/phx_utils.h"
#include "phxcomm/phx_log.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/define/errordef.h"

using std::vector;
using std::string;
using phxsql::LogVerbose;
using phxsql::ColorLogError;
using phxsql::ColorLogInfo;
using phxsql::ColorLogWarning;
using phxsql::Utils;

namespace phxbinlog {

MasterAgent::MasterAgent(const Option *option, PaxosAgent *paxosagent) {
    master_manager_ = MasterManager::GetGlobalMasterManager(option);

    paxos_agent_ = paxosagent;
    paxos_agent_->AddExecuter(new MasterExecuter(option, master_manager_));
    master_executer_ = new MasterExecuter(option, master_manager_);
    paxos_agent_->SetMemberListHandler(&MembershipChangeCallback);
    option_ = option;
}

MasterAgent::~MasterAgent() {
    delete master_executer_;
}

int MasterAgent::InitMaster() {
    int ret = paxos_agent_->SetMaster();
    if (ret) {
        return ret;
    }

    //start monitor to run the agent
    master_manager_->MonitorStart();
    LogVerbose("%s paxos init master done", __func__);

    return OK;
}

void MasterAgent::MembershipChangeCallback(const int group_idx, phxpaxos::NodeInfoList &node_info_list) {
    vector < string > member_list;
    for (size_t i = 0; i < node_info_list.size(); ++i) {
        LogVerbose("%s get member ip %s", __func__, node_info_list[i].GetIP().c_str());
        member_list.push_back(node_info_list[i].GetIP());
    }

    MasterManager *mastermanager = MasterManager::GetGlobalMasterManager(Option::GetDefault());
    mastermanager->SetMemberList(member_list);
}

void MasterAgent::SetHoldPaxosLogCount(const uint64_t &hold_paxos_log_count) {
    paxos_agent_->SetHoldPaxosLogCount(hold_paxos_log_count);
}

int MasterAgent::SetExportIP(const string &export_ip, const uint32_t &port) {
    return SetMaster(option_->GetBinLogSvrConfig()->GetEngineIP(), &export_ip, &port);
}

int MasterAgent::SetMaster(const string &master_ip, const string *export_ip, const uint32_t *port) {

    LogVerbose("%s set master info ip %s export ip %d", __func__, master_ip.c_str(), export_ip);

    MasterInfo info;
    int ret = master_manager_->GenMasterInfoByLocal(master_ip, &info);
    if (ret) {
        return ret;
    }

    if (export_ip && port) {
        info.set_export_ip(*export_ip);
        info.set_export_port(*port);
    }

    return SetMaster(info);
}

int MasterAgent::SetMaster(MasterInfo &info) {

    if (!master_manager_->IsTimeOut(info)) {
        if (!master_manager_->IsMaster()) {
            ColorLogWarning("%s set master from not a master node", __func__);
            return MASTER_CONFLICT;
        }
    }

    info.set_update_time(time(NULL));
    info.set_lease(option_->GetBinLogSvrConfig()->GetMasterLeaseTime());
    info.set_expire_time(0);
    return SendMasterInfo(info);
}

int MasterAgent::SendMasterInfo(const MasterInfo &master_info) {
    ColorLogInfo("%s ip %s svr id %d version %llu expire time %u admin username size %u replica username size %u",
                 __func__, Utils::GetIP(master_info.svr_id()).c_str(), master_info.svr_id(), master_info.version(),
                 master_info.expire_time(), master_info.admin_userinfo_size(), master_info.admin_userinfo_size());
    string buffer;
    if (!master_info.SerializeToString(&buffer)) {
        return BUFFER_FAIL;
    }

    int ret = paxos_agent_->Send(buffer, master_executer_);
    if (ret) {
        ColorLogError("%s set master fail, ret %d", __func__, ret);
    }
    return ret;
}

int MasterAgent::GetMaster(string *master_ip, uint32_t *update_time, uint32_t *expire_time, uint32_t *version) {
    return master_manager_->GetMasterInfo(master_ip, update_time, expire_time, version);
}

int MasterAgent::MasterNotify() {
    MasterInfo now_masterinfo;
    int ret = master_manager_->GetMasterInfo(&now_masterinfo);
    if (ret == 0) {
        return SendMasterInfo(now_masterinfo);
    }
    return ret;
}

int MasterAgent::SetHeartBeat(const uint32_t &flag) {
    master_manager_->SetHeartBeat(flag);
    return 0;
}

int MasterAgent::AddMember(const string &ip) {
    return paxos_agent_->AddMember(ip);
}

int MasterAgent::RemoveMember(const string &ip) {
    int ret = master_manager_->RemoveMemberAdminPermission(ip);
    if (ret) {
        ColorLogError("%s remove member %s fail", __func__, ip.c_str());
        return ret;
    }
    return paxos_agent_->RemoveMember(ip);
}

int MasterAgent::GetMemberIPList(vector<string> *member_list) {
    master_manager_->GetMemberIPList(member_list);
    return 0;
}

int MasterAgent::SetMySqlAdminInfo(const std::string &now_admin_username, const std::string &now_admin_pwd,
                                   const std::string &new_username, const std::string &new_pwd) {
    LogVerbose("%s now admin username %s, new username %s", __func__, now_admin_username.c_str(), new_username.c_str());
    if (new_username == "") {
        ColorLogError("%s username empty, invalid", __func__, new_username.c_str());
        return REQ_INVALUD;
    }
    MasterInfo info;
    if (master_manager_->GetMasterInfo(&info)) {
        return SVR_FAIL;
    }

    int ret = master_manager_->SetMySqlAdminInfo(now_admin_username, now_admin_pwd, new_username, new_pwd, &info);
    if (ret) {
        ColorLogError("%s set admin info username %s fail, ret %d", __func__, new_username.c_str(), ret);
        return ret;
    }
    return SetMaster(info);
}

int MasterAgent::SetMySqlReplicaInfo(const std::string &now_admin_username, const std::string &now_admin_pwd,
                                     const std::string &new_username, const std::string &new_pwd) {
    LogVerbose("%s now admin username %s, new username %s", __func__, now_admin_username.c_str(), new_username.c_str());
    if (new_username == "") {
        ColorLogError("%s username empty, invalid", __func__, new_username.c_str());
        return REQ_INVALUD;
    }

    MasterInfo info;
    if (master_manager_->GetMasterInfo(&info)) {
        return SVR_FAIL;
    }

    int ret = master_manager_->SetMySqlReplicaInfo(now_admin_username, now_admin_pwd, new_username, new_pwd, &info);
    if (ret) {
        ColorLogError("%s set replica info username %s fail, ret %d", __func__, new_username.c_str(), ret);
        return ret;
    }
    return SetMaster(info);
}

}

