/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include "phxbinlogsvr.pb.h"

#include <string>

namespace phxbinlog {
class Option;
class PhxBinLogAgent;
struct PaxosNodeInfo;
}

namespace phxbinlogsvr {

class PhxBinlogSvrLogic {
 public:
    PhxBinlogSvrLogic(phxbinlog::Option *option, phxbinlog::PhxBinLogAgent *binlogagent);

 private:
    int SendBinLog(const std::string &event_buffer);
    int SetMaster(const std::string &ip);
    int SetExportIP(const std::string &ip);
    int GetLastSendGTIDFromGlobal(const std::string &uuid, std::string *resp_buffer);
    int GetMasterInfoFromGlobal(std::string *resp_buffer);
    int HeartBeat(const std::string &req_buffer);

    int AddMember(const std::string &buffer);
    int RemoveMember(const std::string &buffer);

    int GetLastSendGTIDFromLocal(const std::string &uuid, std::string *resp_buffer);
    int GetMasterInfoFromLocal(std::string *resp_buffer);

    int SetMySqlAdminInfo(const std::string &req_buffer);
    int SetMySqlReplicaInfo(const std::string &req_buffer);

    int GetMemberList(std::string *resp_buffer);

    int InitBinlogSvrMaster(const std::string &req_buffer);

 private:
    int RealGetMasterInfo(phxbinlogsvr::MasterInfo *masterinfo);
    int RealSendBinLog(const phxbinlogsvr::BinLogBuffer req);
    int RealSetMaster(const std::string &ip);
    int RealSetExportIP(const std::string &ip, const uint32_t &port);
    int RealGetLastSendGTID(const std::string &uuid, std::string *gtid);

    int RealGetLocalMasterInfo(phxbinlogsvr::MasterInfo *masterinfo);
    int RealGetLocalLastSendGTID(const std::string &uuid, std::string *gtid);
    int RealSetHeartBeat(const phxbinlogsvr::HeartBeatInfo &req);

    int RealAddMember(const std::string &ip);
    int RealRemoveMember(const std::string &ip);

    int RealSetMySqlAdminInfo(const phxbinlogsvr::MySqlUserModefiedInfo &info);
    int RealSetMySqlReplicaInfo(const phxbinlogsvr::MySqlUserModefiedInfo &info);
    int RealGetMemberList(phxbinlogsvr::MemberList *info);
    int RealInitBinLogSvrMaster();

 private:
    int GetIPList(std::vector<std::string> *iplist);
	uint32_t GetCurrentTime();

 private:
    phxbinlog::Option *option_;
    phxbinlog::PhxBinLogAgent *binlogagent_;

    friend class PhxBinLogSvrHandler;
};

}
