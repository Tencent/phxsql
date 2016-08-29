/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "phxbinlogsvr/client/phxbinlog_client_platform_info.h"

namespace phxbinlogsvr {

PhxBinlogClientPlatformInfo *PhxBinlogClientPlatformInfo::GetDefault() {
    static PhxBinlogClientPlatformInfo platforminfo;
    return &platforminfo;
}

PhxBinlogClientPlatformInfo::PhxBinlogClientPlatformInfo() {
    client_factory_ = NULL;
}

PhxBinlogClientPlatformInfo::~PhxBinlogClientPlatformInfo() {
    ClearClientFactory();
}

void PhxBinlogClientPlatformInfo::ClearClientFactory() {
    if (client_factory_)
        delete client_factory_, client_factory_ = NULL;
}

PhxBinLogClientFactoryInterface *PhxBinlogClientPlatformInfo::GetClientFactory() {
    return client_factory_;
}

void PhxBinlogClientPlatformInfo::RegisterClientFactory(PhxBinLogClientFactoryInterface *client_factory) {
    ClearClientFactory();
    client_factory_ = client_factory;
}

}

