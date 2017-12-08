/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at

	https://opensource.org/licenses/GPL-2.0

	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "phxbinlog_service_impl.h"

#include "phxbinlog_svr_handler.h"
#include "phxbinlog_server_config.h"
#include "phxbinlogsvr.pb.h"

using phxbinlogsvr::PhxBinLogSvrHandler;

PhxbinlogServiceImpl::PhxbinlogServiceImpl(ServiceArgs_t * service_args) :
        svr_handler_(service_args->svr_handler_) {
}

PhxbinlogServiceImpl::~PhxbinlogServiceImpl() {
}

int PhxbinlogServiceImpl::PhxEcho(const google::protobuf::StringValue &req, google::protobuf::StringValue *resp) {
    resp->set_value(req.value());
    return 0;
}

int PhxbinlogServiceImpl::SendBinLog(const google::protobuf::BytesValue &req, google::protobuf::Empty *resp) {
    return svr_handler_->SendBinLog(req.value());
}

int PhxbinlogServiceImpl::GetMasterInfoFromGlobal(const google::protobuf::Empty &req,
                                                  google::protobuf::BytesValue *resp) {
    return svr_handler_->GetMasterInfoFromGlobal(resp->mutable_value());
}

int PhxbinlogServiceImpl::GetMasterInfoFromLocal(const google::protobuf::Empty &req,
                                                 google::protobuf::BytesValue *resp) {
    return svr_handler_->GetMasterInfoFromLocal(resp->mutable_value());
}

int PhxbinlogServiceImpl::GetLastSendGtidFromGlobal(const google::protobuf::BytesValue &req,
                                                    google::protobuf::BytesValue *resp) {
    return svr_handler_->GetLastSendGTIDFromGlobal(req.value(), resp->mutable_value());
}

int PhxbinlogServiceImpl::GetLastSendGtidFromLocal(const google::protobuf::BytesValue &req,
                                                   google::protobuf::BytesValue *resp) {
    return svr_handler_->GetLastSendGTIDFromLocal(req.value(), resp->mutable_value());
}

int PhxbinlogServiceImpl::SetExportIP(const google::protobuf::BytesValue &req, google::protobuf::Empty *resp) {
    return svr_handler_->SetExportIP(req.value());
}

int PhxbinlogServiceImpl::HeartBeat(const google::protobuf::BytesValue &req, google::protobuf::Empty *resp) {
    return svr_handler_->HeartBeat(req.value());
}

int PhxbinlogServiceImpl::AddMember(const google::protobuf::BytesValue &req, google::protobuf::Empty *resp) {
    return svr_handler_->AddMember(req.value());
}

int PhxbinlogServiceImpl::RemoveMember(const google::protobuf::BytesValue &req, google::protobuf::Empty *resp) {
    return svr_handler_->RemoveMember(req.value());
}

int PhxbinlogServiceImpl::SetMySqlAdminInfo(const google::protobuf::BytesValue & req, google::protobuf::Empty * resp) {
    return svr_handler_->SetMySqlAdminInfo(req.value());
}

int PhxbinlogServiceImpl::SetMySqlReplicaInfo(const google::protobuf::BytesValue & req,
                                              google::protobuf::Empty * resp) {
    return svr_handler_->SetMySqlReplicaInfo(req.value());
}

int PhxbinlogServiceImpl::GetMemberList(const google::protobuf::Empty& req, google::protobuf::BytesValue *resp) {
    return svr_handler_->GetMemberList(resp->mutable_value());
}

int PhxbinlogServiceImpl::InitBinlogSvrMaster(const google::protobuf::BytesValue & req,
                                              google::protobuf::Empty * resp) {
    return svr_handler_->InitBinlogSvrMaster(req.value());
}

