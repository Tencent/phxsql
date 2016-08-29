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

#include <string>
#include <vector>

namespace phxbinlog {

class ExecuterBase;
class Option;
struct PaxosNodeInfo;

class PaxosAgent {
 protected:
    PaxosAgent(const Option *options);
    ~PaxosAgent();
    friend class PaxosAgentManager;
    friend class MasterAgent;

 public:

    void AddExecuter(ExecuterBase *sm);

    int Send(const std::string &value, ExecuterBase *sm);
    int AddMember(const std::string &ip);
    int RemoveMember(const std::string &ip);
    int GetMemberList(std::vector<PaxosNodeInfo> *member_list);

    void Close();

    void SetMemberListHandler(phxpaxos::MembershipChangeCallback call_back);
    void SetHoldPaxosLogCount(const uint64_t &hold_paxos_log_count);

 protected:
    static bool EncodeData(const std::string &value, const uint64_t &llInstanceID, std::string *decode_buffer);
    static bool DecodeData(const std::string &value, uint64_t *llInstanceID, std::string *decode_buffer);

    void SetPaxosOption();
    bool SetFollower();
    void SetPaxosNodeList(bool master);
 private:
    int SetMaster();
    void Run();
    void ReRun();
    void ReleaseNode();

 private:
    phxpaxos::Options paxos_option_;
    phxpaxos::Node *paxos_node_;
    const Option *option_;
};

}
