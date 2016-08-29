/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include <string>
#include <stdint.h>

namespace phxsql {

enum {
    OK = 0,
    SOCKET_FAIL = -1,
    SOCKET_LOST = -1000,
};

class NetIO {
 public:
    static int Bind(const char *ip, const uint32_t &port);
    static int Accept(const int &fd);
    static int Connect(const char *ip, const uint32_t &port);
    static int Receive(const int &fd, std::string *recv_buffer);
    static int Receive(const int &fd, char *buffer, const size_t &buffer_len);
    static int Send(const int &fd, const std::string &send_buffer);
    static int SendWithSeq(const int &fd, const std::string &send_buff, const unsigned char &seq);
    static void Close(const int &fd);
    static int SetSendTimeOut(const int &fd);
};

}
