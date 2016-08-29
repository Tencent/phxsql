/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once
#include <string>
#include <stdint.h>
#include <mysql.h>
#include <vector>

namespace phxbinlog {

struct MySqlOption {
    std::string ip;
    uint32_t port;
    std::string username;
    std::string pwd;
};

class MySQLCommand {
 public:
    MySQLCommand(const MySqlOption *option);
    ~MySQLCommand();

    int Query(const std::string &command);
    int GetQueryResults(const std::string &query_str, std::vector<std::vector<std::string> > *results,
                        std::vector<std::string> *fields = NULL);
    int GetValue(const std::string &value_type, const std::string &value_key, std::string *value);

 private:
    int GetResults(std::vector<std::vector<std::string> > *results, std::vector<std::string> *fields);
    int Connect();
    int DoQuery(const std::string &command);

 private:
    MYSQL *mysql_;
    bool is_connect_;
    const MySqlOption *option_;
};

}
