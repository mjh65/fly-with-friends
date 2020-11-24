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

#include "clientlink.h"
#include "datagrams.h"
#include "engine.h"
#include "ilogger.h"

namespace fwf {

static const bool infoLogging = true;
static const bool verboseLogging = false;
static const bool detailLogging = false;

ClientLink::ClientLink(IPv4Address & a, int pn, const char * n, const char * cs, const char * pc)
:   SessionDatabase(),
    UdpSocketOwner(),
    sessionServerAddress(a),
    sessionServerPort(pn),
    name(n),
    callsign(cs),
    passcode(pc),
    sessionUUID(0),
    serverConnection(this,std::string("SMGR")),
    frameNumber(1),
    sessionState(JOINING)
{
    std::string t1 = std::string(n) + callsign + passcode;
    unsigned int t2;
    memcpy(&t2, t1.c_str(), sizeof(unsigned int));
    t2 |= (unsigned int)time(0);
    srand(t2);
    sessionUUID = rand();
}

ClientLink::~ClientLink()
{
    while ((leaveCompleted.valid()) && (leaveCompleted.wait_for(std::chrono::milliseconds(10)) != std::future_status::ready)) {}
    while ((flightLoopDone.valid()) && (flightLoopDone.wait_for(std::chrono::milliseconds(10)) != std::future_status::ready)) {}
}

void ClientLink::LeaveSession()
{
    std::lock_guard<std::mutex> lock(guard);
    if ((sessionState == JOINING) || (sessionState == JOINED))
    {
        leaveCompleted = std::async(std::launch::async, &ClientLink::AsyncDisconnectFromSession, this);
        sessionState = LEAVING;
    }
    else
    {
        sessionState = GONE;
    }
}

float ClientLink::RunCycle(AircraftPosition& us, AircraftPosition* others, unsigned int* active)
{
    std::lock_guard<std::mutex> lock(guard);
    if (sessionState == GONE) return (float)10.0;
    if (sessionState == LEAVING) return (float)0.5;

    if (!flightLoopDone.valid() || (flightLoopDone.wait_for(std::chrono::seconds(0)) == std::future_status::ready))
    {
        flightLoopDone = std::async(std::launch::async, &ClientLink::AsyncFlightLoop, this, us);
        LOG_VERBOSE(verboseLogging, "async flight loop triggered");
    }
    else
    {
        LOG_VERBOSE(verboseLogging, "async flight loop skipped as previous loop still running");
    }
    if (others && active)
    {
        // provide positions for the other aircraft, and set flags
        *active = GetActiveMemberPositions(others);
    }

    return (sessionState == JOINING) ? (float)1.0 : (float)0.1;
}

bool ClientLink::Connected(std::string & addr, int & port, unsigned int & np)
{
    addr = sessionServerAddress.GetQuad();
    port = sessionServerPort;
    return Connected(np);
}

bool ClientLink::Connected(unsigned int & np)
{
    std::lock_guard<std::mutex> lock(guard);
    np = ActiveMemberCount();
    return (sessionState == JOINED);
}

unsigned long ClientLink::GetRcvdPacketCount()
{
    return serverConnection.RcvdDatagramCount();
}

bool ClientLink::GetFlierIdentifiers(unsigned int id, std::string & n, std::string & cs)
{
    std::lock_guard<std::mutex> lock(guard);
    if (id == 0)
    {
        n = name;
        cs = callsign;
        return true;
    }

    std::shared_ptr<SessionMember> m = FindMemberByIndex(id-1);
    if (m)
    {
        n = m->Name();
        cs = m->Callsign();
        return true;
    }
    return false;
}

bool ClientLink::AsyncDisconnectFromSession()
{
    // first wait until main thread has changed state to leaving
    while (1)
    {
        std::lock_guard<std::mutex> lock(guard);
        if (sessionState == LEAVING) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    
    std::shared_ptr<Datagram> dg = std::make_shared<Datagram>();
    std::shared_ptr<AddressedDatagram> leave = std::make_shared<AddressedDatagram>(dg, sessionServerAddress, sessionServerPort);
    dg->SetCommand(Datagram::LEAVING);
    char * p = dg->Payload();

    uint32_t uuid = htonl(sessionUUID);
    memcpy(p, &uuid, sizeof(uuid));
    p += sizeof(uuid);

    dg->SetPayloadLength((unsigned int)(p - dg->Payload()));
    dg->SetSequenceNumber(frameNumber++);

    // UDP may not be reliable so send the notification a few times before stopping
    for (size_t i=0; i<10; ++i)
    {
        LOG_INFO(infoLogging,"sending leave notification");
        serverConnection.QueueDatagram(leave);
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
    }
    {
        std::lock_guard<std::mutex> lock(guard);
        sessionState = GONE;
    }
    return true;
}

// called from socket thread
void ClientLink::IncomingDatagram(AddressedDatagram dgin)
{
    // extract seq-num, command, payload-length
    Datagram * d = dgin.Data();
    uint32_t sn = d->SequenceNumber();
    Datagram::CommandCode cmd = d->Command();
    uint16_t pl = d->PayloadLength();
    LOG_INFO(infoLogging,"received datagram - %08x, %d, %d", sn, cmd, pl);

    if ((cmd == Datagram::WORLDSTATE) && d->ValidPayload())
    {
        IncomingWorldState(d->Payload(), pl);
    }

    std::lock_guard<std::mutex> lock(guard);
}

// called from socket thread
void ClientLink::IncomingWorldState(const char * payload, unsigned int length)
{
    const char * p = payload;
    size_t pl = length;
    if (pl < 2)
    {
        LOG_WARN("ignoring short peer notification datagram");
        return;
    }

    // first 2 bytes are record counts for departed members and position updates
    uint8_t numDeparted = *p++;
    uint8_t numPositions = *p++;
    pl -= 2;

    // next come the list of players that have departed from the session
    while (numDeparted-- && (pl >= sizeof(uint32_t)))
    {
        uint32_t uuid;
        memcpy(&uuid, p, sizeof(uint32_t));
        uuid = ntohl(uuid);
        p += sizeof(uint32_t);
        pl -= sizeof(uint32_t);
        RemoveMember(uuid);
    }

    // now the list of updated aircraft positions
    while (numPositions-- && (pl >= (sizeof(uint32_t) + SessionMember::EncodedPositionLength())))
    {
        uint32_t uuid;
        memcpy(&uuid, p, sizeof(uint32_t));
        uuid = ntohl(uuid);
        p += sizeof(uint32_t);
        pl -= sizeof(uint32_t);

        AircraftPosition ap;
        unsigned int apl = DecodeAircraftPosition(ap, p);
        p += apl;
        pl -= apl;

        if (uuid == sessionUUID)
        {
            std::lock_guard<std::mutex> lock(guard);
            if (sessionState == JOINING) sessionState = JOINED;
            continue; // ignore broadcast report of ourselves
        }

        std::shared_ptr<SessionMember> m = FindMember(uuid);
        if (!m)
        {
            LOG_INFO(infoLogging, "first report from %08x", uuid);
            m = std::make_shared<SessionMember>(uuid);
            if (AddMember(uuid, m) == MAX_IN_SESSION) continue;
        }
        m->UpdatePosition(ap);
    }

    // potentially a name/callsign of one of the members
    if (pl >= (sizeof(uint32_t) + 4))
    {
        uint32_t uuid;
        memcpy(&uuid, p, sizeof(uint32_t));
        uuid = ntohl(uuid);
        p += sizeof(uint32_t);
        pl -= sizeof(uint32_t);

        std::shared_ptr<SessionMember> m = FindMember(uuid);
        if (!m) return; // ignore if it's us or just an unknown member

        if (memchr(p, 0, pl))
        {
            char str[32];
            memcpy(str, p, std::min<size_t>(pl, 32u));
            str[31] = 0;
            const char* name = p;
            pl -= (strlen(p) + 1);
            p += (strlen(p) + 1);

            if (memchr(p, 0, pl))
            {
                memcpy(str, p, std::min<size_t>(pl, 32u));
                str[31] = 0;
                const char* callsign = p;
                pl -= (strlen(p) + 1);
                p += (strlen(p) + 1);

                m->SetName(name);
                m->SetCallsign(callsign);
            }
        }
    }
}

bool ClientLink::AsyncFlightLoop(AircraftPosition ap)
{
    LOG_VERBOSE(verboseLogging,"async flight loop started");
    
    // generate the datagram that reports our location
    LOG_VERBOSE(verboseLogging,"creating aircraft position reporting dgram");
    std::shared_ptr<Datagram> d = std::make_shared<Datagram>();
    std::shared_ptr<AddressedDatagram> report = std::make_shared<AddressedDatagram>(d, sessionServerAddress, sessionServerPort);
    d->SetCommand(Datagram::REPORT);
    d->SetSequenceNumber(frameNumber++);

    // payload consists of our ID, a timestamp, aircraft position data, optional name and callsign
    char* p = d->Payload();
    uint32_t uuid = htonl(sessionUUID);
    memcpy(p, &uuid, sizeof(uuid));
    p += sizeof(uuid);

    p += EncodeAircraftPosition(ap, p);

    bool sendNameCallsign = false;
    {
        std::lock_guard<std::mutex> lock(guard);
        sendNameCallsign = (sessionState == JOINING);
    }
    if (sendNameCallsign)
    {
        unsigned int pl = MAX_PAYLOAD_LEN - ((unsigned int)(p - d->Payload()));
        strcpy_s(p, pl, name.c_str());
        p += name.size() + 1;
        pl -= (unsigned int)(name.size() + 1);
        strcpy_s(p, pl, callsign.c_str());
        p += callsign.size() + 1;
        pl -= (unsigned int)(callsign.size() + 1);
    }

    d->SetPayloadLength((unsigned int)(p - d->Payload()));
    serverConnection.QueueDatagram(report);

    CheckLapsedMembership();
    RemoveExpiredMembership();

    LOG_VERBOSE(verboseLogging,"async flight loop completed");
    return true;
}

}
