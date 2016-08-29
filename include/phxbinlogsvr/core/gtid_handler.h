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
#include <vector>
#include "phxbinlogsvr/define/errordef.h"

namespace phxbinlog {

struct EventInfo {
    uint8_t event_type;
    std::string gtid;
    uint64_t seq_no;
    uint32_t domain_id;
    uint32_t server_id;
    uint64_t commit_id;
    uint32_t time_stamp;
    uint32_t event_size;
    std::vector<std::pair<std::string, std::string> > previous_gtidlist;
    EventInfo() :
            event_type(0),
            gtid(""),
            seq_no(0),
            domain_id(0),
            server_id(0),
            commit_id(0),
            time_stamp(0),
            event_size(0) {
        seq_no = 0;
        commit_id = 0;
    }
};

class GtidHandler {
 public:
    static int ParseEvent(const unsigned char *buffer, const size_t &len, EventInfo *info);
    static int ParseEventList(const std::string &buffer, std::vector<std::string> *event_list,
                              bool need_charset = false, std::string *max_gtid = NULL,
                              std::vector<std::string> *gtid_list = NULL);

    static int ParsePreviousGTIDEvent(const unsigned char *buffer, const size_t &len, EventInfo *info);
    static int ParsePreviousGTIDCommand(const unsigned char *buffer, const size_t &len, EventInfo *info);

    static size_t GetEventHeaderSize();

    static int GTIDCmp(const std::string &gtid1, const std::string &gtid2);

    static std::string GenGTID(const std::string &uuid, const uint64_t &seqno);
    static std::string GetUUIDByGTID(const std::string &gtid);

    static std::pair<std::string, size_t> ParseGTID(const std::string &gtid);
 private:
    static const unsigned char *GetMariaDBGTID(const unsigned char *pos, EventInfo *info);
    static std::string GenGTID(const EventInfo &info);

    static const unsigned char *GetPerconaGTID(const unsigned char *pos, const size_t &size, EventInfo *info);
    static std::string GenUUID(const unsigned char *bytes_arg, const int &size);

    static const unsigned char *GetBuffer(const unsigned char *buffer, void *value, const size_t &value_size);
};

}
