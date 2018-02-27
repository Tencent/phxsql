/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/


#include "event_executer.h"

#include "storage_manager.h"
#include "event_manager.h"
#include "master_manager.h"
#include "paxos_def.h"
#include "paxosdata.pb.h"

#include "phxcomm/phx_timer.h"
#include "phxcomm/phx_utils.h"
#include "phxcomm/phx_log.h"
#include "phxbinlogsvr/statistics/phxbinlog_stat.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/define/errordef.h"

using std::string;
using std::vector;
using phxsql::LogVerbose;
using phxsql::ColorLogWarning;
using phxsql::ColorLogInfo;
using phxsql::ColorLogError;
using phxsql::Utils;
using phxsql::PhxTimer;

namespace phxbinlog {

EventExecuter::EventExecuter(const Option *option, EventManager *event_manager) :
        ExecuterBase(option) {
    option_ = option;
    storage_ = StorageManager::GetGlobalStorageManager(option_);
    event_manager_ = event_manager;
    master_manager_ = MasterManager::GetGlobalMasterManager(option);
    gtid_ = "";
}

EventExecuter::~EventExecuter() {
}

const string &EventExecuter::GetResultGTID() const {
    return gtid_;
}

int EventExecuter::SMExecute(const uint64_t &instance_id, const string &paxos_value) {
    PhxTimer timer;
    PaxosEventData data;

    if (!data.ParseFromString(paxos_value)) {
        ColorLogError("%s from buffer fail data size %zu", __func__, paxos_value.size());
        return PAXOS_BUFFER_FAIL;
    }

    if (data.buffer().empty() || data.event_gtid().empty()) {
        ColorLogInfo("empty value to update, skip");
        return OK;
    }

    LogVerbose("%s svr id %u data current gtid %s data event gtid %s data buffer len %zu", __func__, data.svrid(),
               data.current_gtid().c_str(), data.event_gtid().c_str(), data.buffer().size());

    //check master
    if (!master_manager_->CheckMasterBySvrID(data.svrid(), false)) {
        ColorLogWarning("%s svr id %d(%s) is not master", __func__, data.svrid(), Utils::GetIP(data.svrid()).c_str());
        STATISTICS(GtidEventMasterConficlt());
        return MASTER_CONFLICT;
    }

    //check if master uses the old data to summit
    string newest_gtid = event_manager_->GetNewestGTID();

    if (newest_gtid.size() > 0 && data.current_gtid() != newest_gtid) {
        if (data.event_gtid() == newest_gtid) {
            ColorLogWarning("%s gtid has been increased to %s, but master send (%s,%s), duplicate", __func__,
                            newest_gtid.c_str(), data.current_gtid().c_str(), data.event_gtid().c_str());
            STATISTICS(GtidEventExit());
            return EVENT_EXIST;
        } else {
            ColorLogError("%s gtid has been increased to %s, but master has the old one (%s,%s)", __func__,
                          newest_gtid.c_str(), data.current_gtid().c_str(), data.event_gtid().c_str());
            STATISTICS(GtidEventConflict());
            return GTID_CONFLICT;
        }
    }
    
    uint64_t old_checksum = 0;
    int ret = event_manager_->GetNowCheckSum(&old_checksum);
    if (ret) {
        ColorLogError("%s get now check sum fail ret %d", __func__, ret);
        return ret;
    }

    LogVerbose("%s current gtid %s event gtid %s sm gtid %s old checksum %llu run %u ms", __func__,
               data.current_gtid().c_str(), data.event_gtid().c_str(), newest_gtid.c_str(), old_checksum,
               timer.GetTime() / 1000);

    uint64_t new_checksum = Utils::GetCheckSum(old_checksum, data.buffer().c_str(), data.buffer().size());
    if (new_checksum != data.checksum()) {
        ColorLogWarning("%s gtid %s local checksum %llu new checksum %llu, check fail", __func__,
                        data.event_gtid().c_str(), new_checksum, data.checksum());
        STATISTICS(GtidEventCheckSumConflict());
        return BUFFER_FAIL;
    } else {
        ColorLogInfo("%s gtid %s local checksum %llu new checksum %llu, check done", __func__,
                     data.event_gtid().c_str(), new_checksum, data.checksum());
    }

    STATISTICS(GtidEventExecute());
    ret = event_manager_->SaveEvents(instance_id, data.event_gtid(), data.buffer(), data.checksum());
    if (ret == 0) {
        ColorLogInfo(
                "save event send gtid %s buffer gtid %s value size %zu check sum %llu, new checksum %llu run %u ms",
                data.current_gtid().c_str(), data.event_gtid().c_str(), data.buffer().size(), data.checksum(),
                new_checksum, timer.GetTime() / 1000);
        STATISTICS(GtidEventExecuteSucess());
    } else {
        ColorLogWarning("save event send gtid %s buffer gtid %s value size %zu fail run %u ms",
                        data.current_gtid().c_str(), data.event_gtid().c_str(), data.buffer().size(),
                        timer.GetTime() / 1000);
        STATISTICS(GtidEventExecuteFail());
    }

    return ret;
}

const int EventExecuter::SMID() const {
    return EVENT_PAXOS_SMID;
}

const uint64_t EventExecuter::GetCheckpointInstanceID(const int group_idx) const {
    return storage_->GetInitialInstanceID();
}

int EventExecuter::LockCheckpointState() {
    return storage_->LockCheckPoint();
}

int EventExecuter::GetCheckpointState(const int group_idx, string &dir_path, vector<string> &file_list) {
    dir_path = option_->GetBinLogSvrConfig()->GetEventDataBaseDir();
    return storage_->GetFile(&file_list);
}

void EventExecuter::UnLockCheckpointState() {
    return storage_->UnLockCheckPoint();
}

int EventExecuter::LoadCheckpointState(const int group_idx, const string &checkpoint_tmp_file_dir_path,
                                       const vector<string> &file_list, const uint64_t checkpoint_instanceid) {
    return storage_->LoadCheckpointFiles(checkpoint_tmp_file_dir_path);
}

}
