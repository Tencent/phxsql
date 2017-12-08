/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at

	https://opensource.org/licenses/GPL-2.0

	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "phxbinlog_tool_impl.h"

#include "phxbinlogsvr/client/phxbinlog_client_factory_phxrpc.h"
#include "phxbinlogsvr/define/errordef.h"
#include "phxrpc/file.h"
#include "phxbinlogsvr.pb.h"
#include <vector>
#include <unistd.h>

using namespace phxrpc;
using namespace phxbinlog;
using std::string;
using std::vector;
using std::pair;

PhxbinlogToolImpl::PhxbinlogToolImpl() {
}

PhxbinlogToolImpl::~PhxbinlogToolImpl() {
}

int PhxbinlogToolImpl::PhxEcho(phxrpc::OptMap & optMap) {
    google::protobuf::StringValue req;
    google::protobuf::StringValue resp;

    if (NULL == optMap.Get('s'))
        return -1;

    req.set_value(optMap.Get('s'));

    return 0;
}

int PhxbinlogToolImpl::SendBinLog(phxrpc::OptMap & optMap) {
    google::protobuf::StringValue req;
    google::protobuf::Empty resp;

    //TODO: fill req from optMap

    return 0;
}

int PhxbinlogToolImpl::GetMasterInfoFromGlobal(phxrpc::OptMap & optMap) {

    const char * ip = optMap.Get('h');
    int port;
    optMap.GetInt('p', &port);

    PhxBinlogClientFactory_PhxRPC factory;
    std::shared_ptr<phxbinlogsvr::PhxBinlogClient> client = factory.CreateClient(ip, port, 1000);

    string master_ip;
    uint32_t expire_time;
    int ret = client->GetMaster(&master_ip, &expire_time);
    if (ret == 0) {
        printf("get master %s expire time %u\n", master_ip.c_str(), expire_time);
    } else {
        printf("get master fail ret %d\n", ret);
    }
    return ret;
}

int PhxbinlogToolImpl::GetMasterInfoFromLocal(phxrpc::OptMap & optMap) {
    google::protobuf::Empty req;
    google::protobuf::StringValue resp;

    //TODO: fill req from optMap

    return 0;
}

int PhxbinlogToolImpl::GetLastSendGtidFromGlobal(phxrpc::OptMap & optMap) {

    const char * ip = optMap.Get('h');
    int port;
    optMap.GetInt('p', &port);

    const char * uuid = optMap.Get('u');

    string master_ip;
    int ret = GetMasterIP(ip, port, &master_ip);
    if (ret) {
        return ret;
    }

    string gtid;
    PhxBinlogClientFactory_PhxRPC factory;
    std::shared_ptr<phxbinlogsvr::PhxBinlogClient> client = factory.CreateClient(master_ip, port, 1000);
    ret = client->GetLastSendGtid(uuid, &gtid);
    if (ret == 0) {
        printf("%s get uuid %s, gtid %s\n", __func__, uuid, gtid.c_str());
    } else {
        printf("%s fail ret %d\n", __func__, ret);
    }
    return ret;
}

int PhxbinlogToolImpl::GetLastSendGtidFromLocal(phxrpc::OptMap & optMap) {
    google::protobuf::StringValue req;
    google::protobuf::StringValue resp;

    //TODO: fill req from optMap

    return 0;
}

int PhxbinlogToolImpl::SetExportIP(phxrpc::OptMap & optMap) {
    google::protobuf::StringValue req;
    google::protobuf::Empty resp;

    //TODO: fill req from optMap

    return 0;
}

int PhxbinlogToolImpl::HeartBeat(phxrpc::OptMap & optMap) {
    google::protobuf::StringValue req;
    google::protobuf::Empty resp;

    //TODO: fill req from optMap

    return 0;
}

