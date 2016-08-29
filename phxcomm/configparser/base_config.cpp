/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "phxcomm/base_config.h"
#include "phxcomm/phx_log.h"

#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

using std::string;

namespace phxsql {

char *PhxBaseConfig::InitInnerIP() {
    int sock_fd;
    static char addr[32] = { "" };
    char tmp_addr[32] = { "" };

    struct in_addr my_addr[2];
    struct ifreq stIfreq[10];
    const char *pInnerList[] = { "10.0.0.0", "10.255.255.255", "172.16.0.0", "172.31.255.255", "192.168.0.0",
            "192.168.255.255", "169.254.0.0", "169.254.255.255" };

    if (-1 == (sock_fd = socket(PF_INET, SOCK_DGRAM, 0))) {
        return addr;
    }

    struct ifconf ifc;
    memset(&ifc, 0, sizeof(ifc));
    ifc.ifc_len = sizeof(stIfreq);
    ifc.ifc_req = stIfreq;
    ioctl(sock_fd, SIOCGIFCONF, &ifc);

    bool is_eth1 = false;
    bool is_get_innerip = false;

    for (size_t i = 0; i < ifc.ifc_len / sizeof(ifreq); i++) {
        sockaddr_in staddr;
        memcpy(&staddr, &ifc.ifc_req[i].ifr_addr, sizeof(staddr));

        if (0 == strncasecmp(ifc.ifc_req[i].ifr_name, "eth1", strlen("eth1"))) {
            is_eth1 = true;
        }

        if (!is_eth1 && is_get_innerip) {
            continue;
        }

        for (size_t j = 0; j < sizeof(pInnerList) / sizeof(pInnerList[0]) / 2; j++) {
            inet_aton(pInnerList[j * 2], &my_addr[0]);
            my_addr[0].s_addr = htonl(my_addr[0].s_addr);
            inet_aton(pInnerList[j * 2 + 1], &my_addr[1]);
            my_addr[1].s_addr = htonl(my_addr[1].s_addr);

            unsigned long lTmp = htonl(staddr.sin_addr.s_addr);
            if (lTmp >= my_addr[0].s_addr && lTmp <= my_addr[1].s_addr) {
                if (is_eth1) {
                    inet_ntop(AF_INET, &staddr.sin_addr, addr, sizeof(addr));
                    close(sock_fd);
                    return addr;
                }
                is_get_innerip = true;
                memset(tmp_addr, 0x0, sizeof(tmp_addr));
                inet_ntop(AF_INET, &staddr.sin_addr, tmp_addr, sizeof(tmp_addr));
            }
        }
        is_eth1 = false;
    }

    memcpy(addr, tmp_addr, sizeof(tmp_addr));
    close(sock_fd);

    return addr;
}

PhxBaseConfig::PhxBaseConfig() {
}

int PhxBaseConfig::ReadFileWithConfigDirPath(const string &filename) {
    return RealReadFile(filename);
}

int PhxBaseConfig::ReadFile(const string &filename) {
    return RealReadFile(GetDefaultPath() + filename);
}

int PhxBaseConfig::RealReadFile(const string &filename) {
    if (INIReader::ReadFile(filename.c_str()) == 0) {
        LogWarning("%s  read path %s done", __func__, filename.c_str());
        ReadConfig();
        return 0;
    }
    LogWarning("%s  read path fail %s, ret %d, error %s", __func__, filename.c_str(), INIReader::ParseError(),
               strerror(errno));
    return 1;
}

string PhxBaseConfig::Get(const string &section, const string &name, const string &default_value) {
    return INIReader::Get(section, name, default_value);
}

int PhxBaseConfig::GetInteger(const string &section, const string &name, const int &default_value) {
    return INIReader::GetInteger(section, name, default_value);
}

string PhxBaseConfig::inner_ip_;

//get local ip
string PhxBaseConfig::GetInnerIP() {
    if (inner_ip_ == "") {
        inner_ip_ = InitInnerIP();
    }

    return inner_ip_;
}

string PhxBaseConfig::s_default_path;

string PhxBaseConfig::GetBasePath() {
    static char szTemp[512] = { 0 };

    char pszExeName[512] = { 0 };

    int iFD = open("/proc/self/cmdline", O_RDONLY, 0);
    if (iFD < 0)
        return pszExeName;

    if (read(iFD, szTemp, sizeof(szTemp) - 1) < 0) {
        close(iFD);
        return "";
    }
    szTemp[511] = 0;
    close(iFD);

    string path = szTemp;

    char real_path[128];

    realpath(path.c_str(), real_path);
    path = real_path;
    size_t pos = path.rfind("sbin/");
    if (pos == string::npos) {
        size_t pos = path.rfind("phxsql/");
        if (pos == string::npos) {
            return "../";
        } else {
            return path.substr(0, pos + 7);
        }
    } else {
        return path.substr(0, pos);
    }
}

//set the default path, where config files are located
void PhxBaseConfig::SetDefaultPath(const char *config_path) {
    s_default_path = config_path;
}

//get the default path, where config files are located
string PhxBaseConfig::GetDefaultPath() {
    if (s_default_path == "") {
        s_default_path = PhxBaseConfig::GetBasePath() + "etc/";
        LogWarning("%s get debuf path %s", __func__, s_default_path.c_str());
    }
    return s_default_path;
}

}

