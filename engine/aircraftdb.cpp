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

#include "aircraftdb.h"
#include "ilogger.h"

namespace fwf {

static const bool infoLogging = true;
//static const bool verboseLogging = false;
//static const bool debugLogging = false;


bool SequenceNumberDatabase::OutOfOrder(SocketAddress& src, uint32_t sn)
{
    auto i = last.find(src);
    if (i == last.end())
    {
        last[src] = sn;
        return false;
    }
    else if (sn > i->second)
    {
        i->second = sn;
        return false;
    }
    return false;
}

template<class AIRCRAFT_TYPE>
AircraftDatabase<AIRCRAFT_TYPE>::AircraftDatabase()
:   activeMembers(0)
{
    for (uint8_t i = 0; i < MAX_IN_SESSION; ++i)
    {
        currentMembers[i].reset();
    }
}

template<class AIRCRAFT_TYPE>
AircraftDatabase<AIRCRAFT_TYPE>::~AircraftDatabase()
{
    for (uint8_t i = 0; i < MAX_IN_SESSION; ++i)
    {
        currentMembers[i].reset();
    }
}

template<class AIRCRAFT_TYPE>
std::shared_ptr<AIRCRAFT_TYPE> AircraftDatabase<AIRCRAFT_TYPE>::FindMember(uint32_t uuid)
{
    std::shared_ptr<AIRCRAFT_TYPE> m;
    std::lock_guard<std::mutex> lock(guard);
    auto i = membersByUuid.find(uuid);
    if (i != membersByUuid.end())
    {
        m = i->second;
    }
    return m;
}

template<class AIRCRAFT_TYPE>
std::shared_ptr<AIRCRAFT_TYPE> AircraftDatabase<AIRCRAFT_TYPE>::FindMemberByIndex(unsigned int i)
{
    std::shared_ptr<AIRCRAFT_TYPE> m;
    std::lock_guard<std::mutex> lock(guard);
    if (i < MAX_IN_SESSION) return currentMembers[i];
    return m;
}

template<class AIRCRAFT_TYPE>
uint8_t AircraftDatabase<AIRCRAFT_TYPE>::AddMember(uint32_t uuid, std::shared_ptr<AIRCRAFT_TYPE>& m)
{
    uint8_t sid = MAX_IN_SESSION;
    std::lock_guard<std::mutex> lock(guard);
    if (activeMembers == MAX_IN_SESSION) return MAX_IN_SESSION;
    for (uint8_t i = 0; i < MAX_IN_SESSION; ++i)
    {
        if (!currentMembers[i])
        {
            m->SetSessionId(sid = i);
            FWF_LOG_INFO(infoLogging, "adding member %08x, putting in slot %d", uuid, i);
            currentMembers[i] = m;
            membersByUuid.insert({ uuid, m });
            ++activeMembers;
            break;
        }
    }
    return sid;
}

template<class AIRCRAFT_TYPE>
void AircraftDatabase<AIRCRAFT_TYPE>::RemoveMember(uint32_t uuid)
{
    std::lock_guard<std::mutex> lock(guard);
    auto i = membersByUuid.find(uuid);
    if (i != membersByUuid.end())
    {
        unsigned int sid = i->second->SessionId();
        ExpireMember(sid);
    }
}

template<class AIRCRAFT_TYPE>
void AircraftDatabase<AIRCRAFT_TYPE>::CheckLapsedMembership(unsigned int ticks)
{
    std::lock_guard<std::mutex> lock(guard);
    for (auto i = membersByUuid.begin(); i != membersByUuid.end(); ++i)
    {
        if (i->second->CheckCounter(ticks))
        {
            // haven't heard from this member in a while
            ExpireMember(i->second->SessionId());
            return;
        }
    }
}

template<class AIRCRAFT_TYPE>
void AircraftDatabase<AIRCRAFT_TYPE>::RemoveExpiredMembership()
{
    std::lock_guard<std::mutex> lock(guard);
    for (auto i = expiredMembers.begin(); i != expiredMembers.end(); ++i)
    {
        if ((*i)->CheckCounter(4))
        {
            expiredMembers.erase(i);
            return;
        }
    }
}

template<class AIRCRAFT_TYPE>
void AircraftDatabase<AIRCRAFT_TYPE>::ExpireMember(unsigned int i)
{
    // not guarded, as all callers already in guarded blocks
    std::shared_ptr<AIRCRAFT_TYPE> m = currentMembers[i];
    expiredMembers.push_back(m);
    currentMembers[i].reset();
    membersByUuid.erase(m->Uuid());
    --activeMembers;
    m->ResetCounter();
}

template class AircraftDatabase<SessionMember>;
template class AircraftDatabase<TrackedAircraft>;



void ServerDatabase::GetBroadcastUpdates(std::vector<std::shared_ptr<SessionMember>>& members)
{
    members.clear();
    std::lock_guard<std::mutex> lock(guard);
    for (auto i = membersByUuid.begin(); i != membersByUuid.end(); ++i)
    {
        if (i->second->PendingPositionBroadcast())
        {
            members.push_back(i->second);
        }
    }
}

void ServerDatabase::GetBroadcastAddresses(std::vector<SocketAddress>& addresses)
{
    addresses.clear();
    std::lock_guard<std::mutex> lock(guard);
    for (auto i = membersByUuid.begin(); i != membersByUuid.end(); ++i)
    {
        addresses.push_back(i->second->Address());
    }
}

std::shared_ptr<SessionMember> ServerDatabase ::NextNameBroadcast()
{
    std::shared_ptr<SessionMember> m;
    std::lock_guard<std::mutex> lock(guard);
    for (auto i = membersByUuid.begin(); i != membersByUuid.end(); ++i)
    {
        if (i->second->PendingNameBroadcast())
        {
            return i->second;
        }
    }
    return m;
}


void ServerDatabase ::GetExpiredMembers(std::vector<uint32_t>& uuids)
{
    uuids.clear();
    std::lock_guard<std::mutex> lock(guard);
    for (auto i = expiredMembers.begin(); i != expiredMembers.end(); ++i)
    {
        uuids.push_back((*i)->ExpiredUuid());
    }
}



unsigned int TrackedAircraftDatabase::GetActiveMemberPositions(AircraftPosition* aps, uint32_t ts)
{
    unsigned int flags = 0;
    std::lock_guard<std::mutex> lock(guard);
    for (auto i = membersByUuid.begin(); i != membersByUuid.end(); ++i)
    {
        unsigned int sid = i->second->SessionId();
        *(aps + sid) = i->second->GetPrediction(ts);
        flags |= (1 << sid);
    }
    return flags;
}



}
