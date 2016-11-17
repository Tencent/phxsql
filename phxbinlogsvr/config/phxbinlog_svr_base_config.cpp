/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "phxbinlogsvr/config/phxbinlog_svr_base_config.h"

#include <iostream>
#include "phxcomm/phx_log.h"
#include "phxcomm/phx_utils.h"

using std::string;
using std::vector;
using std::pair;
using std::max;
using std::make_pair;
using phxsql::Utils;
using phxsql::ColorLogInfo;

namespace phxbinlog {

PHXBinlogSvrBaseConfig::PHXBinlogSvrBaseConfig(const char *filename) {
    rpc_timeout_ = 0;
    rpc_utimeout_ = 0;

    ReadFile(filename);
}

PHXBinlogSvrBaseConfig::~PHXBinlogSvrBaseConfig() {
}

void PHXBinlogSvrBaseConfig::ReadConfig() {
    ReadAgentConfig();
    ReadPaxosConfig();
    return;
}

void PHXBinlogSvrBaseConfig::ReadPaxosConfig() {
    paxos_log_path_ = Get("PaxosOption", "PaxosLogPath", "");
    paxos_port_ = GetInteger("PaxosOption", "PaxosPort", 0);
    paxos_log_num_ = GetInteger("PaxosOption", "LogNum", 1000000);
    packet_mode_ = GetInteger("PaxosOption", "PacketMode", 0);
    udp_max_size_ = GetInteger("PaxosOption", "UDPMaxSize", 4096);
}

void PHXBinlogSvrBaseConfig::ReadAgentConfig() {
    ColorLogInfo("%s read agent config", __func__);

    engine_ip_ = Get("Server", "IP", "");
    binlogsvr_port_ = GetInteger("Server", "Port", 17000);
    specified_master_ip_ = Get("Server", "SpecifiedMasterIP", "");

    log_level_ = GetInteger("Server", "LogLevel", 0);
    log_path_ = Get("Server", "LogFilePath", "");
    log_maxsize_ = GetInteger("Server", "LogMaxSize", 0);

    package_name_ = Get("Server", "PackageName", "phxbinlogsvr");

	if(engine_ip_.empty() || engine_ip_[0]=='\0' || engine_ip_[0] == '$' )
		engine_ip_ = GetInnerIP();

    engine_port_ = GetInteger("AgentOption", "AgentPort", 6000);
    event_data_base_path_ = Get("AgentOption", "EventDataDir", "");
    event_data_db_path_ = event_data_base_path_ + "/event_lb/";
    event_data_backup_path_ = event_data_base_path_ + "/bakup/";
    event_max_file_size_ = GetInteger("AgentOption", "MaxFileSize", 10000);
    event_max_file_size_ = max(event_max_file_size_, 10000u);

    max_event_push_ = GetInteger("AgentOption", "MaxItemPushCount", 10);

    master_lease_time_ = GetInteger("AgentOption", "MasterLease", 20);
	mysql_checkstatus_timeperiod_=master_lease_time_/4;
	if(mysql_checkstatus_timeperiod_<1)
		mysql_checkstatus_timeperiod_=1;
	if(mysql_checkstatus_timeperiod_>5)
		mysql_checkstatus_timeperiod_=5;

    checkpoint_making_timeperiod_ = GetInteger("AgentOption", "CheckPointTime", 5);
    checkpoint_making_timeperiod_ *= 60;
    max_delete_checkpoint_file_num_ = GetInteger("AgentOption", "MaxDeleteCheckPointFileNum", 5);
    force_makeing_checkpoint_ = GetInteger("AgentOption", "ForceMakingCheckpoint", 1);

    engine_svrid_ = Utils::GetSvrID(engine_ip_);

    rpc_timeout_ = GetInteger("AgentOption", "Timeout", 1);
    rpc_utimeout_ = GetInteger("AgentOption", "UTimeout", 1);

    follow_ip_ = Get("AgentOption", "FollowIP", "");

    ColorLogInfo("%s load db path %s engine ip %s port %d package name %s", __func__, event_data_base_path_.c_str(),
                 engine_ip_.c_str(), engine_port_, package_name_.c_str());
}

int PHXBinlogSvrBaseConfig::GetPaxosPort() const {
    return paxos_port_;
}

const char * PHXBinlogSvrBaseConfig::GetSpecifiedMasterIP() const {
    return specified_master_ip_.c_str();
}

const char * PHXBinlogSvrBaseConfig::GetPaxosLogPath() const {
    return paxos_log_path_.c_str();
}

int PHXBinlogSvrBaseConfig::GetPackageMode() const {
    return packet_mode_;
}

uint32_t PHXBinlogSvrBaseConfig::GetPaxosLogNum() const {
    return paxos_log_num_;
}

uint32_t PHXBinlogSvrBaseConfig::GetUDPMaxSize() const {
    return udp_max_size_;
}

uint32_t PHXBinlogSvrBaseConfig::GetMasterLeaseTime() const {
    return master_lease_time_;
}

void PHXBinlogSvrBaseConfig::SetExtLeaseTime(const uint32_t &master_ext_lease_time) {
    master_ext_lease_time_ = master_ext_lease_time;
}

uint32_t PHXBinlogSvrBaseConfig::GetMasterExtLeaseTime() const {
    return master_ext_lease_time_;
}

const char * PHXBinlogSvrBaseConfig::GetBinlogSvrIP() const {
    return engine_ip_.c_str();
}

int PHXBinlogSvrBaseConfig::GetBinlogSvrPort() const {
    return binlogsvr_port_;
}

const char * PHXBinlogSvrBaseConfig::GetEngineIP() const {
    return engine_ip_.c_str();
}

int PHXBinlogSvrBaseConfig::GetEnginePort() const {
    return engine_port_;
}

uint32_t PHXBinlogSvrBaseConfig::GetEngineSvrID() const {
    return engine_svrid_;
}

const char * PHXBinlogSvrBaseConfig::GetEventDataBaseDir() const {
    return event_data_base_path_.c_str();
}

const char * PHXBinlogSvrBaseConfig::GetEventDataStorageDBDir() const {
    return event_data_db_path_.c_str();
}

const char * PHXBinlogSvrBaseConfig::GetEventDataBackUPDir() const {
    return event_data_backup_path_.c_str();
}

uint32_t PHXBinlogSvrBaseConfig::GetMaxEventCountToPush() const {
    return max_event_push_;
}

uint32_t PHXBinlogSvrBaseConfig::GetMaxEventFileSize() const {
    return event_max_file_size_;
}

uint32_t PHXBinlogSvrBaseConfig::GetCheckPointMakingPeriod() const {
    return checkpoint_making_timeperiod_;
}

uint32_t PHXBinlogSvrBaseConfig::GetMonitorCheckStatusPeriod() const {
    return mysql_checkstatus_timeperiod_;
}

pair<uint32_t, uint32_t> PHXBinlogSvrBaseConfig::GetTimeOut() const {
    return make_pair(rpc_timeout_, rpc_utimeout_ * 1000);
}

uint32_t PHXBinlogSvrBaseConfig::GetLogLevel() const {
    return log_level_;
}

uint32_t PHXBinlogSvrBaseConfig::GetLogMaxSize() const {
    return log_maxsize_;
}

const char * PHXBinlogSvrBaseConfig::GetLogFilePath() const {
    return log_path_.c_str();
}

uint32_t PHXBinlogSvrBaseConfig::GetMaxDeleteCheckPointFileNum() const {
    return max_delete_checkpoint_file_num_;
}

bool PHXBinlogSvrBaseConfig::IsForceMakingCheckPoint() const {
    return force_makeing_checkpoint_;
}

uint32_t PHXBinlogSvrBaseConfig::GetTimeOutMS() const {
    pair < uint32_t, uint32_t > time_out = GetTimeOut();
    return time_out.first * 1000 + (time_out.second / 1000) % 1000;
}

string PHXBinlogSvrBaseConfig::GetPackageName() const {
    return package_name_;
}

string PHXBinlogSvrBaseConfig::GetFollowIP() const {
    return follow_ip_;
}

}
