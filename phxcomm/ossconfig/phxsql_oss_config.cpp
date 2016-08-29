/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "phxcomm/phxsql_oss_config.h"

#include <string.h>

namespace phxsql{

PhxSQLOssConfig * PhxSQLOssConfig::GetDefault(const char *filename) {
	static PhxSQLOssConfig config(filename);
	return &config;
}

PhxSQLOssConfig::PhxSQLOssConfig(const char * filename) {
    ReadFile(filename);
}

PhxSQLOssConfig::~PhxSQLOssConfig() {
}

void PhxSQLOssConfig::ReadConfig() {
    ReadPaxosPluginMonitorConfig();
    ReadSqlProxyMonitorConfig();
    ReadBinLogSvrMonitorConfig();
    ReadMysqlMonitorConfig();
    return;
}

void PhxSQLOssConfig::ReadSqlProxyMonitorConfig() {
    sql_proxy_svrkit_ossid_ = GetInteger("phxsqlproxy", "SvrkitOssAttrID", 0);
    sql_proxy_masterport_logic_ossid_ = GetInteger("phxsqlproxy", "MasterPortLogicOssAttrID", 0);
    sql_proxy_slaveport_logic_ossid_ = GetInteger("phxsqlproxy", "SlavePortLogicOssAttrID", 0);
}

void PhxSQLOssConfig::ReadMysqlMonitorConfig() {
    mysql_ossid_ = GetInteger("mysql", "SvrkitOssAttrID", 0);
}

void PhxSQLOssConfig::ReadPaxosPluginMonitorConfig() {
    paxos_plugin_ossid_ = GetInteger("phxbinlogsvr_paxos", "OssAttrID", 0);
    paxos_plugin_usetime_ossid_ = GetInteger("phxbinlogsvr_paxos", "UseTimeOssAttrID", 0);
}

void PhxSQLOssConfig::ReadBinLogSvrMonitorConfig() {
    binlogsvr_logic_ossid_ = GetInteger("phxbinlogsvr", "LogicOssAttrID", 0);
    binlogsvr_svrkit_ossid_ = GetInteger("phxbinlogsvr", "SvrkitOssAttrID", 0);
}

uint32_t PhxSQLOssConfig::GetSqlProxyOssAttrID() const {
    return sql_proxy_svrkit_ossid_;
}

uint32_t PhxSQLOssConfig::GetSqlProxySlavePortLogicOssAttrID() const {
    return sql_proxy_slaveport_logic_ossid_;
}

uint32_t PhxSQLOssConfig::GetMysqlOssAttrID() const {
    return mysql_ossid_;
}

uint32_t PhxSQLOssConfig::GetSqlProxyMasterPortLogicOssAttrID() const {
    return sql_proxy_masterport_logic_ossid_;
}

uint32_t PhxSQLOssConfig::GetBinlogsvrSvrkitOssAttrID() const {
    return binlogsvr_svrkit_ossid_;
}

uint32_t PhxSQLOssConfig::GetBinlogsvrLogicOssAttrID() const {
    return binlogsvr_logic_ossid_;
}

uint32_t PhxSQLOssConfig::GetPluginPaxosOssAttrID() const {
    return paxos_plugin_ossid_;
}

uint32_t PhxSQLOssConfig::GetPluginPaxosUseTimeOssAttrID() const {
    return paxos_plugin_usetime_ossid_;
}

}

