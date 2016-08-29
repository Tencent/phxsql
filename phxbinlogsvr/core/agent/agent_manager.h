/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include <string>
#include <vector>

namespace phxbinlog {

class AgentMonitor;
class MasterAgent;
class Option;
struct PaxosNodeInfo;
class AgentManager {
 public:
    AgentManager(const Option *option);
    ~AgentManager();

    int InitAgentMaster();
    int GetMaster(std::string *master_ip, uint32_t *update_time, uint32_t *expire_time, uint32_t *version);
    int SetMaster(const std::string &master_ip);
    int SetExportIP(const std::string &export_ip, const uint32_t &port);
    int SetHeartBeat(const uint32_t &flag);
    int AddMember(const std::string &ip);
    int RemoveMember(const std::string &ip);
    int GetMemberList(std::vector<std::string> *member_list);
    int SetMySqlAdminInfo(const std::string &now_admin_username, const std::string &now_admin_pwd,
                          const std::string &new_username, const std::string &new_pwd);
    int SetMySqlReplicaInfo(const std::string &now_admin_username, const std::string &now_admin_pwd,
                            const std::string &new_username, const std::string &new_pwd);

 private:
    AgentMonitor *agent_monitor_;
    MasterAgent *master_agent_;
    const Option *option_;
};

}
