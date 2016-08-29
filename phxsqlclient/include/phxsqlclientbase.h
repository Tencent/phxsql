/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once 

#include <vector>
#include <map>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "mysql.h"
#include "phxsqlclienterrdef.h"

namespace phxsql {
typedef struct tagMySQLEndPoint {
    std::string m_sMySQLIP;
    int32_t m_iPort;
    std::string ToString() {
        char sBuf[64] = { 0 };
        snprintf(sBuf, sizeof(sBuf), "%s:%u", m_sMySQLIP.c_str(), m_iPort);
        return std::string(sBuf);
    }
    std::string ToString() const {
        return ((tagMySQLEndPoint *) this)->ToString();
    }
} MySQLEndPoint_t;

typedef struct tagMySQLProp {
    MySQLEndPoint_t tEndPoint;

    char pcUser[64];
    char pcPass[64];
    char pcDB[256];
    char pcNULL[1];
    char iClientFlag;

    MYSQL connection;
    bool bConnected;
} MySQLProp_t;

class PhxSQLClientConfig {
 public:
    PhxSQLClientConfig();

    virtual ~PhxSQLClientConfig();

    virtual void SetMySQLEndPoints(const std::vector<MySQLEndPoint_t> & vecEndPoints);

    virtual void ResetEndPoints();

    const std::vector<MySQLEndPoint_t> & GetEndPoints();

    int GetConnTimeout();

    void SetConnTimeout(int iConnTimeout);

    int GetReadTimeout();

    void SetReadTimeout(int iReadTimeout);

 protected:
    std::vector<MySQLEndPoint_t> m_vecMySQLEndPoint;

    int m_iConnTimeout;

    int m_iReadTimeout;
};

class PhxSQLClientDisasterStrategy {
 public:
    PhxSQLClientDisasterStrategy();

    virtual ~PhxSQLClientDisasterStrategy();

 public:
    virtual int ReportEndPoint(const MySQLEndPoint_t & tEndPoint, int iRet, int iCostInMS);

    virtual int GetEndPoint(const std::vector<MySQLEndPoint_t> & vecMySQLEndPoints,
                            const MySQLEndPoint_t & tPreviousEndPoint, MySQLEndPoint_t & tEndPoint);
};

//MySQLConnect() is called, it will new another one. 
//waning!! it is not thread-safe
class PhxSQLClientBase {
 public:
    PhxSQLClientBase(PhxSQLClientConfig * poConfig, PhxSQLClientDisasterStrategy * poDisasterStrategy,
                     const char *pcUser, const char *pcPasswd, const char *pcDB, uint32_t iClientFlag);

    virtual ~PhxSQLClientBase();

    PhxSQLClientConfig * GetConfig();
    void SetConfig(PhxSQLClientConfig * poConfig);

    PhxSQLClientDisasterStrategy * GetDisasterStrategy();
    void SetDisasterStrategy(PhxSQLClientDisasterStrategy * poDisasterStrategy);

//mysql api
 public:
    MYSQL * GetMySQLFD();

    //Connect Mysql, if connection exists, just return OK
    int Connect();

    int SetOption(mysql_option option, const char *arg);

    //substitute of mysql_query()
    int Query(const char* szSqlString);

    //substitute of mysql_query(), bPingFirst = true will cause a mysql_ping() before query, 
    //which will cost one more RTT and maybe create another connection, be carefull if u need this feature.
    //After bPingFirst is set, u don't need to invoke Connect() before Query().
    int Query(const char* szSqlString, bool bPingFirst);

    int Ping();

    void CloseConnection();

    const MySQLEndPoint_t & GetConnectEndPoint();

 private:
    void InitLibrary();

    void InitProperty(MySQLProp_t * poProperty);

 protected:
    PhxSQLClientConfig * m_poConfig;

    MySQLProp_t * m_poMySQLProp;

    PhxSQLClientDisasterStrategy * m_poDisasterStrategy;
};

class PhxSQLClient_Random : public PhxSQLClientBase {
 public:
    PhxSQLClient_Random(const char *pcUser, const char *pcPasswd, const char *pcDB, uint32_t iClientFlag);

    virtual ~PhxSQLClient_Random();
};

}
;

