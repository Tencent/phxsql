/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include <vector>
#include <map>
#include <string>
#include <string.h>
#include <stddef.h>
#include <limits.h>

#include "freqctrlconfig.h"

#include "phxcomm/phx_log.h"

using phxsql::LogWarning;

namespace phxsqlproxy {

FreqCtrlBucket::FreqCtrlBucket() {
    memset(&db_token_, 0, sizeof(db_token_));
    memset(&db_bucket_struct_, 0, sizeof(db_bucket_struct_));
}

FreqCtrlBucket::~FreqCtrlBucket() {
}

bool FreqCtrlBucket::ApplyToken(int * for_apply, int count) {
    uint32_t last_refill_time = (unsigned int) time(NULL);
    last_refill_time = last_refill_time - last_refill_time % (db_token_.period);

    if (db_bucket_struct_.last_refill_time != last_refill_time) {
        db_bucket_struct_.last_refill_time = last_refill_time;
        db_bucket_struct_.read_bucket = db_token_.read_token;
        db_bucket_struct_.write_bucket = db_token_.write_token;
    }

    bool is_apply_ok = false;
    if (*for_apply >= count) {
        *for_apply -= count;
        is_apply_ok = true;
    }
    return is_apply_ok;
}

bool FreqCtrlBucket::ApplyReadToken(int count) {
    return ApplyToken(&db_bucket_struct_.read_bucket, count);
}

bool FreqCtrlBucket::ApplyWriteToken(int count) {
    return ApplyToken(&db_bucket_struct_.write_bucket, count);
}

void FreqCtrlBucket::Refill() {
    uint32_t last_refill_time = (unsigned int) time(NULL);
    last_refill_time = last_refill_time - last_refill_time % (db_token_.period);

    db_bucket_struct_.last_refill_time = last_refill_time;
    db_bucket_struct_.read_bucket = db_token_.read_token;
    db_bucket_struct_.write_bucket = db_token_.write_token;
}

void FreqCtrlBucket::Sedb_token(const DBToken_t & db_token) {
    memcpy(&db_token_, &db_token, sizeof(db_token));
    Refill();
}

bool FreqCtrlBucket::HasToken(int * for_check) {
    uint32_t last_refill_time = (unsigned int) time(NULL);
    last_refill_time = last_refill_time - last_refill_time % (db_token_.period);

    if (db_bucket_struct_.last_refill_time != last_refill_time) {
        db_bucket_struct_.last_refill_time = last_refill_time;
        db_bucket_struct_.read_bucket = db_token_.read_token;
        db_bucket_struct_.write_bucket = db_token_.write_token;
    }

    return *for_check > 0;
}

bool FreqCtrlBucket::HasReadToken() {
    return HasToken(&db_bucket_struct_.read_bucket);
}

bool FreqCtrlBucket::HasWriteToken() {
    return HasToken(&db_bucket_struct_.write_bucket);
}

//////////////FreqCtrlBucket end

FreqCtrlConfig::FreqCtrlConfig() {
}

FreqCtrlConfig::~FreqCtrlConfig() {
    for (auto & itr : db_bucket_) {
        delete itr.second, itr.second = NULL;
    }
}

FreqCtrlConfig * FreqCtrlConfig::GetDefault() {
    static FreqCtrlConfig config;
    return &config;
}

void FreqCtrlConfig::ReadConfig() {
    for (auto & itr : db_bucket_) {
        delete itr.second, itr.second = NULL;
    }
    db_bucket_.clear();

    int db_count = 0;
    db_count = GetInteger("General", "DBCount", 0);
    for (int i = 0; i < db_count; ++i) {
        char section[64] = { 0 };
        snprintf(section, sizeof(section), "DB%d", i);

        std::string db_name = "";
        FreqCtrlBucket::DBToken_t token;

        db_name = Get(section, "DBName", "");
        token.read_token = GetInteger(section, "ReadToken", INT_MAX);
        token.write_token = GetInteger(section, "WriteToken", INT_MAX);
        token.period = GetInteger(section, "Period", 60);  //in second

        if (db_name == "")
            continue;

        FreqCtrlBucket * bucket = new FreqCtrlBucket();
        bucket->Sedb_token(token);

        db_bucket_[db_name] = bucket;

        LogWarning("%s:%d read %dth db [%s] readtoken [%d] writetoken [%d] period [%d]", __func__, __LINE__, i,
                   db_name.c_str(), token.read_token, token.write_token, token.period);
    }
    return;
}

bool FreqCtrlConfig::Apply(const std::string & db_name, bool is_write_query) {
    auto itr = db_bucket_.find(db_name);
    if (itr == db_bucket_.end()) {
        return true;
    }
    bool ret = true;
    if (is_write_query) {
        ret = itr->second->ApplyWriteToken(1);
    } else {
        ret = itr->second->ApplyReadToken(1);
    }
    return ret;
}

}

