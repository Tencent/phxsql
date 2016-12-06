/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include "phxsqlproxyconfig.h"
#include "group_status_cache.h"

namespace phxsqlproxy {

class IOChannel {
 public:
    IOChannel(PHXSqlProxyConfig * config,
              WorkerConfig_t    * worker_config,
              GroupStatusCache  * group_status_cache);

    virtual ~IOChannel();

    void SetReqUniqID(uint64_t req_uniq_id);

    void Clear();

 public:
    int TransMsg(int client_fd, int sqlsvr_fd);

 private:
    int WriteToDest(int dest_fd, const char * buf, int write_size);

    int TransMsgDirect(int source_fd, int dest_fd, struct pollfd[], int nfds);

    int FakeClientIPInAuthBuf(char * buf, size_t buf_len); // forward compatibility

    void GetDBNameFromAuthBuf(const char * buf, int buf_size);

    void GetDBNameFromReqBuf(const char * buf, int buf_size);

    virtual bool CanExecute(const char * buf, int size) = 0;

 // monitor func
 private:
    void ByteFromConnectDestSvr(uint32_t byte_size);

    void ByteFromMysqlClient(uint32_t byte_size);

 protected:
    uint64_t req_uniq_id_;

    PHXSqlProxyConfig * config_;
    WorkerConfig_t    * worker_config_;
    GroupStatusCache  * group_status_cache_;

    int client_fd_;
    int sqlsvr_fd_;

    std::string connect_dest_;
    int         connect_port_;

    bool        is_authed_;
    std::string db_name_;

    int      last_read_fd_;
    uint64_t last_received_request_timestamp_;

    std::string client_ip_; // for FakeClientIPInAuthBuf, forward compatibility
};

class MasterIOChannel : public IOChannel {
 public:
    MasterIOChannel(PHXSqlProxyConfig * config,
                    WorkerConfig_t    * worker_config,
                    GroupStatusCache  * group_status_cache);

    ~MasterIOChannel() override;

 private:
    bool CanExecute(const char * buf, int size) override;
};

class SlaveIOChannel : public IOChannel {
 public:
    SlaveIOChannel(PHXSqlProxyConfig * config,
                   WorkerConfig_t    * worker_config,
                   GroupStatusCache  * group_status_cache);

    ~SlaveIOChannel() override;

 private:
    bool CanExecute(const char * buf, int size) override;
};

}
