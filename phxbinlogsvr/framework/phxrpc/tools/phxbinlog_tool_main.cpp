/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "phxrpc_phxbinlog_tool.h"
#include "phxbinlog_tool_impl.h"

#include "phxrpc/file.h"

using namespace phxrpc;

void showUsage(const char * program) {
    printf("\nUsage: %s [-c <config>] [-f <func>] [-v]\n", program);

    PhxbinlogTool::Name2Func_t * name2func = PhxbinlogTool::GetName2Func();

    for (int i = 0;; i++) {
        PhxbinlogTool::Name2Func_t * iter = &(name2func[i]);

        if (NULL == iter->name)
            break;

        printf("    -f %s %s\n", iter->name, iter->usage);
    }
    printf("\n");
    exit(0);
}

int main(int argc, char * argv[]) {
    const char * func = NULL;

    for (int i = 1; i < argc - 1; i++) {
        if (0 == strcmp(argv[i], "-f")) {
            func = argv[++i];
        }
        if (0 == strcmp(argv[i], "-v")) {
            showUsage(argv[0]);
        }
    }

    if (NULL == func)
        showUsage(argv[0]);

    PhxbinlogTool::Name2Func_t * target = NULL;

    PhxbinlogTool::Name2Func_t * name2func = PhxbinlogTool::GetName2Func();

    for (int i = 0; i < 100; i++) {
        PhxbinlogTool::Name2Func_t * iter = &(name2func[i]);

        if (NULL == iter->name)
            break;

        if (0 == strcasecmp(func, iter->name)) {
            target = iter;
            break;
        }
    }

    if (NULL == target)
        showUsage(argv[0]);

    OptMap opt_map(target->opt_string);

    if (!opt_map.Parse(argc, argv))
        showUsage(argv[0]);

    PhxbinlogTool::ToolFunc_t targefunc = target->func;

    PhxbinlogToolImpl tool;

    if (0 != (tool.*targefunc)(opt_map))
        showUsage(argv[0]);

    return 0;
}

