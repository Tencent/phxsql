/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "phxbinlog_stub_interface.h"

#include <string>
#include <stdint.h>
#include <vector>

#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxcomm/phx_log.h"

using std::string;
using phxbinlog::Option;
using phxsql::ColorLogError;
using phxsql::LogVerbose;

namespace phxbinlogsvr {

PhxBinlogStubInterface::PhxBinlogStubInterface() {
    Option *option = Option::GetDefault();

    if (option == NULL) {
        ColorLogError("%s load config fail", __func__);
        return;
    }
    timeout_ms_ = option->GetBinLogSvrConfig()->GetTimeOutMS();
    ip_ = option->GetBinLogSvrConfig()->GetBinlogSvrIP();
    port_ = option->GetBinLogSvrConfig()->GetBinlogSvrPort();
    package_name_ = option->GetBinLogSvrConfig()->GetPackageName();
}

PhxBinlogStubInterface::PhxBinlogStubInterface(const string &ip, const uint32_t &port) {
    ip_ = ip;
    port_ = port;

    Option *option = Option::GetDefault();
    if (option == NULL) {
        ColorLogError("%s load config fail", __func__);
        return;
    }
    timeout_ms_ = option->GetBinLogSvrConfig()->GetTimeOutMS();
    package_name_ = option->GetBinLogSvrConfig()->GetPackageName();
}

PhxBinlogStubInterface::PhxBinlogStubInterface(const string &ip, const uint32_t &port, const uint32_t timeout_ms) {
    ip_ = ip;
    port_ = port;
    timeout_ms_ = timeout_ms;
    package_name_ = "";
}

void PhxBinlogStubInterface::SetPackageName(const string &package_name) {
    package_name_ = package_name;
}

string PhxBinlogStubInterface::GetPackageName() const {
    return package_name_;
}

PhxBinlogStubInterface::~PhxBinlogStubInterface() {
}

uint32_t PhxBinlogStubInterface::GetTimeOutMS() {
    return timeout_ms_;
}

void PhxBinlogStubInterface::SetTimeOutMS(const uint32_t &ms) {
    timeout_ms_ = ms;
}

string PhxBinlogStubInterface::GetIP() {
    return ip_;
}

uint32_t PhxBinlogStubInterface::GetPort() {
    return port_;
}

}

