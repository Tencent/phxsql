/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "phxbinlogsvr/client/phxbinlog_client.h"
#include "phxbinlogsvr/define/errordef.h"
#include "phxbinlogsvr/core/gtid_handler.h"

#include "phxcomm/phx_log.h"

#include "phxbinlog_stub_interface.h"
#include "phxbinlogsvr.pb.h"

using std::string;
using std::vector;
using std::pair;
using std::shared_ptr;

using phxbinlog::GtidHandler;

using phxsql::ColorLogError;
using phxsql::ColorLogInfo;
using phxsql::LogVerbose;

//=========================================================
namespace phxbinlogsvr {

PhxBinlogClient::PhxBinlogClient(PhxBinlogStubInterface* stub_interface) {
    stub_interface_ = stub_interface;
}

PhxBinlogClient::~PhxBinlogClient() {
    delete stub_interface_;
}

int PhxBinlogClient::SendBinLog(const string &oldgtid, const string &event_buffer) {
    string max_gtid;
    return SendBinLog(oldgtid, event_buffer, &max_gtid);
}

int PhxBinlogClient::SendBinLog(const string &oldgtid, const string &event_buffer, string *max_gtid) {
    if (max_gtid == NULL) {
        return phxbinlog::ARG_FAIL;
    }
    vector < string > eventlist;
    *max_gtid = "";
    int ret = GtidHandler::ParseEventList(event_buffer, &eventlist, false, max_gtid);
    if (ret) {
        ColorLogError("%s pasrse event fail", __func__);
        return phxbinlog::BUFFER_FAIL;
    }

    return SendBinLog(oldgtid, *max_gtid, event_buffer);
}

int PhxBinlogClient::SendBinLog(const string &oldgtid, const string &newgtid, const string &event_buffer) {
    string buffer;

    if (!EncodeBinLogBuffer(oldgtid, newgtid, event_buffer, &buffer)) {
        ColorLogError("%s encode fail", __func__);
        return phxbinlog::BUFFER_FAIL;
    }

    uint32_t old_timeout = stub_interface_->GetTimeOutMS();
    stub_interface_->SetTimeOutMS(0); // never timeout
    int ret = stub_interface_->SendBinLog(buffer);
    stub_interface_->SetTimeOutMS(old_timeout);
    return ret;

}

int PhxBinlogClient::SetExportIP(const string &ip, const uint32_t &port) {
    string req_buffer;
    if (!EncodeExportIPInfo(ip, port, &req_buffer)) {
        ColorLogError("%s encode fail", __func__);
        return phxbinlog::BUFFER_FAIL;
    }

    return stub_interface_->SetExportIP(req_buffer);
}

int PhxBinlogClient::GetLastSendGtid(const string &uuid, string *last_send_gtid) {
    return stub_interface_->GetLastSendGtidFromGlobal(uuid, last_send_gtid);
}

int PhxBinlogClient::GetMaster(string *ip, uint32_t *expire_time) {
    uint32_t t_version = 0;
    return GetMaster(ip, expire_time, &t_version);
}

int PhxBinlogClient::GetMaster(string *ip, uint32_t *expire_time, uint32_t *version) {
    string resp_buffer;
    int ret = stub_interface_->GetMasterInfoFromGlobal(&resp_buffer);
    if (ret == 0) {
        if (!DecodeMasterInfo(resp_buffer, ip, expire_time, version)) {
            ColorLogError("%s:%d decode fail", __func__, __LINE__);
            return phxbinlog::BUFFER_FAIL;
        }
    }
    return ret;
}

int PhxBinlogClient::GetGlobalMaster(const std::vector<std::string> &iplist, std::string *ip,
                                     uint32_t *expire_time, uint32_t *version, bool require_majority) {
    return GetGlobalMaster(iplist, stub_interface_->port_, ip, expire_time, version, require_majority);
}

int PhxBinlogClient::GetLocalLastSendGtid(const string &uuid, string *last_send_gtid) {
    return stub_interface_->GetLastSendGtidFromLocal(uuid, last_send_gtid);
}

int PhxBinlogClient::GetLocalMaster(string *ip, uint32_t *expire_time, uint32_t *version) {
    string resp_buffer;
    int ret = stub_interface_->GetMasterInfoFromLocal(&resp_buffer);
    if (ret == 0) {
        if (!DecodeMasterInfo(resp_buffer, ip, expire_time, version)) {
            ColorLogError("%s:%d decode fail", __func__, __LINE__);
            return phxbinlog::BUFFER_FAIL;
        }
    }
    return ret;
}

int PhxBinlogClient::GetGlobalMaster(const vector<string> &iplist, const uint32_t &port, string *ip,
                                     uint32_t *expire_time, uint32_t *version, bool require_majority) {
    uint32_t ok_num = 0;
    vector < pair<string, int> > resp_bufferlist;
    int ret = stub_interface_->GetMasterInfoFromLocal(iplist, port, &resp_bufferlist);
    if (ret == 0) {
        for (size_t i = 0; i < resp_bufferlist.size(); ++i) {
            if (resp_bufferlist[i].second) {
                ret = resp_bufferlist[i].second;
                continue;
            } else {
                uint32_t t_version = 0;
                string t_ip;
                uint32_t t_expire_time;
                if (!DecodeMasterInfo(resp_bufferlist[i].first, &t_ip, &t_expire_time, &t_version)) {
                    LogVerbose("%s:%d decode fail, ip %s", __func__, __LINE__, iplist[i].c_str());
                    ret = phxbinlog::BUFFER_FAIL;
                    continue;
                }

                ok_num++;
                ColorLogError("%s get data from ip %s, master ip %s version %u expiretime %u", __func__,
                              iplist[i].c_str(), t_ip.c_str(), t_version, t_expire_time);
                if (t_version > *version || (t_version == *version && t_expire_time < *expire_time)) {
                    *version = t_version;
                    *ip = t_ip;
                    *expire_time = t_expire_time;
                }
            }
        }
        // master may not up to date if require_majority == false
        if ((!require_majority && ok_num > 0) || (ok_num << 1) >= iplist.size()) {
            ColorLogInfo("%s resp num %u get ip %s version %u expiretime %u",
                         __func__, ok_num, ip->c_str(), *version, *expire_time);
            return phxbinlog::OK;
        }
    }

    for (size_t i = 0; i < resp_bufferlist.size(); ++i) {
        if (resp_bufferlist[i].second) {
            ret = resp_bufferlist[i].second;
            ColorLogError("%s get master fail, ip %s ret %d",__func__, iplist[i].c_str(), ret);
        }
    }

    return ret;
}

int PhxBinlogClient::GetGlobalLastSendGtid(const vector<string> &iplist, const uint32_t &port, const string &uuid,
                                           string *last_send_gtid) {
    uint32_t ok_num = 0;
    vector < pair<string, int> > resp_bufferlist;
    int ret = stub_interface_->GetLastSendGtidFromLocal(iplist, port, uuid, &resp_bufferlist);
    if (ret == 0) {
        for (size_t i = 0; i < resp_bufferlist.size(); ++i) {
            if (resp_bufferlist[i].second) {
                ret = resp_bufferlist[i].second;
                ColorLogError("%s fail, ip %s ret %d", __func__, iplist[i].c_str(), ret);
            } else {
                string t_last_send_gtid = resp_bufferlist[i].first;
                LogVerbose("%s get from %s, last gtid %s", __func__, iplist[i].c_str(), t_last_send_gtid.c_str());
                ++ok_num;
                if (GtidHandler::GTIDCmp(*last_send_gtid, t_last_send_gtid) < 0) {
                    *last_send_gtid = t_last_send_gtid;
                }
            }
        }
        if ((ok_num << 1) >= iplist.size()) {
            return phxbinlog::OK;
        }
    }

    return ret;
}

int PhxBinlogClient::AddMember(const string &ip) {
    LogVerbose("%s ip %s", __func__, ip.c_str());
    string req_buffer;

    phxbinlogsvr::ChangeMemberInfo change_member_info;
    change_member_info.set_ip(ip);

    if (!change_member_info.SerializeToString(&req_buffer)) {
        ColorLogError("%s encode fail", __func__);
        return phxbinlog::BUFFER_FAIL;
    }

    return stub_interface_->AddMember(req_buffer);
}

int PhxBinlogClient::RemoveMember(const string &ip) {
    LogVerbose("%s ip %s", __func__, ip.c_str());
    string req_buffer;

    phxbinlogsvr::ChangeMemberInfo change_member_info;
    change_member_info.set_ip(ip);

    if (!change_member_info.SerializeToString(&req_buffer)) {
        ColorLogError("%s encode fail", __func__);
        return phxbinlog::BUFFER_FAIL;
    }

    return stub_interface_->RemoveMember(req_buffer);
}

int PhxBinlogClient::HeartBeat(const uint32_t &flag) {
    string req_buffer;
    if (!EncodeHeartBeatInfo(flag, &req_buffer)) {
        return phxbinlog::BUFFER_FAIL;
    }

    return stub_interface_->HeartBeat(req_buffer);
}

int PhxBinlogClient::SetMySqlAdminInfo(const string &now_admin_username, const string &now_admin_pwd,
                                       const string &new_admin_username, const string &new_admin_pwd) {

    string req_buffer;
    MySqlUserModefiedInfo info;
    info.set_now_admin_username(now_admin_username);
    info.set_now_admin_pwd(now_admin_pwd);
    info.set_new_username(new_admin_username);
    info.set_new_pwd(new_admin_pwd);

    if (!info.SerializeToString(&req_buffer)) {
        return phxbinlog::BUFFER_FAIL;
    }

    return stub_interface_->SetMySqlAdminInfo(req_buffer);
}

int PhxBinlogClient::SetMySqlReplicaInfo(const string &now_admin_username, const string &now_admin_pwd,
                                         const string &new_replica_username, const string &new_replica_pwd) {

    string req_buffer;
    MySqlUserModefiedInfo info;
    info.set_now_admin_username(now_admin_username);
    info.set_now_admin_pwd(now_admin_pwd);
    info.set_new_username(new_replica_username);
    info.set_new_pwd(new_replica_pwd);

    if (!info.SerializeToString(&req_buffer)) {
        return phxbinlog::BUFFER_FAIL;
    }

    return stub_interface_->SetMySqlReplicaInfo(req_buffer);
}

int PhxBinlogClient::GetMemberList(vector<string> *ip_list, uint32_t *port) {

    string resp_buffer;
    int ret = stub_interface_->GetMemberList(&resp_buffer);
    if (ret == 0) {
        MemberList member_list;
        if (!member_list.ParseFromString(resp_buffer)) {
            return phxbinlog::BUFFER_FAIL;
        }
        for (int i = 0; i < member_list.ip_list_size(); ++i) {
            ip_list->push_back(member_list.ip_list(i));
        }
        *port = member_list.port();
    }
    return ret;
}

int PhxBinlogClient::InitBinlogSvrMaster() {
    string req_buffer;
    return stub_interface_->InitBinlogSvrMaster(req_buffer);
}

bool PhxBinlogClient::EncodeHeartBeatInfo(const uint32_t &flag, string *encode_buffer) {
    phxbinlogsvr::HeartBeatInfo info;
    info.set_flag(flag);
    return info.SerializeToString(encode_buffer);
}

bool PhxBinlogClient::EncodeBinLogBuffer(const string &old_gtid, const string &new_gtid, const string &eventbuffer,
                                         string *encode_buffer) {
    phxbinlogsvr::BinLogBuffer binlogbuffer;
    binlogbuffer.set_oldgtid(old_gtid);
    binlogbuffer.set_newgtid(new_gtid);
    binlogbuffer.set_eventbuffer(eventbuffer);

    return binlogbuffer.SerializeToString(encode_buffer);
}

bool PhxBinlogClient::EncodeExportIPInfo(const string &ip, const uint32_t &port, string *encode_buffer) {
    phxbinlogsvr::ExportIPInfo export_ipinfo;
    export_ipinfo.set_ip(ip);
    export_ipinfo.set_port(port);

    return export_ipinfo.SerializeToString(encode_buffer);
}

bool PhxBinlogClient::DecodeMasterInfo(const string &decode_buffer, string *ip, uint32_t *expire_time,
                                       uint32_t *version) {
    phxbinlogsvr::MasterInfo master_info;
    if (!master_info.ParseFromString(decode_buffer)) {
        ColorLogError("%s master fail", __func__);
        return false;
    }

    *ip = master_info.ip();
    if (master_info.update_time() == 0) {
        *expire_time = master_info.expire_time();
    } else {
        uint32_t delta_time = 0;
        if( master_info.current_time() ) {
            delta_time = master_info.expire_time() - master_info.current_time();
        }
        else {
            delta_time = master_info.expire_time() - time (NULL);
        }

        *expire_time = time(NULL) + delta_time;
    }
    *version = master_info.version();
    return true;

}

}
