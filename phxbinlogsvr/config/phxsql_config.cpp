/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "phxbinlogsvr/config/phxsql_config.h"

#include "phxcomm/phx_log.h"

using std::string;

namespace phxbinlog {

PhxMySqlConfig::PhxMySqlConfig(const char *filename) {
    mysql_port_ = 0;
    mysql_ip_ = "127.0.0.1";
    ReadFile(filename);
}

PhxMySqlConfig::~PhxMySqlConfig() {
}

int PhxMySqlConfig::GetMySQLPort() const {
    return mysql_port_;
}

const char * PhxMySqlConfig::GetMySQLIP() const {
    return mysql_ip_.c_str();
}

const char * PhxMySqlConfig::GetMySQLSocket() const {
    return mysql_socket_.c_str();
}

void PhxMySqlConfig::ReadConfig() {
    mysql_port_ = GetInteger("mysqld", "port", 4306);
    mysql_socket_ = Get("mysqld", "socket", "");
    phxsql::LogVerbose("%s read mysql port %d", __func__, mysql_port_);
}

}

