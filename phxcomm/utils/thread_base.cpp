/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "phxcomm/thread_base.h"

namespace phxsql {

ThreadBase::ThreadBase() {
}

ThreadBase::~ThreadBase() {
}

int ThreadBase::Run() {
    return pthread_create(&thread_id_, NULL, &ThreadProcess, this);
}

int ThreadBase::WaitStop() {
    pthread_join(thread_id_, NULL);
    return 0;
}

void *ThreadBase::ThreadProcess(void *arg) {
    ThreadBase *runner = (ThreadBase *) arg;

    runner->Process();
    runner->Close();

    return NULL;
}

void ThreadBase::Close() {
}

}

