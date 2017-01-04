/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "mysql_manager.h"

#include "mysql_command.h"
#include "mysql_string_helper.h"

#include "master_storage.h"
#include "storage_manager.h"

#include "phxcomm/phx_log.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/define/errordef.h"
#include "phxbinlogsvr/core/gtid_handler.h"

#include <string.h>
#include <assert.h>

using std::string;
using std::vector;
using std::pair;
using phxsql::ColorLogWarning;
using phxsql::ColorLogError;
using phxsql::LogVerbose;

namespace phxbinlog {

MySqlManager * MySqlManager::GetGlobalMySqlManager(const Option *option) {
    static MySqlManager master_monitor(option);
    return &master_monitor;
}

MySqlManager::MySqlManager(const Option *option) {
    option_ = option;

    master_storage_ = StorageManager::GetGlobalStorageManager(option)->GetMasterStorage();
}

std::string MySqlManager::GetAdminUserName() {
    return master_storage_->GetAdminUserName();
}

std::string MySqlManager::GetAdminPwd() {
    return master_storage_->GetAdminPwd();
}

std::string MySqlManager::GetReplicaUserName() {
    return master_storage_->GetReplicaUserName();
}

std::string MySqlManager::GetReplicaPwd() {
    return master_storage_->GetReplicaPwd();
}

std::string MySqlManager::GetPendingReplicaUserName() {
    MasterInfo master_info;
    master_storage_->GetMaster(&master_info);
    if (master_info.replica_userinfo_size()) {
        ;
        return master_info.replica_userinfo(0).username();
    }
    return "";
}
std::string MySqlManager::GetPendingReplicaPwd() {
    MasterInfo master_info;
    master_storage_->GetMaster(&master_info);
    if (master_info.replica_userinfo_size()) {
        ;
        return master_info.replica_userinfo(0).pwd();
    }
    return "";
}

bool MySqlManager::HasReplicaGrant() {
    string replica_username = GetReplicaUserName();
    string replica_pwd = GetReplicaPwd();
    if (replica_username == "") {
        return false;
    }
    if (MySqlManager::CheckReplicaUser(replica_username, replica_pwd)) {
        return false;
    }
    return true;
}

int MySqlManager::Query(const string& query_string, const string &ip) {
    MySqlOption mysql_option;
    mysql_option.ip = ip;
    mysql_option.port = option_->GetMySqlConfig()->GetMySQLPort();
    mysql_option.username = GetAdminUserName();
    mysql_option.pwd = GetAdminPwd();
    if (mysql_option.username == "") {
        mysql_option.username = "root";
        mysql_option.pwd = "";
    }
    LogVerbose("%s username %s pwd %s query %s", __func__, mysql_option.username.c_str(), mysql_option.pwd.c_str(),
               query_string.c_str());

    return MySQLCommand(&mysql_option).Query(query_string);
}

int MySqlManager::Query(const string &query_string, vector<vector<string> > *results, std::vector<std::string> *fields,
                        const string &ip) {
    MySqlOption mysql_option;
    mysql_option.ip = ip;
    mysql_option.port = option_->GetMySqlConfig()->GetMySQLPort();
    mysql_option.username = GetAdminUserName();
    mysql_option.pwd = GetAdminPwd();
    if (mysql_option.username == "") {
        mysql_option.username = "root";
        mysql_option.pwd = "";
    }
    LogVerbose("%s username %s pwd %s query %s", __func__, mysql_option.username.c_str(), mysql_option.pwd.c_str(),
               query_string.c_str());

    return MySQLCommand(&mysql_option).GetQueryResults(query_string, results);
}

int MySqlManager::GetValue(const std::string &value_type, const std::string &value_key, std::string *value,
                           const std::string &ip) {
    MySqlOption mysql_option;
    mysql_option.ip = ip;
    mysql_option.port = option_->GetMySqlConfig()->GetMySQLPort();
    mysql_option.username = GetAdminUserName();
    mysql_option.pwd = GetAdminPwd();
    if (mysql_option.username == "") {
        mysql_option.username = "root";
        mysql_option.pwd = "";
    }
    LogVerbose("%s username %s pwd %s", __func__, mysql_option.username.c_str(), mysql_option.pwd.c_str());

    return MySQLCommand(&mysql_option).GetValue(value_type, value_key, value);
}

bool MySqlManager::IsMySqlInit() {
    return master_storage_->HasReplicaUserName();
}

bool MySqlManager::IsReplicaProcess() {
    return master_storage_->GetReplicaUserName() != "";
}

int MySqlManager::CheckUserExist(const string &username) {

    if (GetAdminUserName() == "") {
        return MYSQL_USER_NOT_EXIST;
    }

    vector < vector<string> > results;
    int ret = Query(MySqlStringHelper::GetShowUserStr(username), &results);
    if (ret == MYSQL_NODATA) {
        return MYSQL_USER_NOT_EXIST;
    } else if (ret) {
        return ret;
    }
    if (results.empty()) {
        return MYSQL_USER_NOT_EXIST;
    }
    return OK;
}

int MySqlManager::CheckUserGrantExist(const string &username, const string &pwd, const string &grant_string,
                                      const string &grant_flag_string) {

    MySqlOption mysql_option;
    mysql_option.ip = option_->GetBinLogSvrConfig()->GetEngineIP();
    mysql_option.port = option_->GetMySqlConfig()->GetMySQLPort();
    mysql_option.username = username;
    mysql_option.pwd = pwd;
    //LogVerbose("%s username %s pwd %s", __func__, mysql_option.username.c_str(), mysql_option.pwd.c_str());

    vector < vector < string >> results;
    int ret = MySQLCommand(&mysql_option).GetQueryResults(grant_string, &results);
    if (ret == MYSQL_NODATA) {
        return MYSQL_USER_NOT_EXIST;
    } else if (ret) {
        return ret;
    }
    if (results.empty()) {
        return MYSQL_USER_NOT_EXIST;
    }
    for (size_t i = 0; i < results.size(); ++i) {
        for (size_t j = 0; j < results[i].size(); ++j) {
            if (results[i][j].find(grant_flag_string) != string::npos) {
                return OK;
            }
        }
    }
    return MYSQL_USER_NOT_EXIST;
}

bool MySqlManager::CheckAdminAccount(const string &admin_username, const string &admin_pwd) {
    return admin_username == GetAdminUserName() && admin_pwd == GetAdminPwd();
}

int MySqlManager::ChangePwd(const string &username, const string &pwd) {
    return Query(MySqlStringHelper::GetChangePwdStr(username,pwd));
}

int MySqlManager::CreateUser(const string &username,const string &pwd) {
    int ret = CheckUserExist(username);
    if (ret == MYSQL_USER_NOT_EXIST) {
        ret = Query(MySqlStringHelper::GetCreateUserStr(username, pwd));
    }
    return ret;
}

int MySqlManager::CheckAdminUser(const string &username, const string &pwd) {
    if (CheckUserGrantExist(username, pwd, MySqlStringHelper::GetShowGrantString(username, "127.0.0.1"),
                            " WITH GRANT OPTION")) {
        ColorLogWarning("%s new user %s not exist in mysql, wait", __func__, username.c_str());
        return MYSQL_USER_NOT_EXIST;
    }
    return OK;
}

int MySqlManager::CheckReplicaUser(const string &username, const string &pwd) {
    if (CheckUserGrantExist(username, pwd, MySqlStringHelper::GetShowGrantString(username, "%"), "REPLICATION")) {
        ColorLogWarning("%s new user %s not exist in mysql, wait", __func__, username.c_str());
        return MYSQL_USER_NOT_EXIST;
    }
    return OK;
}

void MySqlManager::CheckMySqlUserInfo() {

    MasterInfo master_info;
    master_storage_->GetMaster(&master_info);

    string current_admin_username = master_storage_->GetAdminUserName();
    string current_admin_pwd = master_storage_->GetAdminPwd();
    string new_admin_username = "", new_admin_pwd = "";

    string current_replica_username = master_storage_->GetReplicaUserName();
    string current_replica_pwd = master_storage_->GetReplicaPwd();
    string new_replica_username = "", new_replica_pwd = "";

    for (int i = master_info.admin_userinfo_size() - 1; i >= 0; --i) {

        string pending_admin_username = master_info.admin_userinfo(i).username();
        string pending_admin_pwd = master_info.admin_userinfo(i).pwd();

        if (pending_admin_username == current_admin_username && pending_admin_pwd == current_admin_pwd) {
            new_admin_username = "";
            new_admin_pwd = "";
            break;
        }
        if (CheckAdminUser(pending_admin_username, pending_admin_pwd) == OK) {
            new_admin_username = pending_admin_username;
            new_admin_pwd = pending_admin_pwd;
            break;
        }
    }

    if (new_admin_username != "") {
        LogVerbose("%s get new admin username %s", __func__, new_admin_username.c_str());
        master_storage_->SetMySqlAdminInfo(master_info, new_admin_username, new_admin_pwd);
    }

    for (int i = master_info.replica_userinfo_size() - 1; i >= 0; --i) {

        string pending_replica_username = master_info.replica_userinfo(i).username();
        string pending_replica_pwd = master_info.replica_userinfo(i).pwd();

        if (pending_replica_username == current_replica_username && pending_replica_pwd == current_replica_pwd) {
            new_replica_username = "";
            new_replica_pwd = "";
            break;
        }
        if (CheckReplicaUser(pending_replica_username, pending_replica_pwd) == OK) {
            new_replica_username = pending_replica_username;
            new_replica_pwd = pending_replica_pwd;
            break;
        }
    }

    if (new_replica_username != "") {
        LogVerbose("%s get new replica username %s", __func__, new_replica_username.c_str());
        master_storage_->SetMySqlReplicaInfo(master_info, new_replica_username, new_replica_pwd);
    }
}

int MySqlManager::CreateAdmin(const string &admin_username, const string &admin_pwd,
                              const std::vector<std::string> &iplist) {
    LogVerbose("%s create user %s", __func__, admin_username.c_str());
    if (admin_username != "root" && admin_username != GetAdminUserName()) {
        int ret = CreateUser(admin_username, admin_pwd);
        if (ret) {
            return ret;
        }
    }
    LogVerbose("%s create user %s done", __func__, admin_username.c_str());

    if (admin_username != GetAdminUserName() || admin_pwd != GetAdminPwd()) {
        auto host_list = iplist;
        host_list.push_back("127.0.0.1");
        host_list.push_back("localhost");
        for (auto host : host_list) {
            string grant_string = MySqlStringHelper::GetGrantAdminUserStr(admin_username, admin_pwd, host);
            int ret = Query(grant_string);
            if (ret) {
                return ret;
            }
            LogVerbose("%s grant %s user %s done", __func__, grant_string.c_str(), admin_username.c_str());
        }
    }
    //not real change pwd. if change, the connection for other operation will be fail.
    return OK;
}

int MySqlManager::CreateReplica(const string &admin_username, const string &replica_username,
                                const string &replica_pwd) {
    if (replica_username != GetReplicaUserName()) {
        int ret = CreateUser(replica_username,"");
        if (ret) {
            return ret;
        }
    }
    LogVerbose("%s create user %s done", __func__, replica_username.c_str());
    if (replica_username != GetReplicaUserName() || replica_pwd != GetReplicaPwd()) {
        string grant_string = MySqlStringHelper::GetGrantReplicaUserStr(replica_username, replica_pwd);
        int ret = Query(grant_string);
        if (ret) {
            return ret;
        }
        LogVerbose("%s grant %s user %s done", __func__, grant_string.c_str(), replica_username.c_str());
    }
    return OK;
}

int MySqlManager::CreateMySqlAdminInfo(const string &now_admin_username, const string &now_admin_pwd,
                                       const string &new_username, const string &new_pwd,
                                       const vector<string> &group_list) {

    if (!CheckAdminAccount(now_admin_username, now_admin_pwd)) {
        ColorLogWarning("%s admin info check fail", __func__);
        return USERINFO_CHANGE_DENIED;
    }
    return CreateAdmin(new_username, new_pwd, group_list);
}

int MySqlManager::CreateMySqlReplicaInfo(const string &now_admin_username, const string &now_admin_pwd,
                                         const string &new_username, const string &new_pwd) {

    if (!CheckAdminAccount(now_admin_username, now_admin_pwd)) {
        ColorLogWarning("%s admin info check fail", __func__);
        return USERINFO_CHANGE_DENIED;
    }
    return CreateReplica(now_admin_username, new_username, new_pwd);
}

int MySqlManager::CheckAdminPermission(const std::vector<std::string> &member_list) {

    MasterInfo master_info;
    master_storage_->GetMaster(&master_info);
    if (master_info.admin_userinfo_size() == 0) {
        return OK;
    }

    uint32_t userinfo_len = master_info.admin_userinfo_size();
    string username = master_info.admin_userinfo(userinfo_len - 1).username();
    string pwd = master_info.admin_userinfo(userinfo_len - 1).pwd();

    for (auto member : member_list) {
        int ret = CheckUserGrantExist(username, pwd, MySqlStringHelper::GetShowGrantString(username, member),
                                      " WITH GRANT OPTION");
        if (ret == MYSQL_USER_NOT_EXIST) {
            ColorLogWarning("%s new user %s, member %s do not have grant add one", __func__, username.c_str(), member.c_str());

            string grant_string = MySqlStringHelper::GetGrantAdminUserStr(username, pwd, member);
            ret = Query(grant_string);
        }
        if (ret) {
            return ret;
        }
    }
    return OK;
}

int MySqlManager::AddMemberAdminPermission(const string &admin_username, const string &admin_pwd,
                                           const string &member_ip) {
    if (admin_username == "") {
        LogVerbose("%s admin user not be ready, skip and later check", __func__);
        return OK;
    }
    string grant_string = MySqlStringHelper::GetGrantAdminUserStr(admin_username, admin_pwd, member_ip);
    return Query(grant_string);
}

int MySqlManager::RemoveMemberAdminPermission(const string &admin_username, const string &admin_pwd,
                                              const string &member_ip) {
    if (admin_username == "") {
        ColorLogError("%s admin user not ready, please wait", __func__);
        return MYSQL_USER_NOT_EXIST;
    }
    string grant_string = MySqlStringHelper::GetRevokeAdminUserStr(admin_username, admin_pwd, member_ip);
    return Query(grant_string);
}

int MySqlManager::AddMemberAdminPermission(const std::string &member_ip) {
    return AddMemberAdminPermission(GetAdminUserName(), GetAdminPwd(), member_ip);
}

int MySqlManager::RemoveMemberAdminPermission(const std::string &member_ip) {
    return RemoveMemberAdminPermission(GetAdminUserName(), GetAdminPwd(), member_ip);
}

string MySqlManager::ReduceGtidByOne(const string &gtid) {
    pair<string, size_t> gtid_item = GtidHandler::ParseGTID(gtid);
    gtid_item.second--;
    return GtidHandler::GenGTID(gtid_item.first, gtid_item.second);
}

}
