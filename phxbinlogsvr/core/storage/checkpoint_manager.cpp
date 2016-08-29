/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "checkpoint_manager.h"

#include "storage.h"
#include "event_file_manager.h"
#include "event_storage.h"
#include "master_storage.h"
#include "storage_file_manager.h"

#include "phxcomm/lock_manager.h"
#include "phxcomm/phx_utils.h"
#include "phxcomm/phx_log.h"
#include "phxbinlogsvr/statistics/phxbinlog_stat.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/define/errordef.h"

#include <map>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>

using std::string;
using std::vector;
using std::pair;
using std::set;
using phxsql::LockManager;
using phxsql::Utils;
using phxsql::LogVerbose;
using phxsql::ColorLogInfo;
using phxsql::ColorLogWarning;

namespace phxbinlog {

class StatusRunningHandler {
 public:
    StatusRunningHandler(CheckPointManager *checkpointmanger) {
        checkpoint_manger_ = checkpointmanger;
        ret_ = OK;
    }

    ~StatusRunningHandler() {
        if (ret_ == OK) {
            checkpoint_manger_->SetStatus(CheckPointManager::RUNNINGSTATUS::NONE);
        }
    }

    int TestAndSet(const CheckPointManager::RUNNINGSTATUS &oldstatus, const CheckPointManager::RUNNINGSTATUS &status) {
        ret_ = checkpoint_manger_->TestAndSet(oldstatus, status);
        return ret_;
    }
 private:
    CheckPointManager *checkpoint_manger_;
    int ret_;
};

#define BACKUPCHECKPOINT "backupcheckpoint"

CheckPointManager::CheckPointManager(const Option *option, EventStorage *eventstorage, MasterStorage *masterstorage) {
    option_ = option;

    event_storage_ = eventstorage;
    master_storage_ = masterstorage;
    last_checkpoint_time = 0;
    run_status_ = RUNNINGSTATUS::NONE;

    pthread_mutex_init(&mutex_, NULL);
}

CheckPointManager::~CheckPointManager() {
    pthread_mutex_destroy(&mutex_);
}

int CheckPointManager::LockCheckPoint() {
    int ret = TestAndSet(RUNNINGSTATUS::NONE, RUNNINGSTATUS::LOCKING);
    if (ret == 1) {
        LogVerbose("%s lock check point fail", __func__);
        return 1;
    }
    return 0;
}

void CheckPointManager::UnLockCheckPoint() {
    TestAndSet(RUNNINGSTATUS::LOCKING, RUNNINGSTATUS::NONE);
}

void CheckPointManager::SetStatus(const RUNNINGSTATUS &status) {
    run_status_ = status;
}

int CheckPointManager::TestAndSet(const RUNNINGSTATUS &oldstatus, const RUNNINGSTATUS &status) {
    LockManager lock(&mutex_);
    if (run_status_ != oldstatus) {
        return 1;
    }
    run_status_ = status;
    return OK;
}

int CheckPointManager::GetFile(vector<string> *checkpointfiles) {
    EventFileManager eventfilemanager(option_);
    const set<string, FileCmpOpt> &files = eventfilemanager.GetFileList();
    for (const auto &it : files) {
        checkpointfiles->push_back(it);
    }
    if (checkpointfiles->size())
        checkpointfiles->pop_back();
    return 0;
}

int CheckPointManager::LoadCheckpointFiles(const string &sCheckpointTmpFileDirPath) {
    STATISTICS(CheckPointTransferFile());
    ColorLogInfo("%s load from check point files %s", __func__, sCheckpointTmpFileDirPath.c_str());

    Storage storage(Storage::MODE::WRITE, event_storage_->GetDataDir(), option_);
    assert(storage.OpenDB(BACKUPCHECKPOINT) == 0);
    assert(storage.Reset() == 0);
    assert(storage.WriteData(sCheckpointTmpFileDirPath) == 0);
    return OK;
}

int CheckPointManager::MakeCheckPoint(const vector<string> &gtidlist) {
    StatusRunningHandler handler(this);
    int ret = handler.TestAndSet(RUNNINGSTATUS::NONE, RUNNINGSTATUS::RUNNING);
    if (ret) {
        ColorLogInfo("%s something is running, skip", __func__);
        return SVR_FAIL;
    }

    uint32_t nowtime = time(NULL);
    if (nowtime - last_checkpoint_time > option_->GetBinLogSvrConfig()->GetCheckPointMakingPeriod()
            || nowtime - last_checkpoint_time > 60) {
        if (last_checkpoint_time > 0)
            ret = RealMakeCheckPoint(gtidlist);
        if (ret == OK) {
            last_checkpoint_time = nowtime;
        }
    } else {
        ColorLogInfo("%s now time %u last check time %u,  interval time %u", __func__, nowtime, last_checkpoint_time,
                     option_->GetBinLogSvrConfig()->GetCheckPointMakingPeriod());
    }

    ColorLogInfo("%s check point check done", __func__);
    return OK;
}

int CheckPointManager::GetNewestCheckPoint(EventData *eventdata, MasterInfo *masterinfo, EventDataInfo *eventdatainfo) {
    StatusRunningHandler handler(this);
    int ret = handler.TestAndSet(RUNNINGSTATUS::NONE, RUNNINGSTATUS::RUNNING);
    if (ret) {
        ColorLogInfo("%s something is running, skip", __func__);
        return SVR_FAIL;
    }

    if (CheckHasBakupCheckPointFiles(eventdata, masterinfo, eventdatainfo) == OK) {
        STATISTICS(CheckPointLoadTransferFile());
        return OK;
    }

    LogVerbose("%s open local checkpoint", __func__);

    EventFileManager eventfilemanager(option_);
    LogVerbose("%s start open newest", __func__);
    ret = eventfilemanager.OpenNewestFile();
    LogVerbose("%s open file ret %d", __func__, ret);
    if (ret) {
        if (ret == NO_FILES) {
            return DATA_EMPTY;
        }
        return ret;
    }

    LogVerbose("%s get new check point", __func__);

    while (true) {
        int ret = eventfilemanager.ReadData(eventdata, eventdatainfo);
        if (ret == DATA_EMPTY) {
        } else if (ret == OK) {
            if (eventdata->gtid().empty() && eventdata->master_data().size()) {
                if (masterinfo->ParseFromString(eventdata->master_data())) {
                    LogVerbose("get check point file %s(%s) instanceid %lu", eventdatainfo->file_name().c_str(),
                               eventdata->event_data_file().c_str(), eventdata->instance_id());
                    return OK;
                } else {
                    ColorLogWarning("parse master data fail, skip");
                }
                break;
            }
        }
        ret = eventfilemanager.GetPreReadFile();
        if (ret) {
            break;
        }
    }
    eventdata->Clear();
    masterinfo->Clear();
    eventdatainfo->Clear();
    return DATA_EMPTY;
}

int CheckPointManager::GetCurrentInstanceInterval() {
    return event_storage_->GetCurrentInstanceInterval();
}

int CheckPointManager::RealMakeCheckPoint(const vector<string> &gtidlist) {
    uint32_t deltime = time(NULL);
    deltime -= option_->GetBinLogSvrConfig()->GetCheckPointMakingPeriod();

    string maxfilename;
    for (const auto &it : gtidlist) {
        EventDataInfo datainfo;
        int ret = event_storage_->GetGTIDInfo(it, &datainfo, true);
        if (ret == 0) {
            LogVerbose("%s get check file name %s", __func__, datainfo.file_name().c_str());
            if (StorageFileManager::FileCmp(maxfilename, datainfo.file_name()) < 0) {
                maxfilename = datainfo.file_name();
            }
        }
    }

    if (maxfilename.empty()) {
        ColorLogInfo("%s get no del check point file", __func__);
        return OK;
    }
    ColorLogInfo("%s get max check point %s", __func__, maxfilename.c_str());

    vector < string > files;
    int ret = event_storage_->DelCheckPointFile(maxfilename, deltime, &files);
    ColorLogInfo("%s get del files %zu, ret %d", __func__, files.size(), ret);
    if (ret == 0) {
        for (const auto &it : files) {
            Utils::ReMoveFile(event_storage_->GetDataDir() + it);
        }
        STATISTICS(CheckPointDeleteCheckPointNum(files.size()));
    }

    return OK;
}

int CheckPointManager::CheckHasBakupCheckPointFiles(EventData *header, MasterInfo *masterinfo,
                                                    EventDataInfo *headerdatainfo) {
    string datadir;
    Storage storage(Storage::MODE::READ, event_storage_->GetDataDir(), option_);
    storage.OpenDB(BACKUPCHECKPOINT);
    int ret = storage.ReadData(&datadir);
    //relesase storage to make it possible to delete
    Utils::ReMoveFile(event_storage_->GetDataDir() + BACKUPCHECKPOINT);
    LogVerbose("%s read backup data ret %d, dir %s", __func__, ret, datadir.c_str());
    if (ret != OK || datadir == "") {
        ret = DATA_EMPTY;
    } else {
        ColorLogInfo("%s begin load check point data, data dir %s", __func__, datadir.c_str());

        ret = Utils::RemoveDir(event_storage_->GetDataDir());
        assert(ret == OK);
        event_storage_->Restart();
        Utils::CheckDir(option_->GetBinLogSvrConfig()->GetEventDataBackUPDir());
        ret = LoadFromCheckPointFileList(datadir, header, masterinfo, headerdatainfo);
        if (ret) {
            STATISTICS(CheckPointLoadDataFail());
        }
        assert(ret == OK);
        ret = Utils::RemoveDir(datadir);
        ColorLogInfo("%s load check point data, data dir %s done", __func__, datadir.c_str());
    }

    return ret;
}

int CheckPointManager::LoadFromCheckPointFileList(const string &datadir, EventData *header, MasterInfo *masterinfo,
                                                  EventDataInfo *headerdatainfo) {
    EventFileManager eventfilemanager(option_, datadir);
    int ret = eventfilemanager.OpenOldestFile();
    if (ret) {
        return ret;
    }
    EventData curheader, curfooter;
    string curfilename;
    while (1) {
        EventData eventdata;
        EventDataInfo eventdatainfo;
        ret = eventfilemanager.ReadData(&eventdata, &eventdatainfo);
        if (ret == OK) {
            if (eventdata.gtid().empty()) {
                if (eventdatainfo.file_name() != curfilename) {
                    curheader = eventdata;
                    //header
                    LogVerbose("%s get header", __func__);
                    if (curfooter.event_data_file().empty()) {
                        //first file
                        event_storage_->SwitchNewFile(curheader, false, NULL);
                        LogVerbose("%s switch first file %s to %s,", __func__, curheader.event_data_file().c_str(),
                                   eventdatainfo.file_name().c_str());
                    }
                } else {
                    LogVerbose("%s get footer", __func__);
                    curfooter = eventdata;
                    event_storage_->SwitchNewFile(curfooter, true, headerdatainfo);
                    LogVerbose("%s switch file %s to new file %s, add footer", __func__,
                               curfooter.event_data_file().c_str(), headerdatainfo->file_name().c_str());
                }
                curfilename = eventdatainfo.file_name();
            } else {
                int ret = event_storage_->AddEvent(eventdata.gtid(), eventdata, false);
                if (ret) {
                    return ret;
                }
            }
        } else if (ret == DATA_EMPTY) {
            *header = curfooter;

            ret = CheckPointManager::GetMasterInfoFromHeader(curfooter, masterinfo);
            if (ret == OK) {
                LogVerbose("%s get master info, master ip %s version %lu", __func__,
                           Utils::GetIP(masterinfo->svr_id()).c_str(), masterinfo->version());
            } else {
                ColorLogWarning("%s parse header fail", __func__);
            }
            break;
        } else {
            break;
        }
    }
    ColorLogInfo("%s load backup checkpoint done from %s, header instanceid %lu file_name %s master varion %lu",
                 __func__, datadir.c_str(), header->instance_id(), headerdatainfo->file_name().c_str(),
                 masterinfo->version());
    return ret;
}

int CheckPointManager::GetMasterInfoFromHeader(const EventData &eventdata, MasterInfo *masterinfo) {
    if (eventdata.gtid().empty() && eventdata.master_data().size()) {
        if (masterinfo->ParseFromString(eventdata.master_data())) {
            LogVerbose("get check point in file %s instanceid %lu", eventdata.event_data_file().c_str(),
                       eventdata.instance_id());
            return OK;
        } else {
            ColorLogWarning("parse master data fail, skip");
        }
    }
    return DATA_EMPTY;
}

}

