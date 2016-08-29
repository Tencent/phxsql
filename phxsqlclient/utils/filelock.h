/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/select.h>
#include <sys/time.h>
#include <stdio.h>

namespace phxsql {

/*!
 *
 *\brief  对fcntl() 函数的封装，实现文件锁,也可以用于进程间同步.
 *
 *\as FileLock  参考  testiFileLock.cpp
 *
 */

class FileLock {
 protected:
    /*!
     *\brief  打开的文件锁fd.
     */
    int m_iFd;

 protected:

    /*!
     *\brief 对fcntl的封装.
     *\param iCmd int ,命令字，请参见man fcntl.
     *\param iType int ,锁类型，请参见man fcntl.
     *\param iOffset int ,相对iWhere的加锁偏移量.
     *\param iLen int ,锁住的字节数.
     *\param iWhere int ,加锁位置，SEEK_XXX.
     *\retval true:成功，false:失败.
     *
     */
    bool Fcntl(int iCmd, int iType, int iOffset, int iLen, int iWhence);

    /*!
     *\brief 对fcntl的封装，经过指定时间还未取得锁，则超时返回失败，前面5个参数参见前文.
     *\param sec int ,指定时间s.
     *
     */
    bool FcntlTimeOut(int iCmd, int iType, int iOffset, int iLen, int iWhence, int sec);

    /*!
     *
     *\brief Fcntl()大文件版本.
     *
     */
    bool Fcntl(int iCmd, int iType, uint64_t iOffset, uint64_t iLen, int iWhence);

    /*!
     *
     *\brief FcntlTimeOut()大文件版本.
     *
     */
    bool FcntlTimeOut(int iCmd, int iType, uint64_t iOffset, uint64_t iLen, int iWhence, int sec);
 public:

    /*!
     *\brief 构造函数.
     *
     */
    FileLock();

    /*!
     *\brief  析构函数，关闭打开的文件描述符.
     *
     */
    virtual ~FileLock();

    /*!
     *\brief 使用一个指定的文件锁.
     *\param sPath const char *,文件锁路径.
     *\retval  true:成功，false:失败.
     *
     */
    bool Open(const char* sPath);

    /*!
     *\brief 关闭锁文件,作用其上的文件锁将会被释放.
     */
    void Close();

    /*!
     *\brief 加读锁.如果没有加锁成功，或者被信号中断，则立即返回false.
     *\param iOffset int, 加锁偏移量.
     *\param iLen int,加锁字节数.
     *\param iWhence int, 加锁起始位置.
     *\retval true:成功，false:失败.
     *
     *\note return false when interupt by singal.
     */
    inline bool ReadLock(int iOffset, int iLen = 1, int iWhence = SEEK_SET) {
        return Fcntl(F_SETLK, F_RDLCK, iOffset, iLen, iWhence);
    }

    /*!
     *\brief ReadLock锁住大文件版本.
     *
     */
    bool ReadLock(uint64_t iOffset, uint64_t iLen = 1LL, int iWhence = SEEK_SET);

    /*!
     *\brief 加写锁.如果没有加锁成功，或者被信号中断，则立即返回false.
     *\param iOffset int , 加锁偏移量.
     *\param iLen int, 加锁字节数.
     *\param iWhence  int加锁起始位置.
     *\retval 硉rue:成功,false:失败.
     *\note return false when interupt by singal.
     */
    inline bool WriteLock(int iOffset, int iLen = 1, int iWhence = SEEK_SET) {
        return Fcntl(F_SETLK, F_WRLCK, iOffset, iLen, iWhence);
    }

    /*!
     *\brief WriteLock大文件版本.
     *
     */
    bool WriteLock(uint64_t iOffset, uint64_t iLen = 1LL, int iWhence = SEEK_SET);

