/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include "storage_file_manager.h"

namespace phxbinlog {

class EventFileManager : public StorageFileManager {
 public:
    explicit EventFileManager(const Option *option, bool read_file_list = true);
    explicit EventFileManager(const Option *option, const std::string &datadir);
    virtual ~EventFileManager();

 public:
    int ReadData(EventData *data, EventDataInfo *data_info = NULL);
    int ReadDataFromFile(EventData *data);
    int WriteData(const EventData &eventdata, EventDataInfo *data_info, bool write_file = true);

    bool IsInitFile(const EventDataInfo &data_info);

 private:
    void CopyDataToDataInfo(const EventData &eventdata, EventDataInfo *info);
};

}

