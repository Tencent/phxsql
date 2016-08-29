/*
Tencent is pleased to support the open source community by making 
PhxSQL available.
Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights 
reserved.
Licensed under the GNU General Public License, Version 2.0 (the 
"License"); you may not use this file except in compliance with 
the License. You may obtain a copy of the License at
https://opensource.org/licenses/GPL-2.0

See the AUTHORS file for names of contributors.

*/

#include <string>

#include <sql_class.h>
#include <log.h>
#include <replication.h>
#include "phxsync_utils.h"
#include "init_with_phxbinlogsvr.h"

#include "phxbinlogsvr/client/phxbinlog_client_factory_interface.h"
#include "phxbinlogsvr/client/phxbinlog_client_platform_info.h"
#include "phxbinlogsvr/core/gtid_handler.h"
#include "init_with_phxbinlogsvr.h"

using std::string;
using phxbinlogsvr::PhxBinlogClientPlatformInfo;
using phxbinlogsvr::PhxBinLogClientFactoryInterface;
using phxbinlogsvr::PhxBinlogClient;
using phxbinlog::GtidHandler;

int GetFuncTime() {
    struct timeval tNow;

    int septime = 0;

    gettimeofday(&tNow, NULL);
    septime = 1000000 * tNow.tv_sec + tNow.tv_usec;
    septime /= 1000;

    return septime;
}

extern File open_binlog_file(IO_CACHE *log, const char *log_file_name, const char **errmsg);

class BinlogGtidState;
typedef struct st_mysql_show_var SHOW_VAR;
typedef struct st_mysql_sys_var SYS_VAR;

int repl_phx_read_binlog_buffer(const char * log_file_name, my_off_t begin_offset, my_off_t end_offset,
                                std::string * binlog_buffer) {
    //open cache
    const char * errmsg = 0;
    IO_CACHE read_cache;
    File file = open_binlog_file(&read_cache, log_file_name, &errmsg);
    if (file < 0) {
        return -__LINE__;
    }

    //seek
    my_b_seek(&read_cache, begin_offset);
    if (end_offset == (my_off_t) - 1) {
        end_offset = my_b_filelength(&read_cache);
    }

    //read
    my_off_t read_size = end_offset - begin_offset;
    uchar * buffer = (uchar *) my_malloc(sizeof(uchar *) * (read_size + 1), MYF(MY_WME));
    my_b_read(&read_cache, buffer, read_size);

    binlog_buffer->append((char *) buffer, (size_t) read_size);

    //gc
    my_free(buffer);
    end_io_cache(&read_cache);
    mysql_file_close(file, MYF(MY_WME));
    return 0;
}

int repl_phx_report_binlog_before_recovery(Binlog_storage_param *param, const char * server_uuid,
                                           PSI_file_key * key_file_binlog_index, const char * log_bin_index) {
    BinlogGtidState * binlog_gtid_state = BinlogGtidState::instance();
    int truncate_binlog_ret = 0;
    if ((truncate_binlog_ret = binlog_gtid_state->truncate_while_init(server_uuid, *key_file_binlog_index, log_bin_index))) {
        sql_print_information("truncate_binlog_ret %d", truncate_binlog_ret);
        unireg_abort(1);
    }
    sql_print_information("%s:%d server_uuid [%s] log_bin_index [%s] ret %d", __func__, __LINE__, server_uuid,
                          log_bin_index, truncate_binlog_ret);
    return truncate_binlog_ret;
}

