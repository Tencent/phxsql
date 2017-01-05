/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/


#include "event_monitor.h"
#include "master_manager.h"
#include "master_monitor.h"
#include "event_storage.h"
#include "storage_manager.h"
#include "mysql_manager.h"

#include "phxcomm/phx_log.h"
#include "phxbinlogsvr/statistics/phxbinlog_stat.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/define/errordef.h"

#include <unistd.h>

using std::string;
using std::vector;
using phxsql::LogVerbose;
using phxsql::ColorLogInfo;

namespace phxbinlog {

EventMonitor::EventMonitor(const Option *option) {
    option_ = option;
    storage_manager_ = StorageManager::GetGlobalStorageManager(option);
	master_manager_ = MasterManager::GetGlobalMasterManager(option);
    stop_ = false;

    Run();
}

EventMonitor::~EventMonitor() {
    stop_ = true;
    WaitStop();
}

int EventMonitor::CheckRunningStatus() {
    int ret = OK;
    vector < string > gtid_list;
    if (option_->GetBinLogSvrConfig()->IsForceMakingCheckPoint()) {
        ret = MasterMonitor::GetMySQLMaxGTIDList(option_, &gtid_list);
    } else {
        vector < string > member_iplist;
        master_manager_->GetMemberIPList(&member_iplist);

        ret = MasterMonitor::GetGlobalMySQLMaxGTIDList(option_, member_iplist, &gtid_list);
    }

    
    LogVerbose("%s get gtid list ret %d", __func__, ret);
    if (ret == OK) {
		
		//use last gtid set instead of current gtid set if it is master
		//some gtid in current set may not be storaged in level db and cost much while searching
		bool is_master = master_manager_->IsMaster();
		if(is_master) {
			if(last_check_gtid_.empty()) {
				last_check_gtid_ = gtid_list;
				return OK;
			}
			ret = storage_manager_->MakeCheckPoint(last_check_gtid_);
			last_check_gtid_=gtid_list;
		}
		else {
			ret = storage_manager_->MakeCheckPoint(gtid_list);
			uint64_t max_instanceid = 0;
			EventStorage *event_storage = storage_manager_->GetEventStorage();
			for (const auto &gtid : gtid_list) {
				EventDataInfo data_info;
				int ret = event_storage->GetGTIDInfo(gtid, &data_info, true);
				if (ret) {
					continue;
				}
				max_instanceid = std::max(max_instanceid, (uint64_t) data_info.instance_id());
            }

            uint64_t storage_instanceid = event_storage->GetLastInstanceID();
            ColorLogInfo("%s current mysql instanceid %lu, binlog svr instanceid %lu", __func__, max_instanceid,
                 storage_instanceid);

            if (max_instanceid && max_instanceid <= storage_instanceid) {
            STATISTICS(MySqlGtidNumDiff(storage_instanceid - max_instanceid));
            }
        }
    }

    return ret;
}

int EventMonitor::Process() {
    while (!stop_) {
        CheckRunningStatus();
        sleep(5);
    }

    return OK;
}

}
