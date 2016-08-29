/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include <string>

#include "phxcomm/thread_base.h"

namespace phxbinlog {

class MasterAgent;
class EventManager;
class MasterManager;
class Option;
class AgentMonitorComm;
class MasterMonitor;

class AgentMonitor : public phxsql::ThreadBase {
 public:
    AgentMonitor(const Option *option);
    ~AgentMonitor();

    static int IsGTIDCompleted(const Option *option, EventManager *eventmanager);

 private:
    virtual int Process();
    int CheckRunning();
    int MasterInit();
    int CheckMasterInit();
    int CheckMasterTimeOut();
    int CheckSlaveRunningStatus();
    bool IsSlaveReady();
    void CheckCheckPointFiles();

 private:
    const Option *option_;
    EventManager *event_manager_;
    MasterManager *master_manager_;
    MasterAgent *master_agent_;

    AgentMonitorComm *agent_monitor_comm_;
    MasterMonitor *master_monitor_;
    bool init_;
};

}