    /*!
     *\brief 永久等待的读锁.一旦获得读锁，或者被信号中断才返回.
     *\param iOffset int ,加锁偏移量.
     *\param iLen  int, 加锁字节数.
     *\param iWhence  int ,SEEK_XXX,加锁起始位置.
     *\retval  true:成功,false:失败.
     *\note return false when interupt by singal.
     */
    inline bool ReadLockW(int iOffset, int iLen = 1, int iWhence = SEEK_SET) {
        return Fcntl(F_SETLKW, F_RDLCK, iOffset, iLen, iWhence);
    }

    /*!
     *
     *\brief ReadLockW()大文件版本.
     */
    bool ReadLockW(uint64_t iOffset, uint64_t iLen = 1LL, int iWhence = SEEK_SET);

    /*!
     *\brief 永久等待的写锁.一旦获得写锁，或者被信号中断才返回.
     *\param iOffset int, 加锁偏移量.
     *\param iLen int ,加锁字节数.
     *\param iWhence  int, 加锁起始位置.
     *\retval 成功返回true，失败返回false
     *\note return false when interupt by singal.
     */
    inline bool WriteLockW(int iOffset, int iLen = 1, int iWhence = SEEK_SET) {
        return Fcntl(F_SETLKW, F_WRLCK, iOffset, iLen, iWhence);
    }

    /*!
     *\brief WriteLockW()大文件版本.
     */
    bool WriteLockW(uint64_t iOffset, uint64_t iLen = 1LL, int iWhence = SEEK_SET);

    /*!
     *\brief 指定等待时间的读锁.
     *\param iOffset int ,加锁偏移量.
     *\param sec int ,等待的时间秒.
     *\param iLen int, 加锁字节数.
     *\param iWhence int , 加锁起始位置.
     *\retval 成功返回true，失败返回false.
     *\note return false when interupt by singal.
     */
    inline bool ReadLockTimeOut(int iOffset, int sec, int iLen = 1, int iWhence = SEEK_SET) {
        return FcntlTimeOut(F_SETLKW, F_RDLCK, iOffset, iLen, iWhence, sec);
    }

    /*!
     *\brief ReadLockTimeOut()大文件版本.
     */
    bool ReadLockTimeOut(uint64_t iOffset, int sec, uint64_t iLen = 1LL, int iWhence = SEEK_SET);

    /*!
     *\brief  指定 时间等待的写锁.
     *\param iOffset int , 加锁偏移量.
     *\param sec  int , 等待的时间秒.
     *\param iLen  int, 加锁字节数.
     *\param iWhence int , 加锁起始位置.
     *\retval 成功返回true，失败返回false.
     *\note return false when interupt by singal.
     */
    inline bool WriteLockTimeOut(int iOffset, int sec, int iLen = 1, int iWhence = SEEK_SET) {
        return FcntlTimeOut(F_SETLKW, F_WRLCK, iOffset, iLen, iWhence, sec);
    }

    /*!
     *\brief WriteLockTimeOut()大文件版本.
     *
     */
    bool WriteLockTimeOut(uint64_t iOffset, int sec, uint64_t iLen = 1LL, int iWhence = SEEK_SET);

    /*!
     *\brief 解锁.
     *\param iOffset int, 加锁偏移量.
     *\param iLen int , 加锁字节数.
     *\param iWhence int , 加锁起始位置.
     *\retval 成功返回true，失败返回false.
     *\note return false when interupt by singal.
     */
    inline bool Unlock(int iOffset, int iLen = 1, int iWhence = SEEK_SET) {
        return Fcntl(F_SETLK, F_UNLCK, iOffset, iLen, iWhence);
    }

    /*!
     *\brief Unlock()大文件版本.
     *
     */
    bool Unlock(uint64_t iOffset, uint64_t iLen = 1LL, int iWhence = SEEK_SET);

    /*!
     *\brief 检测锁文件是否被opened.
     *
     */
    inline bool IsOpened() {
        return m_iFd != -1;
    }
};

}
