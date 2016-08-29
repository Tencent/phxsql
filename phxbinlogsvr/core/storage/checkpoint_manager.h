/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include <stdint.h>
#include <vector>
#include <string>
#include <map>

#include "eventdata.pb.h"
#include "masterinfo.pb.h"

namespace phxbinlog {

class Option;
class EventStorage;
class MasterStorage;
class CheckPointManager {
 public:
    enum class RUNNINGSTATUS {
        NONE = 0,
        RUNNING = 1,
        LOCKING = 2,
    };
 public:
    CheckPointManager(const Option *option, EventStorage *eventstorage, MasterStorage *masterstorage);
    ~CheckPointManager();

    int MakeCheckPoint(const std::vector<std::string> &gtidlist);

    int GetNewestCheckPoint(EventData *eventdata, MasterInfo *masterinfo, EventDataInfo *eventdatainfo);

    //for sm
    int LockCheckPoint();
    void UnLockCheckPoint();
    int GetFile(std::vector<std::string> *checkpointfiles);
    int GetCurrentInstanceInterval();

    int LoadCheckpointFiles(const std::string &sCheckpointTmpFileDirPath);

    void SetStatus(const RUNNINGSTATUS &status);
    int TestAndSet(const RUNNINGSTATUS &oldstatus, const RUNNINGSTATUS &status);

 private:
    int CheckHasBakupCheckPointFiles(EventData *eventdata, MasterInfo *masterinfo, EventDataInfo *eventdatainfo);
    int LoadFromCheckPointFileList(const std::string &datadir, EventData *eventdata, MasterInfo *masterinfo,
                                   EventDataInfo *eventdatainfo);
    int GetMasterInfoFromHeader(const EventData &eventdata, MasterInfo *masterinfo);
    int RealMakeCheckPoint(const std::vector<std::string> &gtidlist);

 private:
    const Option *option_;
    EventStorage *event_storage_;
    MasterStorage *master_storage_;

    uint32_t last_checkpoint_time;
    RUNNINGSTATUS run_status_;
    pthread_mutex_t mutex_;
};

}
