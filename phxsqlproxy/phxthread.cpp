/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "phxthread.h"

#include <thread>

namespace phxsqlproxy {

PhxThread::PhxThread() :
        thr_(NULL) {
}

PhxThread::~PhxThread() {
    if (thr_)
        delete thr_;
}

void PhxThread::start() {
    thr_ = new std::thread(&PhxThread::run, this);
}

void PhxThread::join() {
    if (thr_)
        thr_->join();
}

void PhxThread::detach() {
    if (thr_)
        thr_->detach();
}

std::thread::id PhxThread::get_id() const {
    return std::this_thread::get_id();
}

std::thread & PhxThread::get_thread() {
    return *thr_;
}

const std::thread & PhxThread::get_thread() const {
    return (const std::thread &) (*thr_);
}

}
