/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#ifndef __USE_FILE_OFFSET64
#define __USE_FILE_OFFSET64
#endif

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "filelock.h"

namespace phxsql {

FileLock::FileLock() {
    m_iFd = -1;
}

bool FileLock::Open(const char* sPath) {
    m_iFd = open(sPath, O_RDWR | O_CREAT | O_LARGEFILE, S_IRUSR | S_IWUSR);
    if (m_iFd < 0) {
        return false;
    } else {
        return true;
    }
}

void FileLock::Close() {
    close(m_iFd);
    m_iFd = -1;
}

FileLock::~FileLock() {
    if (m_iFd > 0)
        close(m_iFd);
    m_iFd = -1;
}

bool FileLock::Fcntl(int iCmd, int iType, int iOffset, int iLen, int iWhence) {

    uint64_t offset = iOffset;
    uint64_t len = iLen;
    return Fcntl(iCmd, iType, offset, len, iWhence);
}

bool FileLock::Fcntl(int iCmd, int iType, uint64_t iOffset, uint64_t iLen, int iWhence) {
#ifdef __WORDSIZE
    if( __WORDSIZE != 64) {
        if( iCmd == 7 )
        {
            iCmd= 14;
        }
        else if( 6 == iCmd )
        {
            iCmd = 13;
        }
        else if( 5 == iCmd )
        {
            iCmd = 12;
        }
    }
#else
    if (iCmd == 7) {
        iCmd = 14;
    } else if (6 == iCmd) {
        iCmd = 13;
    } else if (5 == iCmd) {
        iCmd = 12;
    }
#endif
    struct flock64 stLock;
    stLock.l_type = iType;
    stLock.l_start = iOffset;
    stLock.l_whence = iWhence;
    stLock.l_len = iLen;
    if (fcntl(m_iFd, iCmd, &stLock) < 0) {
        return false;
    }

    return true;
}

static void sigalrm(int signo) {
    // do nothing
}

bool FileLock::FcntlTimeOut(int iCmd, int iType, int iOffset, int iLen, int iWhence, int sec) {
    uint64_t offset = iOffset;
    uint64_t len = iLen;
    return FcntlTimeOut(iCmd, iType, offset, len, iWhence, sec);
}
bool FileLock::FcntlTimeOut(int iCmd, int iType, uint64_t iOffset, uint64_t iLen, int iWhence, int sec) {
    struct sigaction act, oact;
    int ret;

#ifdef __WORDSIZE
    if( __WORDSIZE != 64) {
        if( iCmd == 7 )
        {
            iCmd= 14;
        }
        else if( 6 == iCmd )
        {
            iCmd = 13;
        }
        else if( 5 == iCmd )
        {
            iCmd = 12;
        }
    }
#else
    if (iCmd == 7) {
        iCmd = 14;
    } else if (6 == iCmd) {
        iCmd = 13;
    } else if (5 == iCmd) {
        iCmd = 12;
    }
#endif
    struct flock64 stLock;
    stLock.l_type = iType;
    stLock.l_start = iOffset;
    stLock.l_whence = iWhence;
    stLock.l_len = iLen;

    if (sec > 0) {
        memset(&act, 0, sizeof(struct sigaction));
        memset(&oact, 0, sizeof(struct sigaction));
        act.sa_handler = sigalrm;
        sigemptyset(&act.sa_mask);
#ifdef SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT;
#endif
        SIGACT: ret = sigaction(SIGALRM, &act, &oact);
        if (ret < 0 && errno == EINTR)
            goto SIGACT;
        if (ret < 0)  // error
            return false;
        alarm(sec);
    }
    ret = fcntl(m_iFd, iCmd, &stLock);
    if (sec > 0) {
        alarm(0);
        sigaction(SIGALRM, &oact, &act);
    }
    if (ret < 0) {
        return false;
    }

    return true;
}

bool FileLock::ReadLock(uint64_t iOffset, uint64_t iLen, int iWhence) {
    return Fcntl(F_SETLK, F_RDLCK, iOffset, iLen, iWhence);
}

bool FileLock::WriteLock(uint64_t iOffset, uint64_t iLen, int iWhence) {
    return Fcntl(F_SETLK, F_WRLCK, iOffset, iLen, iWhence);
}

bool FileLock::ReadLockW(uint64_t iOffset, uint64_t iLen, int iWhence) {
    return Fcntl(F_SETLKW, F_RDLCK, iOffset, iLen, iWhence);
}

bool FileLock::WriteLockW(uint64_t iOffset, uint64_t iLen, int iWhence) {
    return Fcntl(F_SETLKW, F_WRLCK, iOffset, iLen, iWhence);
}

bool FileLock::ReadLockTimeOut(uint64_t iOffset, int sec, uint64_t iLen, int iWhence) {
    return FcntlTimeOut(F_SETLKW, F_RDLCK, iOffset, iLen, iWhence, sec);
}

bool FileLock::WriteLockTimeOut(uint64_t iOffset, int sec, uint64_t iLen, int iWhence) {
    return FcntlTimeOut(F_SETLKW, F_WRLCK, iOffset, iLen, iWhence, sec);
}

bool FileLock::Unlock(uint64_t iOffset, uint64_t iLen, int iWhence) {
    return Fcntl(F_SETLK, F_UNLCK, iOffset, iLen, iWhence);
}

}


