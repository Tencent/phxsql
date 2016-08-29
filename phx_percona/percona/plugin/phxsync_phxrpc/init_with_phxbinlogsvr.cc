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

#include <vector>
#include <string>
#include <net/if.h>
#include <sys/socket.h>

#include "phxbinlogsvr/client/phxbinlog_client_factory_phxrpc.h"
#include "phxbinlogsvr/client/phxbinlog_client_platform_info.h"
#include "phxbinlogsvr/config/phxbinlog_config.h" 

#include "phxcomm/phx_log.h"

#include "rpl_gtid.h"
#include "init_with_phxbinlogsvr.h"
#include "sql_priv.h"
#include "mysql/psi/mysql_file.h"
#include "binlog.h"

using std::string;
using std::vector;

using phxbinlogsvr::PhxBinlogClientPlatformInfo;
using phxbinlogsvr::PhxBinlogClient;
using phxbinlogsvr::PhxBinLogClientFactoryInterface;

BinlogGtidState::BinlogGtidState() {
}

BinlogGtidState::~BinlogGtidState() {
}

en_find_gtid_ret BinlogGtidState::is_gtid_in_binlog_file(Sid_map * sid_map, const char * binlog_file_name) {
    Gtid_set prev_gtids(sid_map);
    enum_read_gtids_from_binlog_status read_ret = read_gtids_from_binlog(binlog_file_name, NULL, &prev_gtids, NULL,
                                                                         NULL, sid_map, 0);

    sql_print_information("%s read [%s] ret %d", __func__, binlog_file_name, read_ret);
    return read_ret == GOT_GTIDS ? FOUND : NOT_FOUND;
}

int BinlogGtidState::truncate_while_init(const char * server_uuid, PSI_file_key & key_file_binlog_index,
                                         const char * binlog_index_file_name) {
    if (server_uuid == NULL || server_uuid[0] == '\0') {
        sql_print_information("server_uuid [%s] is empty, no need to truncate", server_uuid);
        return 0;
    }

    std::string last_commit_gtid;
    std::vector < string > binlog_filename_vector;

    int ret = 0;
    do {
        //step 1 : get last gtid in paxos
        if ((ret = get_last_gtid_in_binlogsvr(server_uuid, last_commit_gtid))) {
            sql_print_information("get gtid failed, serverid [%s] ret %d", server_uuid, ret);
            return -__LINE__;
        }

        Sid_map sid_map(NULL);
        Gtid last_gtid;

        sql_print_information("get last send gtid svrid [%s] ret %d, last gtid [%s]", server_uuid, ret,
                              last_commit_gtid.c_str());
        if (last_commit_gtid == "") {
            last_commit_gtid = string(server_uuid) + ":1";
            enum_return_status parse_ret = last_gtid.parse(&sid_map, last_commit_gtid.c_str());
            if (parse_ret != RETURN_STATUS_OK) {
                sql_print_information("parse gtid [%s] failed, ret %d", last_commit_gtid.c_str(), parse_ret);
                return -__LINE__;
            }
        } else {
            enum_return_status parse_ret = last_gtid.parse(&sid_map, last_commit_gtid.c_str());
            if (parse_ret != RETURN_STATUS_OK) {
                sql_print_information("parse gtid [%s] failed, ret %d", last_commit_gtid.c_str(), parse_ret);
                return -__LINE__;
            }
            last_gtid.gno += 1;
        }

        //step 2 : find all binlog files
        if ((ret = get_binlog_file_list(key_file_binlog_index, binlog_index_file_name, binlog_filename_vector))) {
            sql_print_information("get_binlog_file_list [%s] failed, ret %d", binlog_index_file_name, ret);
            ret = -__LINE__;
            break;
        }

        //step 3 : truncate binlog files
        en_find_gtid_ret find_ret = NOT_FOUND;
        vector < std::string > new_binlog_filename_vector;
        for (size_t i = binlog_filename_vector.size(); i != 0; --i) {
            find_ret = is_gtid_in_binlog_file(&sid_map, binlog_filename_vector[i - 1].c_str());
            if (find_ret == FOUND) {
                ret = truncate_by_gtid(binlog_filename_vector, i - 1, &sid_map, last_gtid, new_binlog_filename_vector);
                break;
            }
        }
        if (ret != 0) {
            sql_print_information("truncate_by_gtid failed, ret %d", ret);
            ret = -__LINE__;
            break;
        }

        //step 4 : generate new index file
        if (new_binlog_filename_vector.size() != binlog_filename_vector.size()
                && new_binlog_filename_vector.size() != 0) {
            ret = generate_new_index_file(binlog_index_file_name, new_binlog_filename_vector);
        }
    } while (false);

    sql_print_information("%s ret %d, m_last_gtid_in_binlog %s ", __func__, ret, m_last_gtid_in_binlog.c_str());
    return ret;
}

