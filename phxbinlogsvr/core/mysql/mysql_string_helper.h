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
class MySqlStringHelper {
 public:
    static std::string GetChangeMasterQueryString(const std::string &master_ip, const uint32_t &master_port,
                                                  const std::string &replica_username, const std::string &replica_pwd);

    static std::string GetSvrIDString(const uint32_t &svr_id);

    static std::string GetCreateUserStr(const std::string &username, const std::string &pwd);
    static std::string GetChangePwdStr(const std::string &username, const std::string &pwd);
    static std::string GetGrantReplicaUserStr(const std::string &username, const std::string &pwd);
    static std::string GetGrantAdminUserStr(const std::string &root_username, const std::string &root_pwd,
                                            const std::string &svr_ip);
    static std::string GetRevokeAdminUserStr(const std::string &admin_username, const std::string &admin_pwd,
                                             const std::string &svr_ip);
	static std::string GetFlushPrivilegeStr();
    static std::string GetShowUserStr(const std::string &username);
    static std::string GetShowGrantString(const std::string &username, const std::string &ip);
};

}
