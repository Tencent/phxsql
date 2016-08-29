/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once
#include "masterinfo.pb.h"

#include <string>
#include <pthread.h>

namespace phxbinlog {

class AgentMonitorComm;
class MySqlManager;
class MasterStorage;
class Option;
struct PaxosNodeInfo;

class MasterManager {
 public:
    static MasterManager *GetGlobalMasterManager(const Option *option);
 public:
    void MonitorStart();
    int SetMasterInfo(const MasterInfo &masterinfo);
    int GetMasterInfo(MasterInfo *info);
    int GenMasterInfoByLocal(const std::string &master_ip, MasterInfo *info);

    bool IsMaster(const MasterInfo &master_info);
    bool IsMaster();
    bool IsTimeOut(const MasterInfo &info);

    int GetMasterInfo(std::string *master_ip, uint32_t *update_time, uint32_t *expire_time, uint32_t *version);
    uint64_t GetMasterInstanceID();
    bool CheckMasterBySvrID(const uint32_t svrid, const bool check_timeout = true);

    void SetHeartBeat(const uint32_t &flag);

    void SetMemberList(const std::vector<std::string> &member_list);
    void GetMemberIPList(std::vector<std::string> *member_list);

    int SetMySqlAdminInfo(const std::string &now_admin_username, const std::string &now_admin_pwd,
                          const std::string &new_username, const std::string &new_pwd, MasterInfo *info);

    int SetMySqlReplicaInfo(const std::string &now_admin_username, const std::string &now_admin_pwd,
                            const std::string &new_username, const std::string &new_pwd, MasterInfo *info);

    int AddMemberAdminPermission(const std::string &member_ip);

    int RemoveMemberAdminPermission(const std::string &member_ip);

 protected:
    MasterManager(const Option *option);
    ~MasterManager();

 private:
    const Option *option_;
    AgentMonitorComm *agent_monitor_comm_;
    MySqlManager *mysql_manager_;
    MasterStorage *storage_;
    std::vector<std::string> member_list_;
};

}
