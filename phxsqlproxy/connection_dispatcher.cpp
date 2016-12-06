/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <cstring>

#include "co_routine.h"
#include "io_routine.h"
#include "phxsqlproxyutil.h"
#include "phxcoroutine.h"
#include "routineutil.h"
#include "monitor_plugin.h"
#include "connection_dispatcher.h"
#include "phxcomm/phx_log.h"

using phxsql::LogError;

namespace phxsqlproxy {

ConnectionDispatcher::ConnectionDispatcher(IORoutineMgr * io_routine_mgr) {
    memset(socket_pair_fd_, 0, sizeof(socket_pair_fd_));
    ::socketpair(AF_UNIX, SOCK_STREAM, 0, socket_pair_fd_);
    io_routine_mgr_ = io_routine_mgr;
    SetNonBlock(socket_pair_fd_[0]);
    SetNonBlock(socket_pair_fd_[1]);
}

ConnectionDispatcher::~ConnectionDispatcher() {
    if (socket_pair_fd_[0]) {
        close(socket_pair_fd_[0]);
        close(socket_pair_fd_[1]);
    }
}

int ConnectionDispatcher::run() {
    co_enable_hook_sys();
    while (true) {
        char buffer[512] = { 0 };
        int n = RoutineReadWithTimeout(socket_pair_fd_[1], buffer, sizeof(buffer), 1000);
        if (n > 0) {
            for (int i = 0; i < n; i += sizeof(int)) {
                int fd = 0;
                memcpy(&fd, buffer + i, sizeof(int));
                if (fd) {
                    SetNonBlock(fd);
                    IORoutine * io_routine = NULL;
                    int get_routine_ret = io_routine_mgr_->GetIORoutine(&io_routine);
                    if (get_routine_ret == 0 && io_routine != NULL) {
                        MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->ResumeRoutine();
                        io_routine->SetClientFD(fd);
                        io_routine->resume();
                    } else {
                        MonitorPluginEntry::GetDefault()->GetMonitorPlugin()->OutOfConn();
                        LogError("%s:%d receive connection but no useful io routine", __func__, __LINE__);
                        close(fd);
                    }
                }
            }
        }
    }
    return 0;
}

int ConnectionDispatcher::AssignFD(int fd) {
    return RoutineWriteWithTimeout(socket_pair_fd_[0], (const char *) (&fd), sizeof(int), 1000);
}

}
