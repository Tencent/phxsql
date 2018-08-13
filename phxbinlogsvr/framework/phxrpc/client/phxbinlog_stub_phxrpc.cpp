/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at

	https://opensource.org/licenses/GPL-2.0

	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "phxbinlog_stub_phxrpc.h"
#include "phxrpc_phxbinlog_stub.h"
#include "phxcomm/phx_log.h"

#include "phxrpc/http.h"
#include "phxrpc/rpc.h"

#include <iostream>
#include <memory>
#include <mutex>

using std::string;
using std::vector;
using std::pair;

using phxsql::LogVerbose;
using phxsql::ColorLogWarning;

static std::shared_ptr<phxrpc::ClientMonitor> global_phxbinlogclient_client_monitor;

void PhxBinlogStub_PhxRPC::InitClientMonitor() {
    static std::mutex monitor_mutex;
    if (!(global_phxbinlogclient_client_monitor.get())) {
        monitor_mutex.lock();
        if (!(global_phxbinlogclient_client_monitor.get())) {
            global_phxbinlogclient_client_monitor = phxrpc::MonitorFactory::GetFactory()->CreateClientMonitor(
                    GetPackageName().c_str());
        }
        monitor_mutex.unlock();
    }
}

PhxBinlogStub_PhxRPC::PhxBinlogStub_PhxRPC() :
        PhxBinlogStubInterface() {
    InitClientMonitor();
}

PhxBinlogStub_PhxRPC::PhxBinlogStub_PhxRPC(const string &ip, const uint32_t &port) :
        PhxBinlogStubInterface(ip, port) {
    InitClientMonitor();
}

PhxBinlogStub_PhxRPC::PhxBinlogStub_PhxRPC(const std::string &ip, const uint32_t &port, const uint32_t &time_ms) :
        PhxBinlogStubInterface(ip, port, time_ms) {
    InitClientMonitor();
}

PhxBinlogStub_PhxRPC::~PhxBinlogStub_PhxRPC() {
}

template<class FuncName, typename ReqType>
int PhxBinlogStub_PhxRPC::RpcCallWithIPList(const FuncName &func, const vector<string> &iplist, const uint32_t &port,
		const ReqType &req, vector<pair<string, int> > *resp_bufferlist) {

	size_t success_count = 0;

	for(size_t i = 0; i < iplist.size(); ++i ) {
		string value = "";
		resp_bufferlist->push_back(make_pair(value, -202));
	}

	uthread_begin;

	for (size_t i = 0; i < iplist.size(); i++) {
		uthread_t [=, &uthread_s, &success_count, &resp_bufferlist](void *) {
			phxrpc::UThreadTcpStream socket;
			if(phxrpc::PhxrpcTcpUtils::Open(&uthread_s, &socket, iplist[i].c_str(), port, GetTimeOutMS(),
					*(global_phxbinlogclient_client_monitor.get()))){
				socket.SetTimeout(GetTimeOutMS());

				ReqType tmp_req = req;
				google::protobuf::BytesValue resp;

				phxrpc::HttpMessageHandlerFactory http_msg_factory;
				PhxbinlogStub stub(socket, *(global_phxbinlogclient_client_monitor.get()), http_msg_factory);
				int ret = (stub.*func)(tmp_req, &resp);
				(*resp_bufferlist)[i]=make_pair(resp.value(), ret);

				if (ret == 0) {
					success_count++;
					if (iplist.size() < (success_count<< 1) ) {
						uthread_s.Close();
					}
				}
			}
		};
	}

	uthread_end;

	return 0;
}

template<class FuncName, typename ReqType, typename RespType>
int PhxBinlogStub_PhxRPC::RpcCallWithIP(const FuncName &func, const string &ip, const int port, const ReqType &req,
		RespType *resp) {
	int ret = -1;
	phxrpc::BlockTcpStream socket;
	if (phxrpc::PhxrpcTcpUtils::Open(&socket, ip.c_str(), port, GetTimeOutMS(), NULL, 0,
				*(global_phxbinlogclient_client_monitor.get()))) {
		socket.SetTimeout(GetTimeOutMS());
		phxrpc::HttpMessageHandlerFactory http_msg_factory;
		PhxbinlogStub stub(socket, *(global_phxbinlogclient_client_monitor.get()), http_msg_factory);
		ret = (stub.*func)(req, resp);
	}
	if (ret) {
		ColorLogWarning("client call fail, ret %d", ret);
	}
	return ret;
}

template<class FuncName, typename ReqType, typename RespType>
int PhxBinlogStub_PhxRPC::RpcCall(const FuncName &func, const ReqType &req, RespType *resp) {
	return RpcCallWithIP(func, GetIP(), GetPort(), req, resp);
}

int PhxBinlogStub_PhxRPC::InitBinlogSvrMaster(const std::string &req_buffer) {

	google::protobuf::BytesValue req;
	google::protobuf::Empty resp;
	req.set_value(req_buffer);
	return RpcCall(&PhxbinlogStub::InitBinlogSvrMaster, req, &resp);
}

