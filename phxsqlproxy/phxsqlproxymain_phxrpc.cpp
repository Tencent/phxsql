/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <errno.h>
#include <stack>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <dlfcn.h>
#include <arpa/inet.h>

#include "phxbinlogsvr/client/phxbinlog_client_factory_phxrpc.h"
#include "phxbinlogsvr/client/phxbinlog_client_platform_info.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxcomm/phx_glog.h"
#include "phxcomm/phx_log.h"
#include "phxsqlproxyconfig.h"
#include "requestfilter_plugin.h"
#include "freqfilter_plugin.h"
//
#include "phxrpc/file/log_utils.h"

using namespace phxsqlproxy;
using namespace std;
using namespace phxbinlog;
using namespace phxsql;

void InitPhxsqlPlugins() {
    phxbinlogsvr::PhxBinlogClientPlatformInfo::GetDefault()->RegisterClientFactory(new PhxBinlogClientFactory_PhxRPC());
    phxsql::ResigterLogFunc(phxsql::PhxGLog::Log);
    phxrpc::setvlog(phxsql::PhxGLog::Log);
}

void InitProxyPlugins() {
    //monitor plugin initialization
    //MonitorPluginEntry * monitor_plugin = MonitorPluginEntry :: GetDefault();
    //monitor_plugin->SetMonitorPlugin( new YourSelfMonitor() );

    phxsqlproxy::RequestFilterPluginEntry * requestfilter_plugin = RequestFilterPluginEntry::GetDefault();
    requestfilter_plugin->SetRequestFilterPlugin(new phxsqlproxy::FreqFilterPlugin());
}

int phxsqlproxymain(int argc, char *argv[], phxsqlproxy::PHXSqlProxyConfig* config);

int main(int argc, char *argv[]) {
    InitPhxsqlPlugins();
    InitProxyPlugins();

    phxsqlproxy::PHXSqlProxyConfig * config = new phxsqlproxy::PHXSqlProxyConfig();
    if (config->ReadFileWithConfigDirPath(argv[1])) {
        phxsql::LogError("ReadConfig [%s] failed", argv[1]);
        printf("ReadConfig [%s] failed\n", argv[1]);
        exit(-1);
    }

    phxsql::PhxGLog::OpenLog("phxsqlproxy", config->GetSvrLogLevel(), config->GetSvrLogPath(), config->GetSvrLogFileMaxSize());

    return phxsqlproxymain(argc, argv, config);
}
