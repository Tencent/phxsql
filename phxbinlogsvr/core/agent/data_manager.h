/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include <string>

namespace phxbinlog {

class ReplicationManager;
class EventAgent;
class EventManager;
class Option;
class DataManager {
 public:
    DataManager(const Option *option);
    ~DataManager();

    int SendEvent(const std::string &oldgtid, const std::string &newgtid, const std::string &event_buffer);
    int GetLastSendGTID(const std::string &uuid, std::string *gtid);
    int FlushData();

    void DealWithSlave(const int fd);

 private:
    EventAgent *event_agent_;
    EventManager *event_manager_;
    ReplicationManager *replica_manager_;
    const Option *option_;
};

}
