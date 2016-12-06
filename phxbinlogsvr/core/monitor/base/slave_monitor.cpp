/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/


#include "slave_monitor.h"
#include "mysql_manager.h"
#include "mysql_string_helper.h"
#include "monitor_comm.h"

#include "phxcomm/lock_manager.h"
#include "phxcomm/phx_log.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/define/errordef.h"

#include <string.h>

using std::vector;
using std::string;
using phxsql::LockManager;
using phxsql::LogVerbose;
using phxsql::ColorLogInfo;
using phxsql::ColorLogError;

namespace phxbinlog {

int SlaveMonitor::GetRelayLog(const Option *option, vector<string> *gtid_list) {

    MySqlManager *mysql_manager = MySqlManager::GetGlobalMySqlManager(option);

    vector < string > fields;
    vector < vector<string> > results;
    string slave_running;
    int ret = mysql_manager->Query("show slave status;", &results, &fields);
    if (ret == 0) {
        if (results.size() == 0) {
            return 0;
        }

        for (size_t i = 0; i < fields.size(); ++i) {
            if (fields[i] == "Retrieved_Gtid_Set") {
                if (i >= results[0].size()) {
                    break;
                }
                LogVerbose("%s get retried gtid %s", __func__, results[0][i].c_str());
                ret = MonitorComm::ParseMaxGTIDList(option, results[0][i], gtid_list);
                if (ret) {
                    break;
                }
            }
        }
        if (gtid_list->size() == 0) {
            for (size_t i = 0; i < fields.size(); ++i) {
                if (fields[i] == "Executed_Gtid_Set") {
                    if (i >= results[0].size()) {
                        break;
                    }
                    LogVerbose("%s get executed gtid %s", __func__, results[0][i].c_str());
                    ret = MonitorComm::ParseMaxGTIDList(option, results[0][i], gtid_list);
                    if (ret) {
                        break;
                    }
                }
            }
        }
    }
    return ret;
}

int SlaveMonitor::SlaveCheckRelayLog(const Option *option, vector<string> *gtid_list) {
    return GetRelayLog(option, gtid_list);
}

int SlaveMonitor::SlaveStart(const Option *option, const string &master_ip, const uint32_t &master_port,
                             bool read_only) {
    MySqlManager *mysql_manager = MySqlManager::GetGlobalMySqlManager(option);

    int ret = mysql_manager->Query("stop slave;");
    if (ret)
        return MYSQL_FAIL;

    ret = mysql_manager->Query(
            MySqlStringHelper::GetSvrIDString(option->GetBinLogSvrConfig()->GetEngineSvrID()).c_str());
    if (ret)
        return MYSQL_FAIL;

    string replica_username = mysql_manager->GetReplicaUserName();
    string replica_pwd = mysql_manager->GetReplicaPwd();
    if (replica_username == "") {
        replica_username = mysql_manager->GetPendingReplicaUserName();
        replica_pwd = mysql_manager->GetPendingReplicaPwd();
    }

    if (replica_username == "") {
		ColorLogError("%s replica user has not been set, wait",__func__);
        return MYSQL_FAIL;
	}

    ret = mysql_manager->Query(
            MySqlStringHelper::GetChangeMasterQueryString(master_ip, master_port, replica_username, replica_pwd));
    if (ret)
        return MYSQL_FAIL;

    ret = mysql_manager->Query(read_only ? "set global super_read_only='on';" : "set global super_read_only='off';");
    if (ret)
        return MYSQL_FAIL;

    ret = mysql_manager->Query(read_only ? "set global read_only='on';" : "set global read_only='off';");
    if (ret)
        return MYSQL_FAIL;

    ret = mysql_manager->Query("start slave;");
    if (ret)
        return MYSQL_FAIL;

    return OK;
}

int SlaveMonitor::CheckSlaveRunningStatus(const Option *option, const char *export_ip, const uint32_t *export_port) {
	if(IsSlaveConnecting()) {
		ColorLogInfo("%s slave is connecting, skip", __func__);
		return OK;
	}
    MySqlManager *mysql_manager = MySqlManager::GetGlobalMySqlManager(option);

    string slave_running;
    int ret = mysql_manager->GetValue("status", "Slave_running", &slave_running);
    if (ret) {
        return OK;
    }
	ColorLogInfo("%s slave is running %s", __func__, slave_running.c_str());

    if (strcasecmp(slave_running.c_str(), "ON")) {
        if (export_ip) {
            return SlaveMonitor::SlaveStart(option, export_ip, *export_port, false);
        } else {
            return SlaveMonitor::SlaveStart(option, "127.0.0.1", option->GetBinLogSvrConfig()->GetEnginePort());
        }
    }

    return OK;
}

pthread_mutex_t SlaveMonitor::mutex_;
bool SlaveMonitor::connecting_;
void SlaveMonitor::SlaveConnecting(bool connecting) {
    LockManager lock(&mutex_);
    connecting_ = connecting;
}

bool SlaveMonitor::IsSlaveConnecting() {
    LockManager lock(&mutex_);
    return connecting_;
}

}

