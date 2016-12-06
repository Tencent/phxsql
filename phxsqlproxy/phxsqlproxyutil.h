/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include <string>
#include <vector>
#include <inttypes.h>

namespace phxsqlproxy {

int CreateTcpSocket(const unsigned short port /* = 0 */, const char *ip /* = "*" */, bool is_reuse /* = false */);

int SetNoDelay(int sock_fd);

bool IsNonBlock(int sock_fd);

int SetNonBlock(int sock_fd);

int SetAddr(const char * ip, const unsigned short port, struct sockaddr_in & addr);

int SetAddr6(const char * ip, const unsigned short port, struct sockaddr_in6 & addr);

bool IsAuthReqPkg(const char * buf, int len);

bool IsAuthFinishPkg(const char * buf, int len);

uint64_t GetTimestampMS();

void GetMysqlBufDebugString(const char * buf, int len, std::string & debug_str);

int SockAddrToIPPort(struct sockaddr * addr, std::string & ip, int & port);

int GetSockName(int fd, std::string & ip, int & port);

int GetPeerName(int fd, std::string & ip, int & port);

uint64_t DecodedLengthBinary(const char * buf, int len, int & len_field_size);

std::string UIntToStr(uint32_t i);

std::vector<std::string> SplitStr(const std::string & str, const std::string & delim);

}
