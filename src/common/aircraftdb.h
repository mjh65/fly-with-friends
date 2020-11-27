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

#include "aircraft.h"
#include "fwfconsts.h"
#include "fwfsocket.h"
#include <memory>
#include <mutex>
#include <list>
#include <map>
#include <vector>

namespace fwf {

template <class AIRCRAFT_TYPE>
class AircraftDatabase
{
public:
    unsigned int ActiveAircraftCount() { std::lock_guard<std::mutex> lock(guard); return activeMembers; }

protected:
    AircraftDatabase();
    virtual ~AircraftDatabase();

    bool IsFull() { return ActiveAircraftCount() < MAX_IN_SESSION; }

    std::shared_ptr<AIRCRAFT_TYPE> FindMember(uint32_t uuid);
    std::shared_ptr<AIRCRAFT_TYPE> FindMemberByIndex(unsigned int i);

    uint8_t AddMember(uint32_t uuid, std::shared_ptr<AIRCRAFT_TYPE> &m);
    void RemoveMember(uint32_t uuid);

    void CheckLapsedMembership();
    void RemoveExpiredMembership();

    void ExpireMember(unsigned int i);

protected:
    std::mutex                          guard;
    unsigned int                        activeMembers;
    std::shared_ptr<AIRCRAFT_TYPE>      currentMembers[MAX_IN_SESSION];
    std::map<uint32_t, std::shared_ptr<AIRCRAFT_TYPE> >
                                        membersByUuid;
    std::list< std::shared_ptr<AIRCRAFT_TYPE> >
                                        expiredMembers;

};

class ServerDatabase : public AircraftDatabase<SessionMember>
{
public:
protected:
    void GetBroadcastUpdates(std::vector<std::shared_ptr<SessionMember> >& members);
    std::shared_ptr<SessionMember> NextNameBroadcast();
    void GetBroadcastAddresses(std::vector<SocketAddress>& addresses);
    void GetExpiredMembers(std::vector<uint32_t>& uuids);
private:
};

class TrackedAircraftDatabase : public AircraftDatabase<TrackedAircraft>
{
public:
    unsigned int GetActiveMemberPositions(AircraftPosition* aps, uint32_t ts);
protected:
private:
};



}
