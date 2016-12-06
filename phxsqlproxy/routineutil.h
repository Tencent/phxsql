/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include <sys/socket.h>

namespace phxsqlproxy {

int RoutineConnectWithTimeout(int fd, const struct sockaddr *address, socklen_t address_len, int timeout_ms);

int RoutineWriteWithTimeout(int dest_fd, const char * buf, int write_size, int timeout_ms);

int RoutineReadWithTimeout(int source_fd, char * buf, int buf_size, int timeout_ms);

int RoutinePeekWithTimeout(int source_fd, char * buf, int buf_size, int timeout_ms);

}
