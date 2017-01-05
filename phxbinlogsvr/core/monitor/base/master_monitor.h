/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include <string>
#include <vector>

namespace phxbinlog {

class PhxMySQL;
class Option;
class MasterMonitor {
 public:
    static int GetGlobalMySQLMaxGTIDList(const Option *option, const std::vector<std::string> &iplist,
                                         std::vector<std::string> *gtid_list);
    static int GetMySQLMaxGTIDList(const Option *option, std::vector<std::string> *gtid_list);

    static int MasterStart(const Option *option);

    static bool IsMySqlInit(const Option *option);
    static bool IsGTIDCompleted(const Option *option, const std::string &max_gtid);
    static bool IsGTIDCompleted(const std::vector<std::string> &excute_gtid_list, const std::string &max_gtid);

    //check running
    static int CheckMasterRunningStatus(const Option *option);
    static int CheckMasterInit(const Option *option);
    static int CheckMySqlUserInfo(const Option *option);
    static int CheckMasterPermisstion(const Option *option, const std::vector<std::string> &member_list);

    static MasterMonitor *GetGlobalMasterMonitor(const Option *option);
};

}
