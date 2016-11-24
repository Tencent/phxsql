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
#include "init_with_phxbinlogsvr.h"
#include "phxsync_utils.h"
#include "phxbinlogsvr/client/phxbinlog_client_factory_phxrpc.h"
#include "phxbinlogsvr/client/phxbinlog_client_platform_info.h"
#include "phxbinlogsvr/config/phxbinlog_config.h" 

#include "phxcomm/phx_log.h"
#include "phxcomm/phx_glog.h"

using std::string;
using phxbinlogsvr::PhxBinlogClientPlatformInfo;

void InitPhx() {
    const phxbinlog::PHXBinlogSvrBaseConfig * phxbinlogsvr_config =
            phxbinlog::Option::GetDefault()->GetBinLogSvrConfig();
    string log_path = phxbinlogsvr_config->GetLogFilePath();
    phxsql :: PhxGLog :: OpenLog("mysqld", phxbinlogsvr_config->GetLogLevel(), log_path.c_str(), phxbinlogsvr_config->GetLogMaxSize() );
    phxsql::ResigterLogFunc(phxsql::PhxGLog::Log);
    PhxBinlogClientPlatformInfo::GetDefault()->RegisterClientFactory(new PhxBinlogClientFactory_PhxRPC());
}

int repl_phx_report_binlog_after_flush(Binlog_storage_param *param, const char * dir_path,
                                       const char * prev_log_file_name, my_off_t prev_log_pos,
                                       const char *log_file_name, my_off_t log_pos) {
    return repl_phx_report_binlog_to_binlogsvr(param, dir_path, prev_log_file_name, prev_log_pos, log_file_name,
                                               log_pos);
}

Binlog_storage_observer storage_observer2 = { sizeof(Binlog_storage_observer),  // len
        repl_phx_report_binlog_after_flush,  // report_update
        repl_phx_report_binlog_before_recovery,  //before recovery
        };

static int phx_sync_master_plugin_init(void *p) {
    InitPhx();
    if (register_binlog_storage_observer(&storage_observer2, p))
        return 1;
    return 0;
}

static int phx_sync_master_plugin_deinit(void *p) {
    if (unregister_binlog_storage_observer(&storage_observer2, p)) {
        sql_print_error("unregister_binlog_storage_observer failed");
        return 1;
    }
    return 0;
}

struct Mysql_replication phx_sync_master_plugin = { MYSQL_REPLICATION_INTERFACE_VERSION };

//static SYS_VAR* phx_sync_master_system_vars[]= { NULL };
//static SHOW_VAR phx_sync_master_status_vars[]= {  {NULL, NULL, SHOW_LONG}, };

/*
 Plugin library descriptor
 */

mysql_declare_plugin (phxsync_master_phxrpc)
{
    MYSQL_REPLICATION_PLUGIN,
    &phx_sync_master_plugin,
    "rpl_phx_sync_master",
    "Mario Huang",
    "phx-synchronous replication",
    PLUGIN_LICENSE_GPL,
    phx_sync_master_plugin_init, /* Plugin Init */
    phx_sync_master_plugin_deinit, /* Plugin Deinit */
    0x0100 /* 1.0 */,
    NULL, /* status variables */
    NULL, /* system variables */
    //phx_sync_master_status_vars,
    //phx_sync_master_system_vars,
    NULL, /* config options */
    0, /* flags */
}
mysql_declare_plugin_end;
