/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include <string>

#include "phxsqlproxyconfig.h"
#include "group_status_cache.h"

namespace phxsqlproxy {

class IORouter {
 public:
    IORouter(PHXSqlProxyConfig * config,
             WorkerConfig_t    * worker_config,
             GroupStatusCache  * group_status_cache);

    virtual ~IORouter();

    void SetReqUniqID(uint64_t req_uniq_id);

    void Clear();

 public:
    int ConnectDest();

    virtual void MarkFailure();

 private:
    virtual int GetDestEndpoint(std::string & dest_ip, int & dest_port) = 0;

 protected:
    uint64_t req_uniq_id_;

    PHXSqlProxyConfig * config_;
    WorkerConfig_t    * worker_config_;
    GroupStatusCache  * group_status_cache_;

    std::string connect_dest_;
    int         connect_port_;
};

class MasterIORouter : public IORouter {
 public:
    MasterIORouter(PHXSqlProxyConfig * config,
                   WorkerConfig_t    * worker_config,
                   GroupStatusCache  * group_status_cache);

    ~MasterIORouter() override;

 private:
    int GetDestEndpoint(std::string & dest_ip, int & dest_port) override;
};

class SlaveIORouter : public IORouter {
 public:
    SlaveIORouter(PHXSqlProxyConfig * config,
                  WorkerConfig_t    * worker_config,
                  GroupStatusCache  * group_status_cache);

    ~SlaveIORouter() override;

    void MarkFailure() override;

 private:
    int GetDestEndpoint(std::string & dest_ip, int & dest_port) override;
};

}
