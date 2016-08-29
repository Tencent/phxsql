/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include <pthread.h>
#include <string>
#include <queue>
#include <stdint.h>

namespace phxbinlog {

class EventAgent;
class ReplicationImpl;
class Option;
class ReplicationManager {
 public:
    ReplicationManager(const Option *option);
    ~ReplicationManager();

    void DealWithSlave(int slave_id);

    const Option *GetOption() const;
 private:
    void AddSlaveFD(int slave_fd);
    int GetSlaveFD();

    void AddImpl(ReplicationImpl *impl);
    void ClearImpl();

    static void *SlaveManager(void *);
 private:
    pthread_mutex_t m_mutex;
    std::queue<int> fd_queue_;
    const Option *option_;
    std::vector<ReplicationImpl *> impllist_;
};

}