BinlogGtidState * BinlogGtidState::instance() {
    static BinlogGtidState binlog_gtid_state;
    return &binlog_gtid_state;
}

void BinlogGtidState::set_last_gtid_in_binlog(const std::string & last_gtid_in_binlog) {
    m_last_gtid_in_binlog = last_gtid_in_binlog;
}

std::string BinlogGtidState::get_last_gtid_in_binlog() {
    return m_last_gtid_in_binlog;
}

int BinlogGtidState::get_last_gtid_in_binlogsvr(const char * server_uuid, std::string & last_commit_gtid) {
    if (server_uuid == NULL || strlen(server_uuid) <= 1) {
        last_commit_gtid = "";
        return 0;
    }

    int ret = -1;
    PhxBinLogClientFactoryInterface * client_factor = PhxBinlogClientPlatformInfo::GetDefault()->GetClientFactory();
    std::shared_ptr<PhxBinlogClient> client = client_factor->CreateClient();
    while (ret < 0) {
        ret = client->GetLastSendGtid(string(server_uuid), &last_commit_gtid);
        if (ret != 0) {
            sql_print_information("get last send gtid failed, svrid [%s] ret %d", server_uuid, ret);
            my_sleep(10 * 1000);
        }
    }
    sql_print_information("get last uuid %s done, last gtid [%s]", server_uuid, last_commit_gtid.c_str());
    return ret;
}

int BinlogGtidState::get_binlog_file_list(PSI_file_key & key_file_binlog_index, const char * binlog_index_file_name,
                                          std::vector<std::string> & binlog_filename_vector) {
    int ret = 0;
    File index_file_fd = 0;
    IO_CACHE index_file_cache;
    char fname[FN_REFLEN], full_log_name[FN_REFLEN];

    do {
        if ((index_file_fd = mysql_file_open(key_file_binlog_index, binlog_index_file_name, O_RDWR | O_BINARY,
                                             MYF(MY_WME))) <= 0) {
            ret = -__LINE__;
            break;
        }

        if (init_io_cache(&index_file_cache, index_file_fd, IO_SIZE, READ_CACHE, 0, 0, MYF(MY_WME))) {
            ret = -__LINE__;
            break;
        }

        int length = 0;
        while (true) {
            if ((length = my_b_gets(&index_file_cache, fname, FN_REFLEN)) <= 1) {
                break;
            }

            if (normalize_binlog_name(full_log_name, fname, 0)) {
                ret = -__LINE__;
                break;
            }
            full_log_name[strlen(full_log_name) - 1] = '\0';  //drop '\n' at last index;
            binlog_filename_vector.push_back(full_log_name);
        }
    } while (false);

    if (index_file_fd >= 0) {
        mysql_file_close(index_file_fd, MYF(0));
        end_io_cache(&index_file_cache);
    }
    return ret;
}

//retval 0 : ok
//retval != 0 : fail
int BinlogGtidState::truncate_by_gtid(const std::vector<std::string> & binlog_filename_vector, size_t begin_index,
                                      Sid_map * sid_map, const Gtid & last_gtid,
                                      std::vector<std::string> & new_binlog_filename_vector) {
    for (size_t i = 0; i < begin_index; ++i) {
        new_binlog_filename_vector.push_back(binlog_filename_vector[i]);
    }

    int truncate_one_ret = 0;
    int truncate_binlog_index = begin_index;
    for (size_t i = begin_index; i < binlog_filename_vector.size(); ++i) {
        new_binlog_filename_vector.push_back(binlog_filename_vector[i]);
        truncate_one_ret = truncate_one(binlog_filename_vector[i].c_str(), sid_map, last_gtid);
        if (truncate_one_ret <= 0) {
            truncate_binlog_index = i;
            break;
        }
    }

    sql_print_information("truncate from [%s], ret %d", binlog_filename_vector[truncate_binlog_index].c_str(),
                          truncate_one_ret);
    if (get_last_gtid_in_binlog() == "") {
        //try get last gtid in binlog if this binlog is empty
        for (size_t i = new_binlog_filename_vector.size(); i != 0; --i) {
            if (is_gtid_in_binlog_file(sid_map, new_binlog_filename_vector[i - 1].c_str()) == FOUND) {
                truncate_one(new_binlog_filename_vector[i - 1].c_str(), sid_map, last_gtid);
                break;
            }
        }
    }
    return truncate_one_ret >= 0 ? 0 : truncate_one_ret;
}

