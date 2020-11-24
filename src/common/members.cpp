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

#include "members.h"
#include "datagrams.h"
#include "ilogger.h"

namespace fwf {

static const bool infoLogging = true;
static const bool verboseLogging = false;
static const bool debugLogging = false;

inline unsigned int calcEpl()
{
    char b[sizeof(AircraftPosition)];
    AircraftPosition ap;
    return EncodeAircraftPosition(ap, b);
}

unsigned int SessionMember::lenEncodedPosition = calcEpl();

SessionMember::SessionMember(uint32_t u)
:   uuid(u),
    address(),
    name(),
    callsign(),
    nextIdBroadcast(0),
    pending(false),
    counter(0)
{
}

SessionMember::SessionMember(uint32_t u, SocketAddress& s)
:   uuid(u),
    address(s),
    name(),
    callsign(),
    nextIdBroadcast(0),
    pending(false),
    counter(0)
{
}

SessionMember::~SessionMember()
{
}

uint32_t SessionMember::ExpiredUuid()
{
    std::lock_guard<std::mutex> lock(guard);
    ++counter;
    return uint32_t();
}

bool SessionMember::PendingPositionBroadcast()
{
    std::lock_guard<std::mutex> lock(guard);
    bool r = pending;
    pending = false;
    return r;
}

bool SessionMember::PendingNameBroadcast()
{
    std::lock_guard<std::mutex> lock(guard);
    bool r = false;
    if (--nextIdBroadcast < 0)
    {
        r = true;
        nextIdBroadcast = 100; // every 10 seconds TODO magic number
    }
    return r;
}

void SessionMember::SetSessionId(unsigned int sid)
{
    std::lock_guard<std::mutex> lock(guard);
    sessionId = sid;
}

void SessionMember::SetName(const char* n)
{
    if (n)
    {
        std::lock_guard<std::mutex> lock(guard);
        name = std::string(n);
        nextIdBroadcast = 0;
    }
}

void SessionMember::SetCallsign(const char* cs)
{
    if (cs)
    {
        std::lock_guard<std::mutex> lock(guard);
        callsign = std::string(cs);
        nextIdBroadcast = 0;
    }
}

void SessionMember::UpdatePosition(AircraftPosition& ap)
{
    std::lock_guard<std::mutex> lock(guard);
    latestPosition = ap;
    pending = true;
    counter = 0;
}

bool SessionMember::CheckCounter(unsigned int limit)
{
    std::lock_guard<std::mutex> lock(guard);
    if (++counter > limit) return true;
    return false;
}

void SessionMember::ResetCounter()
{
    std::lock_guard<std::mutex> lock(guard);
    counter = 0;
}





SessionDatabase::SessionDatabase()
:   activeMembers(0)
{
    for (uint8_t i = 0; i < MAX_IN_SESSION; ++i)
    {
        currentMembers[i].reset();
    }
}

SessionDatabase::~SessionDatabase()
{
    for (uint8_t i = 0; i < MAX_IN_SESSION; ++i)
    {
        currentMembers[i].reset();
    }
}

std::shared_ptr<SessionMember> SessionDatabase::FindMember(uint32_t uuid)
{
    std::shared_ptr<SessionMember> m;
    std::lock_guard<std::mutex> lock(guard);
    auto i = membersByUuid.find(uuid);
    if (i != membersByUuid.end())
    {
        m = i->second;
    }
    return m;
}

void SessionDatabase::GetExpiredMembers(std::vector<uint32_t>& uuids)
{
    uuids.clear();
    std::lock_guard<std::mutex> lock(guard);
    for (auto i = expiredMembers.begin(); i != expiredMembers.end(); ++i)
    {
        uuids.push_back((*i)->ExpiredUuid());
    }
}

std::shared_ptr<SessionMember> SessionDatabase::FindMemberByIndex(unsigned int i)
{
    std::shared_ptr<SessionMember> m;
    std::lock_guard<std::mutex> lock(guard);
    if (i < MAX_IN_SESSION) return currentMembers[i];
    return m;
}

void SessionDatabase::GetBroadcastUpdates(std::vector<std::shared_ptr<SessionMember>>& members)
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

void SessionDatabase::GetBroadcastAddresses(std::vector<SocketAddress>& addresses)
{
    addresses.clear();
    std::lock_guard<std::mutex> lock(guard);
    for (auto i = membersByUuid.begin(); i != membersByUuid.end(); ++i)
    {
        addresses.push_back(i->second->Address());
    }
}

std::shared_ptr<SessionMember> SessionDatabase::NextNameBroadcast()
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

uint8_t SessionDatabase::AddMember(uint32_t uuid, std::shared_ptr<SessionMember>& m)
{
    uint8_t sid = MAX_IN_SESSION;
    std::lock_guard<std::mutex> lock(guard);
    if (activeMembers == MAX_IN_SESSION) return MAX_IN_SESSION;
    for (uint8_t i = 0; i < MAX_IN_SESSION; ++i)
    {
        if (!currentMembers[i])
        {
            m->SetSessionId(sid = i);
            LOG_INFO(infoLogging, "adding member %08x, putting in slot %d", uuid, i);
            currentMembers[i] = m;
            membersByUuid.insert({ uuid, m });
            ++activeMembers;
            break;
        }
    }
    return sid;
}

void SessionDatabase::RemoveMember(uint32_t uuid)
{
    std::lock_guard<std::mutex> lock(guard);
    auto i = membersByUuid.find(uuid);
    if (i != membersByUuid.end())
    {
        unsigned int sid = i->second->SessionId();
        ExpireMember(sid);
    }
}

void SessionDatabase::CheckLapsedMembership()
{
    std::lock_guard<std::mutex> lock(guard);
    for (auto i = membersByUuid.begin(); i != membersByUuid.end(); ++i)
    {
        if (i->second->CheckCounter(50)) // 50 ~= 5s
        {
            // haven't heard from this member in a while
            ExpireMember(i->second->SessionId());
            return;
        }
    }
}

unsigned int SessionDatabase::GetActiveMemberPositions(AircraftPosition* aps)
{
    unsigned int flags = 0;
    std::lock_guard<std::mutex> lock(guard);
    for (auto i = membersByUuid.begin(); i != membersByUuid.end(); ++i)
    {
        unsigned int sid = i->second->SessionId();
        *(aps + sid) = i->second->Position();
        flags |= (1 << sid);
    }
    return flags;
}

void SessionDatabase::RemoveExpiredMembership()
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

void SessionDatabase::ExpireMember(unsigned int i)
{
    // not guarded, as all callers already in guarded blocks
    std::shared_ptr<SessionMember> m = currentMembers[i];
    expiredMembers.push_back(m);
    currentMembers[i].reset();
    membersByUuid.erase(m->Uuid());
    --activeMembers;
    m->ResetCounter();
}



}
