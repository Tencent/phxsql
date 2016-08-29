/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include "phxbinlog_stub_interface.h"
#include "phxbinlogsvr.pb.h"

class PhxBinlogStub_PhxRPC : public phxbinlogsvr::PhxBinlogStubInterface {
 public:
    static bool Init(const char * configFile);
 public:
    PhxBinlogStub_PhxRPC();
    PhxBinlogStub_PhxRPC(const std::string &ip, const uint32_t &port);
    PhxBinlogStub_PhxRPC(const std::string &ip, const uint32_t &port, const uint32_t &time_ms);
    ~PhxBinlogStub_PhxRPC();

    virtual int InitBinlogSvrMaster(const std::string &req_buffer);
    virtual int HeartBeat(const std::string &req_buffer);

    virtual int AddMember(const std::string &req_buffer);
    virtual int RemoveMember(const std::string &req_buffer);

    virtual int SetExportIP(const std::string &req_buffer);
    virtual int SendBinLog(const std::string &req_buffer);
    virtual int SetMySqlAdminInfo(const std::string &req_buffer);
    virtual int SetMySqlReplicaInfo(const std::string &req_buffer);

    virtual int GetMemberList(std::string *resp_buffer);

    virtual int GetLastSendGtidFromGlobal(const std::string &req_buffer, std::string *resp_buffer);
    virtual int GetLastSendGtidFromLocal(const std::string &req_buffer, std::string *resp_buffer);
    virtual int GetLastSendGtidFromLocal(const std::vector<std::string> &iplist, const uint32_t &port,
                                         const std::string &req_buffer,
                                         std::vector<std::pair<std::string, int> > *resp_bufferlist);

    virtual int GetMasterInfoFromGlobal(std::string *resp_buffer);
    virtual int GetMasterInfoFromLocal(std::string *resp_buffer);
    virtual int GetMasterInfoFromLocal(const std::vector<std::string> &iplist, const uint32_t &port,
                                       std::vector<std::pair<std::string, int> > *resp_bufferlist);

 private:
    template<class FuncName, typename ReqType, typename RespType>
    int RpcCallWithIP(const FuncName &func, const std::string &ip, const int port, const ReqType &req, RespType *resp);

    template<class FuncName, typename ReqType, typename RespType>
    int RpcCall(const FuncName &func, const ReqType &req, RespType *resp);

    template<class FuncName, typename ReqType>
    int RpcCallWithIPList(const FuncName &func, const std::vector<std::string> &iplist, const uint32_t &port,
                          const ReqType &req, std::vector<std::pair<std::string, int> > *resp_bufferlist);

    void InitClientMonitor();
};
