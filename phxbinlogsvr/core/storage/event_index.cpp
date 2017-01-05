/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/


#include "event_index.h"

#include "phxcomm/phx_log.h"
#include "phxbinlogsvr/core/gtid_handler.h"

using std::string;
using std::pair;
using phxsql::LogVerbose;
using phxsql::ColorLogError;
using phxsql::ColorLogWarning;

namespace phxbinlog {

int EventComparator::Compare(const leveldb::Slice &a, const leveldb::Slice &b) const {
    pair < string, size_t > key1 = GtidHandler::ParseGTID(a.ToString());
    pair < string, size_t > key2 = GtidHandler::ParseGTID(b.ToString());

    if (key1.first == key2.first) {
        if (key1.second == key2.second)
            return 0;
        return key1.second < key2.second ? -1 : 1;
    }

    return key1.first < key2.first ? -1 : 1;

}

EventIndex::EventIndex(const string &event_path) {
    OpenDB(event_path);
}

EventIndex::~EventIndex() {
    CloseDB();
}

void EventIndex::OpenDB(const string &event_path) {
    leveldb::Options options;
    options.create_if_missing = true;
    options.comparator = &comparator_;
    leveldb::Status status = leveldb::DB::Open(options, event_path, &level_db_);
    if (!status.ok()) {
        ColorLogError("%s path %s open db fail, err %s", __func__, event_path.c_str(), status.ToString().c_str());
    }
    assert(status.ok());
}

void EventIndex::CloseDB() {
    if (level_db_)
        delete level_db_;
}

event_index_status EventIndex::SetGTIDIndex(const string &gtid, const ::google::protobuf::Message &data_info) {
    string buffer;
    if (!data_info.SerializeToString(&buffer)) {
        ColorLogError("buffer serialize fail");
        return event_index_status::DATA_ERROR;
    }

    //LogVerbose("%s gtid %s", __func__, gtid.c_str());
    leveldb::Status status = level_db_->Put(leveldb::WriteOptions(), gtid, buffer);
    return status.ok() ? event_index_status::OK : event_index_status::DB_ERROR;
}

event_index_status EventIndex::GetGTID(const string &gtid, ::google::protobuf::Message *data_info) {
    return GetGTIDIndex(gtid, data_info, false );
}

event_index_status EventIndex::GetLowerBoundGTID(const string &gtid, 
		::google::protobuf::Message *data_info) {
    return GetGTIDIndex(gtid, data_info, true);
}

event_index_status EventIndex::GetGTIDIndex(const string &gtid, ::google::protobuf::Message *data_info,
                                            bool lower_bound) {
    string buffer;

	leveldb::ReadOptions rop = leveldb::ReadOptions();
    leveldb::Iterator* it = level_db_->NewIterator(rop);
    if (it == NULL) {
        return event_index_status::DB_ERROR;
    }

    event_index_status ret = event_index_status::DATA_NOT_FOUND;
    it->Seek(gtid);
    if (it->Valid()) {
    //    LogVerbose("%s find key %s, want key %s", 
	//			__func__, it->key().ToString().c_str(), gtid.c_str());

        if (it->key().ToString() == gtid
                || (lower_bound && GtidHandler::GetUUIDByGTID(it->key().ToString()) == GtidHandler::GetUUIDByGTID(gtid))) {

            buffer = it->value().ToString();
            if (buffer.size()) {
                if (!data_info->ParseFromString(buffer)) {
                    ColorLogWarning("%s from buffer fail data size %zu, want gtid %s", __func__, buffer.size(),
                                    gtid.c_str());
                    ret = event_index_status::DATA_ERROR;
                } else {
                    LogVerbose("%s gtid %s ok, get gtid %s, want gtid %s", __func__, gtid.c_str(),
                               it->key().ToString().c_str(), gtid.c_str());
                    ret = event_index_status::OK;
                }
            } else {
                ColorLogWarning("%s get gtid %s, not exist", __func__, gtid.c_str());
            }
        } else {
            ColorLogWarning("%s get gtid %s, not exist", __func__, gtid.c_str());
        }
    }
    delete it;
    return ret;
}

event_index_status EventIndex::IsExist(const string &gtid) {
    string buffer;
    leveldb::Status status = level_db_->Get(leveldb::ReadOptions(), gtid, &buffer);
    if (status.ok()) {
        return event_index_status::OK;
    }
    return event_index_status::DATA_NOT_FOUND;
}

event_index_status EventIndex::DeleteGTIDIndex(const string &gtid) {
    leveldb::Status status = level_db_->Delete(leveldb::WriteOptions(), gtid);

    if (status.ok() || status.IsNotFound())
        return event_index_status::OK;
    return event_index_status::DB_ERROR;
}

}