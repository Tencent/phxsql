/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "monitor_comm.h"

#include "mysql_manager.h"

#include "phxcomm/phx_log.h"
#include "phxbinlogsvr/core/gtid_handler.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/define/errordef.h"

#include <string>
#include <string.h>

using std::string;
using std::vector;
using phxsql::ColorLogWarning;
using phxsql::LogVerbose;

namespace phxbinlog {

int MonitorComm::GetMySQLMaxGTIDList(const Option *option, const string &gtid_str, vector<string> *gtid_list,
                                     const string &query_ip) {
    string gtid_pos;
    int ret = MySqlManager::GetGlobalMySqlManager(option)->GetValue("global variables", gtid_str, &gtid_pos, query_ip);
    if (ret)
        return ret;
    return ParseMaxGTIDList(option, gtid_pos, gtid_list);
}

int MonitorComm::ParseMaxGTIDList(const Option *option, const string &gtid_str, vector<string> *gtid_list) {
    string val = "";
    for (size_t i = 0; i <= gtid_str.size(); ++i) {
        if (gtid_str[i] == '\n')
            continue;
        if (gtid_str[i] == ',' || i == gtid_str.size()) {
            if (val.size() > 0) {
                char uuid[128];
                size_t end_pos = 0;
                sscanf(val.c_str(), "%[^:]:", uuid);
                size_t uuid_len = strlen(uuid);
                for (size_t j = val.size() - 1; j >= uuid_len; --j) {
                    if (!(val[j] >= '0' && val[j] <= '9')) {
                        for (size_t k = j + 1; k < val.size(); ++k) {
                            end_pos = end_pos * 10 + val[k] - '0';
                        }

                        for (; j > uuid_len; --j) {
                            if (val[j] == ':') {
                                ColorLogWarning("%s invalid max gtid %s from mysql", __func__, val.c_str());
                                return GTID_FAIL;
                            }
                        }
                        break;
                    }
                }

                val = "";
                string gtid = GtidHandler::GenGTID(uuid, end_pos);
                gtid_list->push_back(gtid);
                string debug_str = "";
                for (size_t j = 0; j < gtid.size(); ++j) {
                    if (gtid[j] == '\n')
                        continue;
                    debug_str += gtid[j];
                }
                LogVerbose("%s get max gtid %s from mysql", __func__, debug_str.c_str());
            }
            if (i == gtid_str.size()) {
                break;
            }
        } else
            val += gtid_str[i];
    }
    return OK;
}

}
