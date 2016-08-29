/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include <vector>
#include <cstdlib>
#include <string>
#include <cstring>

#include <mysql.h>
#include <errmsg.h>

#include "phxsqlutils.h"
#include "random.h"
#include "include/phxsqlclienterrdef.h"
#include "include/phxsqlclientbase.h"

using namespace std;

namespace phxsql {

PhxSQLClientConfig::PhxSQLClientConfig() {
}

PhxSQLClientConfig::~PhxSQLClientConfig() {
}

const std::vector<MySQLEndPoint_t> & PhxSQLClientConfig::GetEndPoints() {
    return m_vecMySQLEndPoint;
}

void PhxSQLClientConfig::SetMySQLEndPoints(const std::vector<MySQLEndPoint_t> & vecEndPoints) {
    m_vecMySQLEndPoint.clear();
    for (auto & itr : vecEndPoints) {
        m_vecMySQLEndPoint.push_back(itr);
    }
}

void PhxSQLClientConfig::ResetEndPoints() {
    m_vecMySQLEndPoint.clear();
}

int PhxSQLClientConfig::GetConnTimeout() {
    return m_iConnTimeout;
}

void PhxSQLClientConfig::SetConnTimeout(int iConnTimeout) {
    m_iConnTimeout = iConnTimeout;
}

int PhxSQLClientConfig::GetReadTimeout() {
    return m_iReadTimeout;
}

void PhxSQLClientConfig::SetReadTimeout(int iReadTimeout) {
    m_iReadTimeout = iReadTimeout;
}
/////////////////////////////
PhxSQLClientDisasterStrategy::PhxSQLClientDisasterStrategy() {
}

PhxSQLClientDisasterStrategy::~PhxSQLClientDisasterStrategy() {
}

int PhxSQLClientDisasterStrategy::ReportEndPoint(const MySQLEndPoint_t & tEndPoint, int iRet, int iCostInMS) {
    return phxsql::OK;
}

int PhxSQLClientDisasterStrategy::GetEndPoint(const std::vector<MySQLEndPoint_t> & vecMySQLEndPoints,
                                              const MySQLEndPoint_t & tPreviousEndPoint, MySQLEndPoint_t & tEndPoint) {
    int iRandomIdx = phxsql::RandomPicker(vecMySQLEndPoints.size());
    tEndPoint.m_sMySQLIP = vecMySQLEndPoints[iRandomIdx].m_sMySQLIP;
    tEndPoint.m_iPort = vecMySQLEndPoints[iRandomIdx].m_iPort;
    return phxsql::OK;
}

////////////////////////////////////////
void PhxSQLClientBase::InitProperty(MySQLProp_t * poProperty) {
    poProperty->tEndPoint.m_iPort = 0;

    memset(poProperty->pcUser, 0, sizeof(poProperty->pcUser));
    memset(poProperty->pcPass, 0, sizeof(poProperty->pcPass));
    memset(poProperty->pcDB, 0, sizeof(poProperty->pcDB));
    memset(poProperty->pcNULL, 0, sizeof(poProperty->pcNULL));
    poProperty->iClientFlag = 0;

    mysql_init(&m_poMySQLProp->connection);
    poProperty->bConnected = false;
}

const MySQLEndPoint_t & PhxSQLClientBase::GetConnectEndPoint() {
    return m_poMySQLProp->tEndPoint;
}

void PhxSQLClientBase::CloseConnection() {
    if (m_poMySQLProp->bConnected) {
        m_poMySQLProp->bConnected = false;
        m_poMySQLProp->tEndPoint.m_iPort = 0;
        m_poMySQLProp->tEndPoint.m_sMySQLIP = "";
        mysql_close(&m_poMySQLProp->connection);
    }
}

PhxSQLClientBase::PhxSQLClientBase(PhxSQLClientConfig * poConfig, PhxSQLClientDisasterStrategy * poDisasterStrategy,
                                   const char *pcUser, const char *pcPasswd, const char *pcDB, uint32_t iClientFlag) {
    InitLibrary();
    m_poMySQLProp = new MySQLProp_t();
    InitProperty(m_poMySQLProp);

    m_poDisasterStrategy = poDisasterStrategy;
    m_poConfig = poConfig;

    strncpy(m_poMySQLProp->pcUser, pcUser, sizeof(m_poMySQLProp->pcUser));
    strncpy(m_poMySQLProp->pcPass, pcPasswd, sizeof(m_poMySQLProp->pcPass));
    strncpy(m_poMySQLProp->pcDB, pcDB, sizeof(m_poMySQLProp->pcDB));
    m_poMySQLProp->iClientFlag = iClientFlag;
}

PhxSQLClientBase::~PhxSQLClientBase() {
    CloseConnection();
    delete m_poMySQLProp, m_poMySQLProp = NULL;
}

MYSQL * PhxSQLClientBase::GetMySQLFD() {
    return &m_poMySQLProp->connection;
}

PhxSQLClientConfig * PhxSQLClientBase::GetConfig() {
    return m_poConfig;
}

void PhxSQLClientBase::SetConfig(PhxSQLClientConfig * poConfig) {
    m_poConfig = poConfig;
}

PhxSQLClientDisasterStrategy * PhxSQLClientBase::GetDisasterStrategy() {
    return m_poDisasterStrategy;
}

void PhxSQLClientBase::SetDisasterStrategy(PhxSQLClientDisasterStrategy * poDisasterStrategy) {
    m_poDisasterStrategy = poDisasterStrategy;
}

int PhxSQLClientBase::Connect() {
    if (!m_poMySQLProp->bConnected) {
        int iConnTimeout = GetConfig()->GetConnTimeout();
        int iReadTimeout = GetConfig()->GetReadTimeout();
        SetOption(MYSQL_OPT_CONNECT_TIMEOUT, (const char *) &iConnTimeout);
        SetOption(MYSQL_OPT_READ_TIMEOUT, (const char *) &iReadTimeout);

        char *dbname = NULL;
        if (m_poMySQLProp->pcDB[0] != '\0') {
            dbname = m_poMySQLProp->pcDB;
        }

        MySQLEndPoint_t tEndPoint, tPreviousEndPoint;
        tEndPoint.m_iPort = 0;
        tPreviousEndPoint.m_iPort = 0;
        for (int32_t i = 0; i < 3; ++i) {
            int ret = m_poDisasterStrategy->GetEndPoint(m_poConfig->GetEndPoints(), tPreviousEndPoint, tEndPoint);
            tPreviousEndPoint = tEndPoint;
            if (ret != phxsql::OK || tEndPoint.m_sMySQLIP == "" || tEndPoint.m_iPort == 0) {
                //do log and monitor here
                return phxsql::GET_END_POINT_FAIL;
            }

            if (mysql_real_connect(&m_poMySQLProp->connection, tEndPoint.m_sMySQLIP.c_str(), m_poMySQLProp->pcUser,
                                   m_poMySQLProp->pcPass, dbname, tEndPoint.m_iPort, NULL, m_poMySQLProp->iClientFlag)
                    == NULL) {
                m_poDisasterStrategy->ReportEndPoint(tEndPoint, -1, 0);
                //do log and monitor here
                continue;
            }
            m_poMySQLProp->tEndPoint = tEndPoint;
            m_poMySQLProp->bConnected = true;
            break;
        }
    }
    //do log and monitor here
    return phxsql::OK;
}

int PhxSQLClientBase::SetOption(mysql_option option, const char *arg) {
    return mysql_options(&m_poMySQLProp->connection, option, arg);
}

int PhxSQLClientBase::Ping() {
    if (!m_poMySQLProp->bConnected) {
        return phxsql::MYSQL_FAIL;
    }

    if (mysql_ping(&m_poMySQLProp->connection) != 0) {
        CloseConnection();
        return phxsql::MYSQL_FAIL;
    }
    return phxsql::OK;
}

int PhxSQLClientBase::Query(const char* szSqlString) {
    return Query(szSqlString, false);
}

int PhxSQLClientBase::Query(const char * szSqlString, bool bPingFirst) {
    if (m_poMySQLProp->bConnected) {
        bool bFindEndPoint = false;
        for (auto itr : m_poConfig->GetEndPoints()) {
            if (itr.m_sMySQLIP == m_poMySQLProp->tEndPoint.m_sMySQLIP
                    && itr.m_iPort == m_poMySQLProp->tEndPoint.m_iPort) {
                bFindEndPoint = true;
                break;
            }
        }
        if (!bFindEndPoint)
            CloseConnection();
    }

    int ret = phxsql::OK;
    if (bPingFirst) {
        if (Ping() != phxsql::OK) {
            if ((ret = Connect()) != phxsql::OK) {
                //TODO: log and monitor
                return ret;
            }
        }
    }

    if (!m_poMySQLProp->bConnected) {
        return phxsql::MYSQL_NOT_CONNECTED;
    }
    if ((ret = mysql_query(&m_poMySQLProp->connection, szSqlString)) != 0) {
        int iErrCode = mysql_errno(&m_poMySQLProp->connection);
        if (CR_SERVER_LOST == iErrCode || CR_SERVER_GONE_ERROR == iErrCode) {
            //TODO: log and monitor
            m_poDisasterStrategy->ReportEndPoint(m_poMySQLProp->tEndPoint, -1, 0);
        }
        return phxsql::MYSQL_FAIL;
    }
    m_poDisasterStrategy->ReportEndPoint(m_poMySQLProp->tEndPoint, 0, 0);
    return phxsql::OK;
}

void PhxSQLClientBase::InitLibrary() {
    static bool init = false;
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    if (!init) {
        pthread_mutex_lock(&mutex);
        if (!init) {
            mysql_library_init(0, NULL, NULL);
            init = true;
        }
        pthread_mutex_unlock(&mutex);
    }
}

//random version
PhxSQLClient_Random::PhxSQLClient_Random(const char *pcUser, const char *pcPasswd, const char *pcDB,
                                         uint32_t iClientFlag) :
        PhxSQLClientBase(NULL, NULL, pcUser, pcPasswd, pcDB, iClientFlag) {
    m_poConfig = new PhxSQLClientConfig();
    m_poDisasterStrategy = new PhxSQLClientDisasterStrategy();
}

PhxSQLClient_Random::~PhxSQLClient_Random() {
    delete m_poConfig, m_poConfig = NULL;
    delete m_poDisasterStrategy, m_poDisasterStrategy = NULL;
}
;

}

