/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include <string>
#include <queue>
#include "eventdata.pb.h"

namespace phxbinlog {

class Option;
class EventFileManager;
class EventUUIDHandler;
class EventIndex;
class MasterStorage;

class EventStorage {
 protected:
    EventStorage(const Option *option, MasterStorage *masterstorage);
    ~EventStorage();

    int LoadFromCheckPoint(const EventData &checkpointdata, const std::string &checkpointfile);

    friend class StorageManager;

 public:

    int AddEvent(const std::string &gtid, const EventData &data, bool switch_file = true);
    int ReSet(const std::string &gtid);
    int GetEvent(EventData *data, bool wait = false);

    std::string GetLastGTIDByUUID(const std::string &uuid);
    std::string GetLastGTIDByGTID(const std::string &gtid);

    int GetGTIDInfo(const std::string &gtid, EventDataInfo *data_info, bool lower_bound);

    std::string GetNewestGTID();
    int GetNewestGTIDInfo(EventDataInfo *info);
    int GetLastGtid(const std::vector<std::string> &gtid_list, std::string *last_gtid);
    int CheckGTID(const std::string &gtid);

    uint64_t GetLastInstanceID();
    uint64_t GetLastestCheckPointInstanceID();
    uint64_t GetCurrentInstanceInterval();

    const std::string &GetDataDir() const;
    int Restart();

    void Notify();
 protected:
    //for checkpoint
    void RemoveFile(const std::string &file_name);
    int DelCheckPointFile(const std::string &maxfilename, const uint32_t &mintime, std::vector<std::string> *delfiles);
    int SwitchNewFile(const EventData &header, bool write_footer = true, EventDataInfo *datainfo = NULL);
    int CheckAndSwitchFile(bool force = true);
    friend class CheckPointManager;

 private:
    int GetFileHeader(EventData *eventdata, EventDataInfo *eventdatainfo);
    void SetInstanceID(const uint64_t &instanceid);

    int Init();
    int InitCheckPointDataFile(const std::string &checkpointfile);

    int Start();
    int Close();

    void Wait();

    int RealGetGTIDInfo(const std::string &gtid, EventDataInfo *data_info);
    int RealGetLowerBoundGTIDInfo(const std::string &gtid, EventDataInfo *data_info);
    int RealCheckGTID(const std::string &gtid, EventDataInfo *info);
    void CheckPointDone();
    static int CheckGtidInData(const std::string &gtid, const EventDataInfo &datainfo, const Option *option);

 private:
    EventFileManager *event_file_manager_;
    EventIndex *event_index_;
    EventUUIDHandler *uuid_handler_;

    MasterStorage *master_storage_;

    pthread_mutex_t mutex_;
    pthread_cond_t cond_;

    const Option *option_;
    uint64_t m_checkpoint_instanceid;
    uint64_t checkpoint_id;

    std::map<std::string, EventDataInfo> checkpoint_data_;
};

}
