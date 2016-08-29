/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include <string>
#include "masterinfo.pb.h"

namespace phxbinlog {

class DataManager;
class AgentManager;
class Option;
struct PaxosNodeInfo;
class PhxBinLogAgent {
 public:
    PhxBinLogAgent(Option *option);
    ~PhxBinLogAgent();

    int Process();

    int SendEvent(const std::string &oldgtid, const std::string &newgtid, const std::string &event_buffer);
    int SetExportIP(const std::string &export_ip, const uint32_t &port);
    int GetMaster(std::string *master_ip, uint32_t *update_time, uint32_t *expire_time, uint32_t *version);
    int GetLastSendGTID(const std::string &uuid, std::string *gtid);
    int SetHeartBeat(const uint32_t &req);

    int AddMember(const std::string &ip);
    int RemoveMember(const std::string &ip);
    int GetMemberList(std::vector<std::string> *node_list);

    int SetMySqlAdminInfo(const std::string &now_admin_username, const std::string &now_admin_pwd,
                          const std::string &new_username, const std::string &new_pwd);
    int SetMySqlReplicaInfo(const std::string &now_admin_username, const std::string &now_admin_pwd,
                            const std::string &new_username, const std::string &new_pwd);

    int FlushData();

    int InitAgentMaster();

 private:
    int Bind(const char *ip, const int port);

 private:
    int server_sockfd_;
    std::string ip_;
    int port_;
    DataManager *data_manager_;
    AgentManager *agent_manager_;
    const Option *option_;
};

}
