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

#include "ilogger.h"
#include <fstream>
#include <mutex>

namespace fwf {

class Logger : public ILogger
{
    friend class ILogger;

protected:
    Logger(const char * logFilePath);
    ~Logger();

    void log(const char pfx, const char *file, const char *function, const int line, const char *format, va_list args) override;

private:
    void v(const char *file, const char *function, const int line, const char *format, ...);
    void i(const char *file, const char *function, const int line, const char *format, ...);
    void w(const char *file, const char *function, const int line, const char *format, ...);
    void e(const char *file, const char *function, const int line, const char *format, ...);

    std::mutex logMutex;
    std::ofstream logFile;
    char scratchFile[1024];
    char scratchFormat[1024];
    char scratchLine[1024];
};

}

