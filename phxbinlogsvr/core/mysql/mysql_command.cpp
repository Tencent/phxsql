/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "mysql_command.h"

#include "phxcomm/phx_log.h"
#include "phxbinlogsvr/statistics/phxbinlog_stat.h"
#include "phxbinlogsvr/config/phxbinlog_config.h"
#include "phxbinlogsvr/define/errordef.h"

#include <string.h>

using std::string;
using std::vector;
using phxsql::LogVerbose;
using phxsql::ColorLogError;
using phxsql::ColorLogWarning;
using phxsql::ColorLogInfo;

namespace phxbinlog {

MySQLCommand::MySQLCommand(const MySqlOption *option) {
    mysql_ = mysql_init(NULL);
    option_ = option;
    is_connect_ = false;
}

MySQLCommand::~MySQLCommand() {
    mysql_close(mysql_);
}

int MySQLCommand::Query(const std::string &command) {
    int ret = Connect();
    if (ret) {
        return ret;
    }

    return DoQuery(command);
}

int MySQLCommand::GetQueryResults(const std::string &query_str, std::vector<std::vector<std::string> > *results,
                                  std::vector<std::string> *fields) {
    int ret = Query(query_str);
    if (ret) {
        return ret;
    }
    return GetResults(results, fields);
}

int MySQLCommand::Connect() {
    if (is_connect_) {
        return OK;
    }
    LogVerbose("%s[mysql] connect ip %s port %d user %s", __func__, option_->ip.c_str(), option_->port,
               option_->username.c_str());
    if (option_->username == "") {
        ColorLogError("%s username empty, skip", __func__);
        return MYSQL_FAIL;
    }
    int conn_timeout = 1, read_timeout = 2;
    mysql_options(mysql_, MYSQL_OPT_CONNECT_TIMEOUT, (char *) &conn_timeout);
    mysql_options(mysql_, MYSQL_OPT_READ_TIMEOUT, (char *) &read_timeout);

    bool reconnect = true;
    mysql_options(mysql_, MYSQL_OPT_RECONNECT, (char*) &reconnect);

    const char *ip = option_->ip.c_str();
    const char *socket = NULL;
    if (option_->ip == "127.0.0.1") {
        ip = NULL;
        socket = Option::GetDefault()->GetMySqlConfig()->GetMySQLSocket();
    }

    if (NULL
            == mysql_real_connect(mysql_, ip, option_->username.c_str(), option_->pwd.c_str(), "",
                                  option_->port, socket, 0)) {
        STATISTICS(MySqlConnectFail());
        ColorLogError("ERR: mysql_real_connect fail, %s", mysql_error(mysql_));
        return MYSQL_FAIL;
    }
    is_connect_ = true;
    return OK;
}

int MySQLCommand::DoQuery(const string &command) {
    int ret = mysql_query(mysql_, command.c_str());

    if (OK != ret) {
        ColorLogWarning("%s[mysql] mysql_query fail %d, %s, command %s", __func__, ret, mysql_error(mysql_),
                        command.c_str());
        if (ret < 0)
            ret = MYSQL_FAIL;
        else
            ret = MYSQL_NODATA;
        STATISTICS(MySqlQueryFail());
    }
    ColorLogInfo("%s mysql_query %s done %d, %s", __func__, command.c_str(), ret, mysql_error(mysql_));

    return ret;
}

int MySQLCommand::GetValue(const string &value_type, const string &value_key, string *value) {
    char cmd[128];
    sprintf(cmd, "show %s like '%s';", value_type.c_str(), value_key.c_str());

    vector < vector<string> > results;
    int ret = GetQueryResults(cmd, &results, NULL);
    if (ret)
        return ret;

    if (results.size() == 0) {
        LogVerbose("%s empty set", __func__);
        *value = "";
        return OK;
    }

    if (results.size() != 1 || results[0].size() != 2 || strcasecmp(results[0][0].c_str(), value_key.c_str())) {
        return MYSQL_FAIL;
    }

    *value = results[0][1];

    string debug_str = "";
    for (size_t i = 0; i < value->size(); ++i) {
        if ((*value)[i] == '\n')
            continue;
        debug_str += (*value)[i];
    }
    return OK;
}

int MySQLCommand::GetResults(vector<vector<string> > *results, vector<string> *fields) {
    MYSQL_RES *result = mysql_store_result(mysql_);
    if (NULL != result) {
        int row_count = result->row_count;
        int field_count = result->field_count;
        if (fields) {
            for (int i = 0; i < field_count; ++i) {
                fields->push_back(result->fields[i].name);
            }
        }
        for (int i = 0; i < row_count; ++i) {
            MYSQL_ROW row = mysql_fetch_row(result);

            if (NULL != row) {
                vector < string > result_row;
                for (int j = 0; j < field_count; ++j) {
                    result_row.push_back(row[j] ? row[j] : "");
                }
                results->push_back(result_row);

            } else {
                break;
            }
        }
        mysql_free_result(result);
    } else {
        return OK;
    }

    return OK;
}

}

