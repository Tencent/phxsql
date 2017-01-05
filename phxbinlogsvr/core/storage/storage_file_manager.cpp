/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/


#include "storage_file_manager.h"

#include "storage.h"

#include "phxcomm/lock_manager.h"
#include "phxcomm/phx_utils.h"
#include "phxcomm/phx_log.h"
#include "phxbinlogsvr/statistics/phxbinlog_stat.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/define/errordef.h"

#include <algorithm>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

using std::string;
using std::set;
using std::vector;
using std::pair;
using phxsql::Utils;
using phxsql::LogVerbose;
using phxsql::ColorLogError;
using phxsql::ColorLogInfo;

namespace phxbinlog {

bool FileCmpOpt::operator()(const string &file1, const string &file2) {
    return StorageFileManager::FileCmp(file1, file2) < 0;
}

StorageFileManager::StorageFileManager(const Option *option, const string &prefix, bool read_file_list) {
    Init(option, option->GetBinLogSvrConfig()->GetEventDataBaseDir(), prefix, read_file_list);
}

StorageFileManager::StorageFileManager(const Option *option, const string &datadir, const string &prefix,
                                       bool read_file_list) {
    Init(option, datadir, prefix, read_file_list);
}

int StorageFileManager::Init(const Option *option, const string &data_dir, const string &prefix, bool read_file_list) {
    data_dir_ = data_dir;
    if (data_dir_[data_dir_.size() - 1] != '/')
        data_dir_ += "/";

    write_storage_ = new Storage(Storage::MODE::WRITE, data_dir_, option);
    read_storage_ = new Storage(Storage::MODE::READ, data_dir_, option);
    option_ = option;
    prefix_ = prefix;

    if (read_file_list)
        assert(Init() == 0);
    return OK;
}

StorageFileManager::~StorageFileManager() {
    delete write_storage_;
    delete read_storage_;
}

const string &StorageFileManager::GetDataDir() const {
    return data_dir_;
}

const string &StorageFileManager::GetPrefix() const {
    return prefix_;
}

const set<string, FileCmpOpt> &StorageFileManager::GetFileList() const {
    return file_list_;
}

int StorageFileManager::Init() {
    int ret = GetAllFiles(data_dir_, prefix_, &file_list_);
    if (ret) {
        return ret;
    }
    return OK;

    if (file_list_.empty()) {
        return OK;
    }

    ret = write_storage_->ReOpen(*--file_list_.end());
    if (ret) {
        return ret;
    }
    return read_storage_->ReOpen(*file_list_.begin());
}

int StorageFileManager::GetAllFiles(const string &datadir, const string &prefix, set<string, FileCmpOpt> *filelist) {
    Utils::CheckDir(datadir);

    DIR *dp = opendir(datadir.c_str());
    if (dp == NULL) {
        ColorLogError("%s open dir %s err strerror(%s)", __func__, datadir.c_str(), strerror(errno));
        return -1;
    }

    struct dirent *dirp;
    while ((dirp = readdir(dp)) != NULL) {
        if (strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0)
            continue;
        struct stat sbuf;
        memset(&sbuf, 0, sizeof(sbuf));
        stat(dirp->d_name, &sbuf);
        if (S_ISDIR(sbuf.st_mode))
            continue;
        if (strncmp(dirp->d_name, prefix.c_str(), prefix.size()) == 0) {
            filelist->insert(dirp->d_name);
        }
    }
    closedir(dp);
    ColorLogInfo("%s get file list %zu", __func__, filelist->size());
    if (filelist->size()) {
        ColorLogInfo("%s get file list %zu, min %s max %s", __func__, filelist->size(), filelist->begin()->c_str(),
                     (--(filelist->end()))->c_str());
    }
    return OK;
}

int StorageFileManager::GetNextReadFile() {
    auto it = file_list_.find(read_storage_->GetFileName());
    if (it == file_list_.end())
        return NO_FILES;

    ++it;
    if (it == file_list_.end())
        return NO_FILES;

    return read_storage_->ReOpen(*it);
}

int StorageFileManager::GetPreReadFile() {
    auto it = file_list_.find(read_storage_->GetFileName());
    if (it == file_list_.end())
        return NO_FILES;

    if (it == file_list_.begin())
        return NO_FILES;
    --it;

    return read_storage_->ReOpen(*it);
}

int StorageFileManager::OpenNewWriteFile(const uint64_t &instanceid) {
    return write_storage_->SetFileWritePos(GetNewFile(instanceid), 0);
}

int StorageFileManager::OpenOldestFile() {
    if (file_list_.empty())
        return NO_FILES;
    else
        return read_storage_->ReOpen(*file_list_.begin());
}
int StorageFileManager::GetOldestInstanceIDofFile() {
    if (file_list_.empty())
        return 0;
    else {
        string file_name = *file_list_.begin();
        return GetFileInstanceID(file_name);
    }
}

int StorageFileManager::OpenNewestFile() {
    LogVerbose("%s file list size %zu", __func__, file_list_.size());
    if (file_list_.empty())
        return NO_FILES;
    else
        return read_storage_->ReOpen(*--file_list_.end());
}

int StorageFileManager::OpenFileForWrite(const uint64_t &instanceid, const string &file_name) {
    LogVerbose("%s files %s, instanceid %u", __func__, file_name.c_str(), instanceid);
    if (file_name == "") {
        ColorLogInfo("%s no files create one, instanceid %u", __func__, instanceid);
        return write_storage_->ReOpen(GetNewFile(instanceid));
    } else {
        return write_storage_->ReOpen(GetNewFile(file_name));
    }
}

int StorageFileManager::ResetFile(const ::google::protobuf::Message &data, const string &file_name) {
    LogVerbose("%s check file %s", __func__, file_name.c_str());
    set<const string *> dellist;
    uint64_t old_instanceid = GetFileInstanceID(file_name);
    for (auto &it : file_list_) {
        LogVerbose("%s check del file %s", __func__, it.c_str());
        if (file_name.empty() || old_instanceid <= GetFileInstanceID(it)) {
            LogVerbose("%s del file %s", __func__, it.c_str());
            Utils::ReMoveFile(data_dir_ + it);
            dellist.insert(&it);
        }
    }

    for (auto &it : dellist) {
        file_list_.erase(file_list_.find(*it));
    }

    LogVerbose("%s open file %s", __func__, file_name.c_str());
    int ret = OpenFileForWrite(old_instanceid, file_name);
    if (ret == 0) {
        ret = WriteData(data);
    }
    return ret;
}

int StorageFileManager::SwitchNewWriteFile(const uint64_t &instanceid) {
    return OpenNewWriteFile(instanceid);
}

int StorageFileManager::DelCheckPointFile(const string &maxfile_name, const uint32_t &mintime,
                                          vector<string> *delfiles) {
    LogVerbose("%s check file %s, file list size %zu", __func__, maxfile_name.c_str(), file_list_.size());
    uint64_t old_instanceid = GetFileInstanceID(maxfile_name);
    for (auto &it : file_list_) {
        if (delfiles->size() + 3 > file_list_.size()) {
            LogVerbose("%s left files less than two, skip( delfile count %zu, file list count %zu)", __func__,
                       delfiles->size(), file_list_.size());
            break;
        }

        //	phxsql::LogVerbose("%s check del file %s",__func__, it.c_str());
        if (old_instanceid > GetFileInstanceID(it)) {
            uint32_t filetime = Utils::GetFileTime(data_dir_ + it);
            //		phxsql::LogVerbose("%s check del file %s file time %u mintime %u",__func__, it.c_str(), filetime, mintime);
            if (filetime <= mintime) {
                ColorLogInfo("%s get del file %s, file time %u, min time %u", __func__, it.c_str(), filetime, mintime);
                delfiles->push_back(it);
            }
        }

        if (delfiles->size() >= option_->GetBinLogSvrConfig()->GetMaxDeleteCheckPointFileNum()) {
            LogVerbose("%s del files size %zu has reached the limit size %d, done)", __func__, delfiles->size(),
                       option_->GetBinLogSvrConfig()->GetMaxDeleteCheckPointFileNum());
            break;
        }
    }

    LogVerbose("%s file size %zu", __func__, file_list_.size());
    for (auto &it : *delfiles) {
        file_list_.erase(file_list_.find(it));
    }
    return OK;
}

int StorageFileManager::WriteData(const ::google::protobuf::Message &data, EventDataInfo *data_info) {
    if (data_info) {
        GetWriteFileInfo(data_info);
    }

    string buffer;
    if (!data.SerializeToString(&buffer)) {
        ColorLogError("buffer serialize fail");
        return false;
    }
    return write_storage_->WriteData(buffer);
}

int StorageFileManager::SetWritePos(const EventDataInfo &info, bool trunc) {
    return write_storage_->SetFileWritePos(info.file_name(), info.offset(), trunc);
}

int StorageFileManager::SetReadPos(const EventDataInfo &info) {
    return read_storage_->SetFileReadPos(info.file_name(), info.offset());
}

int StorageFileManager::GetData(const EventDataInfo &info, ::google::protobuf::Message *data) {
    Storage read_storage(Storage::MODE::READ, data_dir_, option_);
    int ret = read_storage.SetFileReadPos(info.file_name(), info.offset());
    if (ret)
        return ret;

    string buffer;
    ret = read_storage.ReadData(&buffer);
    if (ret) {
        return ret;
    }

    if (!data->ParseFromString(buffer)) {
        STATISTICS(GtidEventFileDataParseFail());
        ColorLogError("%s from buffer fail data size %zu", __func__, buffer.size());
        return DATA_ERR;
    }
    return OK;
}

int StorageFileManager::ReadData(::google::protobuf::Message *data, EventDataInfo *data_info, bool change_file) {
    string buffer;
    while (true) {
        if (data_info) {
            GetReadFileInfo(data_info);
        }

        int ret = read_storage_->ReadData(&buffer);
        if (ret == DATA_EMPTY) {
            if (change_file) {
                ret = GetNextReadFile();
                if (ret == NO_FILES) {
                    return DATA_EMPTY;
                } else if (ret == OK) {
                    continue;
                }
            } else {
                return DATA_EMPTY;
            }
        }
        if (ret) {
            return ret;
        }
        break;
    }

    if (!data->ParseFromString(buffer)) {
        STATISTICS(GtidEventFileDataParseFail());
        ColorLogError("%s from buffer fail data size %zu", __func__, buffer.size());
        return DATA_ERR;
    }
    return OK;
}

void StorageFileManager::GetReadFileInfo(EventDataInfo *data_info) {
    pair < string, size_t > file_info = read_storage_->GetFileInfo();

    data_info->set_file_name(file_info.first);
    data_info->set_offset(file_info.second);
}

void StorageFileManager::GetWriteFileInfo(EventDataInfo *data_info) {
    pair < string, size_t > file_info = write_storage_->GetFileInfo();

    data_info->set_file_name(file_info.first);
    data_info->set_offset(file_info.second);
}

string StorageFileManager::GenFileName(const uint64_t &instanceid) {
    char tmp[128];
    sprintf(tmp, "%s-%lu", prefix_.c_str(), instanceid);
    return tmp;
}

string StorageFileManager::GetNewFile(const uint64_t &instanceid) {
    char tmp[128];
    sprintf(tmp, "%s-%lu", prefix_.c_str(), instanceid);
    file_list_.insert(tmp);
    return tmp;
}

string StorageFileManager::GetNewFile(const string &file_name) {
    file_list_.insert(file_name);
    return file_name;
}

uint64_t StorageFileManager::GetFileInstanceID(const string &file_name) {
    uint64_t instanceid = 0;
    if (file_name.empty())
        return instanceid;

    sscanf(file_name.c_str(), "%*[^-]-%lu", &instanceid);
    return instanceid;
}

void StorageFileManager::RemoveFile(const std::string &file_name) {
    auto it = file_list_.find(file_name);
    if (it != file_list_.end()) {
        file_list_.erase(it);
    }
}

int StorageFileManager::FileCmp(const string &file1, const string &file2) {
    if (file1.empty())
        return -1;

    if (file2.empty())
        return 1;

    return GetFileInstanceID(file1) < GetFileInstanceID(file2) ? -1 : 1;
}

void StorageFileManager::Flush() {
    write_storage_->FSync();
}

}