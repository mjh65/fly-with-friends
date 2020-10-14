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
#include <cstdarg>
#include <cstring>
#include <libgen.h>

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

void ILogger::verbose(bool enable, const char *file, const char *function, const int line, const char *format, ...)
{
    if (!enable) return;
    if (!theLogger) return;
    va_list args;
    va_start(args, format);
    theLogger->log('V', file, function, line, format, args);
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

void ILogger::warn(const char *file, const char *function, const int line, const char *format, ...)
{
    if (!theLogger) return;
    va_list args;
    va_start(args, format);
    theLogger->log('W', file, function, line, format, args);
    va_end(args);
}

void ILogger::error(const char *file, const char *function, const int line, const char *format, ...)
{
    if (!theLogger) return;
    va_list args;
    va_start(args, format);
    theLogger->log('E', file, function, line, format, args);
    va_end(args);
}



Logger::Logger(const char * logFilePath)
{
    logFile.open(logFilePath);
}

Logger::~Logger()
{
    logFile.close();
}

void Logger::log(const char pfx, const char *file, const char *function, const int line, const char *format, va_list args)
{
    std::lock_guard<std::mutex> lock(logMutex);
    
    strncpy(scratchFile, file, sizeof(scratchFile) - 1);
    snprintf(scratchFormat,  sizeof(scratchFormat), "%c:%s::%s():%d %s", pfx, basename(scratchFile), function, line, format);
    vsnprintf(scratchLine, sizeof(scratchLine), scratchFormat, args);
    logFile << scratchLine << std::endl;
    logFile.flush();
}



}

