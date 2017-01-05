/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "master_monitor.h"
#include "mysql_manager.h"
#include "mysql_string_helper.h"

#include "monitor_comm.h"
#include "slave_monitor.h"

#include "phxcomm/phx_log.h"
#include "phxbinlogsvr/statistics/phxbinlog_stat.h"
#include "phxbinlogsvr/core/gtid_handler.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/define/errordef.h"

#include <string.h>
#include <assert.h>

using std::string;
using std::vector;
using std::map;
using std::pair;
using phxsql::ColorLogInfo;
using phxsql::ColorLogWarning;
using phxsql::ColorLogError;
using phxsql::LogVerbose;

namespace phxbinlog {

int MasterMonitor::MasterStart(const Option *option) {

    MySqlManager *mysql_manager = MySqlManager::GetGlobalMySqlManager(option);

    int ret = mysql_manager->Query("stop slave;");
    if (ret)
        return ret;

    ret = mysql_manager->Query("set global super_read_only='off';");
    if (ret)
        return ret;

    ret = mysql_manager->Query("set global read_only='off';");
    if (ret)
        return ret;
    return OK;
}

int MasterMonitor::GetMySQLMaxGTIDList(const Option *option, vector<string> *gtid_list) {
    return MonitorComm::GetMySQLMaxGTIDList(option, "gtid_executed", gtid_list);
}

int MasterMonitor::GetGlobalMySQLMaxGTIDList(const Option *option, const vector<string> &iplist,
                                             vector<string> *gtid_list) {
    map<string, vector<size_t> > gtid_res;

    for (auto ip : iplist) {
        vector < string > tmp_gtid_list;
        int ret = MonitorComm::GetMySQLMaxGTIDList(option, "gtid_executed", &tmp_gtid_list, ip);
        if (ret) {
            ColorLogError("%s get from ip %s fail", __func__, ip.c_str());
            if (ret == GTID_FAIL && ip == option->GetBinLogSvrConfig()->GetEngineIP()) {
                STATISTICS(MySqlGTIDBroken());
                assert(ret == OK);
            }
            return ret;
        }
        for (auto gtid : tmp_gtid_list) {
            pair < string, size_t > parse_list = GtidHandler::ParseGTID(gtid);
            gtid_res[parse_list.first].push_back(parse_list.second);
            LogVerbose("%s get gtid %s-%u from ip %s", __func__, parse_list.first.c_str(), parse_list.second,
                       ip.c_str());
        }
    }

    for (auto &gtid_check : gtid_res) {
        const vector<size_t> &list = gtid_check.second;
        if (list.size() != iplist.size()) {
            continue;
        }

        size_t max_gtid = (size_t) - 1;
        for (auto gtid : list) {
            max_gtid = std::min(max_gtid, gtid);
        }
        gtid_list->push_back(GtidHandler::GenGTID(gtid_check.first, max_gtid));
    }

    return OK;
}

bool MasterMonitor::IsGTIDCompleted(const vector<string> &excute_gtid_list, const string &max_gtid) {
    pair < string, size_t > max_gtid_pair = GtidHandler::ParseGTID(max_gtid);

    if (max_gtid == "") {
        ColorLogInfo("%s get gtid empty , binlog svr max gtid %s, master start", __func__, max_gtid.c_str());
        return true;
    }
    for (size_t i = 0; i < excute_gtid_list.size(); ++i) {
        LogVerbose("%s get max gtid from mysql %s", __func__, excute_gtid_list[i].c_str());

        pair < string, size_t > excute_gtid_pair = GtidHandler::ParseGTID(excute_gtid_list[i]);

        if (excute_gtid_pair.first == max_gtid_pair.first) {
            if (max_gtid_pair.second <= excute_gtid_pair.second) {
                ColorLogInfo("%s get gtid %s max gtid %s, master start", __func__, excute_gtid_list[i].c_str(),
                             max_gtid.c_str());
                return true;
            }
        }
    }

    ColorLogWarning("max gtid %s, not match", max_gtid.c_str());

    return false;
}

int MasterMonitor::CheckMasterRunningStatus(const Option *option) {
    MySqlManager *mysql_manager = MySqlManager::GetGlobalMySqlManager(option);
    string slave_running;
    int ret = mysql_manager->GetValue("status", "Slave_running", &slave_running);
    if (ret) {
        return MYSQL_FAIL;
    }

    if (slave_running == "ON") {
        ColorLogWarning("something wrong, restart");
        if (SlaveMonitor::SlaveStart(option, "127.0.0.1", option->GetBinLogSvrConfig()->GetEnginePort()))
            return MYSQL_FAIL;
        return OK;
    }
    ColorLogInfo("master is running(maybe)");

    return OK;
}

bool MasterMonitor::IsMySqlInit(const Option *option) {
    MySqlManager *mysql_manager = MySqlManager::GetGlobalMySqlManager(option);
    return mysql_manager->IsMySqlInit();
}

int MasterMonitor::CheckMasterInit(const Option *option) {
    MySqlManager *mysql_manager = MySqlManager::GetGlobalMySqlManager(option);
    int ret = mysql_manager->Query(MySqlStringHelper::GetSvrIDString(option->GetBinLogSvrConfig()->GetEngineSvrID()));
    if (ret) {
        ColorLogError("%s master init failed ", __func__);
        return ret;
    }

    //flush grant
    ret = mysql_manager->Query("flush privileges;");
    if (ret) {
        ColorLogError("%s master flush failed ", __func__);
        return ret;
    }
    ColorLogInfo("%s master init done", __func__);

    return OK;
}

int MasterMonitor::CheckMySqlUserInfo(const Option *option) {
    MySqlManager::GetGlobalMySqlManager(option)->CheckMySqlUserInfo();
    return OK;
}

int MasterMonitor::CheckMasterPermisstion(const Option *option, const std::vector<std::string> &member_list) {
    return MySqlManager::GetGlobalMySqlManager(option)->CheckAdminPermission(member_list);
}

}
