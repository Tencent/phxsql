/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "replication_base.h"

#include "replication_ctx.h"

namespace phxbinlog {

ReplicationBase::ReplicationBase(ReplicationCtx *ctx) {
    ctx_ = ctx;
}

ReplicationBase::~ReplicationBase() {
    ctx_ = NULL;
}

int ReplicationBase::Run() {
    pthread_create(&thread_id_, NULL, &ThreadProcess, this);
    return 0;
}

int ReplicationBase::WaitStop() {
    pthread_join(thread_id_, NULL);
    return 0;
}

void *ReplicationBase::ThreadProcess(void *arg) {
    ReplicationBase *runner = (ReplicationBase *) arg;

    int ret = runner->Process();
    if (ret)
        runner->Close();

    return NULL;
}

}

