/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include <vector>
#include <string>
#include <stdint.h>

#include "phxcomm/base_config.h"

namespace phxbinlog {

class PHXBinlogSvrBaseConfig : public phxsql::PhxBaseConfig {

 public:
    uint32_t GetLogLevel() const;
    uint32_t GetLogMaxSize() const;
    const char * GetLogFilePath() const;
    uint32_t GetMaxDeleteCheckPointFileNum() const;
    bool IsForceMakingCheckPoint() const;

    //for paxos lib
    int GetPaxosPort() const;
    const char * GetPaxosLogPath() const;
    int GetPackageMode() const;
    uint32_t GetPaxosLogNum() const;
    uint32_t GetUDPMaxSize() const;

    uint32_t GetMasterLeaseTime() const;
    uint32_t GetMasterExtLeaseTime() const;

    const char * GetEngineIP() const;
    int GetEnginePort() const;
    uint32_t GetEngineSvrID() const;
    const char * GetBinlogSvrIP() const;
    int GetBinlogSvrPort() const;
    const char * GetSpecifiedMasterIP() const;

    const char * GetEventDataBaseDir() const;
    const char * GetEventDataStorageDBDir() const;
    const char * GetEventDataBackUPDir() const;
    uint32_t GetMaxEventCountToPush() const;
    uint32_t GetMaxEventFileSize() const;

    uint32_t GetCheckPointMakingPeriod() const;
    uint32_t GetMonitorCheckStatusPeriod() const;

    std::string GetPackageName() const;

    //get the expire time  for the rpc call
    std::pair<uint32_t, uint32_t> GetTimeOut() const;
    uint32_t GetTimeOutMS() const;

    std::string GetFollowIP() const;

 protected:
    PHXBinlogSvrBaseConfig(const char * filename = "phxbinlogsvr.conf");
    ~PHXBinlogSvrBaseConfig();
    virtual void ReadConfig();

    void SetExtLeaseTime(const uint32_t &master_ext_lease_time);

    friend class Option;

 private:
    void ReadAgentConfig();
    void ReadPaxosConfig();

 private:
    std::string engine_ip_;
    std::string specified_master_ip_;
    int engine_port_;
    int binlogsvr_port_;
    uint32_t engine_svrid_;

    std::string event_data_base_path_, event_data_db_path_, event_data_backup_path_;
    uint32_t event_max_file_size_;
    uint32_t max_event_push_;
    uint32_t rpc_timeout_, rpc_utimeout_;
    std::string package_name_;

    uint32_t mysql_checkstatus_timeperiod_;

    uint32_t master_lease_time_;
    uint32_t master_ext_lease_time_;
    uint32_t checkpoint_making_timeperiod_;

    std::string paxos_log_path_;
    int paxos_port_;
    uint32_t paxos_log_num_;
    uint32_t packet_mode_;
    uint32_t udp_max_size_;

    uint32_t log_level_;

    uint32_t log_maxsize_;
    uint32_t max_delete_checkpoint_file_num_;
    bool force_makeing_checkpoint_;

    std::string log_path_;
    std::string follow_ip_;
};

}

