/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "paxos_agent_manager.h"

#include "event_agent.h"
#include "master_agent.h"
#include "paxos_agent.h"

namespace phxbinlog {

PaxosAgentManager *PaxosAgentManager::GetGlobalPaxosAgentManager(const Option *option) {
    static PaxosAgentManager agent(option);
    return &agent;
}

PaxosAgentManager::PaxosAgentManager(const Option *option) {
    paxos_agent_ = new PaxosAgent(option);
    event_agent_ = new EventAgent(option, paxos_agent_);
    master_agent_ = new MasterAgent(option, paxos_agent_);

    paxos_agent_->Run();
}

int PaxosAgentManager::SetMaster() {
    return paxos_agent_->SetMaster();
}

PaxosAgentManager::~PaxosAgentManager() {
    delete paxos_agent_;
    delete event_agent_;
    delete master_agent_;
}

void PaxosAgentManager::Close() {
    paxos_agent_->Close();
}

EventAgent *PaxosAgentManager::GetEventAgent() {
    return event_agent_;
}

MasterAgent *PaxosAgentManager::GetMasterAgent() {
    return master_agent_;
}

}

