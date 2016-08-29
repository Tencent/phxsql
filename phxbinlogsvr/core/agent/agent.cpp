/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "agent.h"

#include "agent_manager.h"
#include "data_manager.h"
#include "paxos_agent_manager.h"

#include "phxcomm/net.h"
#include "phxcomm/phx_log.h"
#include "phxbinlogsvr/statistics//phxbinlog_stat.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/define/errordef.h"

#include <assert.h>
#include <unistd.h>
#include <signal.h>

using std::string;
using std::vector;
using phxsql::NetIO;
using phxsql::ColorLogError;
using phxsql::LogVerbose;

namespace phxbinlog {

PhxBinLogAgent::PhxBinLogAgent(Option *option) {

    ip_ = "127.0.0.1";
    port_ = option->GetBinLogSvrConfig()->GetEnginePort();
    server_sockfd_ = -1;

    data_manager_ = new DataManager(option);
    agent_manager_ = new AgentManager(option);
    option_ = option;
}

PhxBinLogAgent::~PhxBinLogAgent() {
    if (server_sockfd_ >= 0) {
        close(server_sockfd_);
        server_sockfd_ = -1;
    }
    delete data_manager_, data_manager_ = NULL;
    delete agent_manager_, agent_manager_ = NULL;
}

int PhxBinLogAgent::InitAgentMaster() {
    return agent_manager_->InitAgentMaster();
}

int PhxBinLogAgent::Bind(const char *ip, const int port) {
    server_sockfd_ = NetIO::Bind(ip, port);
    if (server_sockfd_ < 0) {
        ColorLogError("init fail ip %s port %d", ip, port);
        return SVR_FAIL;
    }
    return OK;
}

int PhxBinLogAgent::Process() {
    if (server_sockfd_ < 0) {
        if (Bind(ip_.c_str(), port_)) {
            server_sockfd_ = -1;
            return SVR_FAIL;
        }
    }

    while (1) {
        int client_fd = NetIO::Accept(server_sockfd_);
        if (client_fd < 0) {
            STATISTICS(MySqlAcceptFail());
            ColorLogError("recv client fd %d fail exit ", client_fd);
            break;
        }
        STATISTICS(MySqlAcceptSucess());
        data_manager_->DealWithSlave(client_fd);
    }

    return OK;
}

int PhxBinLogAgent::SendEvent(const string &oldgtid, const string &newgtid, const string &event_buffer) {
    return data_manager_->SendEvent(oldgtid, newgtid, event_buffer);
}

int PhxBinLogAgent::GetLastSendGTID(const string &uuid, string *gtid) {
    return data_manager_->GetLastSendGTID(uuid, gtid);
}

int PhxBinLogAgent::SetExportIP(const string &export_ip, const uint32_t &port) {
    return agent_manager_->SetExportIP(export_ip, port);
}

int PhxBinLogAgent::GetMaster(string *master_ip, uint32_t *update_time, uint32_t *expire_time, uint32_t *version) {
    return agent_manager_->GetMaster(master_ip, update_time, expire_time, version);
}

int PhxBinLogAgent::SetHeartBeat(const uint32_t &flag) {
    return agent_manager_->SetHeartBeat(flag);
}

int PhxBinLogAgent::AddMember(const string &ip) {
    return agent_manager_->AddMember(ip);
}

int PhxBinLogAgent::RemoveMember(const string &ip) {
    return agent_manager_->RemoveMember(ip);
}

int PhxBinLogAgent::GetMemberList(vector<string> *member_list) {
    return agent_manager_->GetMemberList(member_list);
}

int PhxBinLogAgent::SetMySqlAdminInfo(const std::string &now_admin_username, const std::string &now_admin_pwd,
                                      const std::string &new_username, const std::string &new_pwd) {
    return agent_manager_->SetMySqlAdminInfo(now_admin_username, now_admin_pwd, new_username, new_pwd);
}

int PhxBinLogAgent::SetMySqlReplicaInfo(const std::string &now_admin_username, const std::string &now_admin_pwd,
                                        const std::string &new_username, const std::string &new_pwd) {
    return agent_manager_->SetMySqlReplicaInfo(now_admin_username, now_admin_pwd, new_username, new_pwd);
}

int PhxBinLogAgent::FlushData() {
    return data_manager_->FlushData();
}

}

