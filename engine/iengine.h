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
struct AircraftPosition;

// Interface to the engine communicating with the FWF server

class IEngine
{
public:
    static const unsigned long MAX_IN_SESSION = 16;

    static std::shared_ptr<IEngine> New(ISimData* sim, const std::string& dir);
    virtual ~IEngine() {}

    // this group called from the UI/control classes

    // returns false if unknown, poll to try again later
    virtual bool GetPublicIPAddress(std::string & ipaddr) = 0;

    // start/stop a server to host the session
    virtual void StartSessionServer(const int p, const char * passcode, bool logging) = 0;
    virtual void StopSessionServer() = 0;

    // connect/disconnect to/from FWF server
    virtual bool JoinSession(const char * addr, const int port, const char * name, const char * callsign, const char * passcode, bool logging) = 0;
    virtual void LeaveSession() = 0;
    virtual bool IsSessionActive() = 0;

    // start or stop recording the current flight
    virtual void StartRecording() = 0;
    virtual void StopRecording() = 0;
    virtual bool IsRecording() = 0;

    // this should be called by the simulation to execute one iteration of the engine
    virtual float DoFlightLoop() = 0;

    // this provides information for the status window
    virtual bool IsConnected(std::string & desc) = 0;
    virtual unsigned int CountOtherFliers() = 0;
    virtual bool OtherFlier(unsigned int id, std::string& nameCS, float& distance, unsigned int& bearing) = 0;

};

}