// retval 1 : need to continue find next file
// retval <= 0 : sth. wrong, eg. truncate() fail, parse binlog file fail
int BinlogGtidState::truncate_one(const char * binlog_file_name, Sid_map * sid_map, const Gtid & last_gtid) {
    int final_ret = 1;
    const char * errmsg = 0;
    IO_CACHE log;
    File file = open_binlog_file(&log, binlog_file_name, &errmsg);
    if (file < 0) {
        return -__LINE__;
    }

    Format_description_log_event fd_ev(BINLOG_VERSION), *fd_ev_p = &fd_ev;

    my_b_seek(&log, BIN_LOG_HEADER_SIZE);
    Log_event *ev = NULL;
    size_t file_length = 0;
    my_off_t last_pos = 0;

    while (final_ret) {
        last_pos = (log.read_pos - log.buffer) + log.pos_in_file;
        ev = Log_event::read_log_event(&log, 0, fd_ev_p, 1);
        if (ev == NULL) {
            sql_print_information("[%s] read event null", binlog_file_name);
            break;
        }
        switch (ev->get_type_code()) {
            case FORMAT_DESCRIPTION_EVENT: {
                if (fd_ev_p != &fd_ev)
                    delete fd_ev_p;
                fd_ev_p = (Format_description_log_event *) ev;
                break;
            }
            case GTID_LOG_EVENT: {
                Gtid_log_event *gtid_ev = (Gtid_log_event *) ev;
                rpl_sidno sidno = gtid_ev->get_sidno(sid_map);
                if (sidno == last_gtid.sidno && gtid_ev->get_gno() >= last_gtid.gno)  //trunk from this event
                        {
                    sql_print_information("%s:%d truncate sidno %u gtidno %d last_gtid.gno %d filepos %d", __func__,
                                          __LINE__, sidno, (int) (gtid_ev->get_gno()), (int) (last_gtid.gno),
                                          (int) last_pos);
                    file_length = last_pos;
                    final_ret = 0;
                } else {
                    Gtid gtid_from_ev;
                    gtid_from_ev.set(sidno, gtid_ev->get_gno());
                    char gtid_buf[512] = { 0 };
                    gtid_from_ev.to_string(sid_map, gtid_buf);
                    set_last_gtid_in_binlog(string(gtid_buf));
                }
                break;
            }
            default:
                break;
        }
        if (ev != NULL && ev != fd_ev_p)
            delete ev, ev = NULL;
    }

    if (fd_ev_p != &fd_ev) {
        delete fd_ev_p, fd_ev_p = NULL;
    }

    mysql_file_close(file, MYF(MY_WME));
    end_io_cache(&log);

    if (final_ret == 0) {
        final_ret = truncate(binlog_file_name, file_length);
        sql_print_information("truncate [%s] to file length %zu, truncate ret %d, errno[%d:%s]", binlog_file_name,
                              file_length, final_ret, errno, strerror(errno));
    }
    return final_ret;
}

int BinlogGtidState::generate_new_index_file(const char * binlog_index_file_name,
                                             const std::vector<std::string> & binlog_filename_list) {
    int result = 0;
    std::string tmp_index_file = string(binlog_index_file_name) + ".phxsync";
    File fd = my_open((const char *) tmp_index_file.c_str(), O_CREAT | O_RDWR | O_BINARY, MYF(MY_WME));
    if (fd < 0) {
        sql_print_error("Failed to create a tmp index file on %s, errno [%d:%s]", tmp_index_file.c_str(), my_errno,
                        strerror(my_errno));
        return -__LINE__;
    }

    IO_CACHE io_cache;
    if (init_io_cache(&io_cache, fd, IO_SIZE * 2, WRITE_CACHE, 0L, 0, MYF(MY_WME))) {
        sql_print_error("Failed to create a tmp index file on (file: %s', errno %d)", tmp_index_file.c_str(), my_errno);
        my_close(fd, MYF(MY_WME));
        return -__LINE__;
    }

    my_b_seek(&io_cache, 0L);
    for (size_t i = 0; i < binlog_filename_list.size(); ++i) {
        my_b_printf(&io_cache, "%s\n", binlog_filename_list[i].c_str());
    }
    if (flush_io_cache(&io_cache) || my_sync(fd, MYF(MY_WME)))
        result = 1;

    my_close(fd, MYF(MY_WME));
    end_io_cache(&io_cache);

    if (result == 0) {
        if (my_rename(tmp_index_file.c_str(), binlog_index_file_name, MYF(MY_WME))) {
            sql_print_error("Failed to rename index file on (file: %s', errno [%d])", tmp_index_file.c_str(), my_errno);
            result = 1;
        }
    }
    return result;
}

