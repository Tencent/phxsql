/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include <string>
#include <pthread.h>
#include <vector>
#include <set>

#include "eventdata.pb.h"

namespace phxbinlog {

class FileCmpOpt {
 public:
    bool operator()(const std::string &file1, const std::string &file2);
};

class Storage;
class Option;
class StorageFileManager {
 public:
    StorageFileManager(const Option *option, const std::string &prefix, bool read_file_list);
    StorageFileManager(const Option *option, const std::string &datadir, const std::string &prefix,
                       bool read_file_list);
    ~StorageFileManager();

    const std::string &GetDataDir() const;
    const std::string &GetPrefix() const;

    //oper
    int ReadData(::google::protobuf::Message *data, EventDataInfo *data_info = NULL, bool change_file = true);
    int WriteData(const ::google::protobuf::Message &data, EventDataInfo *data_info = NULL);

    //write
    int SwitchNewWriteFile(const uint64_t &instanceid);
    int ResetFile(const ::google::protobuf::Message &data, const std::string &filename);
    int DelCheckPointFile(const std::string &maxfilename, const uint32_t &mintime, std::vector<std::string> *delfiles);
    int SetWritePos(const EventDataInfo &info, bool trunc = false);

    //read
    int SetReadPos(const EventDataInfo &info);
    int OpenOldestFile();
    int OpenNewestFile();
    int GetOldestInstanceIDofFile();
    int GetData(const EventDataInfo &info, ::google::protobuf::Message *data);

    int GetNextReadFile();
    int GetPreReadFile();

    //get info
    void GetWriteFileInfo(EventDataInfo *data_info);
    void GetReadFileInfo(EventDataInfo *data_info);

    const std::set<std::string, FileCmpOpt> &GetFileList() const;

    static int FileCmp(const std::string &file1, const std::string &file2);

    static int GetAllFiles(const std::string &datadir, const std::string &prefix,
                           std::set<std::string, FileCmpOpt> *m_filelist);
    static uint64_t GetFileInstanceID(const std::string &filename);

    std::string GenFileName(const uint64_t &instanceid);
    void RemoveFile(const std::string &file_name);
    void Flush();
 private:
    int Init();
    int Init(const Option *option, const std::string &datadir, const std::string &prefix, bool read_file_list);

    std::string GetNewFile(const uint64_t &instanceid);
    std::string GetNewFile(const std::string &filename);

    int OpenNewWriteFile(const uint64_t &instanceid);
    int OpenFileForWrite(const uint64_t &instanceid, const std::string &filename);

 private:
    std::string data_dir_;
    std::string m_datapath;
    std::set<std::string, FileCmpOpt> file_list_;

    const Option *option_;
    Storage *read_storage_, *write_storage_;
    std::string prefix_;
};

}

