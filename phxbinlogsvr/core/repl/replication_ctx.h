/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/


#pragma once

#include <stdint.h>
#include <string>
#include <pthread.h>

namespace phxbinlog {

enum {
    NONE = 0,
    COMMAND = 3,
    COMMAND_FILED = 4,
};

class Option;
class ReplicationCtx {
 public:
    ReplicationCtx(const Option *option);
    ~ReplicationCtx();
 public:
    uint8_t GetSeq();
    void SetSeq(const uint8_t &seq);

    void Notify();
    void Wait();

    void SetMasterFD(const int &master_fd);
    int GetMasterFD();
    void SetSlaveFD(const int &slave_fd);
    int GetSlaveFD();

    void SelfConn(bool self);
    bool IsSelfConn();
	bool IsClose();

    void CloseMasterFD();
    void CloseSlaveFD();
	void CloseRepl();

    void Close();

    const Option *GetOption() const;

 private:
    uint8_t m_seq;
    bool is_selfconn_, is_inited_, is_close_;
    int master_fd_;
    int slave_fd_;

    const Option *option_;

    pthread_cond_t cond_;
    pthread_mutex_t mutex_, info_mutex_;
};

}

