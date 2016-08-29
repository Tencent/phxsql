/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "agent_manager.h"

#include "master_agent.h"
#include "agent_monitor.h"
#include "paxos_agent_manager.h"

#include "phxcomm/phx_log.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/define/errordef.h"

using std::string;
using std::vector;

namespace phxbinlog {

AgentManager::AgentManager(const Option *option) {
    master_agent_ = PaxosAgentManager::GetGlobalPaxosAgentManager(option)->GetMasterAgent();
    option_ = option;
    agent_monitor_ = new AgentMonitor(option);
}

AgentManager::~AgentManager() {
    delete agent_monitor_;
}

int AgentManager::InitAgentMaster() {
    return master_agent_->InitMaster();
}

int AgentManager::GetMaster(string *master_ip, uint32_t *update_time, uint32_t *expire_time, uint32_t *version) {
    return master_agent_->GetMaster(master_ip, update_time, expire_time, version);
}

int AgentManager::SetExportIP(const string &export_ip, const uint32_t &port) {
    return master_agent_->SetExportIP(export_ip, port);
}

int AgentManager::SetHeartBeat(const uint32_t &flag) {
    return master_agent_->SetHeartBeat(flag);
}

int AgentManager::AddMember(const string &ip) {
    return master_agent_->AddMember(ip);
}

int AgentManager::RemoveMember(const string &ip) {
    return master_agent_->RemoveMember(ip);
}

int AgentManager::GetMemberList(vector<string> *member_list) {
    return master_agent_->GetMemberIPList(member_list);
}

int AgentManager::SetMySqlAdminInfo(const std::string &now_admin_username, const std::string &now_admin_pwd,
                                    const std::string &new_username, const std::string &new_pwd) {
    return master_agent_->SetMySqlAdminInfo(now_admin_username, now_admin_pwd, new_username, new_pwd);
}

int AgentManager::SetMySqlReplicaInfo(const std::string &now_admin_username, const std::string &now_admin_pwd,
                                      const std::string &new_username, const std::string &new_pwd) {
    return master_agent_->SetMySqlReplicaInfo(now_admin_username, now_admin_pwd, new_username, new_pwd);
}

}

