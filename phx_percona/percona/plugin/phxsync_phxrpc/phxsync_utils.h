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

#include <sql_class.h>
#include <log.h>
#include <replication.h>

int repl_phx_read_binlog_buffer(const char * log_file_name, my_off_t begin_offset, my_off_t end_offset,
                                std::string * binlog_buffer);

int repl_phx_report_binlog_to_binlogsvr(Binlog_storage_param *param, const char * dir_path,
                                        const char * prev_log_file_name, my_off_t prev_log_pos,
                                        const char *log_file_name, my_off_t log_pos);

int repl_phx_report_binlog_before_recovery(Binlog_storage_param *param, const char * server_uuid,
                                           PSI_file_key * key_file_binlog_index, const char * log_bin_index);

int GetFuncTime();
