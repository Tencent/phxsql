/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include "replication_base.h"

namespace phxbinlog {

class EventAgent;
class Option;
class EventManager;
class MasterManager;
class ReplicationImpl {
 public:
    ReplicationImpl(const Option *option, const int &slave_fd);
    virtual ~ReplicationImpl();

 public:
    virtual int Process();
    void Close();

 private:
    int InitFakeMaster();

 private:
    ReplicationCtx *ctx_;
    const Option *option_;
    EventManager *event_manager_;
    MasterManager *master_manager_;
};

}
