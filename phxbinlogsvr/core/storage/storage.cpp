/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "storage.h"

#include "phxcomm/phx_log.h"
#include "phxbinlogsvr/statistics/phxbinlog_stat.h"
#include "phxbinlogsvr/define/errordef.h"

#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

using std::string;
using std::pair;
using phxsql::ColorLogError;
using phxsql::ColorLogWarning;
using phxsql::ColorLogInfo;
using phxsql::LogVerbose;


namespace phxbinlog {

Storage::Storage(const MODE &mode, const string &datadir, const Option *option) {
    data_dir_ = datadir;
    mode_ = mode;
    fd_ = -1;
    path_ = "";
}

Storage::~Storage() {
    CloseDB();
}

int Storage::ReOpen(const string &path) {
    CloseDB();
    return OpenDB(path);
}

int Storage::OpenDB(const string &path) {
    string file_path = data_dir_ + path;
    if (mode_ == MODE::WRITE) {
        fd_ = open(file_path.c_str(), O_WRONLY | O_CREAT, 0666);
    } else if (mode_ == MODE::READ) {
        fd_ = open(file_path.c_str(), O_RDONLY | O_CREAT, 0666);
    }

    if (fd_ < 0) {
        ColorLogWarning("%s open db %s fail %s mode %d", __func__, file_path.c_str(), strerror(errno), mode_);
        assert(fd_ >= 0);
        return FILE_FAIL;
    }

    struct stat sbuf;
    memset(&sbuf, 0, sizeof(sbuf));
    stat(file_path.c_str(), &sbuf);

    if (mode_ == MODE::WRITE) {
        lseek(fd_, sbuf.st_size, SEEK_SET);
    }

    path_ = path;
    ColorLogInfo("%s fd %d open db %s offset %d done mode %d file size %zu", __func__, fd_, file_path.c_str(),
                        GetOffSet(), mode_, sbuf.st_size);
    return OK;
}

int Storage::Reset() {
    return lseek(fd_, 0, SEEK_SET);
}

int Storage::SetFileReadPos(const string &path, const size_t &offset) {
    return SetFilePos(path, offset, MODE::READ);
}

int Storage::SetFileWritePos(const string &path, const size_t &offset, bool trunc) {
    return SetFilePos(path, offset, MODE::WRITE, trunc);
}

int Storage::SetFilePos(const string &path, const size_t &offset, const MODE &mode, bool trunc) {
    if (mode_ != mode) {
        ColorLogWarning("%s not read mode file", __func__);
        return FILE_FORBIDDEN;
    }

    if (path_ != path) {
        int ret = ReOpen(path);
        if (ret)
            return ret;
    }

    off_t ret = lseek(fd_, offset, SEEK_SET);
    if (ret == (off_t) - 1) {
        return FILE_FAIL;
    }
    if (trunc) {
        int ret = ftruncate(fd_, offset);
        if (ret) {
            ColorLogError("%s trunc file fail, path %s(%d) offset %u ret %d error %s", __func__, path.c_str(),
                                 fd_, offset, ret, strerror(errno));
            return FILE_FAIL;
        }
        struct stat sbuf;
        memset(&sbuf, 0, sizeof(sbuf));
        stat((data_dir_ + path).c_str(), &sbuf);
        phxsql::LogVerbose("%s set file pos %s offset %u, size after trunc %u", __func__, (data_dir_ + path).c_str(),
                           GetOffSet(), sbuf.st_size);
    } 
    return OK;
}

int Storage::CloseDB() {
    if (fd_ >= 0) {
        close(fd_);
    }
    fd_ = -1;
    path_ = "";
    return OK;
}

size_t Storage::GetOffSet() {
    return lseek(fd_, 0, SEEK_CUR);
}

string Storage::GetFileName() {
    phxsql::LogVerbose("%s get file name %s", __func__, path_.c_str());
    return path_;
}

pair<string, size_t> Storage::GetFileInfo() {
    return make_pair(path_, GetOffSet());
}

int Storage::WriteData(const string &data) {
    return WriteData((const void *) data.c_str(), data.size());
}

int Storage::WriteData(const void *data, const size_t len) {
    if (mode_ != MODE::WRITE) {
        ColorLogWarning("%s not write mode file", __func__);
        return FILE_FORBIDDEN;
    }

    char *data_buff = new char[(len << 1) + sizeof(len)];
    memset(data_buff, 0, ((len << 1) + sizeof(len)) * sizeof(char));

    memcpy(data_buff, &len, sizeof(len));
    memcpy(data_buff + sizeof(len), data, len);

    size_t write_len = write(fd_, data_buff, len + sizeof(len));
    delete data_buff;
    if (write_len != len + sizeof(len)) {
		STATISTICS(StorageWriteDataFail());
        ColorLogError("%s write data fail, error %s, fd %d write len %zu want len %zu", __func__,
                               strerror(errno), fd_, write_len, len + sizeof(len));
        return FILE_FAIL;
    }
    return OK;
}

int Storage::FlushData() {
    return fsync(fd_);
}

int Storage::ReadData(string *data) {
    if (mode_ != MODE::READ) {
        return FILE_FORBIDDEN;
    }

    size_t pre_offset = GetOffSet();

    int ret = OK;
    size_t len = 0;
    size_t read_len = read(fd_, &len, sizeof(len));
    if (read_len != sizeof(len)) {
        if (read_len == 0) {
            LogVerbose("%s no data", __func__);
            return DATA_EMPTY;
        }
        ColorLogError("%s read len fail, error %s", __func__, strerror(errno));
        ret = FILE_FAIL;
    } else {
        if (len > 10000000000l) {
            ColorLogError("%s data has something wrong, skip, roll back to %zu, len %zu", __func__, pre_offset,
                                   len);
            ret = FILE_FAIL;
        } else {
            data->resize(len);
            read_len = read(fd_, (char *) data->c_str(), len);

            if (read_len != len) {
                ColorLogError("%s read data fail, read len %d want len %d error %s, roll back to %zu",
                                       __func__, read_len, len, strerror(errno), pre_offset);
                ret = FILE_FAIL;
            }
        }
    }
    if (ret == FILE_FAIL) {
        FixOffSet(pre_offset);
        ret = DATA_EMPTY;
    }
    return ret;
}

void Storage::FixOffSet(size_t offset) {
    lseek(fd_, offset, SEEK_SET);
	STATISTICS(StorageReadDataFail());
}

void Storage::FSync() {
    if (fd_ >= 0)
        fsync(fd_);
    LogVerbose("%s sync fd %d", __func__, fd_);
}

}