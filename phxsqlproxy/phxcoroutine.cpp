/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include <cstdlib>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "co_routine.h"
#include "phxsqlproxyutil.h"
#include "phxcoroutine.h"

#include "phxcomm/phx_log.h"

using phxsql::LogWarning;

namespace phxsqlproxy {

static int g_routine_id = 0;

static void *RoutineRun(void * p) {
    g_routine_id++;
    LogWarning("routine %d running....", g_routine_id);
    //phoenix::SetLogWorkerIdx( g_routine_id );

    Coroutine * routine = (Coroutine *) p;
    routine->SetRoutineIdx(g_routine_id);
    int ret = routine->run();
    if (ret) {
        //do something
    }
    return NULL;
}

Coroutine::Coroutine() {
    routine_ = NULL;
    routine_idx_ = 0;
}

Coroutine::~Coroutine() {
}

void Coroutine::SetRoutineIdx(int routine_idx) {
    routine_idx_ = routine_idx;
}

int Coroutine::start() {
    co_create(&routine_, NULL, RoutineRun, this);
    resume();
    assert(routine_ != NULL);
    return 0;
}

void Coroutine::resume() {
    co_resume(routine_);
}

void Coroutine::yield() {
    co_yield_ct();
}

}