int repl_phx_report_binlog_to_binlogsvr(Binlog_storage_param *param, const char * dir_path,
                                        const char * prev_log_file_name, my_off_t prev_log_pos,
                                        const char *log_file_name, my_off_t log_pos) {
    /*
     if ( !get_enable_phxsync_plugin() )
     {
     return 0;
     }
     */

    std::string prev_log_file = std::string(dir_path) + "/" + std::string(prev_log_file_name);
    std::string log_file = std::string(dir_path) + "/" + std::string(log_file_name);
    std::string binlog_buffer;
    int ret = 0;

    if (strcmp(prev_log_file_name, log_file_name) == 0) {
        ret = repl_phx_read_binlog_buffer(prev_log_file.c_str(), prev_log_pos, log_pos, &binlog_buffer);
    } else {
        ret = repl_phx_read_binlog_buffer(prev_log_file.c_str(), prev_log_pos, (my_off_t) - 1, &binlog_buffer);
        ret |= repl_phx_read_binlog_buffer(log_file.c_str(), (my_off_t) 0, log_pos, &binlog_buffer);
    }

    if (ret != 0) {
        sql_print_information(
                "%s:%d read binlog buffer failed, prev_log_file [%s], prev_log_pos [%d], log_file [%s], log_pos [%d]",
                __func__, __LINE__, prev_log_file_name, (int) prev_log_pos, log_file_name, (int) log_pos);
        exit(-1);
        return -__LINE__;
    }

    std::string sMaxGtid;
    std::string sServerUuid(server_uuid);
    std::string sMaxGtidInBinlog = BinlogGtidState::instance()->get_last_gtid_in_binlog();
    ret = GtidHandler::ParseEventList(binlog_buffer, NULL, false, &sMaxGtid);

    if (ret || sMaxGtid.size() < sServerUuid.size()) {
        sql_print_information(
                "%s:%d parse event list failed, prev_log_file [%s], prev_log_pos [%d], log_file [%s], log_pos [%d], sMaxGtid [%s] sServerUuid [%s] ret %d",
                __func__, __LINE__, prev_log_file_name, (int) prev_log_pos, log_file_name, (int) log_pos,
                sMaxGtid.c_str(), sServerUuid.c_str(), ret);
        exit(-1);
        return -__LINE__;
    }

    static int32_t debug_print = 0;
    debug_print++;
    if (debug_print % 10000 == 0) {
        debug_print = 0;
        sql_print_information(
                "%s:%d parse event list, prev_log_file [%s], prev_log_pos [%d], log_file [%s], log_pos [%d], sMaxGtid [%s] sServerUuid [%s] max_gtid_in_binlog [%s] super_read_only %d ret %d",
                __func__, __LINE__, prev_log_file_name, (int) prev_log_pos, log_file_name, (int) log_pos,
                sMaxGtid.c_str(), sServerUuid.c_str(), sMaxGtidInBinlog.c_str(), super_read_only, ret);
    }

    if (super_read_only == 1)
    //sMaxGtid.substr( 0, sServerUuid.size() ) != sServerUuid )
            {
        //sql_print_information("%s:%d no need to report, prev_log_file [%s], prev_log_pos [%d], log_file [%s], log_pos [%d], sMaxGtid [%s] sServerUuid [%s]",
        //		__func__, __LINE__, prev_log_file_name, (int)prev_log_pos, log_file_name, (int)log_pos,
        //		sMaxGtid.c_str(), sServerUuid.c_str() );
    } else {
        int pre_clock = GetFuncTime();
        PhxBinLogClientFactoryInterface * client_factor = PhxBinlogClientPlatformInfo::GetDefault()->GetClientFactory();
        std::shared_ptr<PhxBinlogClient> client = client_factor->CreateClient();

        while (true) {
            ret = client->SendBinLog(sMaxGtidInBinlog, binlog_buffer);
            if (ret != 0) {
                sql_print_information(
                        " %s:%d send binlog buffer failed, prev_log_file [ %s ], prev_log_pos [ %d ], log_file [ %s ], log_pos [ %d ], ret %d",
                        __func__, __LINE__, prev_log_file_name, (int) prev_log_pos, log_file_name, (int) log_pos, ret);
            }
            int cur_clock = GetFuncTime();
            if (cur_clock - pre_clock > 30) {
                sql_print_information(" %s:%d send binlog use %d ms, max gtid %s binlog gtid %s", __func__, __LINE__,
                                      cur_clock - pre_clock, sMaxGtid.c_str(), sMaxGtidInBinlog.c_str());
            }
            if (ret == 3000)
                ret = 0;
            if (ret >= 0) {
                break;
            }
        }

        if (ret) {
            sql_print_information(
                    "%s:%d send binlog buffer failed, prev_log_file [%s], prev_log_pos [%d], log_file [%s], log_pos [%d] maxgtid [%s] my maxgtid [%s] ret %d",
                    __func__, __LINE__, prev_log_file_name, (int) prev_log_pos, log_file_name, (int) log_pos,
                    sMaxGtidInBinlog.c_str(), sMaxGtid.c_str(), ret);
            exit(-1);
        }
    }
    BinlogGtidState::instance()->set_last_gtid_in_binlog(sMaxGtid);
    return ret;
}

