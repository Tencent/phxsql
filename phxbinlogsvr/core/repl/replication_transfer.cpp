/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "replication_transfer.h"

#include "event_manager.h"
#include "event_agent.h"
#include "replication_ctx.h"

#include "phxcomm/net.h"
#include "phxcomm/phx_utils.h"
#include "phxcomm/phx_log.h"
#include "phxbinlogsvr/statistics/phxbinlog_stat.h"
#include "phxbinlogsvr/core/gtid_handler.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/define/errordef.h"

#include <unistd.h>
#include <stdio.h>

using std::string;
using std::vector;
using phxsql::NetIO;
using phxsql::LogVerbose;
using phxsql::ColorLogInfo;
using phxsql::ColorLogWarning;
using phxsql::ColorLogError;
using phxsql::Utils;

namespace phxbinlog {

ReplicationTransfer::ReplicationTransfer(ReplicationCtx *ctx)
        : ReplicationBase(ctx) {
    event_manager_ = EventManager::GetGlobalEventManager(ctx->GetOption());
    checksum_ = 0;
}

ReplicationTransfer::~ReplicationTransfer() {
    Close();
}

void ReplicationTransfer::Close() {
    ctx_->CloseMasterFD();
    ctx_->CloseSlaveFD();
}

int ReplicationTransfer::Process() {
    ctx_->Wait();

    ColorLogInfo("dc process begin");

    if (ctx_->GetSlaveFD() < 0) {
        ColorLogError("%s slave has closed, exit", __func__);
        return SLAVE_FAIL;
    }

    int ret = OK;
    //init the gtid to get the data pos
    while (!ctx_->IsClose()) {
        vector < string > recv_buff;
        ret = ReadDataFromDC(&recv_buff);
        if (ret != OK && ret != DATA_EMPTY)
            break;

        if (ctx_->GetSlaveFD() < 0) {
            ColorLogError("%s slave has closed, exit", __func__);
            break;
        }

        if (recv_buff.size() == 0)
            continue;

        for (size_t i = 0; i < recv_buff.size(); ++i) {
            int seq = ctx_->GetSeq();
            ret = NetIO::SendWithSeq(ctx_->GetSlaveFD(), recv_buff[i], seq++);
            ctx_->SetSeq(seq);
            if (ret) {
                ColorLogError("%s send buffer fail len %zu ret %d seq %d", "slave", recv_buff[i].size(), ret, seq);
                STATISTICS(ReplSendDataFail()); 
                break;
            }
        }

        if (ret)
            break;

        LogVerbose("%s recv buffer list size %zu ret %d", "dc", recv_buff.size(), ret);
    }

    ColorLogInfo("%s transfer done, ret %d", __func__, ret);
    return ret;
}

int ReplicationTransfer::ReadDataFromDC(vector<string> *buffer) {
    return GetEvents(buffer);
}

int ReplicationTransfer::GetEvents(vector<string> *event_list) {
    uint32_t get_num = 0;
    if (checksum_ == 0) {
        get_num = 1;
    }

    string event_data;
    int ret = event_manager_->GetEvents(&event_data, get_num);
    if (ret) {
        return ret;
    }
    STATISTICS(GtidEventTransferNum(event_list->size()));

    std::string max_gtid;
    ret = GtidHandler::ParseEventList(event_data, event_list, true, &max_gtid);

    CountCheckSum(*event_list, max_gtid);
    return ret;
}

void ReplicationTransfer::CountCheckSum(const vector<string> &event_list, const string &max_gtid) {
    LogVerbose("%s count check sum %llu, event size %zu", __func__, checksum_, event_list.size());
    if (checksum_ == 0) {
        EventDataInfo info;
        event_manager_->GetGTIDInfo(max_gtid, &info);
        checksum_ = info.checksum();
        LogVerbose("%s get data check sum %llu for init", __func__, checksum_);
    } else {
        vector < size_t > checksum_for_log;
        checksum_for_log.push_back(checksum_);
        size_t datasize = 0;
        size_t header_size = GtidHandler::GetEventHeaderSize();
        for (size_t i = 0; i < event_list.size(); ++i) {
            checksum_ = Utils::GetCheckSum(checksum_, event_list[i].c_str() + header_size,
                                           event_list[i].size() - header_size);
            datasize += event_list[i].size() - header_size;
            checksum_for_log.push_back(checksum_);
        }

        EventDataInfo info;
        int ret = event_manager_->GetGTIDInfo(max_gtid, &info);
        if (ret) {
            ColorLogError("%s get gtid data %s info fail", __func__, max_gtid.c_str());
            assert(ret == 0);
            return;
        }

        ColorLogInfo("%s get data check sum %llu, max gtid %s, gtid check sum %llu data size %zu", __func__, checksum_,
                    max_gtid.c_str(), info.checksum(), datasize);

        if (checksum_ != info.checksum()) {
            ColorLogError("%s get data check sum %llu, check fail, should be %llu", __func__, checksum_,
                           info.checksum());
            for (size_t i = 0; i < checksum_for_log.size(); ++i)
                ColorLogError("%s get check sum %llu", __func__, checksum_for_log[i]);
            checksum_for_log.push_back(checksum_);
            STATISTICS(ReplCheckSumFail());
            assert(checksum_ == info.checksum());
        }
    }
}

}

