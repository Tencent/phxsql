/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include "executer_base.h"

namespace phxbinlog {

class Option;
class MasterManager;
class MasterExecuter : public ExecuterBase {
 public:
    MasterExecuter(const Option *option, MasterManager *data_manager);
    virtual ~MasterExecuter();

    virtual int SMExecute(const uint64_t &instanceid, const std::string &paxos_value);

    virtual const int SMID() const;

 public:

    virtual const uint64_t GetCheckpointInstanceID(const int group_idx) const;

    virtual int LockCheckpointState();

    virtual int GetCheckpointState(const int group_idx, std::string &sDirPath, std::vector<std::string> &file_list);

    virtual void UnLockCheckpointState();

    virtual int LoadCheckpointState(const int group_idx, const std::string &checkpoint_tmp_file_dir_path,
                                    const std::vector<std::string> &file_list, const uint64_t checkpoint_instanceid);

    int GetExtLeaseTime();
 private:
    MasterManager *master_manager_;
    const Option *option_;
};

}
