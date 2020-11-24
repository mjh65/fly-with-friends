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

#include "fwfconsts.h"
#include "fwfsocket.h"
#include "isimdata.h"
#include <list>
#include <map>
#include <mutex>
#include <vector>

namespace fwf {

class SessionMember
{
public:
    SessionMember(uint32_t uuid);
    SessionMember(uint32_t uuid, SocketAddress& skt);
    ~SessionMember();

    const SocketAddress& Address() const { return address; }
    uint32_t Uuid() const { return uuid; }
    uint32_t ExpiredUuid();
    unsigned int SessionId() const { return sessionId; }
    std::string Name() const { return name; }
    std::string Callsign() const { return callsign; }
    const AircraftPosition& Position() const { return latestPosition;  }
    static unsigned int EncodedPositionLength() { return lenEncodedPosition; }

    bool PendingPositionBroadcast();
    bool PendingNameBroadcast();

    void SetSessionId(unsigned int sid);
    void SetName(const char* name);
    void SetCallsign(const char* callsign);
    void UpdatePosition(AircraftPosition& ap);
    bool CheckCounter(unsigned int limit);
    void ResetCounter();

protected:

private:
    static unsigned int         lenEncodedPosition;
    std::mutex                  guard;
    uint32_t                    uuid;
    unsigned int                sessionId;
    SocketAddress               address;
    std::string                 name;
    std::string                 callsign;
    int                         nextIdBroadcast;
    AircraftPosition            latestPosition;
    bool                        pending;
    unsigned int                counter;

};


class SessionDatabase
{
public:
    unsigned int ActiveMemberCount() { std::lock_guard<std::mutex> lock(guard); return activeMembers; }

protected:
    SessionDatabase();
    virtual ~SessionDatabase();

    bool IsFull() { return ActiveMemberCount() < MAX_IN_SESSION; }

    std::shared_ptr<SessionMember> FindMember(uint32_t uuid);
    std::shared_ptr<SessionMember> FindMemberByIndex(unsigned int i);

    void GetExpiredMembers(std::vector<uint32_t>& uuids);
    void GetBroadcastUpdates(std::vector<std::shared_ptr<SessionMember> >& members);
    std::shared_ptr<SessionMember> NextNameBroadcast();
    void GetBroadcastAddresses(std::vector<SocketAddress>& addresses);
    unsigned int GetActiveMemberPositions(AircraftPosition* aps);

    uint8_t AddMember(uint32_t uuid, std::shared_ptr<SessionMember> &m);
    void RemoveMember(uint32_t uuid);

    void CheckLapsedMembership();
    void RemoveExpiredMembership();

    void ExpireMember(unsigned int i);

private:
    std::mutex                          guard;
    unsigned int                        activeMembers;
    std::shared_ptr<SessionMember>      currentMembers[MAX_IN_SESSION];
    std::map<uint32_t, std::shared_ptr<SessionMember> >
                                        membersByUuid;
    std::list< std::shared_ptr<SessionMember> >
                                        expiredMembers;
};


}
