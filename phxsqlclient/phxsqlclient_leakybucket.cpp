/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include <vector>
#include <string>
#include <sys/stat.h>
#include <assert.h>
#include <sys/types.h>

#include "phxsqlutils.h"
#include "random.h"
#include "include/phxsqlclient_leakybucket.h"
#include "include/phxsqlclientbase.h"
#include "include/phxsqlclienterrdef.h"

using namespace std;

namespace phxsql {

PhxSQLClientDisasterStrategy_LeakyBucket::PhxSQLClientDisasterStrategy_LeakyBucket(int iBucketSize, int iFillPeriod) {
    m_iBucketSize = iBucketSize;
    m_iFillPeriod = iFillPeriod;
    int ret = mkdir(PHXSQL_LEAKEYBUCKET_DIR.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    assert(ret == -1);
}

PhxSQLClientDisasterStrategy_LeakyBucket::~PhxSQLClientDisasterStrategy_LeakyBucket() {
    for (auto & itr : m_mapBucket) {
        if (itr.second != NULL) {
            delete itr.second, itr.second = NULL;
        }
    }
    m_mapBucket.clear();
}

int PhxSQLClientDisasterStrategy_LeakyBucket::GetEndPoint(const vector<MySQLEndPoint_t> & vecEndPoints,
                                                          const MySQLEndPoint_t & tPreviousEndPoint,
                                                          MySQLEndPoint_t & tEndPoint) {
    int iBegIdx = phxsql::RandomPicker(vecEndPoints.size());

    int iCurrPos = (iBegIdx + 1) % vecEndPoints.size();

    bool bFound = false;
    while (iCurrPos != iBegIdx) {
        if (vecEndPoints[iCurrPos].m_sMySQLIP == tPreviousEndPoint.m_sMySQLIP
                && vecEndPoints[iCurrPos].m_iPort == tPreviousEndPoint.m_iPort) {
            iCurrPos = (iCurrPos + 1) % vecEndPoints.size();
            continue;
        }

        std::string sKey = vecEndPoints[iCurrPos].ToString();

        auto find_itr = m_mapBucket.find(sKey);
        if (find_itr == m_mapBucket.end()) {
            MmapLeakyBucket * bucket = new MmapLeakyBucket();

            MmapLeakyBucket::Config_t tConfig;
            tConfig.iBucketSize = m_iBucketSize;
            tConfig.tRefillPeriod = m_iFillPeriod;

            bucket->Init(&tConfig, string(PHXSQL_LEAKEYBUCKET_DIR + "/" + sKey).c_str());

            m_mapBucket[sKey] = bucket;

            find_itr = m_mapBucket.find(sKey);
        }

        if (find_itr->second->HasToken()) {
            tEndPoint.m_sMySQLIP = vecEndPoints[iCurrPos].m_sMySQLIP;
            tEndPoint.m_iPort = vecEndPoints[iCurrPos].m_iPort;
            bFound = true;
            break;
        } else {
            iCurrPos = (iCurrPos + 1) % vecEndPoints.size();
        }
    }

    if (!bFound) {
        //no useful endpoint, just choose the random one;
        tEndPoint.m_sMySQLIP = vecEndPoints[iCurrPos].m_sMySQLIP;
        tEndPoint.m_iPort = vecEndPoints[iCurrPos].m_iPort;
    }
    return phxsql::OK;
}

int PhxSQLClientDisasterStrategy_LeakyBucket::ReportEndPoint(const MySQLEndPoint_t & tEndPoint, int iRet,
                                                             int iCostInMS) {
    if (!iRet) {
        return phxsql::OK;
    }

    std::string sKey = tEndPoint.ToString();

    auto find_itr = m_mapBucket.find(sKey);
    if (find_itr == m_mapBucket.end()) {
        MmapLeakyBucket * bucket = new MmapLeakyBucket();

        MmapLeakyBucket::Config_t tConfig;
        tConfig.iBucketSize = m_iBucketSize;
        tConfig.tRefillPeriod = m_iFillPeriod;

        bucket->Init(&tConfig, string(PHXSQL_LEAKEYBUCKET_DIR + "/" + sKey).c_str());

        m_mapBucket[sKey] = bucket;

        find_itr = m_mapBucket.find(sKey);
    }

    find_itr->second->Apply(1);
    return 0;
}

PhxSQLClient_LeakyBucket::PhxSQLClient_LeakyBucket(const char *pcUser, const char *pcPasswd, const char *pcDB,
                                                   uint32_t iClientFlag, int iBucketSize, int iFillPeriod) :
        PhxSQLClientBase(NULL, NULL, pcUser, pcPasswd, pcDB, iClientFlag) {
    m_poConfig = new PhxSQLClientConfig();
    m_poDisasterStrategy = new PhxSQLClientDisasterStrategy_LeakyBucket(iBucketSize, iFillPeriod);
}

PhxSQLClient_LeakyBucket::~PhxSQLClient_LeakyBucket() {
    delete m_poConfig, m_poConfig = NULL;
    delete m_poDisasterStrategy, m_poDisasterStrategy = NULL;
}

}

