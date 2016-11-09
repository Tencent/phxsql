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
#include <memory>

namespace phxbinlogsvr {

class PhxBinlogStubInterface;

class PhxBinlogClient {
 public:
    PhxBinlogClient(PhxBinlogStubInterface * stub_interface);
    virtual ~PhxBinlogClient();

    int GetMaster(std::string *ip, uint32_t *expire_time);
    int GetMaster(std::string *ip, uint32_t *expire_time, uint32_t *version);
    int GetGlobalMaster(const std::vector<std::string> &iplist, std::string *ip, uint32_t *expire_time,
                        uint32_t *version, bool require_majority = true);
    int SetExportIP(const std::string &ip, const uint32_t &port);

    int SendBinLog(const std::string &old_gtid, const std::string &new_gtid, const std::string &event_buffer);
    int SendBinLog(const std::string &old_gtid, const std::string &event_buffer, std::string *max_gtid);
    int SendBinLog(const std::string &old_gtid, const std::string &event_buffer);

    int GetLastSendGtid(const std::string &uuid, std::string * last_send_gtid);

    int AddMember(const std::string &ip);
    int RemoveMember(const std::string &ip);

    int HeartBeat(const uint32_t &flag = 1);

    int SetMySqlAdminInfo(const std::string &now_admin_username, const std::string &now_admin_pwd,
                          const std::string &new_admin_username, const std::string &new_admin_pwd);
    int SetMySqlReplicaInfo(const std::string &now_admin_username, const std::string &now_admin_pwd,
                            const std::string &new_replica_username, const std::string &new_replica_pwd);

    int GetMemberList(std::vector<std::string> *ip_list, uint32_t *port);
    int InitBinlogSvrMaster();

 protected:
    int GetGlobalMaster(const std::vector<std::string> &iplist, const uint32_t &port, std::string *ip,
                        uint32_t *expire_time, uint32_t *version, bool require_majority = true);
    int GetGlobalLastSendGtid(const std::vector<std::string> &iplist, const uint32_t &port, const std::string &uuid,
                              std::string * last_send_gtid);

    int GetLocalMaster(std::string *ip, uint32_t * expire_time, uint32_t *version);
    int GetLocalLastSendGtid(const std::string &uuid, std::string * last_send_gtid);

    friend class PhxBinlogSvrLogic;

 private:
    //coding interface
    bool EncodeBinLogBuffer(const std::string &old_gtid, const std::string &new_gtid, const std::string &event_buffer,
                            std::string *encode_buffer);
    bool EncodeExportIPInfo(const std::string &ip, const uint32_t &port, std::string *encode_buffer);
    bool EncodeHeartBeatInfo(const uint32_t &flag, std::string * encode_buffer);
    bool EncodeMemberList(const std::string &from_ip, const std::string &to_ip, std::string * buffer);

    bool DecodeMasterInfo(const std::string &decode_buffer, std::string *ip, uint32_t *expire_time, uint32_t *version);

    PhxBinlogStubInterface* stub_interface_;
};

}

