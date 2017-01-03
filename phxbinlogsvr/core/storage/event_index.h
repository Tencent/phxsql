/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include "leveldb/db.h"
#include "leveldb/comparator.h"
#include "eventdata.pb.h"

namespace phxbinlog {

enum class event_index_status {
    DB_ERROR = -1,
    OK = 0,
    DATA_NOT_FOUND = 1,
    DATA_ERROR = 2,
};

class EventComparator : public leveldb::Comparator {
 public:
    int Compare(const leveldb::Slice &a, const leveldb::Slice &b) const;

    const char *Name() const {
        return "EventComparator";
    }

    void FindShortestSeparator(std::string *, const leveldb::Slice &) const {
    }

    void FindShortSuccessor(std::string *) const {
    }
};

class EventIndex {
 public:
    EventIndex(const std::string &event_path);
    ~EventIndex();

    event_index_status GetGTID(const std::string &gtid, ::google::protobuf::Message *data_info);
    event_index_status GetLowerBoundGTID(const std::string &gtid, ::google::protobuf::Message *data_info);

    event_index_status SetGTIDIndex(const std::string &gtid, const ::google::protobuf::Message &data_info);
    event_index_status DeleteGTIDIndex(const std::string &gtid);
    event_index_status IsExist(const std::string &gtid);

 private:
    void CloseDB();
    void OpenDB(const std::string &event_path);

    event_index_status GetGTIDIndex(const std::string &gtid, ::google::protobuf::Message *data_info,
                                    bool lower_bound);

 private:
    leveldb::DB *level_db_;
    EventComparator comparator_;
};

}