int PhxbinlogToolImpl::GetMasterIP(const string &connect_ip, const int &connect_port, string *master_ip) {
    PhxBinlogClientFactory_PhxRPC factory;
    std::shared_ptr<phxbinlogsvr::PhxBinlogClient> client = factory.CreateClient(connect_ip, connect_port, 1000);

    uint32_t expire_time;
    int ret = client->GetMaster(master_ip, &expire_time);
    if (ret == 0) {
        printf("get master %s expire time %u\n", master_ip->c_str(), expire_time);
    } else {
        printf("get master fail ret %d\n", ret);
    }
    return ret;

}

int PhxbinlogToolImpl::AddMember(phxrpc::OptMap & optMap) {

    const char * ip = optMap.Get('h');
    int port;
    optMap.GetInt('p', &port);

    const char * member_ip = optMap.Get('m');

    string master_ip;
    int ret = GetMasterIP(ip, port, &master_ip);
    if (ret) {
        return ret;
    }

    PhxBinlogClientFactory_PhxRPC factory;
    std::shared_ptr<phxbinlogsvr::PhxBinlogClient> client = factory.CreateClient(master_ip, port, 1000000);
    ret = client->AddMember(member_ip);
    if (ret == 0) {
        printf("%s %s done\n", __func__, member_ip);
    } else {
        printf("%s fail ret %d\n", __func__, ret);
    }
    return ret;
}

int PhxbinlogToolImpl::RemoveMember(phxrpc::OptMap & optMap) {

    const char * ip = optMap.Get('h');
    int port;
    optMap.GetInt('p', &port);

    const char * member_ip = optMap.Get('m');

    string master_ip;
    int ret = GetMasterIP(ip, port, &master_ip);
    if (ret) {
        return ret;
    }

    PhxBinlogClientFactory_PhxRPC factory;
    std::shared_ptr<phxbinlogsvr::PhxBinlogClient> client = factory.CreateClient(master_ip, port, 1000000);
    ret = client->RemoveMember(member_ip);
    if (ret == 0) {
        printf("%s %s done\n", __func__, member_ip);
    } else {
        printf("%s fail ret %d\n", __func__, ret);
    }
    return ret;
}

int PhxbinlogToolImpl::SetMySqlAdminInfo(phxrpc::OptMap & optMap) {

    const char * ip = optMap.Get('h');
    int port;
    optMap.GetInt('p', &port);

    const char * admin_username = optMap.Get('u');
    const char * admin_pwd = optMap.Get('d');

    const char * new_admin_username = optMap.Get('U');
    const char * new_admin_pwd = optMap.Get('D');

    string master_ip;
    int ret = GetMasterIP(ip, port, &master_ip);
    if (ret) {
        return ret;
    }

    PhxBinlogClientFactory_PhxRPC factory;
    std::shared_ptr<phxbinlogsvr::PhxBinlogClient> client = factory.CreateClient(master_ip, port, 1000);

    ret = client->SetMySqlAdminInfo(admin_username, admin_pwd, new_admin_username, new_admin_pwd);
    if (ret == 0) {
        printf("%s admin username %s -> new admin username %s\n", __func__, admin_username, new_admin_username);
    } else {
        if (ret == USERINFO_CHANGE_DENIED) {
            printf("%s admin username or pwd incorrect\n", __func__);
        } else {
            printf("%s fail ret %d\n", __func__, ret);
        }
    }
    return ret;
}

int PhxbinlogToolImpl::SetMySqlReplicaInfo(phxrpc::OptMap & optMap) {

    const char * ip = optMap.Get('h');
    int port;
    optMap.GetInt('p', &port);

    const char * admin_username = optMap.Get('u');
    const char * admin_pwd = optMap.Get('d');

    const char * new_username = optMap.Get('U');
    const char * new_pwd = optMap.Get('D');

    string master_ip;
    int ret = GetMasterIP(ip, port, &master_ip);
    if (ret) {
        return ret;
    }

    PhxBinlogClientFactory_PhxRPC factory;
    std::shared_ptr<phxbinlogsvr::PhxBinlogClient> client = factory.CreateClient(master_ip, port, 1000);
    ret = client->SetMySqlReplicaInfo(admin_username, admin_pwd, new_username, new_pwd);
    if (ret == 0) {
        printf("%s set new replica username %s\n", __func__, new_username);
    } else {
        if (ret == USERINFO_CHANGE_DENIED) {
            printf("%s admin username or pwd incorrect\n", __func__);
        } else {
            printf("%s fail ret %d\n", __func__, ret);
        }
    }
    return ret;
}

