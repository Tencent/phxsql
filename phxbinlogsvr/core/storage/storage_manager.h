/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include <stdint.h>
#include <string>
#include <vector>

namespace phxbinlog {

class Option;
class EventStorage;
class MasterStorage;
class CheckPointManager;
class StorageManager {
 public:
    static StorageManager *GetGlobalStorageManager(const Option *option);

    StorageManager(const Option *option);
    ~StorageManager();

    int LoadFromCheckPoint();

    EventStorage *GetEventStorage();
    MasterStorage *GetMasterStorage();
    CheckPointManager * GetCheckPointManager();

    uint64_t GetInitialInstanceID();

    int MakeCheckPoint(const std::vector<std::string> &gtidlist);

    //for sm
    int LockCheckPoint();
    void UnLockCheckPoint();
    int GetFile(std::vector<std::string> *checkpointfiles);
    int LoadCheckpointFiles(const std::string &sCheckpointTmpFileDirPath);

 private:
    EventStorage *event_storage_;
    MasterStorage *master_storage_;
    CheckPointManager *checkpoint_manager_;
    uint64_t initial_instanceid_;
};

}
