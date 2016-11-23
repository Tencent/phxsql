/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <poll.h>
#include <stack>

#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <string>
#include <cstring>
#include <errno.h>

#include "phxsqlproxyutil.h"

#include "phxcomm/phx_log.h"

using namespace std;
using phxsql::LogError;

namespace phxsqlproxy {

int SetNoDelay(int sock_fd) {
    int one = 1;
    return setsockopt(sock_fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
}

bool IsNonBlock(int sock_fd) {
    int flags;
    flags = fcntl(sock_fd, F_GETFL, 0);
    if (flags & O_NONBLOCK) {
        return true;
    }
    return false;
}

int SetNonBlock(int sock_fd) {
    int flags;
    flags = fcntl(sock_fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    flags |= O_NDELAY;
    int ret = fcntl(sock_fd, F_SETFL, flags);
    return ret;
}

void SetAddr(const char *ip_string, const unsigned short port, struct sockaddr_in &addr) {
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    int ip = 0;
    if (!ip_string || '\0' == *ip_string || 0 == strcmp(ip_string, "0") || 0 == strcmp(ip_string, "0.0.0.0")
            || 0 == strcmp(ip_string, "*")) {
        ip = htonl(INADDR_ANY);
    } else {
        ip = inet_addr(ip_string);
    }
    addr.sin_addr.s_addr = ip;

}

int CreateTcpSocket(const unsigned short port /* = 0 */, const char *ip /* = "*" */, bool is_reuse /* = false */) {
    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd >= 0) {
        if (port != 0) {
            if (is_reuse) {
                int reuse_addr = 1;
                setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));
            }
            struct sockaddr_in addr;
            SetAddr(ip, port, addr);
            int ret = bind(fd, (struct sockaddr*) &addr, sizeof(addr));
            if (ret != 0) {
                close(fd);
                return -1;
            }
        }
    }
    return fd;
}

bool IsAuthReqPkg(const char * buf, int len) {
    if (len < 5) {
        return false;
    }
    return buf[4] == 0xA;
}

bool IsAuthFinishPkg(const char * buf, int len) {
    if (len < 5) {
        return false;
    }
    return buf[4] == 0x0;
}

uint64_t GetTimestampMS() {
    uint64_t u = 0;
    struct timeval now;
    gettimeofday(&now, NULL);
    u += now.tv_sec;
    u *= 1000;
    u += now.tv_usec / 1000;
    return u;
}

void GetMysqlBufDebugString(const char * buf, int len, std::string & debug_str) {
    debug_str = "";
    for (int i = 0; i < len; ++i) {
        char debug_buf[16] = { 0 };
        snprintf(debug_buf, sizeof(debug_buf), "(%u,0x%X)", (unsigned int) ((unsigned char) buf[i]), buf[i]);
        debug_str = debug_str + " " + string(debug_buf);
    }
}

int GetSockName(int fd, std::string & ip, int & port) {
    struct sockaddr_in socket_address;
    socklen_t sock_len = sizeof(socket_address);

    int ret = getsockname(fd, (struct sockaddr*) &socket_address, &sock_len);
    if (ret == -1) {
        LogError("getsockname fd [%d] failed, ret %d, errno (%d:%s)", fd, ret, errno, strerror(errno));
        return -__LINE__;
    }

    char buf[INET6_ADDRSTRLEN] = { 0 };
    if (inet_ntop(socket_address.sin_family, &socket_address.sin_addr, buf, INET6_ADDRSTRLEN) == NULL) {
        LogError("inet_ntop failed, ret NULL, errno(%d:%s)", errno, strerror(errno));
        return -__LINE__;
    }
    ip = string(buf);
    port = ntohs(socket_address.sin_port);
    return 0;
}

int GetPeerName(int fd, std::string & ip, int & port) {
    struct sockaddr_in socket_address;
    socklen_t sock_len = sizeof(socket_address);

    int ret = getpeername(fd, (struct sockaddr*) &socket_address, &sock_len);
    if (ret == -1) {
        LogError("getpeername fd [%d] failed, ret %d, errno (%d:%s)", fd, ret, errno, strerror(errno));
        return -__LINE__;
    }

    char buf[INET6_ADDRSTRLEN] = { 0 };
    if (inet_ntop(socket_address.sin_family, &socket_address.sin_addr, buf, INET6_ADDRSTRLEN) == NULL) {
        LogError("inet_ntop failed, ret NULL, errno(%d:%s)", errno, strerror(errno));
        return -__LINE__;
    }
    ip = string(buf);
    port = ntohs(socket_address.sin_port);
    return 0;
}

uint64_t DecodedLengthBinary(const char * buf, int len, int & len_field_size) {
    uint8_t first_bit = (uint8_t)(buf[0]);
    uint64_t package_len = 0;
    switch (first_bit) {
        case 251: {
            package_len = 0;
            len_field_size = 1;
            break;
        }
        case 252: {
            if (len < 3) {
                package_len = -1;
            } else {
                memcpy(&package_len, buf + 1, sizeof(char) * 2);
                len_field_size = 3;
            }
            break;
        }
        case 253: {
            if (len < 4) {
                package_len = -1;
            } else {
                memcpy(&package_len, buf + 1, sizeof(char) * 3);
                len_field_size = 4;
            }
            break;
        }
        case 254: {
            if (len < 9) {
                package_len = -1;
            } else {
                memcpy(&package_len, buf + 1, sizeof(char) * 8);
                len_field_size = 9;
            }
            break;
        }
        default:
            package_len = first_bit;
            len_field_size = 1;
            break;
    }
    return package_len;
}

std::string UIntToStr(uint32_t i) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%u", i);
    return std::string(buf);
}

}

