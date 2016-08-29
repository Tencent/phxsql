/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <assert.h>

#include "phxbinlog_server.h"

void showUsage(const char * program) {
    printf("\n");
    printf("Usage: %s [-c <config>] [-v]\n", program);
    printf("\n");
    exit(0);
}

void SignalHandler() {
    assert(signal(SIGPIPE, SIG_IGN) != SIG_ERR);
    assert(signal(SIGALRM, SIG_IGN) != SIG_ERR);
    assert(signal(SIGCHLD, SIG_IGN) != SIG_ERR);
    return;
}

int main(int argc, char * argv[]) {
    SignalHandler();
    Server server;
    server.Run();

    return 0;
}

