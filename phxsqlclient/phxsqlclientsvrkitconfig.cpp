/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include <string>
#include <vector>

#include "iLog.h"
#include "include/phxsqlclientsvrkitconfig.h"
#include "include/phxsqlclientbase.h"
#include "iFileFunc.h"

namespace phxsql {

PhxSQLClientSvrkitConfig::PhxSQLClientSvrkitConfig(const char * pcFile) {
    Comm::CBaseConfig::SetFileIfUnset(pcFile);
}

PhxSQLClientSvrkitConfig::~PhxSQLClientSvrkitConfig() {
}

int PhxSQLClientSvrkitConfig::Read(Comm::CConfig * poConfig, bool isReload) {
    ResetEndPoints();

    std::vector<MySQLEndPoint_t> vecEndPoints;
    bool bFound = false;

    int iSetCount = 0;
    poConfig->ReadItem("Phxsql", "SetCount", 0, iSetCount);
    poConfig->ReadItem("Phxsql", "ConnTimeout", 0, m_iConnTimeout);
    poConfig->ReadItem("Phxsql", "ReadTimeout", 0, m_iReadTimeout);
    for (int i = 0; i < iSetCount; ++i) {
        vecEndPoints.clear();

        char pcSect[16] = { 0 };
        int iServerCount = 0;

        snprintf(pcSect, sizeof(pcSect), "Server%d", i);
        poConfig->ReadItem(pcSect, "ServerCount", 0, iServerCount);

        for (int j = 0; j < iServerCount; ++j) {
            MySQLEndPoint_t tEndPoint;

            char pcKey[16] = { 0 };

            snprintf(pcKey, sizeof(pcKey), "SVR_IP%d", j);
            poConfig->ReadItem(pcSect, pcKey, "", tEndPoint.m_sMySQLIP);

            snprintf(pcKey, sizeof(pcKey), "SVR_PORT%d", j);
            poConfig->ReadItem(pcSect, pcKey, 0, tEndPoint.m_iPort);

            if (tEndPoint.m_sMySQLIP != "" && tEndPoint.m_iPort) {
                vecEndPoints.push_back(tEndPoint);
            }

            if (tEndPoint.m_sMySQLIP == LOGID_BASE::GetInnerIP()) {
                bFound = true;
            }
        }

        if (bFound) {
            SetMySQLEndPoints(vecEndPoints);
            break;
        }
    }
    return phxsql::OK;
}

const std::vector<MySQLEndPoint_t> & PhxSQLClientSvrkitConfig::GetEndPoints() {
    LoadIfModified();
    return m_vecMySQLEndPoint;
}

}

