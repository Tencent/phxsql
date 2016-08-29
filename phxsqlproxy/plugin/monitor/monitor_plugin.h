/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include "phxsqlproxyconfig.h"

namespace phxsqlproxy {

class MonitorPlugin {
 public:
    MonitorPlugin();

    virtual ~MonitorPlugin();

    virtual void SetConfig(phxsqlproxy::PHXSqlProxyConfig * config, phxsqlproxy::WorkerConfig_t * worker_config);

 public:
    virtual void AcceptFail();
    virtual void AcceptSuccess();
    virtual void OutOfConn();
    virtual void ResumeRoutine();

    virtual void RecycleRoutine();
    virtual void AllocRoutine();
    virtual void ConnectDest();
    virtual void ConnectDestFail();
    virtual void ConnectMysqlSvr(int fd);
    virtual void ConnectMysqlSvrRunTime(uint64_t cost_ms);
    virtual void WriteNetwork(uint32_t write_size);
    virtual void WriteNetworkFail(uint32_t write_size);
    virtual void RequestExecuteCost(uint64_t cost_ms);
    virtual void ReadNetworkFail();
    virtual void Epocost(uint64_t cost_ms);
    virtual void ReceiveByteFromConnectDestSvr(uint32_t byte_size);
    virtual void ReceiveByteFromMysqlClient(uint32_t byte_size);
    virtual void ReceiveByteFromMysql(uint32_t byte_size);
    virtual void ConnectSlavePortOnMaster();

    virtual void MysqlQueryCost(uint64_t cost_ms);
    virtual void DestSvrRespPacket();
    virtual void ClientReqPacket();
    virtual void MysqlClientConnect(uint32_t byte_size);

    virtual void CheckMasterInvalid();
    virtual void GetMasterInBinLogFail();

    virtual void WorkingRoutine(int32_t routine_count);
};

class MonitorPluginEntry {
 public:
    MonitorPluginEntry();

    virtual ~MonitorPluginEntry();

    static MonitorPluginEntry * GetDefault();

    void SetConfig(phxsqlproxy::PHXSqlProxyConfig * config, phxsqlproxy::WorkerConfig_t * worker_config);

    void SetMonitorPlugin(MonitorPlugin * monitor);

    MonitorPlugin * GetMonitorPlugin();

 private:
    MonitorPlugin * monitor_plugin_;

    bool is_set_;
};

}
