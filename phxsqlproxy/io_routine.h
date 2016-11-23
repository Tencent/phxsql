/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include "co_routine.h"
#include "phxcoroutine.h"
#include <string>
#include <stack>

using std::stack;

namespace phxsqlproxy {

#define MAX_ACTIVE_FD_PER_ROUTINE 1024

class PHXSqlProxyConfig;
typedef struct tagWorkerConfig WorkerConfig_t;
class IORoutineMgr;
class GroupStatusCache;

class IORoutine : public Coroutine {

 public:
    IORoutine(IORoutineMgr * routine_mgr, GroupStatusCache * group_status_cache);

    virtual ~IORoutine();

    void SetClientFD(int fd);

 protected:
    GroupStatusCache * GetGroupStatusCache();

 private:
    int run();

    void ClearAll();

    void ReleaseFD(int & fd);

    int ConnectDest();

    int WriteToDest(int dest_fd, const char * buf, int write_size);

    int TransMsgDirect(int source_fd, int dest_fd, struct pollfd[], int nfds);

//monitor func
 private:
    void TimeCostMonitor(uint64_t cost);

    void ByteFromConnectDestSvr(uint32_t byte_size);

    void ByteFromMysqlClient(uint32_t byte_size);

 private:
    void ClearVariablesAndStatus();

 private:
    virtual int GetDestEndpoint(std::string & dest_ip, int & dest_port) = 0;

    virtual WorkerConfig_t * GetWorkerConfig() = 0;

    virtual bool CanExecute(const char * buf, int size);

    void GetDBNameFromAuthBuf(const char * buf, int buf_size);

    void GetDBNameFromReqBuf(const char * buf, int buf_size);
 protected:
    uint64_t req_uniq_id_;

    PHXSqlProxyConfig * config_;
    std::string connect_dest_;
    int connect_port_;
    bool is_authed_;
    std::string db_name_;

 private:

    IORoutineMgr * io_routine_mgr_;
    GroupStatusCache * group_status_cache_;

    int client_fd_;
    int sqlsvr_fd_;
    std::string listen_ip_;
    int listen_port_;
    std::string client_ip_;
    int client_port_;
    uint64_t last_received_request_timestamp_;
    uint64_t last_sent_request_timestamp_;
    int last_read_fd_;
};

class MasterIORoutine : public IORoutine {
 public:
    MasterIORoutine(IORoutineMgr * routine_mgr, GroupStatusCache * group_status_cache);

    virtual ~MasterIORoutine();

 private:
    int GetDestEndpoint(std::string & dest_ip, int & dest_port);

    bool CanExecute(const char * buf, int size);

    WorkerConfig_t * GetWorkerConfig();

};

class SlaveIORoutine : public IORoutine {
 public:
    SlaveIORoutine(IORoutineMgr * routine_mgr, GroupStatusCache * group_status_cache);

    virtual ~SlaveIORoutine();

 private:
    int GetDestEndpoint(std::string & dest_ip, int & dest_port);

    WorkerConfig_t * GetWorkerConfig();

    bool CanExecute(const char * buf, int size);
};

class IORoutineMgr {

 public:
    IORoutineMgr(PHXSqlProxyConfig * config, WorkerConfig_t * worker_config);

    PHXSqlProxyConfig * GetConfig();

    virtual ~IORoutineMgr();

    int AddIORoutine(IORoutine * io_routine_ptr);

    int GetIORoutine(IORoutine ** io_routine_pointer);

    int GetUsedIORoutine() const;

    bool IsAllIORoutineUsed();

    WorkerConfig_t * GetWorkerConfig();

 private:
    std::stack<IORoutine *> io_routine_stack_;

    PHXSqlProxyConfig * config_;

    WorkerConfig_t * worker_config_;

    uint32_t max_size_;
};

}
