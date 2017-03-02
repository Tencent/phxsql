/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include <string>
#include <map>
#include "eventdata.pb.h"

namespace phxbinlog {

bool operator <(const EventDataInfo &info1, const EventDataInfo &info2);

class EventFileManager;
class EventIndex;
class Option;
class EventUUIDHandler {
 public:
    EventUUIDHandler(EventFileManager *filemanager, EventIndex *event_index, const Option *option);
    EventUUIDHandler(const Option *option);
    ~EventUUIDHandler();

    int LoadFromCheckPoint(const EventData &checkpointdata);

 public:

    int UpdateEventUUIDInfoByUUID(const std::string &uuid, const EventDataInfo &data_info);
    int UpdateEventUUIDInfoByGTID(const std::string &gtid, const EventDataInfo &data_info);
    int GetLastestUUIDPos(EventDataInfo * info);

    std::string GetLastestGTIDByGTID(const std::string &gtid);
    std::string GetLastestGTIDByUUID(const std::string &uuid);
    uint64_t GetLastInstanceID();

    int GetLastestGTIDEventByGTID(const std::string &gtid, EventDataInfo *data_info);
    int GetLastestGTIDEventByUUID(const std::string &uuid, EventDataInfo *data_info);

    int FlushEventUUIDInfo();

    std::string GetLastestUpdateGTID();
    int GetLastestUpdateGTIDInfo(EventDataInfo *data_info);

    int ParseFromString(const std::string &buffer);
    int SerializeToBuffer(std::string *buffer);

 private:
    int InitUUIDHandler();
    int InitNewestInfo();

    int LoadUUIDInfo();

    int ParseFromEventInfoList(const EventDataInfoList &info);
    int SerializeToEventInfoList(EventDataInfoList *info);

    int FlushEventUUIDInfo(EventDataInfoList *list);

    static std::string GetKeyForUUID();
    static std::string GetGTIDKeyForLastRead();

 private:
    std::map<std::string, EventDataInfo> map_uuid_;
    EventFileManager *event_file_manager_;
    EventIndex *event_index_;
    EventDataInfo newest_info_;
    const Option *option_;
    uint64_t last_instanceid_;
};

}
