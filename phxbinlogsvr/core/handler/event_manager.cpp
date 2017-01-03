/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "event_manager.h"

#include "event_monitor.h"
#include "event_storage.h"
#include "storage_manager.h"

#include "phxcomm/phx_log.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/define/errordef.h"

#include <unistd.h>

using std::vector;
using std::string;
using phxsql::LogVerbose;
using phxsql::ColorLogInfo;

namespace phxbinlog {

EventManager *EventManager::GetGlobalEventManager(const Option *option) {
    static EventManager manager(option);
    return &manager;
}

EventManager::EventManager(const Option *option) {
    StorageManager *storage_manager = StorageManager::GetGlobalStorageManager(option);
    event_storage_ = storage_manager->GetEventStorage();
    event_monitor_ = new EventMonitor(option);
    option_ = option;
}

EventManager::~EventManager() {
    delete event_monitor_;
}

int EventManager::GetLastGtid(const vector<string> &gtid_list, string *gtid) {
    return event_storage_->GetLastGtid(gtid_list, gtid);
}

int EventManager::SignUpGTID(const vector<string> &gtid_list) {
    string gtid = "";
    int ret = GetLastGtid(gtid_list, &gtid);
    if (ret) {
        return ret;
    }
    return SignUpGTID(gtid);
}

int EventManager::SignUpGTID(const string &gtid) {
    return event_storage_->ReSet(gtid);
}

string EventManager::GetLastSendGTID(const string &gtid) {
    return event_storage_->GetLastGTIDByGTID(gtid);
}

int EventManager::GetGTIDInfo(const string &gtid, EventDataInfo *info) {
    return event_storage_->GetGTIDInfo(gtid, info, false);
}

string EventManager::GetNewestGTID() {
    return event_storage_->GetNewestGTID();
}

int EventManager::GetNowCheckSum(uint64_t *checksum) {
    EventDataInfo datainfo;
    int ret = event_storage_->GetNewestGTIDInfo(&datainfo);
    if (ret) {
        return ret;
    }
    *checksum = datainfo.checksum();
    return ret;
}

int EventManager::SaveEvents(const uint64_t &instanceid, const string &gtid, const string &data,
                             const uint64_t &check_sum) {
    if (gtid.empty()) {
        ColorLogInfo("%s no gtid data, skip", __func__);
        return OK;
    }

    EventData event_data;
    event_data.set_gtid(gtid);
    event_data.set_data(data);
    event_data.set_checksum(check_sum);
    event_data.set_instance_id(instanceid);

    return event_storage_->AddEvent(gtid, event_data);
}

int EventManager::GetEvents(string *data, uint32_t want_num) {
    if (want_num == 0) {
        want_num = option_->GetBinLogSvrConfig()->GetMaxEventCountToPush();
    }

    size_t get_num = 0;
    int ret = OK;
    while (get_num < want_num) {
        EventData event_data;
        ret = event_storage_->GetEvent(&event_data, get_num == 0);
        if (ret) {
            if (ret == EVENT_EMPTY) {
                if (get_num == 0) {
                    ColorLogInfo("%s no data", __func__);
                } else {
                    ret = OK;
                    break;
                }
            }
            break;
        }
        LogVerbose("get event %s data size %zu", event_data.gtid().c_str(), event_data.data().size());
        if (event_data.data().size() > 0)
            data->append(event_data.data());
        ++get_num;
    }
    return ret;
}

void EventManager::Notify() {
    event_storage_->Notify();
}

}
