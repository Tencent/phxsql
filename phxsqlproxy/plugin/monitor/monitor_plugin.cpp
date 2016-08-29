/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "phxsqlproxyconfig.h"
#include "monitor_plugin.h"

namespace phxsqlproxy {

MonitorPlugin::MonitorPlugin() {
}

MonitorPlugin::~MonitorPlugin() {
}

void MonitorPlugin::SetConfig(phxsqlproxy::PHXSqlProxyConfig * config, phxsqlproxy::WorkerConfig_t * worker_config) {
}

void MonitorPlugin::AcceptFail() {
}

void MonitorPlugin::AcceptSuccess() {
}

void MonitorPlugin::OutOfConn() {
}

void MonitorPlugin::ResumeRoutine() {
}

void MonitorPlugin::RecycleRoutine() {
}

void MonitorPlugin::AllocRoutine() {
}

void MonitorPlugin::ConnectDest() {
}

void MonitorPlugin::ConnectDestFail() {
}

void MonitorPlugin::ConnectMysqlSvr(int fd) {
}

void MonitorPlugin::ConnectMysqlSvrRunTime(uint64_t cost_ms) {
}

void MonitorPlugin::WriteNetwork(uint32_t write_size) {
}

void MonitorPlugin::WriteNetworkFail(uint32_t write_size) {
}

void MonitorPlugin::RequestExecuteCost(uint64_t cost_ms) {
}

void MonitorPlugin::ReadNetworkFail() {
}

void MonitorPlugin::Epocost(uint64_t cost_ms) {
}

void MonitorPlugin::ReceiveByteFromConnectDestSvr(uint32_t byte_size) {
}

void MonitorPlugin::ReceiveByteFromMysqlClient(uint32_t byte_size) {
}

void MonitorPlugin::ReceiveByteFromMysql(uint32_t byte_size) {
}

void MonitorPlugin::ConnectSlavePortOnMaster() {
}

void MonitorPlugin::MysqlQueryCost(uint64_t cost_ms) {
}

void MonitorPlugin::DestSvrRespPacket() {
}
void MonitorPlugin::ClientReqPacket() {
}

void MonitorPlugin::MysqlClientConnect(uint32_t byte_size) {
}

void MonitorPlugin::CheckMasterInvalid() {
}

void MonitorPlugin::GetMasterInBinLogFail() {
}

void MonitorPlugin::WorkingRoutine(int32_t routine_count) {
}

MonitorPluginEntry::MonitorPluginEntry() {
    monitor_plugin_ = new MonitorPlugin();
    is_set_ = false;
}

MonitorPluginEntry::~MonitorPluginEntry() {
    if (!is_set_) {
        delete monitor_plugin_, monitor_plugin_ = NULL;
    }
}

MonitorPluginEntry * MonitorPluginEntry::GetDefault() {
    static MonitorPluginEntry entry;
    return &entry;
}

void MonitorPluginEntry::SetConfig(phxsqlproxy::PHXSqlProxyConfig * config,
                                   phxsqlproxy::WorkerConfig_t * worker_config) {
    if (monitor_plugin_)
        monitor_plugin_->SetConfig(config, worker_config);
}

void MonitorPluginEntry::SetMonitorPlugin(MonitorPlugin * monitor) {
    if (!is_set_) {
        delete monitor_plugin_, monitor_plugin_ = NULL;
    }
    monitor_plugin_ = monitor;
    is_set_ = true;
}

MonitorPlugin * MonitorPluginEntry::GetMonitorPlugin() {
    return monitor_plugin_;
}

}

