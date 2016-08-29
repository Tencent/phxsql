/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "data_manager.h"

#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <stdio.h>

#include "event_manager.h"
#include "event_agent.h"
#include "paxos_agent_manager.h"
#include "replication.h"

#include "phxcomm/phx_log.h"
#include "phxcomm/phx_timer.h"
#include "phxbinlogsvr/statistics/phxbinlog_stat.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/define/errordef.h"

using std::string;
using phxsql::ColorLogWarning;
using phxsql::ColorLogError;
using phxsql::LogVerbose;
using phxsql::PhxTimer;

namespace phxbinlog {

DataManager::DataManager(const Option *option) {
    event_agent_ = PaxosAgentManager::GetGlobalPaxosAgentManager(option)->GetEventAgent();
    event_manager_ = EventManager::GetGlobalEventManager(option);
    replica_manager_ = new ReplicationManager(option);
    option_ = option;
    LogVerbose("%s init data manager done", __func__);
}

DataManager::~DataManager() {
    delete replica_manager_, replica_manager_ = NULL;
}

int DataManager::SendEvent(const string &oldgtid, const string &newgtid, const string &event_buffer) {
    STATISTICS(MySqlBinlogSend());
    PhxTimer timer;
    if (event_buffer.size() == 0) {
        ColorLogWarning("send event value empty fail");
        return BUFFER_FAIL;
    }
    int ret = event_agent_->SendValue(oldgtid, newgtid, event_buffer);
    if (ret != OK) {
        STATISTICS(MySqlBinlogSendFail());
        ColorLogError("send value fail %d", ret);
    }
    STATISTICS(MySqlSendEventTime(timer.GetTime()));
    return ret;
}

int DataManager::GetLastSendGTID(const string &uuid, string *gtid) {
    STATISTICS(MySqlGetLastGTID());
    *gtid = event_manager_->GetLastSendGTID(uuid);
    return OK;
}

void DataManager::DealWithSlave(const int fd) {
    replica_manager_->DealWithSlave(fd);
}

int DataManager::FlushData() {
    return event_agent_->FlushData();
}

}

