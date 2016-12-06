/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/


#include "replication_slave.h"

#include <string.h>
#include <unistd.h>

#include "event_manager.h"
#include "master_manager.h"
#include "agent_monitor.h"
#include "slave_monitor.h"
#include "master_agent.h"
#include "paxos_agent_manager.h"
#include "replication_ctx.h"

#include "phxcomm/net.h"
#include "phxcomm/phx_log.h"
#include "phxbinlogsvr/statistics/phxbinlog_stat.h"
#include "phxbinlogsvr/core/gtid_handler.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/define/errordef.h"

using std::string;
using std::vector;
using std::pair;
using std::make_pair;
using phxsql::NetIO;
using phxsql::ColorLogWarning;
using phxsql::ColorLogInfo;
using phxsql::ColorLogError;
using phxsql::LogVerbose;

namespace phxbinlog {

ReplicationSlave::ReplicationSlave(ReplicationCtx *ctx) :
        ReplicationBase(ctx) {
    event_manager_ = EventManager::GetGlobalEventManager(ctx->GetOption());
    master_manager_ = MasterManager::GetGlobalMasterManager(ctx->GetOption());
}

ReplicationSlave::~ReplicationSlave() {
    Close();
}

void ReplicationSlave::Close() {
    ctx_->Close();
}

int ReplicationSlave::Process() {
    //stop monitor to check connection

    SlaveMonitor::SlaveConnecting(true);
    status_ = CommandStatus::MSG;
    int ret = OK;
    while (1) {
        ret = ProcessWithMasterMsg();
        if (ret) {
            ColorLogError("%s process master msg fail ret %d", __func__, ret);
			break;
        }

        if (status_ == CommandStatus::RUNNING) {
            //all things is done
            int ret = CheckIsMaster();
            if (ret == NOT_MASTER) {
                STATISTICS(ReplSlaveLogin());
                ColorLogInfo("%s it is not master, do with slave", __func__);
                ret = OK;
            } else if (ret != OK) {
                ColorLogInfo("%s check master fail", __func__);
            } else if (ret == OK) {
                STATISTICS(ReplMasterLogin());
            }
            SlaveInitDone();
            ColorLogInfo("check master ret %d", ret);
            break;
        }

        ret = ProcessWithSlaveMsg();

        if (ret) {
            ColorLogError("%s process slave msg fail ret %d", __func__, ret);
            break;
        }

    }
    if (ret == 0) {
        STATISTICS(ReplLoginSucess());
    } else {
        STATISTICS(ReplLoginFail());
    }
    ColorLogInfo("%s slave exit, ret %d", __func__, ret);
    SlaveMonitor::SlaveConnecting(false);
    return ret;
}

int ReplicationSlave::ProcessWithSlaveMsg() {
    string recv_buff;
    int ret = NetIO::Receive(ctx_->GetSlaveFD(), &recv_buff);
    if (ret) {
        ColorLogError("%s slave recv fail %d", __func__, ret);
        STATISTICS(ReplRecvDataFail());
        return SOCKET_FAIL;
    }

    if (recv_buff.size() == 0) {
        ColorLogError("%s slave connect is lost", __func__);
        return DATA_EMPTY;
    }

    ret = DealWithSlaveMsg(recv_buff);
    if (ret)
        return ret;

    ret = NetIO::Send(ctx_->GetMasterFD(), recv_buff);
    LogVerbose("%s send master buffer len %zu ret %d", __func__, recv_buff.size(), ret);
    if (ret) {
        STATISTICS(ReplSendDataFail());
        return SOCKET_FAIL;
    }

    return OK;
}

int ReplicationSlave::DealWithSlaveMsg(string &recv_buff) {
    COMMAND command_id = (COMMAND) recv_buff[4];

    ColorLogInfo("%s recv slave buffer len %zu, command id %u ", __func__, recv_buff.size(), command_id);
    switch (command_id) {
        case COMMAND::QUERY: {
            //LogVerbose("%s command %d %s", __func__, command_id, recv_buff.c_str() + 5);
            status_ = CommandStatus::QUERY;
            break;
        }
        case COMMAND::AUTH:
        case COMMAND::REGISTER: {
            status_ = CommandStatus::MSG;
            break;
        }
        case COMMAND::GTID_COMMAND: {
            int ret = DealWithGTIDCommand(recv_buff.c_str() + 5, recv_buff.size() - 5);
            if (ret) {
                return ret;
            }

            status_ = CommandStatus::EVENT;
            //LogVerbose("%s command %d", __func__, command_id);
            break;
        }
        case COMMAND::QUIT:
            return SLAVE_FAIL;
        default:
            ColorLogError("%s unknown command %d, exit", __func__, command_id);
            return SLAVE_FAIL;
    }
    return OK;
}

int ReplicationSlave::ProcessWithMasterMsg() {
    do {
        LogVerbose("%s get msg status %d", __func__, status_);
        vector < string > send_buffer;
        int ret = OK;
        switch (status_) {
            case CommandStatus::MSG:
                ret = DealWithSingalMsg(send_buffer);
                break;
            case CommandStatus::QUERY:
                ret = DealWithQuery(send_buffer);
                break;
            case CommandStatus::EVENT:
                ret = DealWithEvent(send_buffer);
                break;
            case CommandStatus::RUNNING:
                return OK;
            default:
                break;
        }

        if (status_ == CommandStatus::RUNNING && send_buffer.size() == 0) {
            return OK;
        }

        if (send_buffer.size() == 0) {
            return DATA_EMPTY;
        }

        if (ret == OK) {
            for (size_t i = 0; i < send_buffer.size(); ++i) {
                const string &recv_buff = send_buffer[i];
                ret = NetIO::Send(ctx_->GetSlaveFD(), recv_buff);
                if (ret) {
                    ColorLogError("%s send to slave fail, ret %d", __func__, ret);
                    STATISTICS(ReplSendDataFail());
                    break;
                }
            }
        }
        if (ret)
            return ret;
    } while (status_ == CommandStatus::EVENT);
    return OK;
}

int ReplicationSlave::DealWithSingalMsg(vector<string> &send_buffer) {
    string recv_buff;
    int ret = ReceivePkg(&recv_buff);
    LogVerbose("%s recv master buffer len %zu ret %d seq %d buffer seq %d", __func__, recv_buff.size(), ret,
               ctx_->GetSeq(), recv_buff[3]);
    if (recv_buff.size()) {
        send_buffer.push_back(recv_buff);
    }
    return ret;
}

bool ReplicationSlave::IsQuerryEnd(const char *buffer, const size_t &size) {
    return size < 8 && buffer[0] == (char) 254;
}

int ReplicationSlave::DealWithQuery(vector<string> &send_buffer) {
    //field count
    {
        string recv_buff;
        int ret = ReceivePkg(&recv_buff);
        if (ret)
            return ret;
        send_buffer.push_back(recv_buff);
        const char *pos = recv_buff.c_str() + 4;
        if (recv_buff.size() < 5 || *pos == 0) {
            ColorLogError("%s field count empty, return", __func__);
            return OK;
        }
    }
    //first read the meta data of fiedls
    int ret = 0;
    while (ret == 0) {
        string recv_buff;
        int ret = ReceivePkg(&recv_buff);
        if (ret == 0)
            send_buffer.push_back(recv_buff);
        if (recv_buff.size() < 5)
            ret = BUFFER_FAIL;
        if (ret || IsQuerryEnd(recv_buff.c_str() + 4, recv_buff.size() - 4)) {
            break;
        }
    }

    //read the data
    while (ret == 0) {
        string recv_buff;
        int ret = ReceivePkg(&recv_buff);
        if (recv_buff.size() < 5)
            ret = BUFFER_FAIL;
        if (ret == OK) {
            if (ctx_->IsSelfConn()) {
                PraseCommand((char *) recv_buff.c_str() + 4, recv_buff.size() - 4);
            }
            if (ret) {
                return ret;
            }
            send_buffer.push_back(recv_buff);
        }
        if (ret || IsQuerryEnd(recv_buff.c_str() + 4, recv_buff.size() - 4)) {
            break;
        }
    }

    return ret;
}

bool ReWriteCommand(const char *key_pos, const uint8_t key_size, char *value_pos, uint8_t value_size,
                    const string &check_key, string *old_value) {
    bool ok = false;
    if (strncasecmp(key_pos, check_key.c_str(), key_size) == 0) {
        *old_value = string(value_pos, value_size);
        for (int j = 1; j < value_size; ++j) {
            if (*(value_pos + j) < '9') {
                (*(value_pos + j))++;
                ok = true;
            }
        }
    }
    return ok;
}

int ReplicationSlave::PraseCommand(char *buffer, const size_t &size) {
    if (IsQuerryEnd(buffer, size)) {
        return 0;
    }

    vector < pair<char *, uint8_t> > fields;

    char *pos = buffer;
    const char *end = buffer + size;
    while (pos < end) {
        uint8_t len = *(uint8_t *) pos;
        pos++;
        if (pos + len > end)
            break;
        string get_buffer = string(pos, len);
        fields.push_back(make_pair(pos, len));
        pos += len;
    }

    for (size_t i = 0; i < fields.size(); ++i) {
        LogVerbose("%s get field len %d value %s", __func__, fields[i].second,
                   string(fields[i].first, fields[i].second).c_str());
        if (i == 1) {
            string old_value;
            if (ReWriteCommand(fields[i - 1].first, fields[i - 1].second, fields[i].first, fields[i].second,
                               "server_id", &old_value)) {
                uint32_t my_svrid = ctx_->GetOption()->GetBinLogSvrConfig()->GetEngineSvrID();

                uint32_t pkg_svrid = 0;
                for (uint8_t j = 0; j < old_value.size(); ++j) {
                    pkg_svrid = pkg_svrid * 10 + old_value[j] - '0';
                }
                if (pkg_svrid != my_svrid) {
                    ColorLogWarning("svr id conflit, mysql svr %d, agent svr id %d", pkg_svrid, my_svrid);
                    return SVR_ID_CONFIT;
                }

                string new_value = string(fields[i].first, fields[i].second);
                LogVerbose("%s old value %s new value %s", __func__, old_value.c_str(), new_value.c_str());
            } else
                ReWriteCommand(fields[i - 1].first, fields[i - 1].second, fields[i].first, fields[i].second,
                               "server_uuid", &old_value);
        }
    }

    return 0;
}

int ReplicationSlave::DealWithEvent(vector<string> &send_buffer) {
    string recv_buff;
    int ret = ReceivePkg(&recv_buff);
    if (ret) {
        if (ret == DATA_EMPTY)
            return ret;
        return ret;
    }

    if (*(uint8_t*) (recv_buff.c_str() + 4) == 0xFE) {
        ColorLogError("%s master err", __func__);
        return MASTER_FAIL;
    }

    EventInfo info;
    ret = GtidHandler::ParseEvent((unsigned char*) recv_buff.c_str() + 5, recv_buff.size() - 5, &info);
    if (ret) {
        STATISTICS(ReplGtidParseFail());
        ColorLogError("%s not event buffer", __func__);
        return ret;
    }

    if (status_ != CommandStatus::RUNNING) {
        if (info.event_type == EVENT_GTID || info.event_type == EVENT_PERCONA_GTID
                || info.event_type == EVENT_HEARTBEAT) {
            ColorLogInfo("mysql init ok");
            status_ = CommandStatus::RUNNING;
        }
    }

    if (status_ == CommandStatus::RUNNING) {
        if (info.event_type == EVENT_HEARTBEAT)
            send_buffer.push_back(recv_buff);
    } else {
        send_buffer.push_back(recv_buff);
    }

    uint8_t seq = *(uint8_t *) (recv_buff.c_str() + 3);
    if (send_buffer.size()) {
        ctx_->SetSeq(seq + 1);
    }

    return 0;
}

int ReplicationSlave::DealWithGTIDCommand(const char *buffer, const size_t &len) {
    EventInfo info;
    int ret = GtidHandler::ParsePreviousGTIDCommand((unsigned char *) buffer, len, &info);
    if (ret) {
        return ret;
    }

    LogVerbose("%s get previous gtid size %zu", __func__, info.previous_gtidlist.size());

    vector < string > previous_gtidlist;
    for (size_t i = 0; i < info.previous_gtidlist.size(); ++i) {
        previous_gtidlist.push_back(info.previous_gtidlist[i].second);
    }

    return event_manager_->SignUpGTID(previous_gtidlist);
}

int ReplicationSlave::ReceivePkg(string *recv_buff) {
    int ret = NetIO::Receive(ctx_->GetMasterFD(), recv_buff);
    if (ret)
        return SOCKET_FAIL;
    if (recv_buff->size() == 0) {
        STATISTICS(ReplRecvDataFail());
        ColorLogError("%s master connect fail", __func__);
        return DATA_EMPTY;
    }

    const char *text = recv_buff->c_str() + 4;
    if (*text == (char) 0xff) {
        STATISTICS(ReplRecvDataFail());
        int err_num = 0;
        memcpy(&err_num, text + 1, 2);
        ColorLogError("%s err %s", __func__, text + 3);
        return OK;
    }

    return OK;
}

int ReplicationSlave::CheckIsMaster() {
    MasterAgent *masteragent = PaxosAgentManager::GetGlobalPaxosAgentManager(ctx_->GetOption())->GetMasterAgent();
    //first check
    if (master_manager_->CheckMasterBySvrID(ctx_->GetOption()->GetBinLogSvrConfig()->GetEngineSvrID())) {
        if (AgentMonitor::IsGTIDCompleted(ctx_->GetOption(), event_manager_)) {
            //update to the newest one
            return masteragent->MasterNotify();
        }
    }
    return NOT_MASTER;
}

int ReplicationSlave::SlaveInitDone() {
    ctx_->Notify();
    return 0;
}

}
