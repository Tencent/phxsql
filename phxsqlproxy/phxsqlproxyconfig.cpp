/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "phxsqlproxyconfig.h"
#include <string.h>
#include <vector>

#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxcomm/phx_log.h"
using namespace std;
using namespace phxbinlog;
using namespace phxsql;

namespace phxsqlproxy {

PHXSqlProxyConfig::PHXSqlProxyConfig(const char * filename) {
    ReadFile(filename);

    memset(&master_worker_config_, 0, sizeof(master_worker_config_));
    memset(&slave_worker_config_, 0, sizeof(slave_worker_config_));
}

PHXSqlProxyConfig::~PHXSqlProxyConfig() {
}

void PHXSqlProxyConfig::ReadConfig() {
    svr_port_ = GetInteger("Server", "Port", 0);
    svr_ip_ = Get("Server", "IP", "");
    if (svr_ip_ == "$InnerIP") {
        svr_ip_ = GetInnerIP();
    }
    phxsql_plugin_config_ = Get("Server", "PluginConfigFile", "");
    is_open_debug_mode_ = GetInteger("Server", "OpenDebugMode", 0);
    is_only_proxy_ = GetInteger("Server", "OnlyProxy", 0);
    is_master_enable_read_port_ = GetInteger("Server", "MasterEnableReadPort", 0);
    is_enable_try_best_ = GetInteger("Server", "TryBestIfBinlogsvrDead", 0);
    freqctrl_config_ = Get("Server", "FreqCtrlConfig", "");
    log_level_ = GetInteger("Server", "LogLevel", 3);
    log_file_max_size_ = GetInteger("Server", "LogFileMaxSize", 1600);
    log_path_ = Get("Server", "LogFilePath", "/tmp/");
    sleep_ = GetInteger("Server", "Sleep", 0);
    connect_timeout_ms_ = GetInteger("Server", "ConnectTimeoutMs", 200);
    write_timeout_ms_ = GetInteger("Server", "WriteTimeoutMs", 1000);
    proxy_protocol_ = GetInteger("Server", "ProxyProtocol", 0);
    proxy_protocol_timeout_ms_ = GetInteger("Server", "ProxyProtocolTimeoutMs", 1000);

    /*
     phxsql_config_ = PhxMySqlConfig :: GetDefault();
     if ( phxsql_plugin_config_ != "" )
     {
     phxsql_config_->ReadFileWithConfigDirPath( phxsql_plugin_config_.c_str() );
     }
     */
    ReadMasterWorkerConfig(&master_worker_config_);
    ReadSlaveWorkerConfig(&slave_worker_config_);
    LogWarning("read plugin config [%s]", phxsql_plugin_config_.c_str());
}

const char * PHXSqlProxyConfig::GetMysqlIP() {
    return Option::GetDefault()->GetMySqlConfig()->GetMySQLIP();
}

int PHXSqlProxyConfig::GetMysqlPort() {
    return Option::GetDefault()->GetMySqlConfig()->GetMySQLPort();
}

int PHXSqlProxyConfig::OpenDebugMode() {
    return is_open_debug_mode_;
}

WorkerConfig_t * PHXSqlProxyConfig::GetMasterWorkerConfig() {
    return &master_worker_config_;
}

WorkerConfig_t * PHXSqlProxyConfig::GetSlaveWorkerConfig() {
    return &slave_worker_config_;
}

void PHXSqlProxyConfig::ReadMasterWorkerConfig(WorkerConfig_t * worker_config) {
    worker_config->listen_ip_ = svr_ip_.c_str();
    worker_config->port_ = svr_port_;
    worker_config->proxy_port_ = GetInteger("Server", "MasterProxyPort", 0);
    worker_config->fork_proc_count_ = GetInteger("Server", "MasterForkProcCnt", 1);
    worker_config->worker_thread_count_ = GetInteger("Server", "MasterWorkerThread", 3);
    worker_config->io_routine_count_ = GetInteger("Server", "MasterIORoutineCnt", 1000);
    if (!worker_config->proxy_port_) {
        worker_config->proxy_port_ = worker_config->port_ + 2;
    }
    worker_config->is_master_port_ = true;
}

void PHXSqlProxyConfig::ReadSlaveWorkerConfig(WorkerConfig_t * worker_config) {
    worker_config->listen_ip_ = svr_ip_.c_str();
    worker_config->port_ = GetInteger("Server", "SlavePort", 0);
    worker_config->proxy_port_ = GetInteger("Server", "SlaveProxyPort", 0);
    worker_config->fork_proc_count_ = GetInteger("Server", "SlaveForkProcCnt", 1);
    worker_config->worker_thread_count_ = GetInteger("Server", "SlaveWorkerThread", 3);
    worker_config->io_routine_count_ = GetInteger("Server", "SlaveIORoutineCnt", 1000);
    if (!worker_config->port_) {
        worker_config->port_ = svr_port_ + 1;
    }
    if (!worker_config->proxy_port_) {
        worker_config->proxy_port_ = worker_config->port_ + 2;
    }
    worker_config->is_master_port_ = false;
}

const char * PHXSqlProxyConfig::GetSpecifiedMasterIP() {
    return Option::GetDefault()->GetBinLogSvrConfig()->GetSpecifiedMasterIP();
}

int PHXSqlProxyConfig::GetOnlyProxy() {
    return is_only_proxy_;
}

int PHXSqlProxyConfig::MasterEnableReadPort() {
    return is_master_enable_read_port_;
}

int PHXSqlProxyConfig::TryBestIfBinlogsvrDead() {
    return is_enable_try_best_;
}

const char * PHXSqlProxyConfig::GetFreqCtrlConfigPath() {
    return freqctrl_config_.c_str();
}

int PHXSqlProxyConfig::GetSvrLogLevel() {
    return log_level_;
}

const char * PHXSqlProxyConfig::GetSvrLogPath() {
    return log_path_.c_str();
}

int PHXSqlProxyConfig::GetSvrLogFileMaxSize() {
    return log_file_max_size_;
}

int PHXSqlProxyConfig::Sleep() {
    return sleep_;
}

uint32_t PHXSqlProxyConfig::ConnectTimeoutMs() {
    return connect_timeout_ms_;
}

uint32_t PHXSqlProxyConfig::WriteTimeoutMs() {
    return write_timeout_ms_;
}

int PHXSqlProxyConfig::ProxyProtocol() {
    return proxy_protocol_;
}

uint32_t PHXSqlProxyConfig::ProxyProtocolTimeoutMs() {
    return proxy_protocol_timeout_ms_;
}

}
