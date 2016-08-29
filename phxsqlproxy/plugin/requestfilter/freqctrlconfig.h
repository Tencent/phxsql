/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include <map>
#include <string>

#include "phxcomm/base_config.h"

namespace phxsqlproxy {

//not thread-safe
class FreqCtrlBucket {
 public:
    typedef struct tagDBToken {
        int read_token;
        int write_token;
        int period;
    } DBToken_t;

    typedef struct db_bucket {
        int read_bucket;
        int write_bucket;
        uint32_t last_refill_time;
    } DBBucket_t;

 public:
    FreqCtrlBucket();

    virtual ~FreqCtrlBucket();

 public:
    bool ApplyReadToken(int count);

    bool ApplyWriteToken(int count);

    void Refill();

    void Sedb_token(const DBToken_t & db_token);

    bool HasReadToken();

    bool HasWriteToken();

 private:
    bool ApplyToken(int * for_apply, int count);

    bool HasToken(int * for_check);
 private:
    DBToken_t db_token_;

    DBBucket_t db_bucket_struct_;
};

class FreqCtrlConfig : public phxsql::PhxBaseConfig {

 public:
    FreqCtrlConfig();

    virtual ~FreqCtrlConfig();

    static FreqCtrlConfig * GetDefault();

    virtual void ReadConfig();

 public:

    bool Apply(const std::string & db_name, bool is_write_query);

    std::map<std::string, FreqCtrlBucket*> db_bucket_;
};

}
