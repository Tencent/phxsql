/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "phxbinlog_svr_logic.h"

#include "phxbinlogsvr.pb.h"
#include "agent.h"

#include "phxbinlogsvr/client/phxbinlog_client_factory_interface.h"
#include "phxbinlogsvr/client/phxbinlog_client_platform_info.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/define/errordef.h"
#include "phxbinlogsvr/statistics/phxbinlog_stat.h"
#include "phxcomm/phx_log.h"
#include "phxcomm/phx_timer.h"

#include <vector>
#include <memory>
#include <unistd.h>

using std::string;
using std::vector;
using phxbinlog::Option;
using phxbinlog::PhxBinLogAgent;
using phxbinlog::PaxosNodeInfo;
using phxsql::ColorLogWarning;
using phxsql::ColorLogError;
using phxsql::LogVerbose;

namespace phxbinlogsvr {

PhxBinlogSvrLogic::PhxBinlogSvrLogic(Option *option, PhxBinLogAgent *binlogagent) {
    option_ = option;
    binlogagent_ = binlogagent;
}

uint32_t PhxBinlogSvrLogic::GetCurrentTime() {
	return time(NULL);
}

int PhxBinlogSvrLogic::GetIPList(vector<string> *iplist) {
    vector < string > memberlist;
    int ret = binlogagent_->GetMemberList(&memberlist);
    if (ret == 0) {
        for (auto it : memberlist) {
            iplist->push_back(it);
        }
    }
    return ret;
}

int PhxBinlogSvrLogic::SendBinLog(const string &event_buffer) {
    if (event_buffer.size() == 0)
        return phxbinlog::BUFFER_FAIL;

    phxbinlogsvr::BinLogBuffer req;

    if (!req.ParseFromString(event_buffer)) {
        ColorLogError("%s from buffer fail", __func__);
        return phxbinlog::BUFFER_FAIL;
    }

    return RealSendBinLog(req);
}

int PhxBinlogSvrLogic::SetExportIP(const string &req_buff) {
    phxbinlogsvr::ExportIPInfo req;

    if (!req.ParseFromString(req_buff)) {
        ColorLogError("%s from buffer fail", __func__);
        return phxbinlog::BUFFER_FAIL;
    }

    LogVerbose("%s get export ip %s port %u", __func__, req.ip().c_str(), req.port());

    return RealSetExportIP(req.ip(), req.port());
}

int PhxBinlogSvrLogic::GetMasterInfoFromGlobal(string *resp_buffer) {
    phxbinlogsvr::MasterInfo master_info;
    int ret = RealGetMasterInfo(&master_info);
    if (ret == 0) {
        if (!master_info.SerializeToString(resp_buffer)) {
            return phxbinlog::BUFFER_FAIL;
        }
    }
    LogVerbose("%s get master info ret %d, ip %s expired time %u", 
			__func__, ret, master_info.ip().c_str(), master_info.expire_time() );
    return ret;
}

int PhxBinlogSvrLogic::GetLastSendGTIDFromGlobal(const string &uuid, string *resp_buffer) {
    return RealGetLastSendGTID(uuid, resp_buffer);
}

int PhxBinlogSvrLogic::GetMasterInfoFromLocal(string *resp_buffer) {
    phxbinlogsvr::MasterInfo masterinfo;
    int ret = RealGetLocalMasterInfo(&masterinfo);
    //LogVerbose("%s get master info from local ret %d", __func__, ret);
    if (ret == 0) {
        if (!masterinfo.SerializeToString(resp_buffer)) {
            return phxbinlog::BUFFER_FAIL;
        }
    }
    return ret;
}

int PhxBinlogSvrLogic::GetLastSendGTIDFromLocal(const string &uuid, string *resp_buffer) {
    return RealGetLocalLastSendGTID(uuid, resp_buffer);
}

int PhxBinlogSvrLogic::HeartBeat(const string &req_buffer) {
    phxbinlogsvr::HeartBeatInfo heartbeatinfo;
    if (!heartbeatinfo.ParseFromString(req_buffer)) {
        return phxbinlog::BUFFER_FAIL;
    }

    return RealSetHeartBeat(heartbeatinfo);
}

int PhxBinlogSvrLogic::AddMember(const string &req_buffer) {
    phxbinlogsvr::ChangeMemberInfo change_member_info;
    if (!change_member_info.ParseFromString(req_buffer)) {
        return phxbinlog::BUFFER_FAIL;
    }

    return RealAddMember(change_member_info.ip());
}

int PhxBinlogSvrLogic::RemoveMember(const string &req_buffer) {
    phxbinlogsvr::ChangeMemberInfo change_member_info;
    if (!change_member_info.ParseFromString(req_buffer)) {
        return phxbinlog::BUFFER_FAIL;
    }

    return RealRemoveMember(change_member_info.ip());
}

int PhxBinlogSvrLogic::SetMySqlAdminInfo(const string &req_buffer) {
    phxbinlogsvr::MySqlUserModefiedInfo info;
    if (!info.ParseFromString(req_buffer)) {
        return phxbinlog::BUFFER_FAIL;
    }
    return RealSetMySqlAdminInfo(info);
}

int PhxBinlogSvrLogic::SetMySqlReplicaInfo(const string &req_buffer) {
    phxbinlogsvr::MySqlUserModefiedInfo info;
    if (!info.ParseFromString(req_buffer)) {
        return phxbinlog::BUFFER_FAIL;
    }
    return RealSetMySqlReplicaInfo(info);
}

int PhxBinlogSvrLogic::GetMemberList(string *resp_buffer) {
    phxbinlogsvr::MemberList info;
    int ret = RealGetMemberList(&info);
    if (ret == 0) {
        if (!info.SerializeToString(resp_buffer)) {
            return phxbinlog::BUFFER_FAIL;
        }
    }
    return ret;
}

int PhxBinlogSvrLogic::InitBinlogSvrMaster(const string &req_buffer) {
    return RealInitBinLogSvrMaster();
}

int PhxBinlogSvrLogic::RealGetMasterInfo(phxbinlogsvr::MasterInfo *masterinfo) {
    vector < string > iplist;
    int ret = GetIPList(&iplist);
    if (ret) {
        return ret;
    }
    string local_ip = option_->GetBinLogSvrConfig()->GetEngineIP();
    uint32_t port = option_->GetBinLogSvrConfig()->GetBinlogSvrPort();

    string ip;
    uint32_t version = 0;
    uint32_t expire_time = 0;

    PhxBinLogClientFactoryInterface *client_factor = PhxBinlogClientPlatformInfo::GetDefault()->GetClientFactory();
    std::shared_ptr<PhxBinlogClient> client = client_factor->CreateClient();

    ret = client->GetGlobalMaster(iplist, port, &ip, &expire_time, &version);

    if (ret == 0) {
        masterinfo->set_ip(ip);
        masterinfo->set_expire_time(expire_time);
        masterinfo->set_version(version);
    }
    return ret;
}

int PhxBinlogSvrLogic::RealSendBinLog(const phxbinlogsvr::BinLogBuffer req) {
    return binlogagent_->SendEvent(req.oldgtid(), req.newgtid(), req.eventbuffer());
}

int PhxBinlogSvrLogic::RealSetExportIP(const string &ip, const uint32_t &port) {
    LogVerbose("%s get export ip %s, port %u", __func__, ip.c_str(), port);

    return binlogagent_->SetExportIP(ip, port);
}

int PhxBinlogSvrLogic::RealGetLastSendGTID(const string &uuid, string *gtid) {
    vector < string > iplist;
    int ret = GetIPList(&iplist);
    if (ret) {
        return ret;
    }
    string local_ip = option_->GetBinLogSvrConfig()->GetBinlogSvrIP();
    uint32_t port = option_->GetBinLogSvrConfig()->GetBinlogSvrPort();

    string ip;
    uint32_t version = 0;
    uint32_t expire_time = 0;

    PhxBinLogClientFactoryInterface *client_factor = PhxBinlogClientPlatformInfo::GetDefault()->GetClientFactory();
    std::shared_ptr<PhxBinlogClient> client = client_factor->CreateClient();

    ret = client->GetGlobalMaster(iplist, port, &ip, &expire_time, &version);
    if (ret) {
        return ret;
    }
    LogVerbose("%s get global ip %s expire time %u", __func__, ip.c_str(), expire_time);

    if (ip == local_ip) {
        ret = binlogagent_->FlushData();
        if (ret == 0) {
            ret = binlogagent_->GetLastSendGTID(uuid, gtid);
            LogVerbose("%s ret = %d get local gtid %s from uuid %s, master ip %s local ip %s", __func__, ret,
                       gtid->c_str(), uuid.c_str(), ip.c_str(), local_ip.c_str());
        }
    } else {
		std::shared_ptr<PhxBinlogClient> master_client = client_factor->CreateClient(ip, port);
        ret = master_client->GetLastSendGtid(uuid, gtid);
        LogVerbose("%s ret = %d get global gtid %s from uuid %s", __func__, ret, gtid->c_str(), uuid.c_str());
    }

    return ret;
}

int PhxBinlogSvrLogic::RealGetLocalMasterInfo(phxbinlogsvr::MasterInfo *masterinfo) {
    string ip;
    uint32_t version = 0;
    uint32_t expire_time = 0;
    uint32_t update_time = 0;
    int ret = binlogagent_->GetMaster(&ip, &update_time, &expire_time, &version);
    if (ret == 0) {
        masterinfo->set_ip(ip);
        masterinfo->set_current_time(GetCurrentTime());
        masterinfo->set_update_time(update_time);
        masterinfo->set_expire_time(expire_time);
        masterinfo->set_version(version);
    }
    return ret;
}

int PhxBinlogSvrLogic::RealGetLocalLastSendGTID(const string &uuid, string *gtid) {
    return binlogagent_->GetLastSendGTID(uuid, gtid);
}

int PhxBinlogSvrLogic::RealSetHeartBeat(const phxbinlogsvr::HeartBeatInfo &req) {
    return binlogagent_->SetHeartBeat(req.flag());
}

int PhxBinlogSvrLogic::RealAddMember(const string &ip) {
    return binlogagent_->AddMember(ip);
}

int PhxBinlogSvrLogic::RealRemoveMember(const string &ip) {
    return binlogagent_->RemoveMember(ip);
}

int PhxBinlogSvrLogic::RealSetMySqlAdminInfo(const phxbinlogsvr::MySqlUserModefiedInfo &info) {
    return binlogagent_->SetMySqlAdminInfo(info.now_admin_username(), info.now_admin_pwd(), info.new_username(),
                                           info.new_pwd());
}

int PhxBinlogSvrLogic::RealSetMySqlReplicaInfo(const phxbinlogsvr::MySqlUserModefiedInfo &info) {
    return binlogagent_->SetMySqlReplicaInfo(info.now_admin_username(), info.now_admin_pwd(), info.new_username(),
                                             info.new_pwd());
}

int PhxBinlogSvrLogic::RealGetMemberList(phxbinlogsvr::MemberList *info) {
    vector < string > ip_list;
    int ret = PhxBinlogSvrLogic::GetIPList(&ip_list);
    if (ret == 0) {
        for (auto it : ip_list) {
            info->add_ip_list(it);
        }
        info->set_port(option_->GetBinLogSvrConfig()->GetBinlogSvrPort());
    }
    return ret;
}

int PhxBinlogSvrLogic::RealInitBinLogSvrMaster() {
    return binlogagent_->InitAgentMaster();
}

}

