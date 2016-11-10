/*
 Copyright 2016 Tencent Inc.

 Everyone is permitted to copy and distribute verbatim copies of this license document, but changing it is not allowed.

 See the AUTHORS file for names of contributors.
 */

#include "phxcomm/phx_glog.h"
#include "phxcomm/phx_log_def.h"

#include <glog/logging.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>

using namespace std;

namespace phxsql {


static int g_min_log_level = 1024;

#define LOG_STR( type, str ) LOG(type) << str;

void PhxGLog::OpenLog(const char *module_name, const int &log_level, const char *path,
                      const uint32_t &log_file_max_size) {

    g_min_log_level = log_level;

    google::InitGoogleLogging(module_name);
    FLAGS_log_dir = path;
    FLAGS_stderrthreshold = google::FATAL;

    //GLOG_INFO = 0, GLOG_WARNING = 1, GLOG_ERROR = 2, GLOG_FATAL = 3,
    switch (log_level) {
        case LogLevel_Verbose:
        case LogLevel_Info:
            FLAGS_minloglevel = google::INFO;
            break;
        case LogLevel_Warning:
            FLAGS_minloglevel = google::WARNING;
            break;
        case LogLevel_Error:
            FLAGS_minloglevel = google::ERROR;
            break;
    }
}

void PhxGLog::Log(int level, const char *format, va_list args) {
    if ( level > g_min_log_level ) {
        return ;
    }

    char log_buffer[1024] = {0};
    vsnprintf(log_buffer, sizeof(log_buffer), format, args);

    switch (level) {
        case LogLevel_Warning:
            LogDebug(log_buffer);
            break;
        case LogLevel_Error:
            LogErr(log_buffer);
            break;
        case LogLevel_Verbose:
        case LogLevel_Info:
            LogInfo(log_buffer);
            break;
    };
}

void PhxGLog::LogDebug(const char *log_str) {
    LOG_STR(WARNING, log_str);
}

void PhxGLog::LogErr(const char *log_str) {
    LOG_STR(ERROR, log_str);
}

void PhxGLog::LogImpt(const char *log_str) {
    LOG_STR(INFO, log_str);
}

void PhxGLog::LogInfo(const char *log_str) {
    LOG_STR(INFO, log_str);
}

}
;

