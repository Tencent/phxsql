/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "phxcomm/net.h"
#include "phxcomm/phx_log.h"

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <string.h>

using std::string;

namespace phxsql {

int NetIO::Bind(const char *ip, const uint32_t &port) {
    int server_sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_sockaddr;
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = htons(port);
    server_sockaddr.sin_addr.s_addr = inet_addr(ip);

    int enable = 1;
    setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    if (bind(server_sockfd, (struct sockaddr *) &server_sockaddr, sizeof(server_sockaddr)) == -1) {
        ColorLogError("bind fail, ip %s port %d", ip, port);
        return SOCKET_FAIL;
    }

    if (listen(server_sockfd, 20) == -1) {
        ColorLogError("listen fail");
        return SOCKET_FAIL;
    }
    ColorLogInfo("bind ip %s port %d ok", ip, port);
    return server_sockfd;
}

int NetIO::SetSendTimeOut(const int &fd) {
    struct timeval timeout = { 1, 0 };
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout))) {
        ColorLogError("%s set fd %d timeout fail, error %s", __func__, fd, strerror(errno));
        return SOCKET_FAIL;
    }
    return OK;
}

int NetIO::Accept(const int &fd) {
    struct sockaddr_in client_addr;
    socklen_t length = sizeof(client_addr);

    int conn = accept(fd, (struct sockaddr*) &client_addr, &length);
    if (conn < 0) {
        ColorLogError("accent fail fd %d", fd);
        return SOCKET_FAIL;
    }
    return conn;
}

int NetIO::Connect(const char *ip, const uint32_t &port) {
    int sock_cli = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = inet_addr(ip);

    if (connect(sock_cli, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        ColorLogError("connect %s:%d fail", ip, port);
        return SOCKET_FAIL;
    }

    ColorLogInfo("connect ip %s port %d done fd %d", ip, port, sock_cli);
    return sock_cli;
}

int NetIO::Receive(const int &fd, string *data) {
    if (fd < 0)
        return SOCKET_FAIL;

    char header[1024];
    int header_len = 4;

    int ret = Receive(fd, header, header_len);
    if (ret) {
        return ret;
    }

    int body_len = 0;
    memcpy(&body_len, header, 3);

    data->resize(body_len + header_len);
    memcpy((char *) data->c_str(), header, header_len);

    return Receive(fd, (char *) data->c_str() + header_len, body_len);
}

int NetIO::Receive(const int &fd, char *buffer, const size_t &buffer_len) {
    if (fd < 0)
        return SOCKET_FAIL;

    int want_len = buffer_len;
    int write_pos = 0;

    while (want_len > 0) {
        int read_len = recv(fd, buffer + write_pos, want_len, 0);
        if (read_len == 0) {
            ColorLogError("recv fd %d len %zu want %d, connect lost", fd, read_len, want_len);
            return SOCKET_LOST;
        }
        if (read_len < 0) {
            ColorLogError("read data fail, fd %d, error %s", fd, strerror(errno));
            //perror("recv");
            return SOCKET_FAIL;
        }
        want_len -= read_len;
        write_pos += read_len;
    }
    return OK;
}

int NetIO::Send(const int &fd, const string &send_buffer) {
    if (fd < 0)
        return SOCKET_FAIL;
    return (send(fd, send_buffer.c_str(), send_buffer.size(), 0) == (int) (send_buffer.size())) ? OK : SOCKET_FAIL;
}

int NetIO::SendWithSeq(const int &fd, const string &send_buffer, const unsigned char &seq) {
    SetSendTimeOut(fd);

    string buffer;
    char header[4];
    int len = send_buffer.size();
    memcpy(header, &len, 3);
    header[3] = seq;

    int ret = send(fd, header, sizeof(header), 0);
    if (ret != sizeof(header))
        return SOCKET_FAIL;

    ret = send(fd, send_buffer.c_str(), send_buffer.size(), 0);
    if (ret != (int) (send_buffer.size()))
        return SOCKET_FAIL;

    return OK;
}

void NetIO::Close(const int &fd) {
    close(fd);
}

}

