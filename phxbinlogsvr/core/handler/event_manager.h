/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/


#pragma once

#include <vector>
#include <string>
#include <stdint.h>

#include "eventdata.pb.h"

namespace phxbinlog {

class EventStorage;
class EventMonitor;
class Option;
class EventManager {
 public:
    static EventManager *GetGlobalEventManager(const Option *option);

    int SaveEvents(const uint64_t &instanceid, const std::string &gtid, const std::string &data,
                   const uint64_t &check_sum = 0);

    int GetEvents(std::string *data, uint32_t want_num = 0);

    int GetNowCheckSum(uint64_t *checksum);

    int SignUpGTID(const std::vector<std::string> &gtid_list);
    int SignUpGTID(const std::string &gtid);

    int GetGTIDInfo(const std::string &gtid, EventDataInfo *info);
    std::string GetLastSendGTID(const std::string &gtid);
    std::string GetNewestGTID();
    int GetLastGtid(const std::vector<std::string> &gtid_list, std::string *gtid);

	void Notify();

 protected:
    EventManager(const Option *option);
    ~EventManager();

    //file db
    int InitData();
    int WriteData(const std::string &gtid, const std::string &data);

 private:
    const Option *option_;
    EventMonitor *event_monitor_;
    EventStorage *event_storage_;
};

}
