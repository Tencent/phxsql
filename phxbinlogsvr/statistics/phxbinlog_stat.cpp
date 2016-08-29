/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "phxbinlogsvr/statistics/phxbinlog_stat.h"

namespace phxbinlog {
PhxBinlogStat * PhxBinlogStatFactory::phxsql_stat_ = new PhxBinlogStat();
PhxBinlogPaxosStat * PhxBinlogStatFactory::phxsql_paxos_stat_ = new PhxBinlogPaxosStat();

PhxBinlogStat * PhxBinlogStatFactory::GetPhxBinlogStat() {
    return phxsql_stat_;
}
PhxBinlogPaxosStat * PhxBinlogStatFactory::GetPhxBinlogPaxosStat() {
    return phxsql_paxos_stat_;
}
void PhxBinlogStatFactory::SetPhxBinlogStat(PhxBinlogStat * phxsql_stat) {
    if (phxsql_stat_)
        delete phxsql_stat_;
    phxsql_stat_ = phxsql_stat;
}
void PhxBinlogStatFactory::SetPhxBinlogPaxosStat(PhxBinlogPaxosStat * phxsql_paxos_stat) {
    if (phxsql_paxos_stat_)
        delete phxsql_paxos_stat_;
    phxsql_paxos_stat_ = phxsql_paxos_stat;
}

}
;

