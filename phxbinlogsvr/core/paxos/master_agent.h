/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include "phxpaxos/options.h"
#include "phxpaxos/node.h"
#include "masterinfo.pb.h"
#include "paxosdata.pb.h"

namespace phxbinlog {

class MasterInfo;
class PaxosAgent;
class MasterExecuter;
class MasterManager;
class Option;
struct PaxosNodeInfo;

class MasterAgent {
 protected:
    MasterAgent(const Option *options, PaxosAgent *paxosagent);
    ~MasterAgent();
    friend class PaxosAgentManager;

 public:
    int InitMaster();
    int SetExportIP(const std::string &export_ip, const uint32_t &port);
    int SetMaster(const std::string &master_ip, const std::string *export_ip = NULL, const uint32_t *port = NULL);
    int GetMaster(std::string *master_ip, uint32_t *update_time, uint32_t *expire_time, uint32_t *version);
    int SetHeartBeat(const uint32_t &flag);
    int AddMember(const std::string &ip);
    int RemoveMember(const std::string &ip);
    int GetMemberIPList(std::vector<std::string> *member_list);
    int SetMySqlAdminInfo(const std::string &now_admin_username, const std::string &now_admin_pwd,
                          const std::string &new_username, const std::string &new_pwd);
    int SetMySqlReplicaInfo(const std::string &now_admin_username, const std::string &now_admin_pwd,
                            const std::string &new_username, const std::string &new_pwd);

    int MasterNotify();

    void SetHoldPaxosLogCount(const uint64_t &hold_paxos_log_count);

 protected:
    bool IsTimeOut(const MasterInfo &info);

    static void MembershipChangeCallback(const int iGroupIdx, phxpaxos::NodeInfoList &vecNodeInfoList);

    int SetMaster(MasterInfo &info);
    int SendMasterInfo(const MasterInfo &master_info);

 private:
    PaxosAgent *paxos_agent_;
    MasterManager *master_manager_;
    MasterExecuter *master_executer_;
    const Option *option_;
};

}
