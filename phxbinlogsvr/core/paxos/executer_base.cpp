/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "executer_base.h"

#include "storage_manager.h"
#include "paxos_agent.h"

#include "phxcomm/phx_log.h"
#include "phxcomm/phx_timer.h"
#include "phxbinlogsvr/statistics/phxbinlog_stat.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/define/errordef.h"

using std::string;
using phxpaxos::SMCtx;
using phxsql::PhxTimer;

namespace phxbinlog {

ExecuterBase::ExecuterBase(const Option *option) {
    option_ = option;
}

bool ExecuterBase::Execute(const int group_idx, const uint64_t instanceid, const string &paxos_value, SMCtx *sm_ctx) {
    PhxTimer timer;
    int result = SMExecute(instanceid, paxos_value);
    if (result < 0)
        return false;

    if (sm_ctx)
        *(int *) (sm_ctx->m_pCtx) = result;
	STATISTICS(PaxosExecuteTime(timer.GetTime()));
    return true;
}

//checkpoint
bool ExecuterBase::ExecuteForCheckpoint(const int group_idx, const uint64_t instanceid, const string &paxos_value) {
    return true;
}

}

