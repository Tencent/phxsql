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
    // Accept connection fail
    virtual void AcceptFail();

    // Accept a new connection
    virtual void AcceptSuccess();

    // Out of io routine (to handle received connection)
    virtual void OutOfConn();

    // Resume a io routine (handle a connection)
    virtual void ResumeRoutine();

    // An io routine is recycled by io routine manager
    virtual void RecycleRoutine();

    // An io routine is allocated by io routine manager
    virtual void AllocRoutine();

    // Connect to mysql or another phxsqlproxy
    virtual void ConnectDest();

    // Connect to mysql or another phxsqlproxy fail
    virtual void ConnectDestFail();

    // Connect to mysql with fd, fd < 0 indicating failure
    virtual void ConnectMysqlSvr(int fd);

    // Cost cost_ms ms to connect mysql
    virtual void ConnectMysqlSvrRunTime(uint64_t cost_ms);

    // Write write_size bytes to mysql or another phxsqlproxy (passthrough), including failed ones
    virtual void WriteNetwork(uint32_t write_size);

    // Write write_size bytes to mysql or another phxsqlproxy fail
    virtual void WriteNetworkFail(uint32_t write_size);

    // Cost cost_ms to execute a request
    virtual void RequestExecuteCost(uint64_t cost_ms);

    // Io routine Receipt a read event but read fail
    virtual void ReadNetworkFail();

    // Epoll cost cost_ms ms in io routine (including time for io)
    virtual void Epocost(uint64_t cost_ms);

    // Receive byte_size bytes from destination (mysql or another phxsqlproxy)
    virtual void ReceiveByteFromConnectDestSvr(uint32_t byte_size);

    // Receive byte_size bytes from client (mysql client or another phxsqlproxy)
    virtual void ReceiveByteFromMysqlClient(uint32_t byte_size);

    // Receive byte_size bytes from mysql
    virtual void ReceiveByteFromMysql(uint32_t byte_size);

    // Client connect master through read only port, reject
    virtual void ConnectSlavePortOnMaster();

    // Mysql query cost cost_ms ms
    virtual void MysqlQueryCost(uint64_t cost_ms);

    // Receive a response packet from destination (mysql or another phxsqlproxy)
    virtual void DestSvrRespPacket();

    // Receive a request packet from client (mysql client or another phxsqlproxy)
    virtual void ClientReqPacket();

    // Write byte_size bytes to mysql
    virtual void MysqlClientConnect(uint32_t byte_size);

    // Get an expired master from phxbinlogsvr (no available master)
    virtual void CheckMasterInvalid();

    // Get master from phxbinlogsvr fail
    virtual void GetMasterInBinLogFail();

    // Routine_count io routines are working
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
