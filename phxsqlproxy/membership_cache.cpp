/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "phxsqlproxyutil.h"
#include "monitor_plugin.h"
#include "group_status_cache.h"
#include "membership_cache.h"

#include "phxbinlogsvr/client/phxbinlog_client_platform_info.h"
#include "phxcomm/phx_log.h"
#include "phxcomm/lock_manager.h"

#include <memory>
#include <vector>
#include <string>

using namespace std;
using phxbinlogsvr::PhxBinlogClientPlatformInfo;
using phxbinlogsvr::PhxBinlogClient;
using phxsql::LogVerbose;
using phxsql::LogError;

namespace phxsqlproxy {

//begin
MembershipCache::MembershipCache() {
}

MembershipCache::~MembershipCache() {
}

int MembershipCache::UpdateGroupStatus(std::vector<std::string> & group_status) {
    std::shared_ptr<PhxBinlogClient> client = PhxBinlogClientPlatformInfo::GetDefault()->GetClientFactory()
            ->CreateClient();
    std::vector < std::string > membership;
    uint32_t port = 0;
    int ret = client->GetMemberList(&membership, &port);

    if (ret == 0 && membership.size() == 0) {
        phxsql::LogError("%s:%d ret 0 but no member inside", __func__, __LINE__);
        ret = -__LINE__;
    }

    if (ret == 0) {
        phxsql::RWLockManager lock(&mutex_, phxsql::RWLockManager::WRITE);
        group_status = std::move(membership);
        phxsql::LogVerbose("%s:%d get membership ret size %zu", __func__, __LINE__, group_status.size());
    }

    return ret;
}

bool MembershipCache::IsInMembership(const std::string & ip) {
    phxsql::RWLockManager lock(&mutex_, phxsql::RWLockManager::READ);
    const std::vector<std::string> & ip_list = GetGroupStatus();
    for (auto itr : ip_list) {
        if (itr == ip) {
            return true;
        }
    }
    return false;
}

int MembershipCache::GetMemberExcept(const std::string & ip, std::string & result) {
    phxsql::RWLockManager lock(&mutex_, phxsql::RWLockManager::READ);
    const std::vector<std::string> & ip_list = GetGroupStatus();
    for (size_t i = 0; i < ip_list.size(); ++i) {
        if (ip_list[i] == ip) {
            result = ip_list[(i + 1) % ip_list.size()];
            return 0;
        }
    }
    return -1;
}

}

