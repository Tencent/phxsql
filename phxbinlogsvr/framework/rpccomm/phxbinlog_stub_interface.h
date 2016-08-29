/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include <string>
#include <stdint.h>
#include <vector>

namespace phxbinlogsvr {

class PhxBinlogStubInterface {
 public:
    PhxBinlogStubInterface();
    PhxBinlogStubInterface(const std::string &ip, const uint32_t &port);
    PhxBinlogStubInterface(const std::string &ip, const uint32_t &port, const uint32_t timeout_ms);

    virtual ~PhxBinlogStubInterface();

    uint32_t GetTimeOutMS();
    void SetTimeOutMS(const uint32_t &ms);

    std::string GetIP();
    uint32_t GetPort();

    void SetPackageName(const std::string &package_name);
    std::string GetPackageName() const;

 protected:
//======================================================================================
    virtual int InitBinlogSvrMaster(const std::string &req_buffer) = 0;
    virtual int HeartBeat(const std::string &req_buffer) = 0;

    virtual int AddMember(const std::string &req_buffer) = 0;
    virtual int RemoveMember(const std::string &req_buffer) = 0;

    virtual int SetExportIP(const std::string &req_buffer) = 0;
    virtual int SendBinLog(const std::string &req_buffer) = 0;

    virtual int SetMySqlAdminInfo(const std::string &req_buffer) = 0;
    virtual int SetMySqlReplicaInfo(const std::string &req_buffer) = 0;

    virtual int GetMemberList(std::string *resp_buffer) = 0;

    virtual int GetLastSendGtidFromGlobal(const std::string &req_buffer, std::string *resp_buffer) = 0;
    virtual int GetLastSendGtidFromLocal(const std::string &req_buffer, std::string *resp_buffer) = 0;
    virtual int GetLastSendGtidFromLocal(const std::vector<std::string> &iplist, const uint32_t &port,
                                         const std::string &req_buffer,
                                         std::vector<std::pair<std::string, int> > *resp_bufferlist) = 0;

    virtual int GetMasterInfoFromGlobal(std::string *resp_buffer) = 0;
    virtual int GetMasterInfoFromLocal(std::string *resp_buffer) = 0;
    virtual int GetMasterInfoFromLocal(const std::vector<std::string> &iplist, const uint32_t &port,
                                       std::vector<std::pair<std::string, int> > *resp_bufferlist) = 0;

    friend class PhxBinlogClient;

 protected:
    std::string ip_;
    uint32_t port_;
    uint32_t timeout_ms_;
    std::string package_name_;
};

}

