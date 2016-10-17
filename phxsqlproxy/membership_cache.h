/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include "group_status_cache.h"

#include <memory>
#include <vector>
#include <string>

namespace phxsqlproxy {

class MembershipCache : public GroupStatusCache<std::vector<std::string> > {
 public:
    MembershipCache();

    virtual ~MembershipCache();

    bool IsInMembership(const std::string & ip);
    int GetMemberExcept(const std::string & ip, std::string & result);

 private:
    int UpdateGroupStatus(std::vector<std::string> & group_status);
};

}
