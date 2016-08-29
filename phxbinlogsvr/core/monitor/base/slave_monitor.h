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

class Option;
class SlaveMonitor {
 public:
    static int SlaveStart(const Option *option, const std::string &master_ip, const uint32_t &master_port,
                          bool read_only = true);

    static int CheckSlaveRunningStatus(const Option *option, const char *export_ip = NULL, const uint32_t *export_port =
                                               0);

    static int SlaveCheckRelayLog(const Option *option, std::vector<std::string> *gtid_list);

    static void SlaveConnecting(bool connecting);
    static bool IsSlaveConnecting();

    static int GetRelayLog(const Option *option, std::vector<std::string> *gtid_list);

 private:
    static std::string GetChangeMasterQueryString(const Option *option, const std::string &master_ip,
                                                  const uint32_t &master_port);
    static std::string GetSvrIDString(const Option *option);
 private:
    static bool connecting_;
    static pthread_mutex_t mutex_;
};

}
