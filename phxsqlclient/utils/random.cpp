/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include <stdlib.h>
#include <inttypes.h>
#include <cassert>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>
#include <limits.h>
#include <string.h>
#include <pthread.h>
#include "random.h"

namespace phxsql {

#ifdef __i386

__inline__ uint64_t rdtsc()
{
    uint64_t x;
    __asm__ volatile ("rdtsc" : "=A" (x));
    return x;
}

#elif __amd64

__inline__ uint64_t rdtsc()
{

    uint64_t a, d;
    __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
    return (d<<32) | a;
}

#endif

struct FastRandomSeed {
    bool init;
    unsigned int seed;
};

static __thread FastRandomSeed seed_routine_safe = { false, 0 };

static void ResetFastRandomSeed() {
    seed_routine_safe.seed = rdtsc();
    seed_routine_safe.init = true;
}

static void InitFastRandomSeedAtFork() {
    pthread_atfork(ResetFastRandomSeed, ResetFastRandomSeed, ResetFastRandomSeed);
}

static void InitFastRandomSeed() {
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, InitFastRandomSeedAtFork);

    ResetFastRandomSeed();
}

unsigned int GetFastRandom() {
    if (!seed_routine_safe.init) {
        InitFastRandomSeed();
    }

    return rand_r(&seed_routine_safe.seed);
}

unsigned int GetNextRandom() {
    return GetFastRandom();
}

bool CheckByRatio(unsigned int base, unsigned int ratio) {
    if (base == 0 || ratio == 0) {
        return false;
    } else if (ratio >= base) {
        return true;
    }

    unsigned int n = GetFastRandom() % base;
    return n < ratio;
}

unsigned int RandomPicker(unsigned int maxVal) {
    return RandomPicker(0, maxVal);
}

unsigned int RandomPicker(unsigned int minVal, unsigned int maxVal) {
    if (maxVal == minVal) {
        return maxVal;
    } else if (maxVal < minVal) {
        unsigned int tmp = minVal;
        minVal = maxVal;
        maxVal = tmp;
    }

    unsigned int n = GetFastRandom() % (maxVal - minVal);
    return n + minVal;
}

unsigned int GetRandomUin() {
    return RandomPicker(10000, UINT_MAX);
}

unsigned int GetRandomUin(unsigned int minUin, unsigned int maxUin) {
    return RandomPicker(minUin, maxUin);
}

}


