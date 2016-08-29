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

namespace phxsql {

class Utils {
 public:
    static std::string GetIP(const uint32_t &svrid);
    static uint32_t GetSvrID(const std::string &ip);
    static uint64_t GetCheckSum(const uint64_t &old_checksum, const char *value, const size_t &value_size);

    static void CheckDir(const std::string &dir);
    static int FileCmpByFileTime(const std::string &file1, const std::string &file2);
    static int MoveFile(const std::string &filename, const std::string &destfilename);
    static uint32_t GetFileTime(const std::string &filename);
    static int ReMoveFile(const std::string &filename);
    static int RemoveDir(const std::string &path);
    static int MoveDir(const std::string &filename, const std::string &destfilename);
};

}
