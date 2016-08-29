/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "master_executer.h"

#include "master_manager.h"
#include "paxos_def.h"

#include "phxcomm/phx_log.h"
#include "phxcomm/phx_utils.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/define/errordef.h"

using std::string;
using std::vector;
using phxsql::LogVerbose;
using phxsql::ColorLogInfo;
using phxsql::Utils;

namespace phxbinlog {

MasterExecuter::MasterExecuter(const Option *option, MasterManager *data_manager) :
        ExecuterBase(option) {
    master_manager_ = data_manager;
    option_ = option;
}

MasterExecuter::~MasterExecuter() {
}

int MasterExecuter::GetExtLeaseTime() {
    return rand() % 10;
}

int MasterExecuter::SMExecute(const uint64_t &instanceid, const string &paxos_value) {
    MasterInfo master_info;
    if (!master_info.ParseFromString(paxos_value)) {
        return BUFFER_FAIL;
    }

    ColorLogInfo("%s instance id %llu execute ip %s svr id %d version %llu export ip %s port %u", __func__, instanceid,
                 Utils::GetIP(master_info.svr_id()).c_str(), master_info.svr_id(), master_info.version(),
                 master_info.export_ip().c_str(), master_info.export_port());

    master_info.set_instance_id(instanceid);
    if (master_info.expire_time() == 0) {
        //new set
        if (master_info.svr_id() == option_->GetBinLogSvrConfig()->GetEngineSvrID()) {
            master_info.set_expire_time(master_info.update_time() + master_info.lease());
        } else {
            master_info.set_expire_time(time(NULL) + master_info.lease() + GetExtLeaseTime());
        }
    }

    return master_manager_->SetMasterInfo(master_info);
}

const int MasterExecuter::SMID() const {
    return MASTER_PAXOS_SMID;
}

const uint64_t MasterExecuter::GetCheckpointInstanceID(const int group_idx) const {
    return -1;
}

int MasterExecuter::LockCheckpointState() {
    LogVerbose("%s sm check point lock, skip", __func__);
    return 0;
}

int MasterExecuter::GetCheckpointState(const int group_idx, string &dir_path, vector<string> &file_list) {
    LogVerbose("%s sm check point get dir, dir %s", __func__, dir_path.c_str());
    return 0;
}

void MasterExecuter::UnLockCheckpointState() {
    LogVerbose("%s sm check point unlock, skip", __func__);
    return;
}

int MasterExecuter::LoadCheckpointState(const int group_idx, const string &checkpoint_tmp_file_dir_path,
                                        const vector<string> &vecFileList, const uint64_t checkpoint_instanceid) {
    LogVerbose("%s sm check point load data, path %s, skip", __func__, checkpoint_tmp_file_dir_path.c_str());
    return 0;
}

}

