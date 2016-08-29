/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

namespace phxbinlog {
enum {
    WRITE_FORBIDDON = -500,
    EVENT_FULL = -501,

    SLAVE_FAIL = -1000,
    MASTER_FAIL = -1001,
    PKG_FAIL = -1002,
    MASTER_PREPARING = -1003,

    //DATA_EMPTY = -2000,
    FILE_FAIL = -2001,
    INDEX_FAIL = -2002,

    PAXOS_FAIL = -2500,
    MYSQL_FAIL = -2600,

    SOCKET_FAIL = -3000,
    SOCKET_LOST = -3001,

    AGENT_INIT_FAIL = -5002,

    SVR_CONTINUE = -5,
    SVR_READ_FAIL = -4,
    SVR_SEND_FAIL = -3,
    SVR_CONNECT_FAIL = -2,
    SVR_FAIL = -1,
    //system error

    ////////////////////////////////

    OK = 0,
    DATA_EMPTY = 1,
    EVENT_EMPTY = 1,
    MYSQL_USER_NOT_EXIST = 1,
    DATA_EXIST = 2,

    REQ_INVALUD = 202,
    PAXOS_BUFFER_FAIL = 400,
    VERSION_CONFLICT = 401,
    BUFFER_FAIL = 500,
    GTID_FAIL = 501,
    GTID_EVENT_FAIL = 502,
    GTID_CONFLICT = 503,
    DATA_TOO_LARGE = 504,
    EVENT_SVR_ID_FAIL = 505,

    USERINFO_CHANGE_DENIED = 911,
    MASTER_INIT_FAIL = 912,
    EVENT_PARSE_FAIL = 1000,
    ARG_FAIL = 1001,

    MASTER_CONFLICT = 2000,
    SLAVE_CONLICT = 2001,
    SVR_ID_CONFIT = 2002,

    EVENT_EXIST = 3000,
    EVENT_NOT_EXIST = 3001,
    SPACE_ENOUGH = 3003,

    MASTER_VERSION_CONFLICT = 4000,
    MASTER_CHECK_FAIL = 4001,
    NOT_MASTER = 4002,

    AGENT_INITED = 5001,
    MYSQL_NODATA = 5002,
    DATA_ERR = 5003,  //something wrong in db data
    DATA_IN_FILE = 5004,

    FILE_FORBIDDEN = 8001,
    NO_FILES = 8002,

//logical error
////////////////////////////////////

};

enum {
    EVENT_HEARTBEAT = 27,
    EVENT_PREVIOUS_GTIDS = 35,
    EVENT_PERCONA_GTID = 33,
    EVENT_GTID = 162,
};

}
