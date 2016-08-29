/*
 Copyright 2016 Tencent Inc.

 Everyone is permitted to copy and distribute verbatim copies of this license document, but changing it is not allowed.

 See the AUTHORS file for names of contributors.
 */

#pragma once

#include <stdarg.h>
#include <stdint.h>
#include <string>

namespace phxsql {

class PhxGLog {
 public:
    PhxGLog() {
    }
    ~PhxGLog() {
    }

 public:
    static void OpenLog(const char *module_name, const int &log_level, const char *path,
                        const uint32_t &log_file_max_size);

    static void Log(int level, const char *format, va_list args);

 private:
    static void LogDebug(const char *log_svr);
    static void LogErr(const char *log_svr);
    static void LogImpt(const char *log_svr);
    static void LogInfo(const char *log_svr);

};

}
