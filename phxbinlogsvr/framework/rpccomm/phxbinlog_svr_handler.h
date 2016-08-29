/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include <pthread.h>

#include "phxbinlog_svr_logic.h"

namespace phxbinlog {
class Option;
class PhxBinLogAgent;
}

namespace phxbinlogsvr {

class PhxBinlogSvrLogic;
class PhxBinLogClientFactoryInterface;

class PhxBinLogSvrHandler {
 public:
    PhxBinLogSvrHandler(phxbinlog::Option *option, PhxBinLogClientFactoryInterface *client_factory);
    virtual ~PhxBinLogSvrHandler();

 public:
    void BeforeRun();
    void AfterRun();

 public:
    int SendBinLog(const std::string &req_buffer);
    int SetExportIP(const std::string &req_buff);

    int GetMasterInfoFromGlobal(std::string *resp_buffer);
    int GetLastSendGTIDFromGlobal(const std::string &req_buffer, std::string *resp_buffer);

    int GetMasterInfoFromLocal(std::string *resp_buffer);
    int GetLastSendGTIDFromLocal(const std::string &req_buffer, std::string *resp_buffer);

    int HeartBeat(const std::string &req_buffer);

    int AddMember(const std::string &req_buffer);
    int RemoveMember(const std::string &req_buffer);

    int SetMySqlAdminInfo(const std::string &req_buffer);
    int SetMySqlReplicaInfo(const std::string &req_buffer);

    int GetMemberList(std::string *resp_buffer);
    int InitBinlogSvrMaster(const std::string &req_buffer);

 private:
    static void *CreateBinLogProcess(void *arg);
    phxbinlog::PhxBinLogAgent *GetBinLogAgent();
    PhxBinlogSvrLogic *GetSvrLogic();

 private:
    phxbinlog::Option *option_;
    phxbinlog::PhxBinLogAgent *binlogagent_;
    PhxBinlogSvrLogic *binlogsvr_logic_;
    pthread_t create_binLog_process_thread_;
};

}
