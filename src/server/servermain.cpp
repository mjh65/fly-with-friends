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

#ifdef TARGET_WINDOWS
#include <WinSock2.h>
#endif
#include "ilogger.h"
#include "sessionhub.h"
#include "fwfporting.h"
#include <iostream>
#include <filesystem>
#include <memory>
#ifdef TARGET_WINDOWS
#include <windows.h>
#else
#include <csignal>
#endif

static void usage(const char * prog);

static const unsigned int SERVER_STARTUP_TIMEOUT = 120;
static const unsigned int SERVER_TIMEOUT = 30;
static unsigned int serverTimeout = SERVER_STARTUP_TIMEOUT;

int main(int argc, const char **argv)
{
    // required arguments are host port number and session passcode
    if (argc < 3)
    {
        std::cerr << "Not enough arguments" << std::endl;
        usage(*argv);
        return 1;
    }
    int port;
    if (SSCANF(*(argv+1), "%d", &port) != 1)
    {
        std::cerr << "Bad port number: " << *(argv+1) << std::endl;
        usage(*argv);
        return 1;
    }
    std::string passcode(*(argv+2));

    std::string logPath = std::filesystem::current_path().string() + "/";
    std::string logFile = logPath + "fwf_server.log";
    std::shared_ptr<fwf::ILogger> logger = std::shared_ptr<fwf::ILogger>(fwf::ILogger::New(logFile.c_str()));
    std::cout << "Starting server on port " << port << " with passcode " << passcode.c_str() << std::endl;

#ifdef TARGET_WINDOWS
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "Server: WSAStartup failed with error " << WSAGetLastError() << std::endl;
        return 2;
    }
#endif

    const char* pktLogPath = 0;
#ifndef NDEBUG
    pktLogPath = logPath.c_str();
#endif

    std::unique_ptr<fwf::SessionHub> hub;
    try
    {
        hub = std::make_unique<fwf::SessionHub>(port, passcode, pktLogPath);
    }
    catch (...)
    {
        std::cerr << "Server startup failed" << std::endl;
#ifdef TARGET_WINDOWS
        WSACleanup();
#endif
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        return 2;
    }

    int err = 0;
    serverTimeout = SERVER_STARTUP_TIMEOUT;
    unsigned int prevPacketCount = 0;
    unsigned int prevAircraftCount = 0;
    try
    {
        // now loop until server is stopped or has been idle for 30s
        while (serverTimeout && hub->IsRunning())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            unsigned int pktCount = hub->RcvdPacketCount();
            if (pktCount > prevPacketCount)
            {
                prevPacketCount = pktCount;
                serverTimeout = SERVER_TIMEOUT;
            }
            unsigned int aircraftCount = hub->ActiveAircraftCount();
            if (prevAircraftCount != aircraftCount)
            {
                std::cout << aircraftCount << " aircraft are now in session" << std::endl;
                prevAircraftCount = aircraftCount;
            }
            if (!aircraftCount) --serverTimeout;
        }
        if (!hub->IsRunning())
        {
            std::cout << "Server has been shutdown normally." << std::endl;
        }
        else if (!prevPacketCount)
        {
            std::cout << "No server connections were made in " << SERVER_STARTUP_TIMEOUT << " seconds since starting. Giving up." << std::endl;
        }
        else if (!hub->ActiveAircraftCount())
        {
            std::cout << "No member activity for " << SERVER_TIMEOUT << " seconds. Shutting down." << std::endl;
        }

        std::cout << "Stopping" << std::endl;
        hub->Stop();
        LOG_INFO(true, "stopped server");
    }
    catch (...)
    {
        std::cerr << "Exception thrown during server operation" << std::endl;
        err = 3;
    }
#ifdef TARGET_WINDOWS
    WSACleanup();
#endif
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    return err;
}

static void usage(const char * prog)
{
    std::cerr << "Usage: " << prog << " <port-number> <passcode>" << std::endl;
}

