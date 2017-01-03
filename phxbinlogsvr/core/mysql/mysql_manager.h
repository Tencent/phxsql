/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include <string>
#include <vector>

namespace phxbinlog {

class Option;
class MasterStorage;

class MySqlManager {
 public:
    static MySqlManager * GetGlobalMySqlManager(const Option *option);

    std::string GetAdminUserName();
    std::string GetAdminPwd();
    std::string GetReplicaUserName();
    std::string GetReplicaPwd();

    std::string GetPendingReplicaUserName();
    std::string GetPendingReplicaPwd();

    bool IsMySqlInit();
    bool IsReplicaProcess();
    bool HasReplicaGrant();

    int Query(const std::string& query_string, const std::string &ip = "127.0.0.1");
    int Query(const std::string &query_string, std::vector<std::vector<std::string> > *results,
              std::vector<std::string> *fields = NULL, const std::string &ip = "127.0.0.1");
    int GetValue(const std::string &value_type, const std::string &value_key, std::string *value,
                 const std::string &ip = "127.0.0.1");

    int CreateMySqlAdminInfo(const std::string &now_admin_username, const std::string &now_admin_pwd,
                             const std::string &new_username, const std::string &new_pwd,
                             const std::vector<std::string> &group_list);
    int CreateMySqlReplicaInfo(const std::string &now_admin_username, const std::string &now_admin_pwd,
                               const std::string &new_username, const std::string &new_pwd);

    void CheckMySqlUserInfo();

    int AddMemberAdminPermission(const std::string &member_ip);
    int RemoveMemberAdminPermission(const std::string &member_ip);
    int CheckAdminPermission(const std::vector<std::string> &member_list);
    static std::string ReduceGtidByOne(const std::string &gtid);
 protected:
    bool CheckAdminAccount(const std::string &admin_username, const std::string &admin_pwd);

    MySqlManager(const Option *option);
    friend class MasterStorage;

 private:

    int CheckUserExist(const std::string &username);
    int CheckUserGrantExist(const std::string &username, const std::string &pwd, const std::string &grant_string,
                            const std::string &grant_flag_string);

    int CheckAdminUser(const std::string &username, const std::string &pwd);
    int CheckReplicaUser(const std::string &username, const std::string &pwd);

    int CreateUser(const std::string &username, const std::string &pwd);
    int ChangePwd(const std::string &usename, const std::string &pwd);
    int CreateAdmin(const std::string &admin_username, const std::string &admin_pwd,
                    const std::vector<std::string> &ip_list);
    int CreateReplica(const std::string &admin_username, const std::string &replica_username,
                      const std::string &replica_pwd);

    int GrantUser(const std::string &username, const std::string &pwd, const std::string &grant_string);

    int AddMemberAdminPermission(const std::string &admin_username, const std::string &admin_pwd,
                                 const std::string &member_ip);
    int RemoveMemberAdminPermission(const std::string &admin_username, const std::string &admin_pwd,
                                    const std::string &member_ip);
 private:
    const Option *option_;
    MasterStorage *master_storage_;
};

}
