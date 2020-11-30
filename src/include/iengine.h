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

#include <memory>
#include <string>

namespace fwf {

class ISimData;

// Interface to the engine running the P2P exchangers and rendezvous server

class IEngine
{
public:
    static const unsigned long MAX_IN_SESSION = 16;

    static std::shared_ptr<IEngine> New(std::shared_ptr<fwf::ISimData> si, std::string& dir);
    virtual ~IEngine() {}

    // this group called from the UI/control classes

    // returns false if unknown, poll to try again later
    virtual bool GetPublicIPAddress(std::string & ipaddr) = 0;

    // start/stop a rendezvous server to host the session
    virtual void StartSessionServer(const int p, const char * passcode) = 0;
    virtual void StopSessionServer() = 0;

    // connect to a rendezvous server, and then start the P2P stuff
    virtual bool JoinSession(const char * addr, const int port, const char * name, const char * callsign, const char * passcode) = 0;
    virtual void LeaveSession() = 0;

    // start or stop recording the curent flight
    virtual void StartRecording() = 0;
    virtual void StopRecording() = 0;
    virtual bool IsRecording() = 0;

    virtual std::string StatusSummary() = 0;
    virtual std::string StatusDetail(unsigned int i) = 0;

    // this group called from the simulation interfaces
    virtual float DoFlightLoop() = 0;

};

}

