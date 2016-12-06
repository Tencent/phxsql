/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#include "co_routine.h"
#include "monitor_routine.h"

#include "phxcomm/phx_log.h"
#include "phxcoroutine.h"
#include "io_routine.h"
#include "phxsqlproxyutil.h"
#include "phxsqlproxyconfig.h"
#include "monitor_plugin.h"

using std::stack;
using phxsql::LogWarning;

namespace phxsqlproxy {

MonitorRoutine::MonitorRoutine(PHXSqlProxyConfig * config, IORoutineMgr * io_routine_mgr) {
    config_ = config;
    io_routine_mgr_ = io_routine_mgr;
}

MonitorRoutine::~MonitorRoutine() {
}

int MonitorRoutine::run() {
    co_enable_hook_sys();
    while (true) {
        int io_routine_used = io_routine_mgr_->GetUsedIORoutine();
        MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->WorkingRoutine(io_routine_used);
        LogWarning("%s: %d IORoutine is working....", __func__, io_routine_used);
        poll(0, 0, 60 * 1000);
    }
    return 0;
}

}
