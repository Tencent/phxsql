/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "mysql_string_helper.h"

#include "phxcomm/phx_log.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/define/errordef.h"

#include <string>
#include <string.h>

using std::string;
using std::vector;
using phxsql::LogVerbose;

namespace phxbinlog {

string MySqlStringHelper::GetChangeMasterQueryString(const string &master_ip, const uint32_t &master_port,
                                                     const string &replica_username, const string &replica_pwd) {
    string connect_master_ip = master_ip;
    uint32_t connect_master_port = master_port;

    char cmd[1024];
    sprintf(cmd, "change master to master_host='%s',master_port=%u,"
            "master_user='%s',master_password='%s',master_heartbeat_period = 20,MASTER_AUTO_POSITION = 1;",
            connect_master_ip.c_str(), connect_master_port, replica_username.c_str(), replica_pwd.c_str());

    return cmd;
}

string MySqlStringHelper::GetSvrIDString(const uint32_t &svr_id) {
    char cmd[1024];
    sprintf(cmd, "set global server_id=%u;", svr_id);
    return cmd;
}

string MySqlStringHelper::GetCreateUserStr(const string &username, const string &pwd) {
    char cmd[1024];
	if(pwd.empty()) {
		sprintf(cmd, "create user %s;", username.c_str());
	}
	else {
		sprintf(cmd, "create user %s@'127.0.0.1' identified by '%s';", username.c_str(), pwd.c_str());
	}
    return cmd;
}

string MySqlStringHelper::GetRevokeAdminUserStr(const string &admin_username, const string &admin_pwd,
                                                const string &svr_ip) {
    char cmd[1024];
	sprintf(cmd, "revoke all privileges on *.* from '%s'@'%s';", admin_username.c_str(), svr_ip.c_str());
    return cmd;
}

string MySqlStringHelper::GetGrantAdminUserStr(const string &admin_username, const string &admin_pwd,
                                               const string &svr_ip) {
    char cmd[1024];
    if (admin_pwd == "") {
        sprintf(cmd, "grant all privileges on *.* to '%s'@'%s' with grant option;", admin_username.c_str(),
                svr_ip.c_str());
    } else {
        sprintf(cmd, "grant all privileges on *.* to '%s'@'%s' identified by '%s' with grant option;",
                admin_username.c_str(), svr_ip.c_str(), admin_pwd.c_str());
    }
    return cmd;
}

string MySqlStringHelper::GetGrantReplicaUserStr(const string &username, const string &pwd) {
    char cmd[1024];
    sprintf(cmd, "grant replication slave on *.* to '%s'@'%%' identified by '%s';", username.c_str(), pwd.c_str());
    return cmd;

}

string MySqlStringHelper::GetShowUserStr(const string &username) {
    char cmd[1024];
    sprintf(cmd, "select user from mysql.user where user='%s';", username.c_str());
    return cmd;
}

string MySqlStringHelper::GetShowGrantString(const string &username, const string &ip) {
    char cmd[1024];
    sprintf(cmd, "show grants for %s@'%s';", username.c_str(), ip.c_str());
    return cmd;
}

string MySqlStringHelper::GetChangePwdStr(const string &username, const string &pwd) {
    char cmd[1024];
	sprintf(cmd, "update mysql.user SET password=password('%s') where user='%s';", 
			pwd.c_str(), username.c_str());
    return cmd;
}

string MySqlStringHelper::GetFlushPrivilegeStr() {
    char cmd[1024];
	sprintf(cmd, "flush privileges;");
    return cmd;
}

}
