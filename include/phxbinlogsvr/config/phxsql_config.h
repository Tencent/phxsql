/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include <stdint.h>

#include "phxcomm/base_config.h"

namespace phxbinlog {

class PhxMySqlConfig : public phxsql::PhxBaseConfig {

 public:
    const char *GetMySQLIP() const;
    int GetMySQLPort() const;
    const char *GetMySQLSocket() const;
 protected:
    PhxMySqlConfig(const char *filename = "my.cnf");
    virtual ~PhxMySqlConfig();

    friend class Option;

 private:
    virtual void ReadConfig();

 private:
    std::string mysql_ip_;
    uint32_t mysql_port_;
    std::string mysql_socket_;
};

}
