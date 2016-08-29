/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include <iostream>
#include <string>
#include <inttypes.h>
#include <cstring>
#include <mysql.h>
#include <signal.h>

#include "iLog.h"
#include "phxcoroutine.h"
#include "io_routine.h"
#include "phxsqlproxyconfig.h"
#include "MmConcurrent.h"

using namespace std;
using namespace mm;
using namespace phxsqlproxy;

void co_init_curr_thread_env();
//void RoutineSetSpecificCallback( RoutineGetSpecificFunc_t pfnGet,RoutineSetSpecificFunc_t set );

class TestPHXSqlProxyRoutine : public phxsqlproxy::Coroutine {
 public:
    TestPHXSqlProxyRoutine(phxsqlproxy::PHXSqlProxyConfig * config, int thread_idx, int routine_idx) {
        config_ = config;
        thread_idx_ = thread_idx;
        routine_idx_ = routine_idx;
    }

    virtual ~TestPHXSqlProxyRoutine() {
    }

    int run() {
        co_enable_hook_sys();

        MYSQL my;
        memset(&my, 0, sizeof(my));
        MYSQL * mysql_connection = mysql_init(&my);

        const int conn_timeout = 200;
        const int read_timeout = 200;
        const int write_timeout = read_timeout;

        //mysql_options( mysql_connection, MYSQL_OPT_CONNECT_TIMEOUT, (const char *)&conn_timeout );
        //mysql_options( mysql_connection, MYSQL_OPT_READ_TIMEOUT, (const char *)&read_timeout );
        //mysql_options( mysql_connection, MYSQL_OPT_WRITE_TIMEOUT, (const char *)&write_timeout );

        //uint64_t s = gt();
        printf("connect %s:%d\n", config_->GetListeip(), config_->GetListenPort());
        mysql_connection = mysql_real_connect(mysql_connection, config_->GetListeip(), "root", "", "abc",
                                              config_->GetListenPort(), NULL, 0ul);
        //uint64_t e = gt();
        //printf("end connect used %llu ms (%s)\n",e - s,strerror( errno ) );
        printf("conn ret %p\n", mysql_connection);

        if (!mysql_connection) {
            printf("conn failed\n");
            yield();
        }
        int query_count = 0;
        while (true) {
            query_count++;
            int ret = mysql_ping(mysql_connection);
            if (ret != 0) {
                printf("ping %d failed, ret %d\n", query_count, ret);
                break;
            }
            if (query_count % 100 == 0) {
                printf("ping %d\n", query_count);
            }
            poll(0, 0, 1000);
        }
        mysql_close(mysql_connection);
        yield();
        //for( int i = 0; i < query_count; ++i )
        //{
        //	//std::string sql_string = "/* phxsql_no_master_required */select * from t1;";
        //	std::string sql_string = "/* */select * from t1;";
        //	//s = gt();
        //	int ret = mysql_real_query( mysql_connection, sql_string.c_str(), sql_string.size() );
        //	//e = gt();
        //	if ( ret == 0 )
        //	{
        //		MYSQL_RES * res = mysql_store_result( mysql_connection );

        //		int rows = -1;
        //		if( res )
        //		{
        //			rows = mysql_num_rows( res );
        //			mysql_free_result( res );
        //		}
        //		if (i % 10 == 0)
        //		{
        //			printf("%d fetch rows %d\n",i, rows);
        //		}
        //	}
        //	else
        //	{
        //		printf("%d query ret %d, used (%s)\n", i, ret, strerror(errno)) ;
        //	}
        //}
        while (1) {
            poll(0, 0, 10000);
        }
        return 0;
    }

 private:
    phxsqlproxy::PHXSqlProxyConfig * config_;

    int thread_idx_;

    int routine_idx_;
};

class TestPHXSqlProxyThread : public mm::Thread {
 public:
    TestPHXSqlProxyThread(phxsqlproxy::PHXSqlProxyConfig * config, int thread_idx, int routine_num) {
        config_ = config;
        thread_idx_ = thread_idx;
        routine_num_ = routine_num;
    }

    virtual ~TestPHXSqlProxyThread() {
    }

    void run() {
        co_init_curr_thread_env();
        for (int i = 0; i < routine_num_; ++i) {
            TestPHXSqlProxyRoutine * routine = new TestPHXSqlProxyRoutine(config_, thread_idx_, i);
            routine->start();
        }
        co_eventloop(co_get_epoll_ct(), 0, 0);
    }

 private:
    phxsqlproxy::PHXSqlProxyConfig * config_;

    int thread_idx_;

    int routine_num_;
};

void showUsage(int argc, char** argv) {
    printf("%s <config> <threadnum> <routinenum>\n", argv[0]);
}

int main(int argc, char ** argv) {
    //RoutineSetSpecificCallback(co_getspecific, co_setspecific);
    if (argc < 4) {
        showUsage(argc, argv);
        exit(-1);
    }

    signal(SIGPIPE, SIG_IGN);
    phxsqlproxy::PHXSqlProxyConfig config;
    if (config.ReadConfig(argv[1]) != 0) {
        printf("ReadConfig [%s] failed\n", argv[1]);
        showUsage(argc, argv);
        exit(-1);
    }

    int thread_num = atoi(argv[2]);
    int routine_num = atoi(argv[3]);

    for (int i = 0; i < thread_num; ++i) {
        TestPHXSqlProxyThread * thread = new TestPHXSqlProxyThread(&config, i, routine_num);
        thread->start();
    }
    co_eventloop(co_get_epoll_ct(), 0, 0);
    return 0;
}


