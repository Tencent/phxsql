/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "phxbinlogsvr/core/gtid_handler.h"
#include "phxcomm/phx_log.h"

#include <stdio.h>
#include <string.h>
#include <sstream>


using std::string;
using std::vector;
using std::pair;
using std::make_pair;
using phxsql::ColorLogError;
using phxsql::ColorLogInfo;

namespace phxbinlog {

const unsigned char *GtidHandler::GetMariaDBGTID(const unsigned char *pos, EventInfo *info) {
    memcpy(&info->seq_no, pos, 8);
    pos += 8;
    memcpy(&info->domain_id, pos, 4);
    pos += 4;

    unsigned char flags2 = *pos;

    if (flags2 & 2) {
        ++pos;
        memcpy(&info->commit_id, pos, 8);
    }
    info->gtid = GenGTID(*info);
    return pos;
}

const unsigned char *GtidHandler::GetPerconaGTID(const unsigned char *pos, const size_t &size, EventInfo *info) {
    if (size > 24) {
        string uuid = GtidHandler::GenUUID(++pos, 16);
        pos += 16;
        memcpy(&info->seq_no, pos, 8);
        pos += 8;
        info->gtid = GtidHandler::GenGTID(uuid, info->seq_no);
    }
    return pos;
}

int GtidHandler::ParseEvent(const unsigned char *buffer, const size_t &buffer_len, EventInfo *info) {
    const unsigned char *pos = buffer;
    if (13 > buffer_len) {
        ColorLogError("%s parse event fail buffer len %zu too small", __func__, buffer_len);
        return EVENT_PARSE_FAIL;
    }
    uint32_t timestamp = *(uint32_t*) (pos);
    uint8_t event_type = *(pos + 4);
    uint32_t server_id = *(uint32_t*) (pos + 5);
    uint32_t event_size = *(uint32_t*) (pos + 9);

    if (event_type >= 200) {
        ColorLogError("%s parse event fail event size %u buffer len %zu event type %d", __func__, event_size,
                               buffer_len, event_type);
        return EVENT_PARSE_FAIL;
    }
    pos += 19;
    if (info) {
        info->time_stamp = timestamp;
        info->event_size = event_size;
        info->event_type = event_type;
        info->server_id = server_id;
        if (event_type == EVENT_GTID) {
            if (buffer_len > 30)
                pos = GetMariaDBGTID(pos, info);
        } else if (event_type == EVENT_PERCONA_GTID) {
            pos = GetPerconaGTID(pos, buffer_len - (pos - buffer), info);
        } else if (event_type == EVENT_PREVIOUS_GTIDS) {
            ParsePreviousGTIDEvent(pos, buffer_len - (pos - buffer), info);
        }
    }
    return 0;
}

size_t GtidHandler::GetEventHeaderSize() {
    //3bit lens and 1 bit seq
    return 1;
}

int GtidHandler::ParseEventList(const string &buffer, vector<string> *event_list, bool need_charset, string *max_gtid,
                                vector<string> *gtid_list) {
    const unsigned char *pos = (unsigned char *) buffer.c_str();
    const unsigned char *end_pos = (unsigned char *) buffer.c_str() + buffer.size();

    while (pos < end_pos) {
        EventInfo info;
        int ret = ParseEvent(pos, (size_t)(end_pos - pos), &info);
        if (ret) {
            return ret;
        }
        if (pos + 13 > end_pos) {
            ColorLogError("event parse fail buffer size %zu invalid", buffer.size());
            return EVENT_PARSE_FAIL;
        }

        if (info.event_size == 0 || pos + info.event_size > (unsigned char*) buffer.c_str() + buffer.size()) {
            ColorLogError("event parse fail event size %u buffer size %zu", info.event_size, buffer.size());
            return EVENT_PARSE_FAIL;
        }

        if (event_list) {
            if (need_charset)
                event_list->push_back(string("\0", 1).append(string((char *) pos, info.event_size)));
            else
                event_list->push_back(string((char *) pos, info.event_size));
        }

        if (gtid_list) {
            gtid_list->push_back(info.gtid);
        }
        pos += info.event_size;
        if (max_gtid && info.gtid.size() > 0)
            *max_gtid = info.gtid;
    }

    if (pos != end_pos) {
        ColorLogError("%s parse buffer fail pos %p end pos %p", __func__, pos, end_pos);

        return EVENT_PARSE_FAIL;
    }
    return OK;
}

int GtidHandler::ParsePreviousGTIDCommand(const unsigned char *buffer, const size_t &len, EventInfo *info) {
    const unsigned char *pos = buffer;
    uint32_t binlog_flag = 0;
    pos = GetBuffer(pos, &binlog_flag, 2);
    uint32_t server_id = 0;
    pos = GetBuffer(pos, &server_id, 4);
    uint32_t name_info_size = 0;
    pos = GetBuffer(pos, &name_info_size, 4);
    pos += name_info_size;
    uint64_t binlog_info_size = 0;
    pos = GetBuffer(pos, &binlog_info_size, 8);

    uint32_t data_size = 0;
    pos = GetBuffer(pos, &data_size, 4);

    if (pos + data_size != buffer + len) {
        ColorLogError("%s parse fail %d buffer size %zu data size %zu", __func__, pos - buffer, data_size, len);
        return BUFFER_FAIL;
    }

    return ParsePreviousGTIDEvent(pos, data_size, info);
}

int GtidHandler::ParsePreviousGTIDEvent(const unsigned char *buffer, const size_t &len, EventInfo *info) {
    const unsigned char *pos = buffer;
    uint64_t sid_num = 0;
    pos = GetBuffer(pos, &sid_num, 8);

    for (size_t i = 0; i < sid_num; ++i) {
        uint64_t sid_no_num = 0;
        if (len - (pos - buffer) < 16 + 8) {
            return EVENT_PARSE_FAIL;
        }
        string uuid = GenUUID(pos, 16);
        pos += 16;

        pos = GetBuffer(pos, &sid_no_num, 8);

        for (size_t i = 0; i < sid_no_num; ++i) {
            uint64_t start = 0;
            pos = GetBuffer(pos, &start, 8);
            uint64_t end = 0;
            pos = GetBuffer(pos, &end, 8);
            --end;
            if (info) {
                char buf[128], last_buf[128];
                sprintf(buf, "%s:%lu-%lu", uuid.c_str(), start, end);
                sprintf(last_buf, "%s:%lu", uuid.c_str(), end);
                info->previous_gtidlist.push_back(make_pair(buf, last_buf));
            }
            ColorLogInfo("%s get gtid %s:%lu-%lu", __func__, uuid.c_str(), start, end);
        }
    }
    if ((size_t)(pos - buffer) != len) {
        return EVENT_PARSE_FAIL;
    }
    return OK;
}

const unsigned char *GtidHandler::GetBuffer(const unsigned char *buffer, void *value, const size_t &value_size) {
    memcpy(value, buffer, value_size);
    return buffer + value_size;
}

string GtidHandler::GenGTID(const EventInfo &info) {
    char gtid_buffer[128];
    sprintf(gtid_buffer, "%u-%u-%lu", info.domain_id, info.server_id, info.seq_no);

    return gtid_buffer;
}

const int NUMBER_OF_SECTIONS = 5;

const int bytes_per_section[NUMBER_OF_SECTIONS] = { 4, 2, 2, 2, 6 };

string GtidHandler::GenUUID(const unsigned char *bytes_arg, const int &size) {
    string gtid;

    static const char byte_to_hex[] = "0123456789abcdef";
    const unsigned char *u = bytes_arg;
    for (int i = 0; i < NUMBER_OF_SECTIONS; i++) {
        if (i > 0) {
            gtid += '-';
        }
        for (int j = 0; j < bytes_per_section[i]; j++) {
            int byte = *u;
            gtid += byte_to_hex[byte >> 4];
            gtid += byte_to_hex[byte & 0xf];
            u++;
        }
    }
    return gtid;
}

int GtidHandler::GTIDCmp(const string &gtid1, const string &gtid2) {
    pair < string, size_t > res1 = ParseGTID(gtid1);
    pair < string, size_t > res2 = ParseGTID(gtid2);
    if (res1 == res2)
        return 0;
    return res1 < res2 ? -1 : 1;
}

pair<string, size_t> GtidHandler::ParseGTID(const string &gtid) {
    int pos = gtid.find(':');
    if (pos == -1) {
        return make_pair(gtid, 0);
    } else {
        size_t seqno = 0;
        sscanf(gtid.c_str() + pos + 1, "%lu", &seqno);

        return make_pair(gtid.substr(0, pos), seqno);
    }
}

string GtidHandler::GetUUIDByGTID(const string &gtid) {
    int pos = gtid.find(':');
    if (pos == -1) {
        return gtid;
    } else {
        return gtid.substr(0, pos);
    }
}

string GtidHandler::GenGTID(const string &uuid, const uint64_t &seqno) {
    char gtidbuffer[128];
    sprintf(gtidbuffer, "%s:%lu", uuid.c_str(), seqno);
    return gtidbuffer;
}

}

