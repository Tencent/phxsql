/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "event_uuid_handler.h"

#include "event_file_manager.h"
#include "event_index.h"

#include "phxcomm/phx_log.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/core/gtid_handler.h"
#include "phxbinlogsvr/define/errordef.h"

using std::string;
using phxsql::LogVerbose;
using phxsql::ColorLogInfo;
using phxsql::ColorLogWarning;
using phxsql::ColorLogError;

namespace phxbinlog {

bool operator <(const EventDataInfo &info1, const EventDataInfo &info2) {
    return info1.instance_id() < info2.instance_id();
}

EventUUIDHandler::EventUUIDHandler(EventFileManager *filemanager, EventIndex *event_index, const Option *option) {
    event_file_manager_ = filemanager;
    event_index_ = event_index;
    option_ = option;
}

EventUUIDHandler::EventUUIDHandler(const Option *option) {
    event_file_manager_ = NULL;
    event_index_ = NULL;
    option_ = option;
}

int EventUUIDHandler::LoadFromCheckPoint(const EventData &checkpointdata) {
    int ret = EventUUIDHandler::ParseFromString(checkpointdata.data());
    if (ret) {
        return ret;
    }
    ColorLogInfo("%s init done ret %d newest gtid %s last instance id %llu", __func__, ret, newest_info_.gtid().c_str(),
                 newest_info_.instance_id());
    return ret;
}

EventUUIDHandler::~EventUUIDHandler() {
}

string EventUUIDHandler::GetKeyForUUID() {
    return "gtid_uuid";
}

string EventUUIDHandler::GetGTIDKeyForLastRead() {
    return "gtid_last_read";
}

int EventUUIDHandler::InitUUIDHandler() {
    EventDataInfoList list;
    int ret = FILE_FAIL;
    event_index_status status = event_index_->GetGTID(GetKeyForUUID(), &list);
    if (status == event_index_status::DATA_NOT_FOUND) {
        ret = OK;
    } else if (status == event_index_status::OK) {
        ret = ParseFromEventInfoList(list);
    } else {
        ret = FILE_FAIL;
    }

    if (ret == OK) {
        ret = InitNewestInfo();
        if (ret) {
            ColorLogWarning("%s init newest info fail ret %d", __func__, ret);
            return ret;
        }

        ret = LoadUUIDInfo();
        for (auto &it : map_uuid_) {
            LogVerbose("%s uuid %s-%s", __func__, it.first.c_str(), it.second.gtid().c_str());
        }
    }
    ColorLogInfo("%s init done ret %d newest gtid %s last instance id %llu", __func__, ret, newest_info_.gtid().c_str(),
                 newest_info_.instance_id());

    return ret;
}

int EventUUIDHandler::InitNewestInfo() {
    return GetLastestUUIDPos(&newest_info_);
}

int EventUUIDHandler::LoadUUIDInfo() {
    int ret = FILE_FAIL;
    if (newest_info_.file_name().empty()) {
        ColorLogInfo("%s no data, open old file", __func__);
        ret = event_file_manager_->OpenOldestFile();
    } else {
        ColorLogInfo("%s open file %s, set offset %d", __func__, newest_info_.file_name().c_str(),
                     newest_info_.offset());
        ret = event_file_manager_->SetReadPos(newest_info_);
    }
    ColorLogInfo("%s newest info gtid %s instance_id %llu ret %d", __func__, newest_info_.gtid().c_str(),
                 newest_info_.instance_id(), ret);
    while (true) {
        EventDataInfo data_info;
        EventData event_data;
        int ret = event_file_manager_->ReadData(&event_data, &data_info);
        if (ret == DATA_EMPTY) {
            return OK;
        } else if (ret == OK) {
            ret = UpdateEventUUIDInfoByGTID(event_data.gtid(), data_info);
        }
        if (ret) {
            return ret;
        }
    }
    return FILE_FAIL;
}

int EventUUIDHandler::ParseFromEventInfoList(const EventDataInfoList &info) {
    map_uuid_.clear();
    for (int i = 0; i < info.info_list_size(); ++i) {
        map_uuid_[GtidHandler::GetUUIDByGTID(info.info_list(i).gtid())] = info.info_list(i);
        LogVerbose("%s get gtid %s instance id %u", __func__, info.info_list(i).gtid().c_str(),
                   info.info_list(i).instance_id());

        if (newest_info_ < info.info_list(i)) {
            newest_info_ = info.info_list(i);
            LogVerbose("%s get lastest %s instance %llu", __func__, newest_info_.gtid().c_str(),
                       newest_info_.instance_id());
        }
    }
    LogVerbose("%s get uuid list size %zu", __func__, map_uuid_.size());
    return OK;
}

int EventUUIDHandler::SerializeToEventInfoList(EventDataInfoList *info) {
    for (const auto &it : map_uuid_) {
        *info->add_info_list() = it.second;
        LogVerbose("%s get gtid %s instance id %u", __func__, it.second.gtid().c_str(), it.second.instance_id());
    }
    return OK;
}

int EventUUIDHandler::UpdateEventUUIDInfoByGTID(const string &gtid, const EventDataInfo &data_info) {
    string uuid = GtidHandler::GetUUIDByGTID(gtid);
    return UpdateEventUUIDInfoByUUID(uuid, data_info);
}

int EventUUIDHandler::UpdateEventUUIDInfoByUUID(const string &uuid, const EventDataInfo &data_info) {
    //LogVerbose("%s update gtid uuid %s-%s instance id %llu", __func__, uuid.c_str(), data_info.gtid().c_str(),
     //          data_info.instance_id());
    map_uuid_[uuid] = data_info;
    newest_info_ = data_info;
    return OK;
}

int EventUUIDHandler::GetLastestUUIDPos(EventDataInfo *info) {
    for (const auto &it : map_uuid_) {
        if (*info < it.second) {
            *info = it.second;
            LogVerbose("%s get lastest %s instance %llu", __func__, it.second.gtid().c_str(), it.second.instance_id());
        }
    }
    return OK;
}

string EventUUIDHandler::GetLastestGTIDByGTID(const string &gtid) {
    string uuid = GtidHandler::GetUUIDByGTID(gtid);
    return GetLastestGTIDByUUID(uuid);
}

int EventUUIDHandler::GetLastestGTIDEventByGTID(const string &gtid, EventDataInfo *data_info) {
    string uuid = GtidHandler::GetUUIDByGTID(gtid);
    return GetLastestGTIDEventByUUID(uuid, data_info);
}

string EventUUIDHandler::GetLastestGTIDByUUID(const string &uuid) {
    auto it = map_uuid_.find(uuid);
    if (it == map_uuid_.end())
        return "";
    return it->second.gtid();
}

int EventUUIDHandler::GetLastestGTIDEventByUUID(const string &uuid, EventDataInfo *data_info) {
    auto it = map_uuid_.find(uuid);
    if (it == map_uuid_.end())
        return 1;
    *data_info=it->second;
	return OK;
}


string EventUUIDHandler::GetLastestUpdateGTID() {
    return newest_info_.gtid();
}

int EventUUIDHandler::GetLastestUpdateGTIDInfo(EventDataInfo *info) {
    *info = newest_info_;
    return OK;
}

int EventUUIDHandler::FlushEventUUIDInfo() {
    EventDataInfoList list;
    return FlushEventUUIDInfo(&list);
}

int EventUUIDHandler::FlushEventUUIDInfo(EventDataInfoList *list) {
    int ret = SerializeToEventInfoList(list);
    if (ret)
        return ret;

    if (list->info_list_size() == 0) {
        ColorLogInfo("%s flush list %zu empty skip", __func__, list->info_list_size());
        return OK;
    }

    event_index_status status = event_index_->SetGTIDIndex(GetKeyForUUID(), *list);
    if (status != event_index_status::OK) {
        return FILE_FAIL;
    }
    ColorLogInfo("%s flush list %zu", __func__, list->info_list_size());
    return OK;
}

uint64_t EventUUIDHandler::GetLastInstanceID() {
    return newest_info_.instance_id();
}

int EventUUIDHandler::SerializeToBuffer(string *buffer) {
    EventDataInfoList list;
    int ret = FlushEventUUIDInfo(&list);
    if (ret) {
        ColorLogError("%s flush event fail", __func__);
        return ret;
    }
    if (!list.SerializeToString(buffer)) {
        ColorLogError("%s to buffer fail", __func__);
        return BUFFER_FAIL;
    }
    return OK;
}

int EventUUIDHandler::ParseFromString(const string &buffer) {
    if (buffer.empty()) {
        return OK;
    }

    EventDataInfoList list;
    if (!list.ParseFromString(buffer)) {
        ColorLogError("%s from buffer fail", __func__);
        return BUFFER_FAIL;
    }
    return ParseFromEventInfoList(list);
}

}
