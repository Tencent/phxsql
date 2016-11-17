/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "paxos_agent.h"

#include "executer_base.h"
#include "paxosdata.pb.h"

#include "phxpaxos/node.h"
#include "phxpaxos_plugin/monitor.h"

#include "phxcomm/phx_log.h"
#include "phxcomm/phx_utils.h"
#include "phxcomm/phx_timer.h"
#include "phxbinlogsvr/statistics/phxbinlog_stat.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/define/errordef.h"

#include <string>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

using std::string;
using std::vector;
using std::pair;

using phxpaxos::FollowerNodeInfoList;
using phxpaxos::FollowerNodeInfo;
using phxpaxos::NodeInfo;
using phxpaxos::GroupSMInfo;
using phxpaxos::Node;
using phxpaxos::SMCtx;
using phxpaxos::PaxosTryCommitRet_Value_Size_TooLarge;
using phxpaxos::Monitor;

using phxsql::LogVerbose;
using phxsql::ColorLogInfo;
using phxsql::ColorLogError;
using phxsql::Utils;
using phxsql::PhxTimer;

#define GROUP_ID 0

namespace phxbinlog {

PaxosAgent::PaxosAgent(const Option *option) {
    paxos_node_ = nullptr;
    option_ = option;

    SetPaxosOption();
}

PaxosAgent::~PaxosAgent() {
    Close();
}

void PaxosAgent::ReleaseNode() {
    return Close();
}

void PaxosAgent::Close() {
    if (paxos_node_)
        delete paxos_node_, paxos_node_ = NULL;
}

void PaxosAgent::SetMemberListHandler(phxpaxos::MembershipChangeCallback call_back) {
    paxos_option_.pMembershipChangeCallback = call_back;
}

bool PaxosAgent::SetFollower() {
    FollowerNodeInfoList &follower_list = paxos_option_.vecFollowerNodeInfoList;
    follower_list.clear();

    string follower_ip = option_->GetBinLogSvrConfig()->GetFollowIP();
    if (follower_ip != "") {
        FollowerNodeInfo follower_info;
        //follower
        follower_info.oMyNode = paxos_option_.oMyNode;
        //leader
        follower_info.oFollowNode.SetIPPort(follower_ip, option_->GetBinLogSvrConfig()->GetPaxosPort());

        follower_list.push_back(follower_info);
    }

    for (size_t i = 0; i < paxos_option_.vecFollowerNodeInfoList.size(); ++i) {
        LogVerbose("%s follower ip %s follow leader ip %s ", __func__,
                   paxos_option_.vecFollowerNodeInfoList[i].oMyNode.GetIP().c_str(),
                   paxos_option_.vecFollowerNodeInfoList[i].oFollowNode.GetIP().c_str());
    }
	return paxos_option_.vecFollowerNodeInfoList.size();
}

void PaxosAgent::SetPaxosOption() {
    paxos_option_.iGroupCount = 1;

    paxos_option_.oMyNode.SetIPPort(option_->GetBinLogSvrConfig()->GetEngineIP(),
                                    option_->GetBinLogSvrConfig()->GetPaxosPort());
    paxos_option_.poBreakpoint = (phxpaxos::Breakpoint*) STATISTICS_PAXOS(GetPaxosCheckPoint())
    ;
    paxos_option_.sLogStoragePath = option_->GetBinLogSvrConfig()->GetPaxosLogPath();
    paxos_option_.bUseMembership = true;
    paxos_option_.pLogFunc = phxsql::GetLogFunc();

    Utils::CheckDir(option_->GetBinLogSvrConfig()->GetPaxosLogPath());
    if (paxos_option_.vecGroupSMInfoList.empty()) {
        GroupSMInfo info;
        info.iGroupIdx = GROUP_ID;
        paxos_option_.vecGroupSMInfoList.push_back(info);
    }

    paxos_option_.bIsLargeValueMode = option_->GetBinLogSvrConfig()->GetPackageMode();
    paxos_option_.iUDPMaxSize = option_->GetBinLogSvrConfig()->GetUDPMaxSize();

    ColorLogInfo("set gourp count %u path %s log path %s, package mode %d, udp size %u", paxos_option_.iGroupCount,
                 paxos_option_.sLogStoragePath.c_str(), option_->GetBinLogSvrConfig()->GetPaxosLogPath(),
                 option_->GetBinLogSvrConfig()->GetPackageMode(), paxos_option_.iUDPMaxSize);
    SetPaxosNodeList(false);
}

void PaxosAgent::AddExecuter(ExecuterBase *sm) {
    LogVerbose("%s sm info list size %zu", __func__, paxos_option_.vecGroupSMInfoList.size());
    paxos_option_.vecGroupSMInfoList[GROUP_ID].vecSMList.push_back(sm);
}

void PaxosAgent::SetPaxosNodeList(bool master) {
	if(SetFollower()){
		paxos_option_.vecNodeInfoList.clear();
		LogVerbose("%s run as %s", __func__, master ? "master" : "slave");
		paxos_option_.vecNodeInfoList.push_back(paxos_option_.oMyNode);
	}else {
		paxos_option_.vecNodeInfoList.clear();
		LogVerbose("%s run as %s", __func__, master ? "master" : "slave");
		if (master) {
			paxos_option_.vecNodeInfoList.push_back(paxos_option_.oMyNode);
		}
	}
}

void PaxosAgent::ReRun() {
    ReleaseNode();
    Run();
}

void PaxosAgent::Run() {
    int ret = Node::RunNode(paxos_option_, paxos_node_);
    if (ret != 0) {
        printf("run node fail, ret %d\n", ret);
        assert(ret == 0);
    }
    paxos_node_->SetHoldPaxosLogCount(1000000);
}

void PaxosAgent::SetHoldPaxosLogCount(const uint64_t &hold_paxos_log_count) {
    paxos_node_->SetHoldPaxosLogCount(hold_paxos_log_count);
}

int PaxosAgent::Send(const string &value, ExecuterBase *sm) {
    int result = 0;
    SMCtx oSMCtx(sm->SMID(), &result);

    PhxTimer timer;
    LogVerbose("%s send value size %zu sm id %d", __func__, value.size(), sm->SMID());
    uint64_t llInstanceID = 0;
    int proposal_ret = paxos_node_->Propose( GROUP_ID, value, llInstanceID, &oSMCtx);

    if (proposal_ret == 0) {
        ColorLogInfo("proposal ok, instance id %llu result %d", llInstanceID, result);
        proposal_ret = result;
    } else {
        if (proposal_ret == PaxosTryCommitRet_Value_Size_TooLarge) {
            proposal_ret = DATA_TOO_LARGE;
        } else {
            proposal_ret = PAXOS_FAIL;
        }
        ColorLogError("proposal fail, ret %d", proposal_ret);
    }
    STATISTICS(PaxosSendTime(timer.GetTime()));
    return proposal_ret;
}

int PaxosAgent::AddMember(const string &ip) {
    if (ip.size()) {
        uint32_t port = option_->GetBinLogSvrConfig()->GetPaxosPort();
        NodeInfo to_node_info;
        to_node_info.SetIPPort(ip.c_str(), port);
        ColorLogInfo("%s add member ip %s port %u", __func__, ip.c_str(), port);
        //add a node
        return paxos_node_->AddMember( GROUP_ID, to_node_info);
    }
    return ARG_FAIL;
}

int PaxosAgent::RemoveMember(const string &ip) {
    if (ip.size()) {
        uint32_t port = option_->GetBinLogSvrConfig()->GetPaxosPort();
        NodeInfo from_node_info;
        from_node_info.SetIPPort(ip.c_str(), port);
        ColorLogInfo("%s delete member ip %s port %u", __func__, ip.c_str(), port);
        //delete a node
        return paxos_node_->RemoveMember( GROUP_ID, from_node_info);
    }
    return ARG_FAIL;
}

int PaxosAgent::GetMemberList(vector<PaxosNodeInfo> *member_list) {
    phxpaxos::NodeInfoList node_info_list;
    int ret = paxos_node_->ShowMembership(GROUP_ID, node_info_list);
    if (ret == 0) {
        for (size_t i = 0; i < node_info_list.size(); ++i) {
            PaxosNodeInfo info;
            info.set_ip(node_info_list[i].GetIP());
            info.set_port(node_info_list[i].GetPort());
            member_list->push_back(info);
        }
    }

    return ret;
}

int PaxosAgent::SetMaster() {
    phxpaxos::NodeInfoList node_info_list;
    int ret = paxos_node_->ShowMembership(GROUP_ID, node_info_list);
    if (ret == 0) {
        if (node_info_list.size()) {
            LogVerbose("%s node has been inited, size %zu", __func__, node_info_list.size());
            for (auto it : node_info_list) {
                LogVerbose("%s get list ip %s", __func__, it.GetIP().c_str());
            }
            return MASTER_INIT_FAIL;
        }
        SetPaxosNodeList(true);
        ReRun();
        LogVerbose("%s inited done", __func__);
    }
    return ret;
}

}
