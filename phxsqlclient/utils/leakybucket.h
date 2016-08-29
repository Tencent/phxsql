/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include <inttypes.h>
#include <time.h>

namespace phxsql {

class FileLock;

class MmapLeakyBucket {
 public:
    typedef struct tagConfig {
        int iBucketSize;		// 桶容量,能容纳令牌的数量,单位:个
        unsigned int tRefillPeriod;  // 补注间隔,单位:秒
    } Config_t;

    typedef struct tagBucket {
        int iTokenCount;			// 令牌余量,单位:个
        unsigned int tLatestRefillTime;  // 最后补注时间
    } Bucket_t;

 public:
    MmapLeakyBucket();
    ~MmapLeakyBucket();

    int Init(Config_t * ptConfig, const char * sPath);

    int Apply(int iCount);

    int Refill(int iBucketSize);

    void SetConfig(Config_t &config);

    bool HasToken();

 private:
    void * getMmapPtr(const char * filePath, uint32_t len, int * isNewFile);

    void freeMmapPtr(void * ptr, uint32_t len);

 private:
    phxsql::FileLock * m_poLock;
    Config_t m_tConfig;
    Bucket_t * m_ptBucket;
};

}

