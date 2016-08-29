/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include <string>
#include <algorithm>
#include <mysql.h>

#include "freqctrlconfig.h"
#include "requestfilter_plugin.h"
#include "freqfilter_plugin.h"

using namespace std;

namespace phxsqlproxy {

bool FreqFilterPlugin::CanExecute(bool is_master_port, const std::string & db_name, const char * buf, int buf_size) {
    if (buf_size < 6) {
        return true;
    }
    int command = buf[4];
    if (command != COM_QUERY) {
        return true;
    }

    //FreqCtrlConfig :: GetDefault()->LoadIfModified();
    if (is_master_port == false) {
        return FreqCtrlConfig::GetDefault()->Apply(db_name, false);
    }

    string sql_string = string(buf + 5, buf_size - 5);
    transform(sql_string.begin(), sql_string.end(), sql_string.begin(), ::toupper);

    static string write_queries[] = { "INSERT", "UPDATE", "REPLACE", "DELETE" };

    //int key = 33;
    bool is_write_query = false;
    size_t begin = 0;
    for (size_t i = 0; i < sql_string.size(); ++i) {
        if (sql_string[i] == ' ' || sql_string[i] == ';') {
            if (i > begin) {
                string word = sql_string.substr(begin, i - begin);
                for (size_t j = 0; j < sizeof(write_queries) / sizeof(string); ++j) {
                    if (word == write_queries[j]) {
                        is_write_query = true;
                        break;
                    }
                }
            }
            i++;
            begin = i;
        }
        if (is_write_query)
            break;
    }
    return FreqCtrlConfig::GetDefault()->Apply(db_name, is_write_query);
}

void FreqFilterPlugin::SetConfig(phxsqlproxy::PHXSqlProxyConfig * config, phxsqlproxy::WorkerConfig_t * worker_config) {
    FreqCtrlConfig * freq_ctrl_config = FreqCtrlConfig::GetDefault();
    freq_ctrl_config->ReadFileWithConfigDirPath(config->GetFreqCtrlConfigPath());
}

}

