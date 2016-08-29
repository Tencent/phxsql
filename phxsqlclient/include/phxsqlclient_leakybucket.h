/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include <map>
#include <string>

#include "phxsqlclientbase.h"

namespace phxsql {

class MmapLeakyBucket;

static std::string PHXSQL_LEAKEYBUCKET_DIR = "/tmp/phxsql_leakybucket/";

class PhxSQLClientDisasterStrategy_LeakyBucket : public PhxSQLClientDisasterStrategy {
 public:
    PhxSQLClientDisasterStrategy_LeakyBucket(int iBucketSize, int iFillPeriod);

    virtual ~PhxSQLClientDisasterStrategy_LeakyBucket();

    virtual int GetEndPoint(const std::vector<MySQLEndPoint_t> & vecMySQLEndPoints,
                            const MySQLEndPoint_t & tPreviousEndPoint, MySQLEndPoint_t & tEndPoint);

    virtual int ReportEndPoint(const MySQLEndPoint_t & tEndPoint, int iRet, int iCostInMS);

 private:
    int m_iBucketSize;
    int m_iFillPeriod;
    std::map<std::string, phxsql::MmapLeakyBucket*> m_mapBucket;
};

class PhxSQLClient_LeakyBucket : public PhxSQLClientBase {
 public:
    PhxSQLClient_LeakyBucket(const char *pcUser, const char *pcPasswd, const char *pcDB, uint32_t iClientFlag,
                             int iBucketSize, int iFillPeriod);

    virtual ~PhxSQLClient_LeakyBucket();
};

}

