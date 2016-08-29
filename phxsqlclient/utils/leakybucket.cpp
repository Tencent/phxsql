/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include <cstring>
#include <sys/mman.h>
#include <errno.h>
#include <sys/stat.h>

#include "leakybucket.h"
#include "filelock.h"

namespace phxsql {

MmapLeakyBucket::MmapLeakyBucket() {
    m_poLock = NULL;
    memset(&m_tConfig, 0, sizeof(m_tConfig));
    m_ptBucket = NULL;
}

MmapLeakyBucket::~MmapLeakyBucket() {
    if (NULL != m_poLock)
        delete m_poLock;
    m_poLock = NULL;

    if (NULL != m_ptBucket) {
        freeMmapPtr((void *) m_ptBucket, sizeof(Bucket_t));
    }
    m_ptBucket = NULL;
}

void * MmapLeakyBucket::getMmapPtr(const char * filePath, uint32_t len, int * isNewFile) {
    *isNewFile = 0;

    int fd = open(filePath, O_RDWR);
    if (fd < 0) {
        if (ENOENT == errno) {
            fd = open(filePath, O_RDWR | O_CREAT, 0660);
            if (fd >= 0) {
                *isNewFile = 1;

                char buffer[1024] = { 0 };
                memset(buffer, 0, sizeof(buffer));
                for (int i = 0; i < (int) len; i += sizeof(buffer)) {
                    write(fd, buffer, sizeof(buffer));
                }

                ftruncate(fd, len);

                lseek(fd, 0, SEEK_SET);
            } else {
                //Log( COMM_LOG_ERR, "ERR: Create %s fail, errno %d, %s\n",
                //filePath, errno, strerror( errno ) );
            }
        } else {
            //Log( COMM_LOG_ERR, "ERR: Open %s fail, errno %d, %s\n",
            //filePath, errno, strerror( errno ) );
        }
    }

    void * ret = NULL;

    if (fd >= 0) {
        struct stat fileStat;
        if (0 == fstat(fd, &fileStat)) {
            if (fileStat.st_size == (int) len) {
                ret = mmap(0, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                if (MAP_FAILED == ret) {
                    //Log( COMM_LOG_ERR, "ERR: mmap %s fail, errno %d, %s\n",
                    //		filePath, errno, strerror( errno ) );
                    ret = NULL;
                }
            } else {
                //Log( COMM_LOG_ERR, "ERR: invalid file size, real %lld, except %i",
                //		(long long)fileStat.st_size, len );
            }
        } else {
            //Log( COMM_LOG_ERR, "ERR: Stat %s fail, errno %d, %s\n",
            //		filePath, errno, strerror( errno ) );
        }

        close(fd);
    }

    return ret;
}

void MmapLeakyBucket::freeMmapPtr(void * ptr, uint32_t len) {
    if (0 != munmap(ptr, len)) {
        //Log( COMM_LOG_ERR, "ERR: munmap fail, errno %d, %s\n",
        //		errno, strerror( errno ) );
    }
}

int MmapLeakyBucket::Init(Config_t * ptConfig, const char * sPath) {
    m_tConfig = *ptConfig;

    int isNew = 0;
    m_ptBucket = (Bucket_t *) getMmapPtr(sPath, sizeof(Bucket_t), &isNew);

    if (NULL == m_ptBucket)
        return -1;

    m_poLock = new phxsql::FileLock();

    if (!m_poLock->Open(sPath))
        return -1;

    return 0;
}

int MmapLeakyBucket::Refill(int iBucketSize) {
    if (NULL == m_poLock || NULL == m_ptBucket)
        return -1;

    m_poLock->WriteLockW(0);

    m_tConfig.iBucketSize = iBucketSize;

    unsigned int tLatestRefillTime = (unsigned int) time(NULL);
    m_ptBucket->tLatestRefillTime = tLatestRefillTime;
    m_ptBucket->iTokenCount = m_tConfig.iBucketSize;

    m_poLock->Unlock(0);
    return 0;
}

bool MmapLeakyBucket::HasToken() {
    if (NULL == m_poLock || NULL == m_ptBucket)
        return -1;

    m_poLock->WriteLockW(0);

    unsigned int tLatestRefillTime = (unsigned int) time(NULL);
    tLatestRefillTime = tLatestRefillTime - tLatestRefillTime % (m_tConfig.tRefillPeriod);

    if (m_ptBucket->tLatestRefillTime != tLatestRefillTime) {
        m_ptBucket->tLatestRefillTime = tLatestRefillTime;
        m_ptBucket->iTokenCount = m_tConfig.iBucketSize;
    }

    bool ret = m_ptBucket->iTokenCount > 0;

    m_poLock->Unlock(0);

    return ret;
}

int MmapLeakyBucket::Apply(int iCount) {
    if (NULL == m_poLock || NULL == m_ptBucket)
        return -1;

    m_poLock->WriteLockW(0);

    unsigned int tLatestRefillTime = (unsigned int) time(NULL);
    tLatestRefillTime = tLatestRefillTime - tLatestRefillTime % (m_tConfig.tRefillPeriod);

    if (m_ptBucket->tLatestRefillTime != tLatestRefillTime) {
        m_ptBucket->tLatestRefillTime = tLatestRefillTime;
        m_ptBucket->iTokenCount = m_tConfig.iBucketSize;
    }

    int ret = -1;
    if (m_ptBucket->iTokenCount >= iCount) {
        m_ptBucket->iTokenCount -= iCount;
        ret = 0;
    }

    m_poLock->Unlock(0);

    return ret;
}

void MmapLeakyBucket::SetConfig(Config_t &config) {
    if (NULL == m_poLock)
        return;

    m_poLock->WriteLockW(0);

    m_tConfig.iBucketSize = config.iBucketSize;
    m_tConfig.tRefillPeriod = config.tRefillPeriod;

    m_poLock->Unlock(0);
}

}


