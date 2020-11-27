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
#include "flightreplay.h"
#include "clientlink.h"
#include "fwfsocket.h"
#include <iostream>
#include <filesystem>

static void usage(const char* prog);

static const unsigned int ROBOT_TIMEOUT = 10;
static unsigned int robotTimeout = ROBOT_TIMEOUT;

int main(int argc, const char** argv)
{    
    // required arguments are server address, port number, session passcode, replay file, number of loops
    if (argc < 6)
    {
        std::cerr << "Not enough arguments" << std::endl;
        usage(*argv);
        return 1;
    }
    fwf::IPv4Address svrAddr(*(argv + 1));
    if (!svrAddr.IsValid())
    {
        std::cerr << "Bad server IP address: " << *(argv + 1) << std::endl;
        usage(*argv);
        return 1;
    }
    int port;
    if (sscanf_s(*(argv + 2), "%d", &port) != 1)
    {
        std::cerr << "Bad port number: " << *(argv + 2) << std::endl;
        usage(*argv);
        return 1;
    }
    std::string passcode(*(argv + 3));
    unsigned int loops;
    if ((sscanf_s(*(argv + 5), "%u", &loops) != 1) || (loops < 1) || (loops > 99))
    {
        std::cerr << "Bad loop count: " << *(argv + 5) << std::endl;
        usage(*argv);
        return 1;
    }

    std::unique_ptr<fwf::FlightReplay> rf;
    try
    {
        rf = std::make_unique<fwf::FlightReplay>(*(argv + 4), loops);
    }
    catch (...)
    {
        std::cerr << "Failed to open or read replay file, or unknown mode" << std::endl;
        return 2;
    }

#ifdef TARGET_WINDOWS
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "Server: WSAStartup failed with error " << WSAGetLastError() << std::endl;
        return 2;
    }
#endif

    std::shared_ptr<fwf::ClientLink> serverComms;
    try
    {
        serverComms = std::make_shared<fwf::ClientLink>(svrAddr, port, rf->Name(), rf->Callsign(), *(argv + 3));
    }
    catch (...)
    {
        std::cerr << "Session client startup failed" << std::endl;
        return 2;
    }

    std::string logFile = std::filesystem::current_path().string() + "/fwf_robot.log";
    std::shared_ptr<fwf::ILogger> logger = std::shared_ptr<fwf::ILogger>(fwf::ILogger::New(logFile.c_str()));

    int err = 0;
    robotTimeout = ROBOT_TIMEOUT;
    unsigned int prevPacketCount = 0;
    try
    {
        std::cout << "Robot flight starting." << std::endl;
        rf->Start(serverComms);
        while (robotTimeout && !rf->Completed())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            unsigned int pktCount = serverComms->GetRcvdPacketCount();
            if (pktCount > prevPacketCount)
            {
                pktCount = prevPacketCount;
                robotTimeout = ROBOT_TIMEOUT;
            }
            --robotTimeout;
        }
        if (rf->Completed())
        {
            std::cout << "Robot flight has completed normally." << std::endl;
        }
        else
        {
            std::cout << "Connection with server has timed out, abandoning flight!" << std::endl;
        }
        std::cout << "Stopping ..." << std::endl;
        rf->Stop();
    }
    catch (...)
    {
        std::cerr << "Exception thrown during flight replay" << std::endl;
        err = 3;
    }

    WSACleanup();
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    return err;
}

static void usage(const char* prog)
{
    std::cerr << "Usage: " << prog << " <server-address> <port-number> <passcode> <replay-file> <loop-count>" << std::endl;
}

