/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "event_agent.h"

#include "paxos_agent.h"
#include "event_executer.h"
#include "event_manager.h"
#include "master_manager.h"
#include "paxosdata.pb.h"

#include "phxpaxos/node.h"

#include "phxcomm/phx_utils.h"
#include "phxcomm/phx_log.h"
#include "phxbinlogsvr/statistics/phxbinlog_stat.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/define/errordef.h"

#include <string>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

using std::string;
using phxsql::Utils;
using phxsql::ColorLogWarning;
using phxsql::ColorLogInfo;
using phxsql::ColorLogError;

namespace phxbinlog {

EventAgent::EventAgent(const Option *option, PaxosAgent *paxosagent) {
    //it shold not be inited twice in global environment
    event_manager_ = EventManager::GetGlobalEventManager(option);
    master_manager_ = MasterManager::GetGlobalMasterManager(option);

    paxos_agent_ = paxosagent;
    paxos_agent_->AddExecuter(new EventExecuter(option, event_manager_));
    event_executer_ = new EventExecuter(option, event_manager_);

    option_ = option;
}

EventAgent::~EventAgent() {
    delete event_executer_;
}

int EventAgent::SendValue(const string &old_gtid, const string &new_gtid, const string &value) {
    PaxosEventData data;
    if (new_gtid.empty() || value.empty()) {
        ColorLogError("%s no gtid or value, skip", __func__, value.size());
        STATISTICS(GtidEventSendFail());
        return GTID_EVENT_FAIL;
    }

    if (new_gtid.size()) {
        if (!master_manager_->IsMaster()) {
            ColorLogWarning("%s is not master, master conflict", __func__, value.size());
            return MASTER_CONFLICT;
        }
    }

    uint64_t now_checksum;
    int ret = event_manager_->GetNowCheckSum(&now_checksum);
    if (ret) {
        return ret;
    }

    data.set_event_gtid(new_gtid);
    data.set_svrid(option_->GetBinLogSvrConfig()->GetEngineSvrID());
    data.set_current_gtid(old_gtid);
    data.set_buffer(value);
    data.set_checksum(Utils::GetCheckSum(now_checksum, value.c_str(), value.size()));

    ColorLogInfo("%s send svrid %u current gtid %s event gtid %s buffer len %zu now checksum %llu, old checksum %llu"
                 "old gtid %s new gtid %s",
                 __func__, data.svrid(), data.current_gtid().c_str(), data.event_gtid().c_str(), data.buffer().size(),
                 data.checksum(), now_checksum, old_gtid.c_str(), new_gtid.c_str());

    string send_buffer;
    if (!data.SerializeToString(&send_buffer)) {
        ColorLogWarning("buffer serialize fail");
        return PAXOS_BUFFER_FAIL;
    }

    ret = paxos_agent_->Send(send_buffer, event_executer_);
    if (ret) {
        ColorLogError("%s paxos send value fail", __func__);
        STATISTICS(GtidEventSendFail());
    }
    return ret;
}

int EventAgent::FlushData() {
    PaxosEventData data;
    data.set_svrid(option_->GetBinLogSvrConfig()->GetEngineSvrID());
    data.set_current_gtid("");

    string send_buffer;
    if (!data.SerializeToString(&send_buffer)) {
        ColorLogError("buffer serialize fail");
        return PAXOS_BUFFER_FAIL;
    }

    int ret = paxos_agent_->Send(send_buffer, event_executer_);
    if (ret) {
        STATISTICS(GtidEventSendFail());
        return ret;
    }

    return ret;
}

}

