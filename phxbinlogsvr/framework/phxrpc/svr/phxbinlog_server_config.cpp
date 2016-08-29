/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "phxbinlog_server_config.h"

PhxbinlogServerConfig::PhxbinlogServerConfig() {
}

PhxbinlogServerConfig::~PhxbinlogServerConfig() {
}

const phxrpc::HshaServerConfig & PhxbinlogServerConfig::GetServerConfig() const {
    return mt_servet_config_;
}

void PhxbinlogServerConfig::SetBindIP(const char *sIP) {
    mt_servet_config_.SetBindIP(sIP);
}

void PhxbinlogServerConfig::SetBindPort(const int &iPort) {
    mt_servet_config_.SetPort(iPort);
}

void PhxbinlogServerConfig::SetThreadNum(const int &num) {
    mt_servet_config_.SetMaxThreads(num);
}

void PhxbinlogServerConfig::SetPackageName(const std::string &package_name) {
    mt_servet_config_.SetPackageName(package_name.c_str());
}

