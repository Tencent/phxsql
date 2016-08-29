/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "requestfilter_plugin.h"
#include <string>

#include "phxsqlproxyconfig.h"

namespace phxsqlproxy {

RequestFilterPlugin::RequestFilterPlugin() {
}

RequestFilterPlugin::~RequestFilterPlugin() {
}

void RequestFilterPlugin::SetConfig(phxsqlproxy::PHXSqlProxyConfig * config,
                                    phxsqlproxy::WorkerConfig_t * worker_config) {
}

bool RequestFilterPlugin::CanExecute(bool is_masterPort, const std::string & db_name, const char * buf, int buf_size) {
    return true;
}

RequestFilterPluginEntry::RequestFilterPluginEntry() {
}

RequestFilterPluginEntry::~RequestFilterPluginEntry() {
}

RequestFilterPluginEntry * RequestFilterPluginEntry::GetDefault() {
    static RequestFilterPluginEntry entry;
    return &entry;
}

void RequestFilterPluginEntry::SetConfig(phxsqlproxy::PHXSqlProxyConfig * config,
                                         phxsqlproxy::WorkerConfig_t * worker_config) {
    if (request_filter_plugin_)
        request_filter_plugin_->SetConfig(config, worker_config);
}

void RequestFilterPluginEntry::SetRequestFilterPlugin(RequestFilterPlugin * plugin) {
    request_filter_plugin_ = plugin;
}

RequestFilterPlugin * RequestFilterPluginEntry::GetRequestFilterPlugin() {
    return request_filter_plugin_;
}

}

