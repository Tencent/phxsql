/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "agent_monitor.h"

#include "event_manager.h"
#include "master_manager.h"
#include "master_monitor.h"
#include "master_agent.h"
#include "slave_monitor.h"
#include "mysql_manager.h"
#include "storage_manager.h"
#include "checkpoint_manager.h"
#include "agent_monitor_comm.h"
#include "paxos_agent_manager.h"

#include "phxcomm/lock_manager.h"
#include "phxcomm/phx_log.h"
#include "phxbinlogsvr/statistics/phxbinlog_stat.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/define/errordef.h"

#include <sys/time.h>
#include <unistd.h>

using std::string;
using std::vector;
using phxsql::LogVerbose;
using phxsql::ColorLogError;
using phxsql::ColorLogWarning;
using phxsql::ColorLogInfo;

namespace phxbinlog {

AgentMonitor::AgentMonitor(const Option *option) {
    option_ = option;
    event_manager_ = EventManager::GetGlobalEventManager(option);
    master_manager_ = MasterManager::GetGlobalMasterManager(option);
    master_agent_ = PaxosAgentManager::GetGlobalPaxosAgentManager(option)->GetMasterAgent();

    agent_monitor_comm_ = AgentMonitorComm::GetGlobalAgentMonitorComm();
    init_ = false;

    Run();
}

AgentMonitor::~AgentMonitor() {
}

int AgentMonitor::Process() {
    LogVerbose("monitor running");
	uint32_t last_check = 0;
    while (1) {
        bool check_master = false;
        {
            struct timeval now;
            gettimeofday(&now, NULL);
            struct timespec outtime;
            int status = agent_monitor_comm_->GetStatus();

            outtime.tv_sec = now.tv_sec + 1;
            outtime.tv_nsec = now.tv_usec;

            LogVerbose("%s monitor status %d wati time %d", __func__, status, outtime.tv_sec);
            agent_monitor_comm_->TimeWait(outtime);

            status = agent_monitor_comm_->GetStatus();

            LogVerbose("%s monitor is running, status %d", __func__, status);
            switch (status) {
                case AgentMonitorComm::NOTHING: {
                    MasterInfo info;
                    int ret = master_manager_->GetMasterInfo(&info);
                    if (ret == OK) {
                        if (info.version() == 0) {
                            ColorLogInfo("%s master is not inited, waiting init, doing nothing", __func__);
                            continue;
                        }
                        ColorLogInfo("%s master data has been here, continue", __func__);
                    } else {
                        ColorLogInfo("%s get master fail, wait init, doing nothing", __func__);
                        continue;
                    }
                }
                case AgentMonitorComm::WAITING: {
                    MasterInfo info;
                    int ret = master_manager_->GetMasterInfo(&info);
                    if (ret == OK && info.version() > 0) {
                        //has master, but in a wrong status
                    } else {
                        check_master = true;
                        break;
                    }
                }
                case AgentMonitorComm::MASTER_CHANGING: {
                    int ret = CheckMasterInit();
                    if (ret == OK) {
                        ColorLogInfo("%s check master init ret %d, master start next status %d", __func__, ret,
                                     AgentMonitorComm::MASTER_CHANGING_INIT);
                        agent_monitor_comm_->SetStatus(AgentMonitorComm::MASTER_CHANGING_INIT);
                    } else {
                        ColorLogWarning("%s check master init fail %d", __func__, ret);
                        STATISTICS(MasterChangeFail());
                        break;
                    }
                }
                case AgentMonitorComm::MASTER_CHANGING_INIT: {
                    int ret = MasterInit();
                    if (ret == OK) {
                        ColorLogInfo("%s master changing init done, next status %d", __func__,
                                     AgentMonitorComm::RUNNING);
                        agent_monitor_comm_->SetStatus(AgentMonitorComm::RUNNING);
                    } else {
                        STATISTICS(MasterInitFail());
                        ColorLogWarning("%s master changeing fail %d", __func__, ret);
                        break;
                    }
                }
                case AgentMonitorComm::RUNNING: {
                    int ret = CheckRunning();
                    ColorLogInfo("%s check running ret %d", __func__, ret);
                    if (ret == OK) {
                        check_master = true;
                    } else {
                        STATISTICS(MonitorCheckFail());
                    }
                    break;
                }
                default:
                    ColorLogInfo("new machine? waiting init");
                    break;

            }
        }
        MasterMonitor::CheckMySqlUserInfo(option_);
        uint32_t now = time(NULL);
        if (check_master && now - last_check >= option_->GetBinLogSvrConfig()->GetMonitorCheckStatusPeriod()) {
            int ret = CheckMasterTimeOut();
            CheckCheckPointFiles();
			if(ret == 0) {
				last_check = now;
			}
        }
    }
    return OK;
}

int AgentMonitor::CheckRunning() {
    MySqlManager * mysql_manager = MySqlManager::GetGlobalMySqlManager(option_);
    string read_only;
    int ret = mysql_manager->GetValue("variables", "super_read_only", &read_only);
    if (ret) {
        return ret;
    }
    //slave?
    bool is_master = master_manager_->CheckMasterBySvrID(option_->GetBinLogSvrConfig()->GetEngineSvrID());
    if (read_only == "OFF") {
        if (is_master) {
            MasterInfo master_info;
            ret = master_manager_->GetMasterInfo(&master_info);
            if (ret == OK) {
                if (master_info.export_ip().empty()) {
                    //master is running?
                    ret = MasterMonitor::CheckMasterRunningStatus(option_);
                    if (ret == 0) {
                        vector < string > member_list;
                        master_manager_->GetMemberIPList(&member_list);
                        ret = MasterMonitor::CheckMasterPermisstion(option_, member_list);
                    }
                } else {
                    uint32_t export_port = master_info.export_port();
                    if (export_port == 0) {
                        export_port = option_->GetBinLogSvrConfig()->GetEnginePort();
                    }

                    ColorLogInfo("%s check export %s:%u status", __func__, master_info.export_ip().c_str(),
                                 export_port);
                    ret = SlaveMonitor::CheckSlaveRunningStatus(option_, master_info.export_ip().c_str(), &export_port);
                }
            }
        } else {
            STATISTICS(MasterStatErr());
            ColorLogWarning("something err in slave?, reboot slave");
            //slave but open read
            if (IsSlaveReady()) {
                ret = SlaveMonitor::SlaveStart(option_, "127.0.0.1", option_->GetBinLogSvrConfig()->GetEnginePort());
            }
        }
    } else {
        if (is_master) {
            STATISTICS(SlaveStatErr());
            ColorLogWarning("something err in master?, check master status");
            ret = CheckMasterInit();
        } else {
            //slave is running?
            ret = CheckSlaveRunningStatus();
        }
    }

    ColorLogInfo("%s check super read only %s is master %d ret %d", __func__, read_only.c_str(), is_master, ret);
    return ret;
}

int AgentMonitor::MasterInit() {
    MasterInfo master_info;
    int ret = master_manager_->GetMasterInfo(&master_info);
    if (ret) {
        return ret;
    }

    LogVerbose("%s master info %u version %u", __func__, master_info.svr_id(), master_info.version());

    if (master_info.svr_id() == option_->GetBinLogSvrConfig()->GetEngineSvrID()
            && !master_manager_->IsTimeOut(master_info)) {
        if (!init_) {
            //do not check root privilege beacuse its readonly option has not been set
            //after master has beed started, check again
            ret = MasterMonitor::CheckMasterInit(option_);
            if (ret == OK) {
                if (!MasterMonitor::IsMySqlInit(option_)) {
                    MySqlManager * mysql_manager = MySqlManager::GetGlobalMySqlManager(option_);
                    string current_admin_username = mysql_manager->GetAdminUserName();
                    string current_admin_pwd = mysql_manager->GetAdminPwd();
                    if (current_admin_username == "") {
                        ret = master_agent_->SetMySqlAdminInfo("", "", "root", "");

                        if (ret) {
                            ColorLogError("%s master set admin info failed ", __func__);
                            return ret;
                        }
                    }

                    string current_replica_username = mysql_manager->GetReplicaUserName();
                    string current_replica_pwd = mysql_manager->GetReplicaPwd();
                    if (current_replica_username == "") {
                        ret = master_agent_->SetMySqlReplicaInfo("", "", "replica", "replica123");
                        if (ret) {
                            ColorLogError("%s master set replica info failed ", __func__);
                            return ret;
                        }
                    }
                    init_ = true;
                }
                LogVerbose("%s master init done, version %u", __func__, master_info.version());
            }
        } else
            LogVerbose("%s master init has done, version %u, skip check init", __func__, master_info.version());
    } else {
        LogVerbose("%s i'm not master, skip check init", __func__);
    }

    return ret;
}

int AgentMonitor::CheckMasterInit() {
    int ret = OK;
    bool is_master = master_manager_->CheckMasterBySvrID(option_->GetBinLogSvrConfig()->GetEngineSvrID());
    if (is_master) {
        // get the current gtid in master binlog
        vector < string > gtid_list;
        int ret = MasterMonitor::GetMySQLMaxGTIDList(option_, &gtid_list);
        if (ret)
            return ret;

        //get the max gtid in agent
        string last_gtid = event_manager_->GetNewestGTID();
        if (MasterMonitor::IsGTIDCompleted(gtid_list, last_gtid)) {
            MasterInfo master_info;
            int ret = master_manager_->GetMasterInfo(&master_info);
            if (ret == 0) {
                LogVerbose("%s set master start, export ip %s:%u", __func__, master_info.export_ip().c_str(),
                           master_info.export_port());
                if (master_info.export_ip().empty()) {
                    ret = MasterMonitor::MasterStart(option_);
                } else {
                    uint32_t export_port = master_info.export_port();
                    if (export_port == 0) {
                        export_port = option_->GetBinLogSvrConfig()->GetEnginePort();
                    }
                    ret = SlaveMonitor::SlaveStart(option_, master_info.export_ip(), export_port, false);
                }
            }
        } else {
            ret = CheckSlaveRunningStatus();
            ColorLogInfo("%s master is loading data, check slave running ret %d", __func__, ret);
            if (ret == 0)
                ret = 1;
        }
        return ret;
    } else {
        vector < string > gtid_list;
        int ret = SlaveMonitor::SlaveCheckRelayLog(option_, &gtid_list);
        if (ret == MYSQL_FAIL)
            return ret;
        ColorLogInfo("%s relay log has existed, relaylog size %zu", __func__, gtid_list.size());

        //return CheckSlaveRunningStatus();
    }

    return ret;
}

int AgentMonitor::IsGTIDCompleted(const Option *option, EventManager *event_manager) {
	vector < string > gtid_list;
	int ret = MasterMonitor::GetMySQLMaxGTIDList(option, &gtid_list);
	if (ret) {
		ColorLogWarning("%s get sql info fail, skip", __func__);
		return SVR_FAIL;
	}

	string last_gtid = event_manager->GetNewestGTID();
	if (MasterMonitor::IsGTIDCompleted(gtid_list, last_gtid)) {
	} else {
		return DATA_EMPTY;
	}

	return OK;
}

int AgentMonitor::CheckMasterTimeOut() {
	bool is_master = master_manager_->CheckMasterBySvrID(option_->GetBinLogSvrConfig()->GetEngineSvrID());
    int ret = OK;
	if(!is_master) {
		ret=IsGTIDCompleted(option_, event_manager_);
		if (ret < 0) {
			return ret;
		}
	}
    
	if (!AgentExternalMonitor::IsHealthy()) {
		ColorLogWarning("%s external monitor not healthy", __func__);
        return SVR_FAIL;
    }

    if (ret == OK) {
        int ret = master_agent_->SetMaster(option_->GetBinLogSvrConfig()->GetEngineIP());
        if (ret == MASTER_CONFLICT) {
            LogVerbose("%s master info does not timeout, skip", __func__);
        } else {
            LogVerbose("%s set new master info, ret %d", __func__, ret);
        }
        return OK;
    } else {
        ColorLogInfo("%s master info not complated, skip, just check slave status", __func__);
        return CheckSlaveRunningStatus();
    }
    return ret;
}

int AgentMonitor::CheckSlaveRunningStatus() {
        return SlaveMonitor::CheckSlaveRunningStatus(option_);}

bool AgentMonitor::IsSlaveReady() {

    vector < string > gtid_list;
    int ret = MasterMonitor::GetMySQLMaxGTIDList(option_, &gtid_list);
    if (ret) {
        ColorLogError("%s get sql info fail", __func__);
        return false;
    }

    string gtid = "";
    ret = event_manager_->GetLastGtid(gtid_list, &gtid);
    if (ret) {
        ColorLogError("%s get last gtid fail, gtid list %zu", __func__, gtid_list.size());
        return false;
    }
    LogVerbose("%s get last gtid done, get last list %s", __func__, gtid.c_str());
    return true;
}

void AgentMonitor::CheckCheckPointFiles() {
    CheckPointManager * checkpoint_manager = StorageManager::GetGlobalStorageManager(option_)->GetCheckPointManager();
    uint64_t current_instanceid_interval = checkpoint_manager->GetCurrentInstanceInterval();
    LogVerbose("%s reset hold paxos count %llu", __func__, current_instanceid_interval * 3);
    master_agent_->SetHoldPaxosLogCount(current_instanceid_interval * 3);
}

}
