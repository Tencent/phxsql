/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "event_file_manager.h"

#include "phxcomm/phx_log.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/define/errordef.h"

using std::string;

namespace phxbinlog {

#define EVENT_PATH_PREF "EVENTDATA"

EventFileManager::EventFileManager(const Option *option, bool read_file_list) :
        StorageFileManager(option, EVENT_PATH_PREF, read_file_list) {
}

EventFileManager::EventFileManager(const Option *option, const string &datadir) :
        StorageFileManager(option, datadir, EVENT_PATH_PREF, true) {
}

EventFileManager::~EventFileManager() {
}

int EventFileManager::ReadData(EventData *data, EventDataInfo *datainfo) {
    int ret = StorageFileManager::ReadData(data, datainfo);
    if (ret == OK) {
        if (datainfo) {
            CopyDataToDataInfo(*data, datainfo);
        }
    }
    return ret;
}

int EventFileManager::ReadDataFromFile(EventData *data) {
    return StorageFileManager::ReadData(data, NULL, false);
}

int EventFileManager::WriteData(const EventData &eventdata, EventDataInfo *datainfo, bool write_file) {
    int ret = OK;
    if (write_file)
        ret = StorageFileManager::WriteData(eventdata, datainfo);

    if (ret == OK) {
        if (datainfo) {
            CopyDataToDataInfo(eventdata, datainfo);
        }
    }
    return ret;
}

void EventFileManager::CopyDataToDataInfo(const EventData &eventdata, EventDataInfo *info) {
    info->set_checksum(eventdata.checksum());
    info->set_instance_id(eventdata.instance_id());
    info->set_gtid(eventdata.gtid());
}

bool EventFileManager::IsInitFile(const EventDataInfo &data_info) {
    string init_file_name = GenFileName(0);
    return data_info.file_name() == init_file_name;
}

}

