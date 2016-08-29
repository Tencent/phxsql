/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once 

#include "co_routine.h"
#include "phxcoroutine.h"

#include "phxcomm/phx_log.h"

#include <atomic> 

namespace phxsqlproxy {

template<class T>
class GroupStatusCache : public Coroutine {
 public:
    GroupStatusCache() {
    }

    virtual ~GroupStatusCache() {
    }

    virtual const T & GetGroupStatus() {
        return group_status_;
    }

 private:
    int run() {
        co_enable_hook_sys();
        while (true) {
            int ret = UpdateGroupStatus(group_status_);

            if (ret != 0) {
                phxsql::LogError("UpdateGroupStatus ret %d", ret);
            }
            phxsql::LogVerbose("UpdateGroupStatus ret %d", ret);

            int sleep_ms = (ret == 0 ? 10000 : 100);
            poll(0, 0, sleep_ms);
        }
        return 0;
    }

    virtual int UpdateGroupStatus(T & group_status) = 0;

 protected:
    T group_status_;
};

}
