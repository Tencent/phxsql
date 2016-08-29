/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include <string>
#include <vector>

#include "replication_base.h"

namespace phxbinlog {

class MasterManager;
class EventManager;
class ReplicationSlave : public ReplicationBase {
 public:
    ReplicationSlave(ReplicationCtx *ctx);
    virtual ~ReplicationSlave();

    virtual int Process();
 protected:
    virtual void Close();

 private:
    enum class CommandStatus {
        MSG = 0,
        QUERY = 1,
        EVENT = 2,
        RUNNING = 3,
    };

    enum class COMMAND {
        QUIT = 1,
        QUERY = 3,
        AUTH = 5,
        REGISTER = 21,
        GTID_COMMAND = 30,
    };
 private:
    int ProcessWithMasterMsg();
    int ProcessWithSlaveMsg();

    int DealWithSlaveMsg(std::string &recv_buff);
    int DealWithSingalMsg(std::vector<std::string> &send_buffer);
    int DealWithQuery(std::vector<std::string> &send_buffer);
    int DealWithEvent(std::vector<std::string> &send_buffer);

    int DealWithCommand(std::string &recv_buff);
    int DealWithGTIDCommand(const char *buffer, const size_t &len);
    int PraseCommand(char *buffer, const size_t &size);
    bool IsQuerryEnd(const char *buffer, const size_t &size);
    void NextStatus();
    int CheckIsMaster();
    int SlaveInitDone();

    int ReceivePkg(std::string *recv_buff);

    int GetWelcomMsg(std::string *recv_buff);
    std::string GetACK();
 private:
    CommandStatus status_;
    EventManager *event_manager_;
    MasterManager *master_manager_;

};

}
