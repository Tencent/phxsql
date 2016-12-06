/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include <poll.h>

#include "heartbeat_thread.h"

#include "phxbinlogsvr/client/phxbinlog_client_platform_info.h"
#include "phxcomm/phx_log.h"

using namespace std;
using phxbinlogsvr::PhxBinlogClientPlatformInfo;
using phxbinlogsvr::PhxBinlogClient;

namespace phxsqlproxy {

HeartBeatThread::HeartBeatThread(PHXSqlProxyConfig * config, WorkerConfig_t * worker_config) {
    config_ = config;
    worker_config_ = worker_config;
}

HeartBeatThread::~HeartBeatThread() {
}

void HeartBeatThread::run() {
    static int count = 0;
    while (true) {
        std::shared_ptr<PhxBinlogClient> client = PhxBinlogClientPlatformInfo::GetDefault()->GetClientFactory()
                ->CreateClient();
        int ret = client->HeartBeat();
        if ((count++) % 60 == 0) {
            phxsql::LogWarning("%s:%d heartbeat ret %d", __func__, __LINE__, ret);
            count = 1;
        }
        poll(0, 0, 1000);
    }
}

}
