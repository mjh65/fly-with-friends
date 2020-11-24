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

#include "members.h"
#include "sockcomms.h"
#include <map>
#include <vector>

namespace fwf
{

class SessionHub : public SessionDatabase, public UdpSocketOwner
{
    // This class implements the FlyWithFriends session server.

public:
    SessionHub(const int port, const std::string & passcode);
    virtual ~SessionHub();

    void Stop();
    bool IsRunning();
    unsigned int RcvdPacketCount();

protected:
    // Implementation of UdpSocketOwner
    void IncomingDatagram(AddressedDatagram dgin) override;

protected:
    void PositionReport(SocketAddress& sender, const char* payload, unsigned int length);
    void SignOut(SocketAddress & sender, const char * payload, unsigned int length);

    void UpdateAirplaneState(SocketAddress& sender, uint32_t uuid, AircraftPosition& ap, const char* name = 0, const char* callsign = 0);

    int AsyncBroadcastGroupState();

    std::shared_ptr<Datagram> PrepareBroadcast();

private:
    std::string                     const passcode;
    UdpSocketLocal                  serviceSocket;
    std::future<int>                loopResult;
    std::mutex                      guard;
    bool                            running;
    unsigned int                    loopNumber;

    //temporary - will change to inherited base class
    //SessionDatabase* baseclass;
};

}
