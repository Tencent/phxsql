/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include <string>
#include <vector>

#include "phxcomm/base_config.h"

namespace phxsqlproxy {

typedef struct tagWorkerConfig {
    const char * listen_ip_;
    int port_;
    int proxy_port_;
    int fork_proc_count_;
    int worker_thread_count_;
    int io_routine_count_;
    bool is_master_port_;
} WorkerConfig_t;

class PHXSqlProxyConfig : public phxsql::PhxBaseConfig {
 public:
    PHXSqlProxyConfig(const char * filename = "phxsqlproxy.conf");
    virtual ~PHXSqlProxyConfig();

    virtual void ReadConfig();

    //config in plugin config
    const char * GetMysqlIP();

    //config in mysql config
    int GetMysqlPort();

    int OpenDebugMode();

    const char * GetSpecifiedMasterIP();

    int GetOnlyProxy();

    int MasterEnableReadPort();

    int TryBestIfBinlogsvrDead();

    const char * GetFreqCtrlConfigPath();

    int GetSvrLogLevel();
    const char * GetSvrLogPath();
    int GetSvrLogFileMaxSize();

    int Sleep();

    uint32_t ConnectTimeoutMs();
    uint32_t WriteTimeoutMs();

    int ProxyProtocol();
    uint32_t ProxyProtocolTimeoutMs();

 public:
    WorkerConfig_t * GetMasterWorkerConfig();

    WorkerConfig_t * GetSlaveWorkerConfig();

 private:
    void ReadMasterWorkerConfig(WorkerConfig_t * worker_config);

    void ReadSlaveWorkerConfig(WorkerConfig_t * worker_config);
 private:

    std::string phxsql_plugin_config_;

    //in plugin config
    std::string mysql_ip_;

    std::string mysql_config_;

    //in mysq config
    uint32_t mysql_port_;

    int is_open_debug_mode_;

    WorkerConfig_t master_worker_config_;

    WorkerConfig_t slave_worker_config_;

    int is_only_proxy_;

    int is_master_enable_read_port_;

    int is_enable_try_best_;

    std::string freqctrl_config_;

    int log_level_;
    std::string log_path_;
    int log_file_max_size_;

    int svr_port_;
    std::string svr_ip_;

    int sleep_;

    uint32_t connect_timeout_ms_;
    uint32_t write_timeout_ms_;

    int proxy_protocol_;
    uint32_t proxy_protocol_timeout_ms_;
};

}
