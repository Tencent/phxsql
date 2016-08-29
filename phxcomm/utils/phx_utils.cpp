/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "phxcomm/phx_utils.h"
#include "phxcomm/phx_log.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>

using std::string;

namespace phxsql {

string Utils::GetIP(const uint32_t &svrid) {
    char tmp[128] = { 0 };
    sprintf(tmp, "%u.%u.%u.%u", (svrid >> 24) & 0xff, (svrid >> 16) & 0xff, (svrid >> 8) & 0xff, svrid & 0xff);
    return tmp;
}

uint32_t Utils::GetSvrID(const string &ip) {
    uint32_t svrid = 0;
    uint32_t tmp[4] = { 0 };

    sscanf(ip.c_str(), "%u.%u.%u.%u", &tmp[3], &tmp[2], &tmp[1], &tmp[0]);
    svrid = (tmp[3] << 24) | (tmp[2] << 16) | (tmp[1] << 8) | tmp[0];
    return svrid;
}

uint64_t Utils::GetCheckSum(const uint64_t &old_checksum, const char *value, const size_t &value_size) {
    uint64_t checksum = old_checksum;
    for (size_t i = 0; i < value_size; ++i) {
        checksum = checksum * 257 + value[i];
    }
    return checksum;
}

void Utils::CheckDir(const string &dir) {
    size_t pos = 0;
    while (1) {
        if (pos == dir.size()) {
            break;
        }
        string check_dir;
        size_t next_pos = dir.find('/', pos);
        if (next_pos == string::npos) {
            pos = dir.size();
            check_dir = dir;
        } else {
            pos = next_pos + 1;
            check_dir = dir.substr(0, pos);
        }
        //check dir if exist
        struct stat fileStat;
        if ((stat(check_dir.c_str(), &fileStat) == 0) && S_ISDIR(fileStat.st_mode)) {
            //exist;
        } else {
            //create dir
            int ret = mkdir(check_dir.c_str(), 0777);
            if (ret) {
                ColorLogError("create dir %s( check %s ) fail %s ret %d", dir.c_str(), check_dir.c_str(),
                              strerror(errno), ret);
                assert(ret == 0);
            }
            ColorLogInfo("%s create dir %s done", __func__, check_dir.c_str());
        }
    }
}

uint32_t Utils::GetFileTime(const string &filename) {
    if (filename.empty())
        return 0;

    struct stat statbuf;
    if (stat(filename.c_str(), &statbuf) == -1) {
        return -1;
    }
    return statbuf.st_mtime;
}

int Utils::FileCmpByFileTime(const string &file1, const string &file2) {
    uint32_t time1 = GetFileTime(file1);
    uint32_t time2 = GetFileTime(file2);
    if (time1 == time2)
        return 0;
    return time1 < time2 ? -1 : 1;
}

int Utils::ReMoveFile(const string &filename) {
    return remove(filename.c_str());
}

int Utils::MoveDir(const string &dirname, const string &destdirname) {
    ColorLogInfo("%s move dir %s to %s", __func__, dirname.c_str(), destdirname.c_str());
    return rename(dirname.c_str(), destdirname.c_str());
}

int Utils::RemoveDir(const string &path) {
    int size = path.size();
    if (path[size - 1] == '/')
        size--;
    char back_path[128];
    sprintf(back_path, "%s_back_%u", path.substr(0, size).c_str(), (uint32_t) time(NULL));

    return MoveDir(path, back_path);
}

}

