/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "event_storage.h"

#include "event_file_manager.h"
#include "event_index.h"
#include "event_uuid_handler.h"
#include "master_storage.h"

#include "phxcomm/lock_manager.h"
#include "phxcomm/phx_timer.h"
#include "phxcomm/phx_utils.h"
#include "phxcomm/phx_log.h"
#include "phxbinlogsvr/statistics/phxbinlog_stat.h"
#include "phxbinlogsvr/core/gtid_handler.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/define/errordef.h"

using std::string;
using std::vector;
using std::pair;
using std::make_pair;
using phxsql::LockManager;
using phxsql::PhxTimer;
using phxsql::Utils;
using phxsql::LogVerbose;
using phxsql::ColorLogInfo;
using phxsql::ColorLogError;
using phxsql::ColorLogWarning;

namespace phxbinlog {

EventStorage::EventStorage(const Option *option, MasterStorage *masterstorage) {
    Utils::CheckDir(option->GetBinLogSvrConfig()->GetEventDataStorageDBDir());

    option_ = option;
    master_storage_ = masterstorage;

    pthread_mutex_init(&mutex_, NULL);
    pthread_cond_init(&cond_, NULL);

    checkpoint_id = -1;

    Start();
}

EventStorage::~EventStorage() {
    pthread_cond_destroy(&cond_);
    pthread_mutex_destroy(&mutex_);

    delete event_index_;
    delete event_file_manager_;
    delete uuid_handler_;
}

int EventStorage::Close() {
    if (event_file_manager_) {
        delete event_file_manager_, event_file_manager_ = NULL;
    }
    if (event_index_) {
        delete event_index_, event_index_ = NULL;
    }
    if (uuid_handler_) {
        delete uuid_handler_, uuid_handler_ = NULL;
    }
    return OK;
}

int EventStorage::Start() {
    event_file_manager_ = new EventFileManager(option_);
    event_index_ = new EventIndex(option_->GetBinLogSvrConfig()->GetEventDataStorageDBDir());
    uuid_handler_ = new EventUUIDHandler(event_file_manager_, event_index_, option_);
    return OK;
}

int EventStorage::Restart() {
    Close();
    Start();
    return OK;
}

const string &EventStorage::GetDataDir() const {
    return event_file_manager_->GetDataDir();
}

int EventStorage::InitCheckPointDataFile(const string &checkpointfile) {
    if (checkpointfile.size() == 0) {
        event_file_manager_->SwitchNewWriteFile(0);
        return 0;
    }
    EventDataInfo info;
    info.set_file_name(checkpointfile);
    info.set_offset(0);

    int ret = event_file_manager_->SetReadPos(info);
    if (ret) {
        return ret;
    }

    vector < pair<string, EventDataInfo> > data_list;
    int num = 0;
    LockManager lock(&mutex_);
    checkpoint_data_.clear();
    EventDataInfo last_data_info;
    EventDataInfo last_info;
    while (true) {
        EventDataInfo data_info;
        EventData data;
        int ret = event_file_manager_->ReadData(&data, &data_info);
        data_list.push_back(make_pair(data.gtid(), data_info));
        if (ret) {
            ColorLogInfo("%s read done gtid %s read offset %u file offset %u ", __func__,
                                last_data_info.gtid().c_str(), last_data_info.offset(), data_info.offset());
            if (last_data_info.offset() == 0) {
                if (ret == DATA_EMPTY) {
                    //only header exists
                    event_file_manager_->SetWritePos(data_info, true);
                } else {
                    return ret;
                }
            } else {
                event_file_manager_->SetWritePos(last_data_info, true);
            }
            break;
        }
        ++num;
        if (ret == OK) {
            if (last_data_info.gtid().size()) {
                checkpoint_data_[last_data_info.gtid()] = last_data_info;
            }
            last_data_info = data_info;
            continue;
        }
        return ret;
    }

    ColorLogInfo("%s get check point data size %zu, real size %u, set last gtid %s, gtid size %zu", __func__,
                        checkpoint_data_.size(), num, last_data_info.gtid().c_str(), data_list.size());

    return OK;
}

int EventStorage::LoadFromCheckPoint(const EventData &checkpoint_data, const string &checkpoint_file) {
    LogVerbose("%s checkpoint data file %s instance id %d", __func__, checkpoint_data.event_data_file().c_str(),
                       checkpoint_data.instance_id());
    int ret = uuid_handler_->LoadFromCheckPoint(checkpoint_data);
    if (ret) {
        return ret;
    }

    ret = InitCheckPointDataFile(checkpoint_file);
    if (ret == 0) {
        if (checkpoint_data.event_data_file().size() > 0)
            SetInstanceID(checkpoint_data.instance_id());
        ColorLogInfo("%s load done, last check instance id %lu, check point data preload size %zu", __func__,
                            GetLastestCheckPointInstanceID(), checkpoint_data_.size());
    }
    return ret;
}

uint64_t EventStorage::GetCurrentInstanceInterval() {
    LockManager lock(&mutex_);
    uint64_t last_instanceid = uuid_handler_->GetLastInstanceID();
    uint64_t oldest_instanceid = event_file_manager_->GetOldestInstanceIDofFile();
	ColorLogInfo("%s %p get newest instance id %llu oldest instanceid %llu",
			__func__, this, last_instanceid, oldest_instanceid); 
	if(last_instanceid < oldest_instanceid) return 0;
	return last_instanceid - oldest_instanceid;
}

int EventStorage::AddEvent(const string &gtid, const EventData &event_data, bool switch_file) {
    PhxTimer timer;
    LockManager lock(&mutex_);

    uint64_t last_instanceid = uuid_handler_->GetLastInstanceID();

    LogVerbose("%s add event gtid %s instance id %lu, laststanceid %lu", __func__, gtid.c_str(),
                       event_data.instance_id(), last_instanceid);
    if (event_data.instance_id() <= last_instanceid) {
		STATISTICS(GtidEventExit());
        ColorLogWarning("%s gtid %s exist, last instance id %llu, current instance id %llu", __func__,
                             gtid.c_str(), last_instanceid, event_data.instance_id());
        return EVENT_EXIST;
    }

    EventDataInfo data_info;
    int ret = RealCheckGTID(gtid, &data_info);
    if (ret != DATA_EMPTY && ret != OK) {
        return ret;
    }

    bool write_file = false;
    if (ret != 0) {
        write_file = true;
    }

    ret = event_file_manager_->WriteData(event_data, &data_info, write_file);
    LogVerbose("%s get gtid %s ret %d, write data gtid %s, check sum %lu file_name %s offset %u", __func__,
                       gtid.c_str(), ret, data_info.gtid().c_str(), data_info.checksum(), data_info.file_name().c_str(),
                       data_info.offset());

    if (ret == 0) {
        uuid_handler_->UpdateEventUUIDInfoByGTID(gtid, data_info);
        event_index_status status = event_index_->SetGTIDIndex(gtid, data_info);
        if (status != event_index_status::OK) {
            return FILE_FAIL;
        }
        if (write_file) {
            Notify();
            if (switch_file)
                CheckAndSwitchFile();
        }
    } else {
		STATISTICS(GtidEventAddEventFail());
	}
    ColorLogInfo("%s write data gtid %s instance id %llu checksum %llu ret %d file %s run %u ms", 
			__func__, event_data.gtid().c_str(), event_data.instance_id(), event_data.checksum(), ret,
                        data_info.file_name().c_str(), timer.GetTime()/1000 );
    return ret;
}

int EventStorage::ReSet(const string &gtid) {
    LockManager lock(&mutex_);

    if (gtid == "") {
        return event_file_manager_->OpenOldestFile();
    } else {
        EventDataInfo data_info;
        int ret = RealGetLowerBoundGTIDInfo(gtid, &data_info);
        if (ret == DATA_EMPTY) {
			STATISTICS(GtidEventResetGtidFilePosFail());
            ColorLogWarning("%s gtid not found", __func__, gtid.c_str());
            return GTID_FAIL;
        } else if (ret == OK) {
            ColorLogInfo("%s reset gtid %s file_name %s offset %d", __func__, gtid.c_str(),
                                data_info.file_name().c_str(), data_info.offset());
            return event_file_manager_->SetReadPos(data_info);
        } else {
            return FILE_FAIL;
        }
    }
    return OK;
}

int EventStorage::GetEvent(EventData *data, bool wait) {
    int ret = FILE_FAIL;
    LockManager lock(&mutex_);
    while (true) {
        ret = event_file_manager_->ReadData(data);
        if (ret == DATA_EMPTY && wait) {
            ColorLogInfo("no data wait");
            Wait();
        }
        if (ret != OK && ret != DATA_EMPTY)
			STATISTICS(GtidEventGetEventFail());
        if (ret == OK && data->gtid().empty()) {
            ColorLogInfo("%s get gtid header skip, ret %d", __func__, ret);
            continue;
        }
        break;
    }
    return ret;
}

int EventStorage::GetGTIDInfo(const string &gtid, EventDataInfo *data_info, bool lower_bound) {

    if (lower_bound) {
		int ret = OK;
		{
			LockManager lock(&mutex_);
			ret = RealGetLowerBoundGTIDInfo(gtid, data_info);
		}
        if (ret == OK) {
            ret = EventStorage::CheckGtidInData(gtid, *data_info, option_);
        }
        return ret;
    } else {
		LockManager lock(&mutex_);
        return RealGetGTIDInfo(gtid, data_info);
    }
}

string EventStorage::GetLastGTIDByUUID(const string &uuid) {
    LockManager lock(&mutex_);
    return uuid_handler_->GetLastestGTIDByUUID(uuid);
}

string EventStorage::GetLastGTIDByGTID(const string &gtid) {
    LockManager lock(&mutex_);
    return uuid_handler_->GetLastestGTIDByGTID(gtid);
}

string EventStorage::GetNewestGTID() {
    LockManager lock(&mutex_);
    return uuid_handler_->GetLastestUpdateGTID();
}

int EventStorage::GetNewestGTIDInfo(EventDataInfo *info) {
    LockManager lock(&mutex_);
    return uuid_handler_->GetLastestUpdateGTIDInfo(info);
}

uint64_t EventStorage::GetLastInstanceID() {
    LockManager lock(&mutex_);
    return uuid_handler_->GetLastInstanceID();
}


void EventStorage::RemoveFile(const string &file_name) {
	LockManager lock(&mutex_);
	event_file_manager_->RemoveFile(file_name);
}

int EventStorage::DelCheckPointFile(const string &maxfilename, const uint32_t &mintime, vector<string> *delfiles) {
    EventFileManager eventfilemanager(option_, true);
    int ret = eventfilemanager.DelCheckPointFile(maxfilename, mintime, delfiles);
    LogVerbose("%s del check point size %zu", __func__, delfiles->size());
    if (ret == 0) {
        for (auto file_name = delfiles->begin(); file_name != delfiles->end(); ++file_name) {
            EventDataInfo info;
            info.set_file_name(*file_name);
            info.set_offset(0);
            int ret = eventfilemanager.SetReadPos(info);
            if (ret) {
                return ret;
            }
            while (true) {
                EventData data;
                ret = eventfilemanager.ReadDataFromFile(&data);
                if (ret == DATA_EMPTY) {
                    break;
                }
                if (ret == DATA_ERR) {
                    break;
                }
                if (data.gtid().size()) {
                    event_index_status status = event_index_->DeleteGTIDIndex(data.gtid());
                    if (status != event_index_status::OK) {
                        return FILE_FAIL;
                    }
					usleep(100);
                }
            }
			RemoveFile(*file_name);
        }
    }
    return OK;
}

int EventStorage::CheckGtidInData(const string &gtid, const EventDataInfo &data_info, const Option * option) {
    int ret = OK;

    if (gtid == "" || data_info.gtid() == "" || data_info.file_name() == "") {
        EventFileManager eventfilemanager(option);
        ret = eventfilemanager.OpenOldestFile();
        if (ret) {
            return ret;
        }

        EventDataInfo old_data_info;
        eventfilemanager.GetReadFileInfo(&old_data_info);

        if (eventfilemanager.IsInitFile(old_data_info)) {
            return OK;
        }

        return DATA_EMPTY;
    }

    EventFileManager event_file_manager(option, false);
    ret = event_file_manager.SetReadPos(data_info);
    if (ret) {
        return ret;
    }

    EventData event_data;
    EventDataInfo event_data_info;
    ret = event_file_manager.ReadData(&event_data, &event_data_info);
    if (ret) {
        return ret;
    }

    //LogVerbose("%s get gtid %s from gtid %s, data size %zu", __func__, gtid.c_str(), event_data.gtid().c_str(),
     //                  event_data.data().size());

    vector < string > gtid_list;
    string max_gtid;
    ret = GtidHandler::ParseEventList(event_data.data(), NULL, false, NULL, &gtid_list);
    if (ret) {
        return ret;
    }

    for (auto it : gtid_list) {
        if (gtid == it) {
            return OK;
        }
    }
    ColorLogWarning("%s gtid %s not exist", __func__, gtid.c_str());

    return DATA_EMPTY;
}

int EventStorage::GetLastGtid(const vector<string> &gtid_list, string *gtid) {
    LockManager lock(&mutex_);
    string lastest_gtid = "";
    EventDataInfo max_data_info;
    int ret = DATA_EMPTY;
    if (gtid_list.empty()) {
        EventDataInfo data_info;
        int check_ret = EventStorage::CheckGtidInData("", data_info, option_);
        if (check_ret == 0) {
            LogVerbose("%s check empty gtid from file_name %s offset %d", __func__,
                                data_info.file_name().c_str(), data_info.offset());
            ret = OK;
        } else {
            LogVerbose("%s check empty gtid fail", __func__);
        }
        LogVerbose("%s check done ret = %d", __func__, ret);
    } else {
        for (size_t i = 0; i < gtid_list.size(); ++i) {
            if (max_data_info.gtid().size()) {
                pair < string, size_t > max_info_pair = GtidHandler::ParseGTID(max_data_info.gtid());
                pair < string, size_t > info_pair = GtidHandler::ParseGTID(gtid_list[i]);
                if (max_info_pair.first == info_pair.first && max_info_pair.second > info_pair.second) {
                    continue;
                }
            }

            EventDataInfo data_info;
            int find_ret = RealGetLowerBoundGTIDInfo(gtid_list[i], &data_info);
            if (find_ret == OK) {
                //LogVerbose("%s get gtid info %s file_name %s offset %d", __func__, data_info.gtid().c_str(),
                 //                  data_info.file_name().c_str(), data_info.offset());
                if (max_data_info < data_info) {
                    int check_ret = EventStorage::CheckGtidInData(gtid_list[i], data_info, option_);
                    if (check_ret == 0) {
                        max_data_info = data_info;
                        lastest_gtid = gtid_list[i];
                        LogVerbose("%s get max gtid info %s file_name %s offset %d", __func__,
                                            max_data_info.gtid().c_str(), max_data_info.file_name().c_str(),
                                            max_data_info.offset());
                        ret = OK;
                    }
                }
            } else {
                LogVerbose("%s find gtid %s fail", __func__, gtid_list[i].c_str());
            }
        }
    }

    ColorLogInfo("%s get gtid %s from gtid list num %zu, ret %d", __func__, max_data_info.gtid().c_str(), gtid_list.size(),ret);
    if (ret == OK) {
        *gtid = max_data_info.gtid();
    }
    return ret;
}

int EventStorage::RealCheckGTID(const string &gtid, EventDataInfo *info) {
    if (checkpoint_data_.empty()) {
        CheckPointDone();
        return DATA_EMPTY;
    }

    auto find_it = checkpoint_data_.find(gtid);
    if (find_it == checkpoint_data_.end()) {
        ColorLogWarning("%s check gtid gtid %s not exist in file, data size %zu", __func__, gtid.c_str(),
                               checkpoint_data_.size());
        CheckPointDone();
        return DATA_EMPTY;
    } else {
        LogVerbose("%s gtid %s file %s pos %d, data size %zu", __func__, gtid.c_str(),
                           find_it->second.file_name().c_str(), find_it->second.offset(), checkpoint_data_.size());
        *info = find_it->second;
    }

    return OK;
}

int EventStorage::RealGetGTIDInfo(const string &gtid, EventDataInfo *data_info) {
	
	int ret = uuid_handler_->GetLastestGTIDEventByGTID(gtid, data_info);
	if(ret==OK) {
		if(data_info->gtid()==gtid) {
			LogVerbose("%s mysql gtid %s, current lastest gtid %s, return competed from uuid, file %s",
					__func__, gtid.c_str(), data_info->gtid().c_str(), 
					data_info->file_name().c_str());
			return OK;
		}
	}

    event_index_status status = event_index_->GetGTID(gtid, data_info);
    LogVerbose("%s get gtid info %s ret %d, file %s", __func__, gtid.c_str(), status,
                       data_info->file_name().c_str());
    if (status == event_index_status::OK)
        return OK;
    else if (status == event_index_status::DATA_NOT_FOUND)
        return DATA_EMPTY;
    return FILE_FAIL;

}

int EventStorage::RealGetLowerBoundGTIDInfo(const string &gtid, EventDataInfo *data_info) {

	int ret = uuid_handler_->GetLastestGTIDEventByGTID(gtid, data_info);
	if(ret==OK) {
		if(data_info->gtid()==gtid) {
			LogVerbose("%s mysql gtid %s, current lastest gtid %s, return competed from uuid, file %s",
					__func__, gtid.c_str(), data_info->gtid().c_str(), 
					data_info->file_name().c_str());
			return OK;
		}
	}
	
    event_index_status status = event_index_->GetLowerBoundGTID(gtid, data_info);
    LogVerbose("%s get gtid info %s ret %d, file %s", __func__, gtid.c_str(), status,
                       data_info->file_name().c_str());
    if (status == event_index_status::OK)
        return OK;
    else if (status == event_index_status::DATA_NOT_FOUND)
        return DATA_EMPTY;
    return FILE_FAIL;

}

int EventStorage::GetFileHeader(EventData *eventdata, EventDataInfo *eventdatainfo) {
    string uuidbuffer;
    int ret = uuid_handler_->SerializeToBuffer(&uuidbuffer);
    if (ret == OK) {
        eventdata->set_gtid("");
        eventdata->set_data(uuidbuffer);
        eventdata->set_instance_id(uuid_handler_->GetLastInstanceID());
        eventdata->set_event_data_file(eventdatainfo->file_name());

        MasterInfo master_info;
        master_storage_->GetMaster(&master_info);
        ColorLogInfo("%s header instanceid %lu buffer size %zu, master instance id %lu", __func__,
                            eventdata->instance_id(), uuidbuffer.size(), master_info.instance_id());
        string extbuffer;
        if (!master_info.SerializeToString(eventdata->mutable_master_data())) {
            return BUFFER_FAIL;
        }
    }
    return ret;
}

int EventStorage::SwitchNewFile(const EventData &header, bool write_footer, EventDataInfo *data_info) {
    int ret = OK;
    if (write_footer) {
        //write footor
        ret = event_file_manager_->WriteData(header, data_info);
        if (ret) {
            ColorLogError("%s write footer fail", __func__);
            return ret;
        }
    }
    event_file_manager_->Flush();
    //switch
    event_file_manager_->SwitchNewWriteFile(header.instance_id());
    //write header
    ret = event_file_manager_->WriteData(header, data_info);
    if (ret) {
        ColorLogError("%s write header fail", __func__);
    } else {
        SetInstanceID(header.instance_id());
    }
    return ret;
}

int EventStorage::CheckAndSwitchFile(bool force) {
    EventDataInfo data_info;
    event_file_manager_->GetWriteFileInfo(&data_info);
    if (!force || data_info.offset() > option_->GetBinLogSvrConfig()->GetMaxEventFileSize()) {
		STATISTICS(GtidEventSwitchDataFile());
        ColorLogInfo("%s file %s size %zu has full, max size %zu, switch", __func__,
                            data_info.file_name().c_str(), data_info.offset(),
                            option_->GetBinLogSvrConfig()->GetMaxEventFileSize());

        EventData event_headerdata;
        int ret = GetFileHeader(&event_headerdata, &data_info);
        if (ret == 0) {
            ret = SwitchNewFile(event_headerdata);
        }
        return ret;
    }
    return OK;
}

void EventStorage::Notify() {
    pthread_cond_broadcast(&cond_);
    return;
}

void EventStorage::Wait() {
	pthread_cond_wait(&cond_, &mutex_);
	return;
}

uint64_t EventStorage::GetLastestCheckPointInstanceID() {
    return checkpoint_id;
}

void EventStorage::SetInstanceID(const uint64_t &instanceid) {
    LogVerbose("%s set instance id %u", __func__, instanceid);
    checkpoint_id = instanceid;
}

void EventStorage::CheckPointDone() {
    if (checkpoint_data_.size()) {
        ColorLogInfo("%s clear checkpoint data %zu", __func__, checkpoint_data_.size());
        CheckAndSwitchFile();
        checkpoint_data_.clear();
    }
}

}