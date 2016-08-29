/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include <string>
#include "executer_base.h"

namespace phxbinlog {

class Option;
class EventManager;
class MasterManager;
class EventExecuter : public ExecuterBase {
 public:
    EventExecuter(const Option *option, EventManager *data_manager);
    virtual ~EventExecuter();

    virtual int SMExecute(const uint64_t &llInstanceID, const std::string &sPaxosValue);
    virtual const int SMID() const;
    const std::string &GetResultGTID() const;

 public:
    virtual const uint64_t GetCheckpointInstanceID(const int iGroupIdx) const;
    virtual int LockCheckpointState();
    virtual int GetCheckpointState(const int iGroupIdx, std::string &sDirPath, std::vector<std::string> &vecFileList);
    virtual void UnLockCheckpointState();
    virtual int LoadCheckpointState(const int iGroupIdx, const std::string &sCheckpointTmpFileDirPath,
                                    const std::vector<std::string> &vecFileList, const uint64_t llCheckpointInstanceID);

 private:
    StorageManager *storage_;
    EventManager *event_manager_;
    MasterManager *master_manager_;
    std::string gtid_;
    const Option *option_;
};

}
