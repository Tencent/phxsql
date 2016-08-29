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
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/define/errordef.h"

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

using phxsql::ColorLogError;

namespace phxbinlog {

ReplicationManager::ReplicationManager(const Option *option) {
    pthread_mutex_init(&m_mutex, NULL);
    option_ = option;
}

ReplicationManager::~ReplicationManager() {
    pthread_mutex_destroy(&m_mutex);
}

const Option *ReplicationManager::GetOption() const {
    return option_;
}

void ReplicationManager::AddSlaveFD(int slave_fd) {
    pthread_mutex_lock(&m_mutex);
    fd_queue_.push(slave_fd);
    pthread_mutex_unlock(&m_mutex);
}

void ReplicationManager::AddImpl(ReplicationImpl *impl) {
    pthread_mutex_lock(&m_mutex);
    impllist_.push_back(impl);
    pthread_mutex_unlock(&m_mutex);
}

void ReplicationManager::ClearImpl() {
    pthread_mutex_lock(&m_mutex);
    for (size_t i = 0; i < impllist_.size(); ++i)
        impllist_[i]->Close();
    impllist_.clear();
    pthread_mutex_unlock(&m_mutex);
}

int ReplicationManager::GetSlaveFD() {
    int fd = -1;
    pthread_mutex_lock(&m_mutex);
    if (!fd_queue_.empty()) {
        fd = fd_queue_.front();
        fd_queue_.pop();
    }
    pthread_mutex_unlock(&m_mutex);
    return fd;
}

void ReplicationManager::DealWithSlave(int slave_id) {
    ClearImpl();
    AddSlaveFD(slave_id);

    pthread_t id;
    pthread_create(&id, NULL, &SlaveManager, this);
}

void *ReplicationManager::SlaveManager(void *arg) {
    ReplicationManager *manager = (ReplicationManager*) arg;

    int slave_id = manager->GetSlaveFD();
    if (slave_id == -1)
        return NULL;

    ReplicationImpl impl(manager->GetOption());

    manager->AddImpl(&impl);

    int ret = impl.Process(slave_id);
    if (ret) {
        ColorLogError("deal with salve fail\n");
    }

    manager->ClearImpl();

    return NULL;
}

}

