/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at

	https://opensource.org/licenses/GPL-2.0

	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include "phxrpc_phxbinlog_service.h"
#include "phxbinlogsvr.pb.h"

class PhxbinlogServerConfig;

namespace phxbinlogsvr {
class PhxBinLogSvrHandler;
}

typedef struct tagServiceArgs {
    phxbinlogsvr::PhxBinLogSvrHandler *svr_handler_;
} ServiceArgs_t;

class PhxbinlogServiceImpl : public PhxbinlogService {
 public:
    PhxbinlogServiceImpl(ServiceArgs_t * service_args);
    virtual ~PhxbinlogServiceImpl();

    virtual int PhxEcho(const google::protobuf::StringValue & req, google::protobuf::StringValue *resp);

    virtual int SendBinLog(const google::protobuf::BytesValue & req, google::protobuf::Empty *resp);

    virtual int GetMasterInfoFromGlobal(const google::protobuf::Empty & req, google::protobuf::BytesValue *resp);

    virtual int GetMasterInfoFromLocal(const google::protobuf::Empty & req, google::protobuf::BytesValue *resp);

    virtual int GetLastSendGtidFromGlobal(const google::protobuf::BytesValue & req, google::protobuf::BytesValue *resp);

    virtual int GetLastSendGtidFromLocal(const google::protobuf::BytesValue & req, google::protobuf::BytesValue *resp);

    virtual int SetExportIP(const google::protobuf::BytesValue & req, google::protobuf::Empty *resp);

    virtual int HeartBeat(const google::protobuf::BytesValue & req, google::protobuf::Empty *resp);

    virtual int AddMember(const google::protobuf::BytesValue & req, google::protobuf::Empty *resp);

    virtual int RemoveMember(const google::protobuf::BytesValue & req, google::protobuf::Empty *resp);

    virtual int SetMySqlAdminInfo(const google::protobuf::BytesValue & req, google::protobuf::Empty * resp);

    virtual int SetMySqlReplicaInfo(const google::protobuf::BytesValue & req, google::protobuf::Empty * resp);

    virtual int GetMemberList(const google::protobuf::Empty& req, google::protobuf::BytesValue *resp);

    virtual int InitBinlogSvrMaster(const google::protobuf::BytesValue & req, google::protobuf::Empty * resp);

 private:
    phxbinlogsvr::PhxBinLogSvrHandler *svr_handler_;
};