int PhxbinlogToolImpl::GetMemberList(phxrpc::OptMap & optMap) {

    const char * ip = optMap.Get('h');
    int port;
    optMap.GetInt('p', &port);

    string master_ip;
    int ret = GetMasterIP(ip, port, &master_ip);
    if (ret) {
        return ret;
    }

    vector < string > ip_list;
    uint32_t member_port;
    PhxBinlogClientFactory_PhxRPC factory;
    std::shared_ptr<phxbinlogsvr::PhxBinlogClient> client = factory.CreateClient(master_ip, port, 1000);
    ret = client->GetMemberList(&ip_list, &member_port);
    if (ret == 0) {
        for (auto it : ip_list) {
            printf("ip %s port %u\n", it.c_str(), member_port);
        }
    } else {
        printf("%s fail ret %d\n", __func__, ret);
    }
    return ret;
}

vector<string> GetIPList(const char *ip_list_str) {
    vector < string > ip_list;
    string ip = "";
    int len = strlen(ip_list_str);
    for (int i = 0; i < len; ++i) {
        if (ip_list_str[i] == ',') {
            ip_list.push_back(ip);
            ip = "";
            continue;
        }
        ip += ip_list_str[i];
    }
    if (ip.size() > 0) {
        ip_list.push_back(ip);
    }
    return ip_list;
}

bool PhxbinlogToolImpl::CheckIPList(const vector<string> &ip_list, const uint32_t &port) {
    if (ip_list.size() == 0) {
        printf("ip list empty, init fail\n");
        return false;
    }
    for (auto ip : ip_list) {
        string master_ip;
        int ret = GetMasterIP(ip, port, &master_ip);
        if (ret) {
            printf("connect machine %s fail\n", ip.c_str());
            return ret;
        }
        if (master_ip.size() > 0) {
            printf("machine %s has been set master(%s)\n", ip.c_str(), master_ip.c_str());
            return false;
        }
    }
    return true;
}

int PhxbinlogToolImpl::InitBinlogSvrMaster(phxrpc::OptMap & opt_map) {

    const char * ip_list_str = opt_map.Get('h');
    int port;
    opt_map.GetInt('p', &port);
    vector < string > ip_list = GetIPList(ip_list_str);
    if (!CheckIPList(ip_list, port)) {
        return -1;
    }

    PhxBinlogClientFactory_PhxRPC factory;
    std::shared_ptr<phxbinlogsvr::PhxBinlogClient> client = factory.CreateClient(ip_list[0], port, 1000000);
    int ret = client->InitBinlogSvrMaster();
    if (ret) {
        printf("init svr fail, ret %d\n", ret);
        return -1;
    }
    printf("init master %s done, start to add member\n", ip_list[0].c_str());
    while (1) {
        string master_ip;
        int ret = GetMasterIP(ip_list[0], port, &master_ip);
        if (ret == -1) {
            printf("get master ip fail, but init is done, please turn to add member step\n");
            return -1;
        }
        if (master_ip == "") {
            printf("waiting master to be started\n");
            sleep(1);
            continue;
        }
        printf("master started, ip %s\n", master_ip.c_str());
        break;
    }

    for (size_t i = 1; i < ip_list.size(); ++i) {
        int ret = client->AddMember(ip_list[i]);
        if (ret) {
            printf("add ip %s to master fail, please use change member to try agin\n", ip_list[i].c_str());
            return -1;
        }
        printf("add ip %s to master done\n", ip_list[i].c_str());
    }
    return 0;
}

