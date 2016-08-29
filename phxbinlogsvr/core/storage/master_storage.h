/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include "masterinfo.pb.h"

namespace phxbinlog {

class Option;
class MasterStorage {
 public:
    int SetMaster(const MasterInfo &info);
    void GetMaster(MasterInfo *info);

    bool CheckMasterBySvrID(const uint32_t &svrid, const bool check_timeout = true);
    bool IsTimeOut(const MasterInfo &info);

    uint64_t GetLastInstanceID();

    int SetMySqlAdminInfo(const MasterInfo &master_info, const std::string &username, const std::string &pwd);
    int SetMySqlReplicaInfo(const MasterInfo &master_info, const std::string &username, const std::string &pwd);

    std::string GetAdminUserName();
    std::string GetAdminPwd();
    std::string GetReplicaUserName();
    std::string GetReplicaPwd();

    bool HasAdminUserName();
    bool HasReplicaUserName();

 protected:
    MasterStorage(const Option *option);
    ~MasterStorage();

    int LoadFromCheckPoint(const MasterInfo &info);
    friend class StorageManager;

 private:
    const Option *option_;
    pthread_rwlock_t mutex_;
    MasterInfo master_;

    std::string admin_username_, admin_pwd_;
    std::string replica_username_, replica_pwd_;
};

}
