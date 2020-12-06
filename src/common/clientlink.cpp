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
#include "fwfporting.h"
#include <sstream>
#include <iomanip>

namespace fwf {

static const bool infoLogging = false;
static const bool verboseLogging = false;

ClientLink::ClientLink(IPv4Address& a, int pn, const char* n, const char* cs, const char* pc, const char* logDirPath)
:   UdpSocketOwner(),
    SequenceNumberDatabase(),
    TrackedAircraftDatabase(),
    sessionServerAddress(a),
    sessionServerPort(pn),
    name(n),
    callsign(cs),
    passcode(pc),
    sessionUUID(0),
    serverConnection(this, std::string("SMGR")),
    frameNumber(1),
    startTimeMs(TIMENOWMS()),
    lastRcvTime(0),
    sessionState(JOINING)
{
    std::string t1 = name + callsign + passcode;
    unsigned int t2;
    memcpy(&t2, t1.c_str(), sizeof(unsigned int));
    t2 |= (unsigned int)time(0);
    srand(t2);
    sessionUUID = rand();

    ourLocation.msTimestamp = LocalTime();
    flightLoopDone = std::async(std::launch::async, &ClientLink::AsyncFlightLoop, this);

    if (logDirPath)
    {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        struct tm timeinfo;
        LOCALTIME(&in_time_t, &timeinfo);
        std::stringstream ss;
        ss << logDirPath << std::put_time(&timeinfo, "ClientPkts-%Y-%m-%d-%H-%M.fwf");
        datagramLog = std::make_unique<std::ofstream>(ss.str().c_str());
    }
}

ClientLink::~ClientLink()
{
    while ((leaveCompleted.valid()) && (leaveCompleted.wait_for(std::chrono::milliseconds(10)) != std::future_status::ready)) {}
    while ((flightLoopDone.valid()) && (flightLoopDone.wait_for(std::chrono::milliseconds(10)) != std::future_status::ready)) {}
}

void ClientLink::LeaveSession()
{
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
    std::unique_lock<std::mutex> lock(flightLoopCVGuard);
    flightLoopBlock.notify_one();
}

void ClientLink::SetCurrentPosition(AircraftPosition& ap)
{
    {
        std::lock_guard<std::mutex> lock(guard);
        ourLocation = ap;
    }

    std::unique_lock<std::mutex> lock(flightLoopCVGuard);
    flightLoopBlock.notify_one();
}

bool ClientLink::Connected(std::string& desc)
{
    std::lock_guard<std::mutex> lock(guard);
    switch (sessionState)
    {
    case JOINING:
        desc = "Connecting";
        return false;
    case JOINED:
        desc = "Connected";
        return true;
    case LEAVING:
        desc = "Disconnecting";
        return true;
    case GONE:
        desc = "Disconnected";
        return false;
    }
    return false;
}

bool ClientLink::OtherFlier(unsigned int id, std::string& nameCS, float& distance, unsigned int& bearing)
{
    assert(id > 0);
    std::lock_guard<std::mutex> lock(guard);
    std::shared_ptr<TrackedAircraft> m = FindMemberByIndex(id - 1);
    if (m)
    {
        nameCS = m->Callsign() + " (" + m->Name() + ')';
        distance = m->Distance();
        bearing = m->Bearing();
        return true;
    }
    return false;
}

unsigned long ClientLink::GetRcvdPacketCount()
{
    return serverConnection.RcvdDatagramCount();
}

unsigned int ClientLink::GetCurrentAircraftPositions(AircraftPosition* aps)
{
    uint32_t ts = LocalTime();
    return GetActiveMemberPositions(aps, ts);
}

bool ClientLink::AsyncDisconnectFromSession()
{
    // first wait until main thread has changed state to leaving
    while (1)
    {
        std::lock_guard<std::mutex> lock(guard);
        if (sessionState == GONE) return true;
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
        if (datagramLog)
        {
            std::string s = leave->LogString();
            std::lock_guard<std::mutex> lock(logGuard);
            (*datagramLog) << "S:" << LocalTime() << ':' << s << std::endl;
        }
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
    uint32_t lt = LocalTime();

    // if enabled, log everything we receive
    if (datagramLog)
    {
        std::string s = dgin.LogString();
        std::lock_guard<std::mutex> lock(logGuard);
        (*datagramLog) << "R:" << lt << ':' << s << std::endl;
    }

    // extract seq-num, command, payload-length
    Datagram * d = dgin.Data();
    uint32_t sn = d->SequenceNumber();
    Datagram::CommandCode cmd = d->Command();
    uint16_t pl = d->PayloadLength();
    LOG_INFO(infoLogging,"received datagram - %08x, %d, %d", sn, cmd, pl);
    if (OutOfOrder(dgin.Address(), sn))
    {
        LOG_WARN("dropping out-of-order datagram", sn, cmd, pl);
        return;
    }

    if (d->ValidPayload())
    {
        lastRcvTime = lt;
        if (cmd == Datagram::WORLDSTATE)
        {
            IncomingWorldState(d->Payload(), pl);
        }
    }
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

    double ourLat, ourLon;
    {
        std::lock_guard<std::mutex> lock(guard);
        ourLat = ourLocation.latitude;
        ourLon = ourLocation.longitude;
    }

    uint32_t rcvTimestamp = LocalTime();

    // first 32 bits are the time since the server started, in ms
    uint32_t tsession;
    memcpy(&tsession, p, sizeof(uint32_t));
    tsession = ntohl(tsession);
    p += sizeof(tsession);
    pl -= sizeof(tsession);

    // next 2 bytes are record counts for departed members and position updates
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
    while (numPositions-- && (pl >= (sizeof(uint32_t) + Aircraft::EncodedPositionLength())))
    {
        uint32_t uuid;
        memcpy(&uuid, p, sizeof(uint32_t));
        uuid = ntohl(uuid);
        p += sizeof(uint32_t);
        pl -= sizeof(uint32_t);

        AircraftPosition ap;
        unsigned int apl = ap.DecodeFrom(p);
        p += apl;
        pl -= apl;

        if (uuid == sessionUUID)
        {
            std::lock_guard<std::mutex> lock(guard);
            if (sessionState == JOINING) sessionState = JOINED;
            continue; // ignore broadcast report of ourselves
        }

        std::shared_ptr<TrackedAircraft> m = FindMember(uuid);
        if (!m)
        {
            LOG_INFO(infoLogging, "first report from %08x", uuid);
            m = std::make_shared<TrackedAircraft>(uuid);
            if (AddMember(uuid, m) == MAX_IN_SESSION) continue;
        }
        m->UpdateTracking(ap, rcvTimestamp, ourLat, ourLon);
    }

    // potentially a name/callsign of one of the members
    if (pl >= (sizeof(uint32_t) + 4))
    {
        uint32_t uuid;
        memcpy(&uuid, p, sizeof(uint32_t));
        uuid = ntohl(uuid);
        p += sizeof(uint32_t);
        pl -= sizeof(uint32_t);

        std::shared_ptr<TrackedAircraft> m = FindMember(uuid);
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

uint32_t ClientLink::LocalTime() const
{
    return ((TIMENOWMS() - startTimeMs) & 0xffffffff);
}

bool ClientLink::AsyncFlightLoop()
{
    LOG_VERBOSE(verboseLogging,"async flight loop started");
    while (1)
    {
        {
            std::unique_lock<std::mutex> lock(flightLoopCVGuard);
            flightLoopBlock.wait_for(lock, std::chrono::milliseconds(2500));
        }

        AircraftPosition ap;
        {
            std::lock_guard<std::mutex> lock(guard);
            if ((sessionState != JOINING) && (sessionState != JOINED)) break;
            ap = ourLocation;
        }
        uint32_t lt = ap.msTimestamp = LocalTime();
        if ((lt - lastRcvTime) > MEMBERSHIP_TIMEOUT_MS)
        {
            std::lock_guard<std::mutex> lock(guard);
            sessionState = GONE;
            break;
        }

        // generate the datagram that reports our location
        LOG_VERBOSE(verboseLogging, "creating aircraft position reporting dgram");
        std::shared_ptr<Datagram> d = std::make_shared<Datagram>();
        std::shared_ptr<AddressedDatagram> report = std::make_shared<AddressedDatagram>(d, sessionServerAddress, sessionServerPort);
        d->SetCommand(Datagram::REPORT);
        d->SetSequenceNumber(frameNumber++);

        // payload consists of our ID, aircraft position data, optional name and callsign
        char* p = d->Payload();
        uint32_t uuid = htonl(sessionUUID);
        memcpy(p, &uuid, sizeof(uuid));
        p += sizeof(uuid);

        p += ap.EncodeTo(p);

        bool sendNameCallsign = false;
        {
            std::lock_guard<std::mutex> lock(guard);
            sendNameCallsign = (sessionState == JOINING) || ((frameNumber & 0x3f) == 0);
        }
        if (sendNameCallsign)
        {
            unsigned int pl = MAX_PAYLOAD_LEN - ((unsigned int)(p - d->Payload()));
            STRCPY(p, pl, name.c_str());
            p += name.size() + 1;
            pl -= (unsigned int)(name.size() + 1);
            STRCPY(p, pl, callsign.c_str());
            p += callsign.size() + 1;
            pl -= (unsigned int)(callsign.size() + 1);
        }

        d->SetPayloadLength((unsigned int)(p - d->Payload()));
        if (datagramLog)
        {
            std::string s = report->LogString();
            std::lock_guard<std::mutex> lock(logGuard);
            (*datagramLog) << "S:" << LocalTime() << ':' << s << std::endl;
        }
        serverConnection.QueueDatagram(report);

        CheckLapsedMembership(MEMBERSHIP_TIMEOUT_MS / CLIENT_UPDATE_PERIOD_MS);
        RemoveExpiredMembership();
    }

    LOG_VERBOSE(verboseLogging,"async flight loop completed");
    return true;
}

}
