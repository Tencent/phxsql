/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include "phxcomm/phx_log.h"

#include <stdio.h>
#include <string>
#include <string.h>

using namespace std;

namespace phxsql {

LogFunc g_logfunc = NULL;

#define IMPL_WITH_VALIST( level, fmt )\
\
	va_list args;\
	va_start ( args, fmt );\
\
	if( g_logfunc ) g_logfunc( level, fmt, args );\
\
	va_end ( args );

void LogWarning(const char *format, ...) {
    IMPL_WITH_VALIST(LogLevel_Warning, format);
}

void LogError(const char *format, ...) {
    IMPL_WITH_VALIST(LogLevel_Error, format);
}

void LogVerbose(const char *format, ...) {
    IMPL_WITH_VALIST(LogLevel_Verbose, format);
}

void LogInfo(const char *sFormat, ...) {
    IMPL_WITH_VALIST(LogLevel_Info, sFormat);
}

void ColorLogError(const char *format, ...) {
    char log_buff[1024];

    string newFormat = "\033[42;44m " + string(format) + " \033[0m";
    va_list args;
    va_start(args, format);
    vsnprintf(log_buff, sizeof(log_buff), newFormat.c_str(), args);
    va_end(args);

    LogError("%s", log_buff);
}

void ColorLogInfo(const char *format, ...) {
    char log_buff[1024];

    string newFormat = "\033[46;34m " + string(format) + " \033[0m";

    va_list args;
    va_start(args, format);
    vsnprintf(log_buff, sizeof(log_buff), newFormat.c_str(), args);
    va_end(args);

    LogInfo("%s", log_buff);
}

void ColorLogWarning(const char *format, ...) {
    char log_buff[1024];

    string newFormat = "\033[47;31m " + string(format) + " \033[0m";

    va_list args;
    va_start(args, format);
    vsnprintf(log_buff, sizeof(log_buff), newFormat.c_str(), args);
    va_end(args);

    LogError("%s", log_buff);
}

void ResigterLogFunc(LogFunc logfunc) {
    g_logfunc = logfunc;
}

LogFunc GetLogFunc() {
    return g_logfunc;
}

}