int PhxBinlogStub_PhxRPC::SendBinLog(const std::string &req_buffer) {
	google::protobuf::BytesValue req;
	google::protobuf::Empty resp;

	req.set_value(req_buffer);

	return RpcCall(&PhxbinlogStub::SendBinLog, req, &resp);
}

int PhxBinlogStub_PhxRPC::HeartBeat(const string &req_buffer) {
	google::protobuf::BytesValue req;
	google::protobuf::Empty resp;
	req.set_value(req_buffer);

	return RpcCall(&PhxbinlogStub::HeartBeat, req, &resp);
}

int PhxBinlogStub_PhxRPC::AddMember(const string &req_buffer) {
	google::protobuf::BytesValue req;
	google::protobuf::Empty resp;
	req.set_value(req_buffer);

	return RpcCall(&PhxbinlogStub::AddMember, req, &resp);
}

int PhxBinlogStub_PhxRPC::RemoveMember(const string &req_buffer) {
	google::protobuf::BytesValue req;
	google::protobuf::Empty resp;
	req.set_value(req_buffer);

	return RpcCall(&PhxbinlogStub::RemoveMember, req, &resp);
}

int PhxBinlogStub_PhxRPC::SetExportIP(const string &req_buffer) {
	google::protobuf::BytesValue req;
	google::protobuf::Empty resp;
	req.set_value(req_buffer);

	return RpcCall(&PhxbinlogStub::SetExportIP, req, &resp);
}

int PhxBinlogStub_PhxRPC::GetLastSendGtidFromGlobal(const string &req_buffer, string *resp_buffer) {
	google::protobuf::BytesValue req;
	google::protobuf::BytesValue resp;
	req.set_value(req_buffer);

	int ret = RpcCall(&PhxbinlogStub::GetLastSendGtidFromGlobal, req, &resp);
	if (ret == 0) {
		*resp_buffer = resp.value();
	}
	return ret;
}

int PhxBinlogStub_PhxRPC::GetLastSendGtidFromLocal(const string &req_buffer, string *resp_buffer) {
	google::protobuf::BytesValue req;
	google::protobuf::BytesValue resp;
	req.set_value(req_buffer);

	int ret = RpcCall(&PhxbinlogStub::GetLastSendGtidFromLocal, req, &resp);
	if (ret == 0) {
		*resp_buffer = resp.value();
	}
	return ret;
}

int PhxBinlogStub_PhxRPC::GetLastSendGtidFromLocal(const vector<string> &iplist, const uint32_t &port,
		const string &req_buffer,
		vector<pair<string, int> > *resp_bufferlist) {
	google::protobuf::BytesValue req;
	req.set_value(req_buffer);

	return RpcCallWithIPList(&PhxbinlogStub::GetLastSendGtidFromLocal, iplist, port, req, resp_bufferlist);
}

int PhxBinlogStub_PhxRPC::GetMasterInfoFromGlobal(string *resp_buffer) {
	google::protobuf::Empty req;
	google::protobuf::BytesValue resp;

	int ret = RpcCall(&PhxbinlogStub::GetMasterInfoFromGlobal, req, &resp);
	if (ret == 0) {
		*resp_buffer = resp.value();
	}
	return ret;
}

int PhxBinlogStub_PhxRPC::GetMasterInfoFromLocal(string *resp_buffer) {
	google::protobuf::Empty req;
	google::protobuf::BytesValue resp;

	int ret = RpcCall(&PhxbinlogStub::GetMasterInfoFromLocal, req, &resp);
	if (ret == 0) {
		*resp_buffer = resp.value();
	}
	return ret;
}

int PhxBinlogStub_PhxRPC::GetMasterInfoFromLocal(const vector<string> &iplist, const uint32_t &port,
		vector<pair<string, int> > *resp_bufferlist) {
	google::protobuf::Empty req;
	return RpcCallWithIPList(&PhxbinlogStub::GetMasterInfoFromLocal, iplist, port, req, resp_bufferlist);
}

int PhxBinlogStub_PhxRPC::SetMySqlAdminInfo(const std::string &req_buffer) {
	google::protobuf::BytesValue req;
	google::protobuf::Empty resp;
	req.set_value(req_buffer);

	return RpcCall(&PhxbinlogStub::SetMySqlAdminInfo, req, &resp);
}

int PhxBinlogStub_PhxRPC::SetMySqlReplicaInfo(const std::string &req_buffer) {
	google::protobuf::BytesValue req;
	google::protobuf::Empty resp;
	req.set_value(req_buffer);

	return RpcCall(&PhxbinlogStub::SetMySqlReplicaInfo, req, &resp);
}

int PhxBinlogStub_PhxRPC::GetMemberList(std::string *resp_buffer) {
	google::protobuf::Empty req;
	google::protobuf::BytesValue resp;

	int ret = RpcCall(&PhxbinlogStub::GetMemberList, req, &resp);
	if (ret == 0) {
		*resp_buffer = resp.value();
	}
	return ret;
}

