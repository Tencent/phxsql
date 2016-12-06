/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>

#include "proxy_protocol_handler.h"

#include "phxcomm/phx_log.h"
#include "phxsqlproxyutil.h"
#include "routineutil.h"

namespace phxsqlproxy {

#define PRETTY_LOG(log, fmt, ...) \
    log("%s:%d requniqid %llu " fmt, __func__, __LINE__, req_uniq_id_, ## __VA_ARGS__)

#define LOG_ERR(...)   PRETTY_LOG(phxsql::LogError,   __VA_ARGS__)
#define LOG_WARN(...)  PRETTY_LOG(phxsql::LogWarning, __VA_ARGS__)
#define LOG_DEBUG(...) PRETTY_LOG(phxsql::LogVerbose, __VA_ARGS__)

// proxy protocol specification

struct ProxyHdr { // proxy protocol v2
    uint8_t sig[12];
    uint8_t ver_cmd;
    uint8_t fam;
    uint16_t len;
    union {
        struct {  // for TCP/UDP over IPv4, len = 12
            uint32_t src_addr;
            uint32_t dst_addr;
            uint16_t src_port;
            uint16_t dst_port;
        } __attribute__((packed)) ip4;
        struct {  // for TCP/UDP over IPv6, len = 36
            uint8_t  src_addr[16];
            uint8_t  dst_addr[16];
            uint16_t src_port;
            uint16_t dst_port;
        } __attribute__((packed)) ip6;
    } addr;
} __attribute__((packed));

enum {
    PP2_VERSION = 0x20
};

enum {
    PP2_CMD_LOCAL = 0x00,
    PP2_CMD_PROXY = 0x01
};

enum {
    PP2_FAM_UNSPEC = 0x00,
    PP2_FAM_TCP4 = 0x11,
    PP2_FAM_TCP6 = 0x21
};

#define PP2_SIGNATURE "\x0D\x0A\x0D\x0A\x00\x0D\x0A\x51\x55\x49\x54\x0A"

// proxy protocol specification end

ProxyProtocolHandler::ProxyProtocolHandler(PHXSqlProxyConfig * config,
                                           WorkerConfig_t    * worker_config,
                                           GroupStatusCache  * group_status_cache) :
        config_(config),
        worker_config_(worker_config),
        group_status_cache_(group_status_cache) {
    Clear();
}

ProxyProtocolHandler::~ProxyProtocolHandler() {
}

void ProxyProtocolHandler::SetReqUniqID(uint64_t req_uniq_id) {
    req_uniq_id_ = req_uniq_id;
}

void ProxyProtocolHandler::Clear() {
    req_uniq_id_ = 0;
    memset(&src_addr_, 0, sizeof(src_addr_));
    memset(&dst_addr_, 0, sizeof(dst_addr_));
}

int ProxyProtocolHandler::GetAddrFromFD(int fd) {
    int ret;

    socklen_t src_len = sizeof(src_addr_);
    ret = getpeername(fd, (struct sockaddr *) &src_addr_, &src_len);
    if (ret < 0) {
        LOG_ERR("getpeername fd [%d] failed, ret %d, errno (%d:%s)", fd, ret, errno, strerror(errno));
        return -__LINE__;
    }

    socklen_t dst_len = sizeof(dst_addr_);
    ret = getsockname(fd, (struct sockaddr *) &dst_addr_, &dst_len);
    if (ret < 0) {
        LOG_ERR("getsockname fd [%d] failed, ret %d, errno (%d:%s)", fd, ret, errno, strerror(errno));
        return -__LINE__;
    }

    return 0;
}

int ProxyProtocolHandler::IsProxyHeaderNeed() { // 1:need, 0:no need, <0:reject
    std::string src_ip;
    std::string dst_ip;
    int src_port;
    int dst_port;

    if (SockAddrToIPPort((struct sockaddr *) &src_addr_, src_ip, src_port) < 0) {
        return -__LINE__;
    }
    if (SockAddrToIPPort((struct sockaddr *) &dst_addr_, dst_ip, dst_port) < 0) {
        return -__LINE__;
    }

    LOG_DEBUG("receive connect from [%s:%d]", src_ip.c_str(), src_port);

    if (dst_port != worker_config_->proxy_port_) {
        return 0;
    }

    if (!group_status_cache_->IsMember(src_ip)) {
        LOG_ERR("non-member ip [%s] connect port [%d], reject", src_ip.c_str(), dst_port);
        return -__LINE__;
    }

    return 1;
}

int ProxyProtocolHandler::ProcessProxyHeader(int fd) {
    int ret = GetAddrFromFD(fd);
    if (ret < 0) {
        return ret;
    }

    ret = IsProxyHeaderNeed();
    if (ret <= 0) {
        return ret;
    }

    union {
        char     v1[108];
        ProxyHdr v2;
    } hdr;

    int hdr_size = RoutinePeekWithTimeout(fd, (char *) &hdr, sizeof(hdr), config_->ProxyProtocolTimeoutMs());
    if (hdr_size < 0) {
        LOG_ERR("read proxy header from fd [%d] failed, ret %d", fd, hdr_size);
        return -__LINE__;
    }

    if (hdr_size >= 16 && memcmp(&hdr.v2, PP2_SIGNATURE, 12) == 0 && (hdr.v2.ver_cmd & 0xF0) == PP2_VERSION) {
        hdr_size = ParseProxyHeaderV2(hdr.v2, hdr_size);
    } else if (hdr_size >= 8 && memcmp(hdr.v1, "PROXY ", 6) == 0) {
        hdr_size = ParseProxyHeaderV1(hdr.v1, hdr_size);
    } else {
        return -__LINE__;
    }

    if (hdr_size > 0) {
        ret = read(fd, &hdr, hdr_size);
        if (ret != hdr_size) {
            LOG_ERR("flush fd [%d] failed, ret %d hdr size %d", fd, ret, hdr_size);
            return -__LINE__;
        }
    }

    return hdr_size;
}

int ProxyProtocolHandler::ParseProxyHeaderV1(const char * hdr, int read_size) {
    const char * end = (const char *) memchr(hdr, '\r', read_size - 1);
    int size = end - hdr + 2;
    if (!end || *(end + 1) != '\n') {
        return -__LINE__;
    }

    std::vector<std::string> tokens = SplitStr(std::string(hdr, end), " ");
    if (tokens[1] == "UNKNOWN") {
        return size;
    }
    if (tokens.size() != 6) {
        return -__LINE__;
    }

    const std::string & src_ip = tokens[2];
    const std::string & dst_ip = tokens[3];
    int src_port = atoi(tokens[4].c_str());
    int dst_port = atoi(tokens[5].c_str());
    if (src_port < 0 || src_port > 65535 || dst_port < 0 || dst_port > 65535) {
        return -__LINE__;
    }

    if (tokens[1] == "TCP4") {
        if (SetAddr(src_ip.c_str(), src_port, (struct sockaddr_in &) src_addr_) != 0) {
            return -__LINE__;
        }
        if (SetAddr(dst_ip.c_str(), dst_port, (struct sockaddr_in &) dst_addr_) != 0) {
            return -__LINE__;
        }
    } else if (tokens[1] == "TCP6") {
        if (SetAddr6(src_ip.c_str(), src_port, (struct sockaddr_in6 &) src_addr_) != 0) {
            return -__LINE__;
        }
        if (SetAddr6(dst_ip.c_str(), dst_port, (struct sockaddr_in6 &) dst_addr_) != 0) {
            return -__LINE__;
        }
    }

    return size;
}

int ProxyProtocolHandler::ParseProxyHeaderV2(const ProxyHdr & hdr, int read_size) {
    int size = 16 + ntohs(hdr.len);
    if (read_size < size) { // truncated or too large header
        return -__LINE__;
    }
    switch (hdr.ver_cmd & 0xF) {
    case PP2_CMD_PROXY:
        switch (hdr.fam) {
        case PP2_FAM_TCP4:
            src_addr_.ss_family = AF_INET;
            ((struct sockaddr_in *) &src_addr_)->sin_addr.s_addr = hdr.addr.ip4.src_addr;
            ((struct sockaddr_in *) &src_addr_)->sin_port = hdr.addr.ip4.src_port;
            dst_addr_.ss_family = AF_INET;
            ((struct sockaddr_in *) &dst_addr_)->sin_addr.s_addr = hdr.addr.ip4.dst_addr;
            ((struct sockaddr_in *) &dst_addr_)->sin_port = hdr.addr.ip4.dst_port;
            return size;
        case PP2_FAM_TCP6:
            src_addr_.ss_family = AF_INET6;
            memcpy(&((struct sockaddr_in6 *) &src_addr_)->sin6_addr, hdr.addr.ip6.src_addr, 16);
            ((struct sockaddr_in6 *) &src_addr_)->sin6_port = hdr.addr.ip6.src_port;
            dst_addr_.ss_family = AF_INET6;
            memcpy(&((struct sockaddr_in6 *) &dst_addr_)->sin6_addr, hdr.addr.ip6.dst_addr, 16);
            ((struct sockaddr_in6 *) &dst_addr_)->sin6_port = hdr.addr.ip6.dst_port;
            return size;
        case PP2_FAM_UNSPEC:
            return size; // unknown protocol, keep local connection address
        default:
            return -__LINE__; // unsupport protocol
        }
    case PP2_CMD_LOCAL:
        return size; // keep local connection address for LOCAL
    default:
        return -__LINE__; // unsupport command
    }
    return -__LINE__;
}

int ProxyProtocolHandler::SendProxyHeader(int fd) {
    int ret = 0;
    std::string header;
    switch (config_->ProxyProtocol()) {
    case 0:
        return 0;
    case 1:
        ret = MakeProxyHeaderV1(header);
        break;
    case 2:
        ret = MakeProxyHeaderV2(header);
        break;
    default:
        return -__LINE__;
    }

    while (ret == 0) {
        ret = RoutineWriteWithTimeout(fd, header.c_str(), header.size(), config_->WriteTimeoutMs());
        if (ret != 0 && ret != (int) header.size()) {
            LOG_ERR("write header to fd [%d] failed, ret %d, hdr size %zu", fd, ret, header.size());
            return -__LINE__;
        }
    }

    return ret;
}

int ProxyProtocolHandler::MakeProxyHeaderV1(std::string & header) {
    if (src_addr_.ss_family == AF_INET && dst_addr_.ss_family == AF_INET) {
        header = "PROXY TCP4 ";
    } else if (src_addr_.ss_family == AF_INET6 && dst_addr_.ss_family == AF_INET6) {
        header = "PROXY TCP6 ";
    } else {
        header = "PROXY UNKNOWN\r\n";
        return 0;
    }

    std::string src_ip;
    std::string dst_ip;
    int src_port;
    int dst_port;

    if (SockAddrToIPPort((struct sockaddr *) &src_addr_, src_ip, src_port) < 0) {
        return -__LINE__;
    }
    if (SockAddrToIPPort((struct sockaddr *) &dst_addr_, dst_ip, dst_port) < 0) {
        return -__LINE__;
    }

    header += src_ip + " " + dst_ip + " " + UIntToStr(src_port) + " " + UIntToStr(dst_port) + "\r\n";
    LOG_DEBUG("make v1 proxy header: %s", header.c_str());
    return 0;
}

int ProxyProtocolHandler::MakeProxyHeaderV2(std::string & header) {
    ProxyHdr hdr;
    memcpy(hdr.sig, PP2_SIGNATURE, sizeof(hdr.sig));
    hdr.ver_cmd = PP2_VERSION | PP2_CMD_PROXY;

    if (src_addr_.ss_family == AF_INET && dst_addr_.ss_family == AF_INET) {
        hdr.fam = PP2_FAM_TCP4;
        hdr.len = htons(sizeof(hdr.addr.ip4));
        hdr.addr.ip4.src_addr = ((struct sockaddr_in *) &src_addr_)->sin_addr.s_addr;
        hdr.addr.ip4.src_port = ((struct sockaddr_in *) &src_addr_)->sin_port;
        hdr.addr.ip4.dst_addr = ((struct sockaddr_in *) &dst_addr_)->sin_addr.s_addr;
        hdr.addr.ip4.dst_port = ((struct sockaddr_in *) &dst_addr_)->sin_port;
        header = std::string((char *) &hdr, 16 + sizeof(hdr.addr.ip4));
    } else if (src_addr_.ss_family == AF_INET6 && dst_addr_.ss_family == AF_INET6) {
        hdr.fam = PP2_FAM_TCP6;
        hdr.len = htons(sizeof(hdr.addr.ip6));
        memcpy(hdr.addr.ip6.src_addr, &((struct sockaddr_in6 *) &src_addr_)->sin6_addr, 16);
        hdr.addr.ip6.src_port = ((struct sockaddr_in6 *) &src_addr_)->sin6_port;
        memcpy(hdr.addr.ip6.dst_addr, &((struct sockaddr_in6 *) &dst_addr_)->sin6_addr, 16);
        hdr.addr.ip6.dst_port = ((struct sockaddr_in6 *) &dst_addr_)->sin6_port;
        header = std::string((char *) &hdr, 16 + sizeof(hdr.addr.ip6));
    } else {
        hdr.fam = PP2_FAM_UNSPEC;
        hdr.len = htons(0);
        header = std::string((char *) &hdr, 16);
    }

    LOG_DEBUG("make v2 proxy header, size: %zu", header.size());
    return 0;
}

}
