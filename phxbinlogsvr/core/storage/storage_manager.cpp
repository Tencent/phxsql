/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "storage_manager.h"

#include "checkpoint_manager.h"
#include "event_storage.h"
#include "master_storage.h"

#include "phxcomm/phx_log.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/define/errordef.h"

#include <unistd.h>

using std::string;
using std::vector;
using phxsql::LogVerbose;
using phxsql::ColorLogError;
using phxsql::ColorLogInfo;

namespace phxbinlog {

StorageManager *StorageManager::GetGlobalStorageManager(const Option *option) {
    static StorageManager manager(option);
    return &manager;
}

StorageManager::StorageManager(const Option *option) {

    master_storage_ = new MasterStorage(option);
    event_storage_ = new EventStorage(option, master_storage_);
    checkpoint_manager_ = new CheckPointManager(option, event_storage_, master_storage_);

    initial_instanceid_ = -1;

    assert(LoadFromCheckPoint() == 0);
}

StorageManager::~StorageManager() {
    delete event_storage_;
    delete master_storage_;
    delete checkpoint_manager_;
}

EventStorage *StorageManager::GetEventStorage() {
    return event_storage_;
}

MasterStorage *StorageManager::GetMasterStorage() {
    return master_storage_;
}

CheckPointManager * StorageManager::GetCheckPointManager() {
    return checkpoint_manager_;
}

int StorageManager::LoadFromCheckPoint() {
    LogVerbose("%s load from check point", __func__);
    EventData checkpoint_data;
    MasterInfo master_info;
    EventDataInfo event_data_info;
    int ret = checkpoint_manager_->GetNewestCheckPoint(&checkpoint_data, &master_info, &event_data_info);
    if (ret != 0 && ret != DATA_EMPTY) {
        ColorLogError("%s get newest check point fail, ret %d", __func__, ret);
        return ret;
    }

    if (ret != DATA_EMPTY) {
        initial_instanceid_ = checkpoint_data.instance_id();
    } else {
        checkpoint_data.set_gtid("");
        checkpoint_data.set_data("");
        checkpoint_data.set_instance_id(0);
    }
    ColorLogInfo("%s start event storage load from checkpoint", __func__);

    ret = event_storage_->LoadFromCheckPoint(checkpoint_data, event_data_info.file_name());
    if (ret) {
        return ret;
    }
    ColorLogInfo("%s event storage load from checkpoint done", __func__);
    if (master_info.version() > 0) {
        return master_storage_->LoadFromCheckPoint(master_info);
    } else {
        return ret;
    }
}

int StorageManager::MakeCheckPoint(const vector<string> &gtidlist) {
    return checkpoint_manager_->MakeCheckPoint(gtidlist);
}

uint64_t StorageManager::GetInitialInstanceID() {
    return event_storage_->GetLastestCheckPointInstanceID();
}

//for sm
int StorageManager::LockCheckPoint() {
    return checkpoint_manager_->LockCheckPoint();
}

void StorageManager::UnLockCheckPoint() {
    checkpoint_manager_->UnLockCheckPoint();
}

int StorageManager::GetFile(vector<string> *checkpointfiles) {
    return checkpoint_manager_->GetFile(checkpointfiles);
}

int StorageManager::LoadCheckpointFiles(const string &sCheckpointTmpFileDirPath) {
    return checkpoint_manager_->LoadCheckpointFiles(sCheckpointTmpFileDirPath);
}

}

