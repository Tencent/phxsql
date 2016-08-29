/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include "group_status_cache.h"

#include <string>
#include <inttypes.h>

namespace phxsqlproxy {

class PHXSqlProxyConfig;

//master cache
typedef struct tagMasterStatus {
    std::string master_ip_;
    uint64_t expired_time_;
} MasterStatus_t;

class MasterCache : public GroupStatusCache<MasterStatus_t> {
 public:
    MasterCache(PHXSqlProxyConfig * config);

    virtual ~MasterCache();

    int GetMaster(std::string & master_ip, uint64_t & expired_time);

 private:
    int UpdateGroupStatus(MasterStatus_t & group_status);

    bool IsMasterValid(const std::string & master_ip, uint64_t expired_time);

 private:
    PHXSqlProxyConfig * config_;
};

}

