/*
 *   Fly-With-Friends - X-Plane plugin for simple group flying
 *   Copyright (C) 2018 Folke Will <folko@solhost.org>
 *   Copyright (C) 2020 Michael Hasling <michael.hasling@gmail.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Affero General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Affero General Public License for more details.
 *
 *   You should have received a copy of the GNU Affero General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "logger.h"
#include "fwfporting.h"
#include <cstdarg>
#include <cstring>
#ifndef _MSC_VER
#include <libgen.h>
#else
static char* basename(char*);
#endif

namespace fwf {

ILogger * ILogger::theLogger = 0;

ILogger * ILogger::New(const char * logFilePath)
{
    if (!theLogger)
    {
        theLogger = new Logger(logFilePath);
    }
    return theLogger;
}

void ILogger::error(const char *file, const char *function, const int line, const char *format, ...)
{
    if (!theLogger) return;
    va_list args;
    va_start(args, format);
    theLogger->log('E', file, function, line, format, args);
    va_end(args);
}

void ILogger::warn(const char *file, const char *function, const int line, const char *format, ...)
{
    if (!theLogger) return;
    va_list args;
    va_start(args, format);
    theLogger->log('W', file, function, line, format, args);
    va_end(args);
}

void ILogger::info(bool enable, const char *file, const char *function, const int line, const char *format, ...)
{
    if (!enable) return;
    if (!theLogger) return;
    va_list args;
    va_start(args, format);
    theLogger->log('I', file, function, line, format, args);
    va_end(args);
}

void ILogger::verbose(bool enable, const char *file, const char *function, const int line, const char *format, ...)
{
    if (!enable) return;
    if (!theLogger) return;
    va_list args;
    va_start(args, format);
    theLogger->log('V', file, function, line, format, args);
    va_end(args);
}

#ifndef NDEBUG
void ILogger::debug(bool enable, const char *file, const char *function, const int line, const char *format, ...)
{
    if (!enable) return;
    if (!theLogger) return;
    va_list args;
    va_start(args, format);
    theLogger->log('D', file, function, line, format, args);
    va_end(args);
}
#endif

Logger::Logger(const char * logFilePath)
{
    logFile.open(logFilePath);
}

Logger::~Logger()
{
    logFile.close();
    if (theLogger)
    {
        theLogger = 0;
    }
}

void Logger::log(const char pfx, const char *file, const char *function, const int line, const char *format, va_list args)
{
    std::lock_guard<std::mutex> lock(logMutex);
    
    STRNCPY(scratchFile, file, sizeof(scratchFile) - 1);
    snprintf(scratchFormat,  sizeof(scratchFormat), "%c:%s::%s():%d %s", pfx, basename(scratchFile), function, line, format);
    vsnprintf(scratchLine, sizeof(scratchLine), scratchFormat, args);
    logFile << scratchLine << std::endl;
    logFile.flush();
}

}

#ifdef _MSC_VER
static char * basename(char * path)
{
    static char dotpath[2] = ".";
    if (!path || !*path) return dotpath;
    char* p = path + strlen(path) - 1;
    while ((p > path) && ((*p == '/') || (*p == '\\')))
    {
        *p-- = 0;
    }
    while ((p > path) && (*p != '/') && (*p != '\\'))
    {
        --p;
    }
    if (p > path)
    {
        *p = 0;
        return p + 1;
    }
    return p;
}
#endif
