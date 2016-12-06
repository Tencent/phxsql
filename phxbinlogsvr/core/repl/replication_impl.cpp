/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "replication_impl.h"

#include "event_manager.h"
#include "master_manager.h"
#include "mysql_manager.h"
#include "replication_ctx.h"
#include "replication_slave.h"
#include "replication_transfer.h"

#include "phxcomm/phx_utils.h"
#include "phxcomm/net.h"
#include "phxcomm/phx_log.h"
#include "phxbinlogsvr/statistics/phxbinlog_stat.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/define/errordef.h"

#include <unistd.h>

using std::string;
using std::vector;
using phxsql::NetIO;
using phxsql::LogVerbose;
using phxsql::ColorLogInfo;
using phxsql::ColorLogWarning;
using phxsql::ColorLogError;
using phxsql::Utils;

namespace phxbinlog {

ReplicationImpl::ReplicationImpl(const Option *option, const int &slave_fd) {
    ctx_ = new ReplicationCtx(option);
    ctx_->SetSlaveFD(slave_fd);
    option_ = option;
    event_manager_ = EventManager::GetGlobalEventManager(option);
    master_manager_ = MasterManager::GetGlobalMasterManager(option);
}

ReplicationImpl::~ReplicationImpl() {
    delete ctx_;
    ctx_ = NULL;
}

void ReplicationImpl::Close() {
    if (ctx_){
        ctx_->Close();
		//wake up transfer
		event_manager_->Notify();
	}
}

int ReplicationImpl::Process() {
	if(ctx_->GetSlaveFD()<0){
		ColorLogInfo("%s slave fd %d close", __func__, ctx_->GetSlaveFD());
		return SOCKET_FAIL;
	}

    int master_fd = InitFakeMaster();
    if (master_fd < 0) {
        return SOCKET_FAIL;
    }

    ColorLogInfo("%p process master fd %d, slave fd %d", this, master_fd, ctx_->GetSlaveFD());

    ctx_->SetMasterFD(master_fd);

    ReplicationSlave *slave = new ReplicationSlave(ctx_);
    ReplicationTransfer *transfer = new ReplicationTransfer(ctx_);

    slave->Run();
    transfer->Run();

    slave->WaitStop();
    transfer->WaitStop();

    delete slave;
    delete transfer;

    return OK;
}

int ReplicationImpl::InitFakeMaster() {
    MasterInfo info;
    int ret = master_manager_->GetMasterInfo(&info);
    if (ret) {
        ColorLogError("%s get master info fail, ret %d", __func__, ret);
        return ret;
    }

    string ip = Utils::GetIP(info.svr_id());
    if (info.svr_id() == option_->GetBinLogSvrConfig()->GetEngineSvrID()) {
        LogVerbose("%s get other ip %s", __func__, ip.c_str());
    } else {
        ctx_->SelfConn(false);
    }

    MySqlManager* mysql_manager = MySqlManager::GetGlobalMySqlManager(option_);
    if (mysql_manager->IsReplicaProcess()) {
        ip = Utils::GetIP(option_->GetBinLogSvrConfig()->GetEngineSvrID());
        ctx_->SelfConn(true);
    }

    LogVerbose("%s connect ip %s port %d", __func__, ip.c_str(), info.port());

    int master_fd = NetIO::Connect(ip.c_str(), info.port());
    if (master_fd < 0) {
		STATISTICS(ReplMySqlConnectFail());
        return SOCKET_FAIL;
    }
    return master_fd;
}

}

