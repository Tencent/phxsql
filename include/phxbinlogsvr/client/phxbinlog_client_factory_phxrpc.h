/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include "phxbinlog_client_factory_interface.h"

class PhxBinlogClientFactory_PhxRPC : public phxbinlogsvr::PhxBinLogClientFactoryInterface {
 public:
    virtual std::shared_ptr<phxbinlogsvr::PhxBinlogClient> CreateClient();
    std::shared_ptr<phxbinlogsvr::PhxBinlogClient> CreateClient(const std::string &ip, const int &port);
    std::shared_ptr<phxbinlogsvr::PhxBinlogClient> CreateClient(const std::string &ip, const int &port,
                                                                const uint32_t &time_ms);
};
