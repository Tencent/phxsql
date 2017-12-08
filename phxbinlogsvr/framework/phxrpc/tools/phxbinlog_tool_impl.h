/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at

	https://opensource.org/licenses/GPL-2.0

	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include <stdio.h>
#include <string>
#include <vector>

#include "phxrpc_phxbinlog_tool.h"
namespace phxrpc {
class OptMap;
}

class PhxbinlogToolImpl : public PhxbinlogTool {
 public:
    PhxbinlogToolImpl();
    virtual ~PhxbinlogToolImpl();

    virtual int PhxEcho(phxrpc::OptMap & opt_map);

    virtual int SendBinLog(phxrpc::OptMap & opt_map);

    virtual int GetMasterInfoFromGlobal(phxrpc::OptMap & opt_map);

    virtual int GetMasterInfoFromLocal(phxrpc::OptMap & opt_map);

    virtual int GetLastSendGtidFromGlobal(phxrpc::OptMap & opt_map);

    virtual int GetLastSendGtidFromLocal(phxrpc::OptMap & opt_map);

    virtual int SetExportIP(phxrpc::OptMap & opt_map);

    virtual int HeartBeat(phxrpc::OptMap & opt_map);

    virtual int AddMember(phxrpc::OptMap & opt_map);

    virtual int RemoveMember(phxrpc::OptMap & opt_map);

    virtual int SetMySqlAdminInfo(phxrpc::OptMap & opt_map);

    virtual int SetMySqlReplicaInfo(phxrpc::OptMap & opt_map);

    virtual int GetMemberList(phxrpc::OptMap & opt_map);

    virtual int InitBinlogSvrMaster( phxrpc::OptMap & opt_map );

 private:
    int GetMasterIP(const std::string &connect_ip, const int &connect_port, std::string *master_ip);
	bool CheckIPList(const std::vector<std::string> &ip_list, const uint32_t &port) ;
};

