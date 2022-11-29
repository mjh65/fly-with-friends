/*
 *   Fly-With-Friends - X-Plane plugin for simple group flying
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

#pragma once

#include <string>

#define FWF_LOG_ERROR(...) fwf::ILogger::error(__FILE__,__FUNCTION__,__LINE__,__VA_ARGS__)
#define FWF_LOG_WARN(...) fwf::ILogger::warn(__FILE__,__FUNCTION__,__LINE__,__VA_ARGS__)
#define FWF_LOG_INFO(enable_expression, ...) fwf::ILogger::info(enable_expression, __FILE__,__FUNCTION__,__LINE__,__VA_ARGS__)
#define FWF_LOG_VERBOSE(enable_expression, ...) fwf::ILogger::verbose(enable_expression, __FILE__,__FUNCTION__,__LINE__,__VA_ARGS__)
#ifndef NDEBUG
#define FWF_LOG_DEBUG(enable_expression, ...) fwf::ILogger::debug(enable_expression, __FILE__,__FUNCTION__,__LINE__,__VA_ARGS__)
#else
#define FWF_LOG_DEBUG(enable_expression, ...)
#endif

namespace fwf {

// Interface for object providing logging

class ILogger
{
public:
    static ILogger * New(const char * logFilePath);
    virtual ~ILogger() {}
    
    static void error(const char *file, const char *function, const int line, const char *format, ...);
    static void warn(const char *file, const char *function, const int line, const char *format, ...);
    static void info(bool enable, const char *file, const char *function, const int line, const char *format, ...);
    static void verbose(bool enable, const char *file, const char *function, const int line, const char *format, ...);
#ifndef NDEBUG
    static void debug(bool enable, const char *file, const char *function, const int line, const char *format, ...);
#endif

    virtual void log(const char pfx, const char *file, const char *function, const int line, const char *format, va_list args) = 0;

protected:
    static ILogger * theLogger;

};

}

