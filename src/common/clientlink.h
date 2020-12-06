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
#include "aircraftdb.h"
#include "sockcomms.h"
#include <vector>

namespace fwf {

class Engine;

class ClientLink : public UdpSocketOwner, public SequenceNumberDatabase, public TrackedAircraftDatabase
{
    // This class owns the UDP socket that's used for the exchange of data
    // with the session server. There is only one instance. The socket ID is
    // allocated by the OS.

public:
    ClientLink(IPv4Address& addr, int port, const char* name, const char* callsign, const char* passcode, const char *logDirPath = 0);
    virtual ~ClientLink();

    // action functions called from the UI/sim/engine
    void LeaveSession();
    void SetCurrentPosition(AircraftPosition& ap);

    // Implementation of UdpSocketOwner
    void IncomingDatagram(AddressedDatagram dgin) override;

    // queries from the engine for reporting back to the UI
    bool Joining() { std::lock_guard<std::mutex> lock(guard); return sessionState == JOINING; }
    bool Leaving() { std::lock_guard<std::mutex> lock(guard); return sessionState == LEAVING; }
    bool Inactive() { std::lock_guard<std::mutex> lock(guard); return sessionState == GONE; }
    bool Connected(std::string& desc);
    bool OtherFlier(unsigned int id, std::string& nameCS, float& distance, unsigned int& bearing);
    unsigned long GetRcvdPacketCount();
    unsigned int GetCurrentAircraftPositions(AircraftPosition* aps);

protected:
    bool AsyncDisconnectFromSession();
    bool AsyncFlightLoop();
    void IncomingWorldState(const char * payload, unsigned int length);

private:
    uint32_t LocalTime() const;

private:
    enum State
    {
        JOINING, JOINED, LEAVING, GONE
    };

    IPv4Address                         const sessionServerAddress;
    int                                 const sessionServerPort;
    std::string                         const name;
    std::string                         const callsign;
    std::string                         const passcode;
    uint32_t                            sessionUUID;
    UdpSocketLocal                      serverConnection;
    std::future<bool>                   leaveCompleted;
    std::future<bool>                   flightLoopDone;
    std::mutex                          guard;
    uint32_t                            frameNumber;
    uint64_t                            startTimeMs;
    uint64_t                            lastRcvTime;
    State                               sessionState;
    AircraftPosition                    ourLocation;
    std::mutex                          flightLoopCVGuard;
    std::condition_variable             flightLoopBlock;
    std::mutex                          logGuard;
    std::unique_ptr<std::ofstream>      datagramLog;
};

}
