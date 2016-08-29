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

#pragma once

#include <vector>
#include <string>
#include "phxsync_utils.h"

//#include "mysql.h"

class Sid_map;
struct Gtid;
struct TNM2Monitor;
class Format_description_log_event;

enum en_find_gtid_ret {
    FOUND,
    NOT_FOUND
};

class BinlogGtidState {
 public:

    virtual ~BinlogGtidState();

    int truncate_while_init(const char * server_uuid, PSI_file_key & key_file_binlog_index,
                            const char * binlog_index_file_name);

 public:
    static BinlogGtidState * instance();

    void set_last_gtid_in_binlog(const std::string & last_gtid_in_binlog);

    std::string get_last_gtid_in_binlog();

 private:
    /*do not permit to new another BinlogGtidState;*/
    BinlogGtidState();

    int get_last_gtid_in_binlogsvr(const char * serer_uuid, std::string & last_commit_gtid);

    int get_binlog_file_list(PSI_file_key & key_file_binlog_index, const char * binlog_index_file_name,
                             std::vector<std::string> & binlog_filename_vector);

    int truncate_by_gtid(const std::vector<std::string> & binlog_filename, size_t begin_index, Sid_map * sid_map,
                         const Gtid & last_gtid, std::vector<std::string> & new_binlog_filename);

    //find the first gtid that sidno == last_gtid.sidno, then delete
    en_find_gtid_ret is_gtid_in_binlog_file(Sid_map * sid_map, const char * binlog_file_name);

    int truncate_one(const char * binlog_file_name, Sid_map * sid_map, const Gtid & last_gtid);

    int generate_new_index_file(const char * binlog_index_file_name,
                                const std::vector<std::string> & binlog_filename_list);

 private:
    std::string m_last_gtid_in_binlog;
};

