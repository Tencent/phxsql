/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/


#include "replication_ctx.h"

#include "phxcomm/lock_manager.h"
#include "phxcomm/phx_log.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/define/errordef.h"

#include <unistd.h>
#include <sys/socket.h>

using phxsql::LockManager;
using phxsql::LogVerbose;
using phxsql::ColorLogError;

namespace phxbinlog {

ReplicationCtx::ReplicationCtx(const Option *option) {
    pthread_cond_init(&cond_, NULL);
    pthread_mutex_init(&mutex_, NULL);
    pthread_mutex_init(&info_mutex_, NULL);
    is_selfconn_ = false;
    is_inited_ = false;
    option_ = option;
    master_fd_ = -1;
    slave_fd_ = -1;
	is_close_=false;
}

ReplicationCtx::~ReplicationCtx() {
    pthread_cond_destroy(&cond_);
    pthread_mutex_destroy(&mutex_);
    pthread_mutex_destroy(&info_mutex_);
}

void ReplicationCtx::CloseRepl() {
    LockManager lock(&info_mutex_);
	is_close_ = true;
}

void ReplicationCtx::Close() {
    CloseMasterFD();
    CloseSlaveFD();
	CloseRepl();
    Notify();
}

uint8_t ReplicationCtx::GetSeq() {
    LockManager lock(&info_mutex_);
    return m_seq;
}

void ReplicationCtx::SetSeq(const uint8_t &seq) {
    LockManager lock(&info_mutex_);
    m_seq = seq;
}

void ReplicationCtx::Notify() {
    pthread_mutex_lock(&mutex_);
    if (!is_inited_) {
        is_inited_ = true;
        pthread_cond_signal(&cond_);
    }
    pthread_mutex_unlock(&mutex_);
    return;
}

void ReplicationCtx::Wait() {
    pthread_mutex_lock(&mutex_);
    if (!is_inited_) {
        pthread_cond_wait(&cond_, &mutex_);
    }
    pthread_mutex_unlock(&mutex_);
    return;
}

void ReplicationCtx::SetMasterFD(const int &fd) {
	is_close_ = false;
    master_fd_ = fd;
}

int ReplicationCtx::GetMasterFD() {
    return master_fd_;
}

void ReplicationCtx::SetSlaveFD(const int &fd) {
    slave_fd_ = fd;
}

int ReplicationCtx::GetSlaveFD() {
    return slave_fd_;
}

void ReplicationCtx::CloseMasterFD() {
    LockManager lock(&info_mutex_);
    if (master_fd_ >= 0) {
        ColorLogError("master fd %d close", master_fd_);
        shutdown(master_fd_, 2);
        close(master_fd_);
        master_fd_ = -1;
    }
}

void ReplicationCtx::CloseSlaveFD() {
    LockManager lock(&info_mutex_);
    if (slave_fd_ >= 0) {
        ColorLogError("slave fd %d close", slave_fd_);
        shutdown(slave_fd_, 2);
        close(slave_fd_);
        slave_fd_ = -1;
    }
}

const Option *ReplicationCtx::GetOption() const {
    return option_;
}

void ReplicationCtx::SelfConn(bool self) {
    is_selfconn_ = self;
}

bool ReplicationCtx::IsSelfConn() {
    return is_selfconn_;
}

bool ReplicationCtx::IsClose() {
    return is_close_;
}

}
