/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include <cstdlib>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "co_routine.h"
#include "phxsqlproxyutil.h"
#include "phxcoroutine.h"

#include "phxcomm/phx_log.h"

using phxsql::LogWarning;

namespace phxsqlproxy {

static int g_routine_id = 0;

static void *RoutineRun(void * p) {
    g_routine_id++;
    LogWarning("routine %d running....", g_routine_id);
    //phoenix::SetLogWorkerIdx( g_routine_id );

    Coroutine * routine = (Coroutine *) p;
    routine->SetRoutineIdx(g_routine_id);
    int ret = routine->run();
    if (ret) {
        //do something
    }
    return NULL;
}

Coroutine::Coroutine() {
    routine_ = NULL;
    routine_idx_ = 0;
}

Coroutine::~Coroutine() {
}

void Coroutine::SetRoutineIdx(int routine_idx) {
    routine_idx_ = routine_idx;
}

int Coroutine::start() {
    co_create(&routine_, NULL, RoutineRun, this);
    resume();
    assert(routine_ != NULL);
    return 0;
}

void Coroutine::resume() {
    co_resume(routine_);
}

void Coroutine::yield() {
    co_yield_ct();
}

int Coroutine::RoutineConnectWithTimeout(int fd, const struct sockaddr *address, socklen_t address_len,
                                         int timeout_ms) {
    assert(IsNonBlock(fd));
    //1.sys call
    int ret = connect(fd, address, address_len);

    //2.wait
    int pollret = 0;
    struct pollfd pf = { 0 };

    for (int i = 0; i < 3; i++)  //25s * 3 = 75s
            {
        memset(&pf, 0, sizeof(pf));
        pf.fd = fd;
        pf.events = (POLLOUT | POLLERR | POLLHUP);

        pollret = poll(&pf, 1, timeout_ms);

        if (1 == pollret) {
            break;
        }
    }

    if (pf.revents & POLLOUT)  //connect succ
    {
        errno = 0;
        return 0;
    }

    //3.set errno
    int err = 0;
    socklen_t errlen = sizeof(err);
    getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &errlen);
    if (err) {
        errno = err;
    } else {
        errno = ETIMEDOUT;
    }
    return ret;
}

int Coroutine::RoutineReadWithTimeout(int source_fd, char * buf, int buf_size, int timeout_ms) {
    assert(IsNonBlock(source_fd));
    struct pollfd pf[1];
    int nfds = 0;
    memset(pf, 0, sizeof(pf));
    pf[0].fd = source_fd;
    pf[0].events = (POLLIN | POLLERR | POLLHUP);
    nfds++;

    int return_fd_count = co_poll(co_get_epoll_ct(), pf, nfds, timeout_ms);
    if (return_fd_count < 0) {
        return return_fd_count;
    }

    if (pf[0].revents & POLLIN) {
        return read(source_fd, buf, buf_size);
    } else if (pf[0].revents & POLLHUP) {
        return return_fd_count;
    } else if (pf[0].revents & POLLERR) {
        return return_fd_count;
    }
    return return_fd_count;
}

int Coroutine::RoutineWriteWithTimeout(int dest_fd, const char * buf, int write_size, int timeout_ms) {
    assert(IsNonBlock(dest_fd));
    int written_once = 0;

    written_once = write(dest_fd, buf, write_size);
    if (written_once > 0) {
        return written_once;
    }

    if (written_once <= 0) {
        if (errno == EAGAIN || errno == EINTR) {
            written_once = 0;
        } else {
            return -__LINE__;
        }
    }

    struct pollfd pf[1];
    int nfds = 0;
    memset(pf, 0, sizeof(pf));
    pf[0].fd = dest_fd;
    pf[0].events = (POLLOUT | POLLERR | POLLHUP);
    nfds++;

    int return_fd_count = co_poll(co_get_epoll_ct(), pf, nfds, timeout_ms);
    if (return_fd_count < 0) {
        return return_fd_count;
    }

    if (pf[0].revents & POLLOUT) {
        written_once = write(dest_fd, buf, write_size);
        if (written_once <= 0) {
            if (errno == EAGAIN || errno == EINTR) {
                written_once = 0;
            } else {
                return written_once;
            }
        }
    } else if (pf[0].revents & POLLHUP) {
        return return_fd_count;
    } else if (pf[0].revents & POLLERR) {
        return return_fd_count;
    }
    return written_once;
}

}

