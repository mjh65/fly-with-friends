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

#include "iengine.h"
#include "isimdata.h"
#include "addrfinder.h"
#include "sessionhub.h"
#include "clientlink.h"
#include <memory>
#include <mutex>
#include <fstream>

namespace fwf {

class Engine : public IEngine
{
public:
    Engine(std::shared_ptr<ISimData> si, std::string& dir);
    ~Engine();

    // IEngine implementation
    bool GetPublicIPAddress(std::string & ipaddr) override;
    void StartSessionServer(const int port, const char * passcode) override;
    void StopSessionServer() override;
    bool JoinSession(const char * addr, const int port, const char * name, const char * callsign, const char * passcode) override;
    void LeaveSession() override;
    void StartRecording() override;
    void StopRecording() override;
    bool IsRecording() override { return  (recording != nullptr); }
    std::string StatusSummary() override;
    std::string StatusDetail(unsigned int i) override;
    float DoFlightLoop() override;

protected:

private:
    const std::string                   recordingsDir;

    // thread access control
    std::mutex                          guard;

    // interface to simulation - for X-Plane API and dataref access
    std::shared_ptr<ISimData>           simulation;

    // the public IP address (other side of the NAT/firewall)
    IPv4Address                         publicInetAddr;

    // background task object to figure out our public IP address
    std::unique_ptr<IPAddrFinder>       addrFinder;
    unsigned int                        findDelay;

    // session server, if hosting
    std::unique_ptr<SessionHub>         sessionHub;

    // background task object coordinating with the session server
    std::unique_ptr<ClientLink>         clientLink;

    // output file if flight is being recorded
    std::unique_ptr<std::ofstream>      recording;

    // monotonically incrementing flight loop, time counter and frame numbers
    std::chrono::time_point<std::chrono::steady_clock>
                                        nextNetworkUpdateTime;
    uint32_t                            frameNumber;
};

}

