/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "replication.h"
#include "replication_impl.h"

#include "phxcomm/net.h"
#include "phxcomm/phx_log.h"
#include "phxcomm/lock_manager.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/define/errordef.h"

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

using phxsql::ColorLogError;
using phxsql::LogVerbose;
using phxsql::LockManager;

namespace phxbinlog {

ReplicationManager::ReplicationManager(const Option *option) {
    pthread_mutex_init(&m_mutex, NULL);
    option_ = option;
	thread_fd_ = 0;
	impl_ = NULL;
}

ReplicationManager::~ReplicationManager() {
    pthread_mutex_destroy(&m_mutex);
	if(impl_)
		delete impl_, impl_=NULL;
}

const Option *ReplicationManager::GetOption() const {
    return option_;
}

ReplicationImpl * ReplicationManager::GetImpl() {
	return impl_;
}

void ReplicationManager::CloseImpl() {
	if(impl_){
		impl_->Close();
	}

	if(thread_fd_>0){
		pthread_join(thread_fd_, NULL);
		thread_fd_ = 0;
	}

	if(impl_) {
		delete impl_;
		impl_ = NULL;
	}
}

void ReplicationManager::StartImpl(const int &slave_fd) {
    CloseImpl();
	impl_ = new ReplicationImpl(option_,slave_fd);
}

void ReplicationManager::DealWithSlave(int slave_fd) {

	StartImpl(slave_fd);
    thread_fd_ = 0;
    pthread_create(&thread_fd_, NULL, &SlaveManager, this);

}

void *ReplicationManager::SlaveManager(void *arg) {
    ReplicationManager *manager = (ReplicationManager*) arg;
    ReplicationImpl * impl = manager->GetImpl();
	if(!impl){
		LogVerbose("%s impl has been closed",__func__);
		return NULL;
	}

    int ret = impl->Process();
    if (ret) {
        ColorLogError("deal with salve fail");
    }
	ColorLogError("deal with salve done, impl %p", &impl);

    return NULL;
}

}
