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

#include "ilogger.h"
#include "icontrol.h"
#include "isimulation.h"
#include "iengine.h"
#include "ipify.h"
#include <filesystem>
#include <memory>
#ifdef _WIN32
#include <WinSock2.h>
#endif

int main()
{
    std::string logFile = std::filesystem::current_path().string() + "/fwf.log";
    std::shared_ptr<fwf::ILogger> logger = std::shared_ptr<fwf::ILogger>(fwf::ILogger::New(logFile.c_str()));
    LOG_INFO(true, "started");
    std::shared_ptr<fwf::IControl> control = std::shared_ptr<fwf::IControl>(fwf::IControl::New());
    std::shared_ptr<fwf::ISimulation> sim = std::shared_ptr<fwf::ISimulation>(fwf::ISimulation::New());
    std::shared_ptr<fwf::IEngine> engine = std::shared_ptr<fwf::IEngine>(fwf::IEngine::New());

    if (!logger || !control || !sim || !engine)
    {
        LOG_ERROR("exiting as app entities were not all created");
        return -1;
    }

#ifdef _WIN32
    WSADATA wsa_data;
    (void)WSAStartup(MAKEWORD(1,1), &wsa_data);
#endif

	char addr[256];
    if (!ipify(addr, sizeof(addr)))
    {
        LOG_INFO(true, "Public IP is %s", addr);
    }
    else
    {
        LOG_INFO(true, "Failed to get public IP");
    }


    LOG_INFO(true, "stopped");
    return 0;
}
