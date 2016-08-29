/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "phxbinlog_svr_handler.h"

#include "phxbinlogsvr/client/phxbinlog_client_platform_info.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxcomm/phx_log.h"
#include "agent.h"

using phxbinlog::Option;
using phxbinlog::PhxBinLogAgent;
using std::string;

using phxsql::LogVerbose;

namespace phxbinlogsvr {

PhxBinLogSvrHandler::PhxBinLogSvrHandler(Option *option, PhxBinLogClientFactoryInterface *client_factory) {
    option_ = option;
    create_binLog_process_thread_ = 0;

    if (client_factory) {
        PhxBinlogClientPlatformInfo *platform = PhxBinlogClientPlatformInfo::GetDefault();
        platform->RegisterClientFactory(client_factory);
    }
}

PhxBinLogSvrHandler::~PhxBinLogSvrHandler() {
}

void PhxBinLogSvrHandler::BeforeRun() {
    binlogagent_ = new PhxBinLogAgent(option_);
    binlogsvr_logic_ = new PhxBinlogSvrLogic(option_, binlogagent_);
    pthread_create(&create_binLog_process_thread_, NULL, &CreateBinLogProcess, this);
}

void PhxBinLogSvrHandler::AfterRun() {
    if (binlogagent_)
        delete binlogagent_, binlogagent_ = NULL;
    if (binlogsvr_logic_)
        delete binlogsvr_logic_, binlogsvr_logic_ = NULL;
}

PhxBinLogAgent *PhxBinLogSvrHandler::GetBinLogAgent() {
    return binlogagent_;
}

PhxBinlogSvrLogic *PhxBinLogSvrHandler::GetSvrLogic() {
    return binlogsvr_logic_;
}

void *PhxBinLogSvrHandler::CreateBinLogProcess(void *arg) {
    LogVerbose("%s binlog svr start", __func__);
    PhxBinLogSvrHandler *svr = (PhxBinLogSvrHandler*) arg;
    svr->GetBinLogAgent()->Process();

    return NULL;
}

int PhxBinLogSvrHandler::SendBinLog(const string &req_buffer) {
    return binlogsvr_logic_->SendBinLog(req_buffer);
}

int PhxBinLogSvrHandler::SetExportIP(const string &req_buffer) {
    return binlogsvr_logic_->SetExportIP(req_buffer);
}

int PhxBinLogSvrHandler::GetMasterInfoFromGlobal(string *resp_buffer) {
    return binlogsvr_logic_->GetMasterInfoFromGlobal(resp_buffer);
}

int PhxBinLogSvrHandler::GetLastSendGTIDFromGlobal(const string &req_buffer, string *resp_buffer) {
    return binlogsvr_logic_->GetLastSendGTIDFromGlobal(req_buffer, resp_buffer);
}

int PhxBinLogSvrHandler::GetMasterInfoFromLocal(string *resp_buffer) {
    return binlogsvr_logic_->GetMasterInfoFromLocal(resp_buffer);
}

int PhxBinLogSvrHandler::GetLastSendGTIDFromLocal(const string &req_buffer, string *resp_buffer) {
    return binlogsvr_logic_->GetLastSendGTIDFromLocal(req_buffer, resp_buffer);
}

int PhxBinLogSvrHandler::HeartBeat(const string &req_buffer) {
    return binlogsvr_logic_->HeartBeat(req_buffer);
}

int PhxBinLogSvrHandler::AddMember(const string &req_buffer) {
    return binlogsvr_logic_->AddMember(req_buffer);
}

int PhxBinLogSvrHandler::RemoveMember(const string &req_buffer) {
    return binlogsvr_logic_->RemoveMember(req_buffer);
}

int PhxBinLogSvrHandler::SetMySqlAdminInfo(const std::string &req_buffer) {
    return binlogsvr_logic_->SetMySqlAdminInfo(req_buffer);
}

int PhxBinLogSvrHandler::SetMySqlReplicaInfo(const std::string &req_buffer) {
    return binlogsvr_logic_->SetMySqlReplicaInfo(req_buffer);
}

int PhxBinLogSvrHandler::GetMemberList(std::string *resp_buffer) {
    return binlogsvr_logic_->GetMemberList(resp_buffer);
}

int PhxBinLogSvrHandler::InitBinlogSvrMaster(const std::string &req_buffer) {
    return binlogsvr_logic_->InitBinlogSvrMaster(req_buffer);
}

}

