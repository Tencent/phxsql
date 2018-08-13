/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at

	https://opensource.org/licenses/GPL-2.0

	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "phxbinlog_server.h"

#include "phxbinlogsvr/client/phxbinlog_client_factory_phxrpc.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxcomm/phx_glog.h"
#include "phxcomm/phx_log.h"
#include "phxrpc_phxbinlog_dispatcher.h"
#include "phxbinlog_svr_handler.h"
#include "phxbinlog_server_config.h"
#include "phxbinlog_service_impl.h"

#include "phxrpc/rpc.h"
#include "phxrpc/http.h"
#include "phxrpc/file.h"

using phxbinlog::Option;
using phxbinlogsvr::PhxBinLogClientFactoryInterface;
using phxbinlogsvr::PhxBinLogSvrHandler;
using phxsql::LogVerbose;

using std::string;

Server::Server() {
    config_ = new PhxbinlogServerConfig;
    phxbin_option_ = Option::GetDefault();
    svr_handler_ = new PhxBinLogSvrHandler(phxbin_option_, GetPhxBinLogClientFactory());
}

Server::~Server() {
    delete svr_handler_;
    delete config_;
}

PhxBinLogClientFactoryInterface * Server::GetPhxBinLogClientFactory() {
    return new PhxBinlogClientFactory_PhxRPC();
}

void Server::InitMonitor() {
    InitLog(phxsql::PhxGLog::Log, phxsql::PhxGLog::OpenLog);
}

void Server::InitConfig() {
    printf("set bind ip %s port %d\n", phxbin_option_->GetBinLogSvrConfig()->GetBinlogSvrIP(),
           phxbin_option_->GetBinLogSvrConfig()->GetBinlogSvrPort());
    config_->SetBindIP(phxbin_option_->GetBinLogSvrConfig()->GetBinlogSvrIP());
    config_->SetBindPort(phxbin_option_->GetBinLogSvrConfig()->GetBinlogSvrPort());

    std::string package_name = phxbin_option_->GetBinLogSvrConfig()->GetPackageName();
    if (package_name == "") {
        package_name = "phxbinlogsvr";
    }
    config_->SetPackageName(package_name);
    config_->SetThreadNum(100);
}

void Server::InitLog(LogFunc log_func, OpenLogFunc openlog_func) {
    if (openlog_func) {
        string log_path = phxbin_option_->GetBinLogSvrConfig()->GetLogFilePath();
        uint32_t log_level = phxbin_option_->GetBinLogSvrConfig()->GetLogLevel();
        uint32_t log_file_size = phxbin_option_->GetBinLogSvrConfig()->GetLogMaxSize();
        openlog_func("phxbinlogsvr", log_level, log_path.c_str(), log_file_size);
        LogVerbose("%s log level %u log file size %u path %s", __func__, log_level, log_file_size, log_path.c_str());
    }
    phxsql::ResigterLogFunc(log_func);
    phxrpc::setvlog(log_func);
}

void Server::BeforeServerRun() {
    InitConfig();
    InitMonitor();

    svr_handler_->BeforeRun();
}

void Server::AfterServerRun() {
    svr_handler_->AfterRun();
}

void Server::Run() {
    BeforeServerRun();

    ServiceArgs_t service_args;
    service_args.svr_handler_ = GetSvrHandler();

    phxrpc::HshaServer server(config_->GetServerConfig(), Server::Dispatch, &service_args);

    server.RunForever();

    AfterServerRun();
}

PhxbinlogServerConfig *Server::GetServerConfig() {
    return config_;
}

PhxBinLogSvrHandler *Server::GetSvrHandler() {
    return svr_handler_;
}

void Server::Dispatch(const phxrpc::BaseRequest &req,
                      phxrpc::BaseResponse *const resp,
                      phxrpc::DispatcherArgs_t *const args) {
    ServiceArgs_t *service_args = (ServiceArgs_t *)(args->service_args);
    PhxbinlogServiceImpl service(service_args);

    PhxbinlogDispatcher dispatcher(service, args);
    phxrpc::BaseDispatcher<PhxbinlogDispatcher>
            base_dispatcher(dispatcher, PhxbinlogDispatcher::GetURIFuncMap());
    if (!base_dispatcher.Dispatch(req, resp)) {
        resp->SetFake(phxrpc::BaseResponse::FakeReason::DISPATCH_ERROR);
    }
}

