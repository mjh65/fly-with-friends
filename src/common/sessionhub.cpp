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

#include "sessionhub.h"
#include "fwfconsts.h"
#include "ilogger.h"

namespace fwf
{

static const bool infoLogging = true;
static const bool verboseLogging = false;
static const bool debugLogging = false;

SessionHub::SessionHub(const int port, const std::string& pc)
:   ServerDatabase(),
    UdpSocketOwner(),
    passcode(pc),
    serviceSocket(this, std::string("SERV"), port), // might throw exception
    running(true),
    loopNumber(0)
{
    loopResult = std::async(std::launch::async, &SessionHub::AsyncBroadcastGroupState, this);
}

SessionHub::~SessionHub()
{
}

void SessionHub::Stop()
{
    serviceSocket.Shutdown();
    {
        std::lock_guard<std::mutex> lock(guard);
        running = false;
    }
    if (loopResult.valid()) loopResult.wait_for(std::chrono::seconds(1));
}

bool SessionHub::IsRunning()
{
    std::lock_guard<std::mutex> lock(guard);
    return running;
}

unsigned int SessionHub::RcvdPacketCount()
{
    return serviceSocket.RcvdDatagramCount();
}

void SessionHub::IncomingDatagram(AddressedDatagram dgin)
{
    // extract seq-num, command, payload-length
    Datagram * d = dgin.Data();
    uint32_t sn = d->SequenceNumber();
    Datagram::CommandCode cmd = d->Command();
    uint16_t pl = d->PayloadLength();
    char * p = d->Payload();
    LOG_VERBOSE(verboseLogging,"received datagram - sn=%08x, cmd=%d, plen=%d", sn, cmd, pl);
    if (!d->ValidPayload())
    {
        LOG_WARN("invalid payload", sn, cmd, pl);
        return;
    }

    if (cmd == Datagram::REPORT)
    {
        PositionReport(dgin.Address(), p, pl);
    }
    else if (cmd == Datagram::LEAVING)
    {
        SignOut(dgin.Address(), p, pl);
    }
}

void SessionHub::PositionReport(SocketAddress& sender, const char* payload, unsigned int payloadLength)
{
    // payload consists of client ID, aircraft position data, optional name and callsign
    if (payloadLength < (sizeof(uint32_t) + AircraftPosition::ENCODED_SIZE)) return;
    const char* p = payload;
    size_t pl = payloadLength;

    uint32_t uuid;
    memcpy(&uuid, p, sizeof(uint32_t));
    uuid = ntohl(uuid);
    p += sizeof(uint32_t);
    pl -= sizeof(uint32_t);

    AircraftPosition ap;
    unsigned int apl = DecodeAircraftPosition(ap, p);
    p += apl;
    pl -= apl;
    if (pl < 4)
    {
        UpdateAirplaneState(sender, uuid, ap);
        return;
    }

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

            UpdateAirplaneState(sender, uuid, ap, name, callsign);
        }
    }
}

void SessionHub::SignOut(SocketAddress & sender, const char * payload, unsigned int length)
{
    if (length < sizeof(uint32_t)) return;

    uint32_t uuid;
    memcpy(&uuid, payload, sizeof(uint32_t));
    uuid = ntohl(uuid);
    LOG_INFO(infoLogging,"sign out from %08x",uuid);

    RemoveMember(uuid);
}

void SessionHub::UpdateAirplaneState(SocketAddress& sender, uint32_t uuid, AircraftPosition& ap, const char* name, const char* callsign)
{
    std::shared_ptr<SessionMember> m = FindMember(uuid);
    if (!m)
    {
        LOG_INFO(infoLogging, "first report from %08x", uuid);
        m = std::make_shared<SessionMember>(uuid, sender);
        if (name) m->SetName(name);
        if (callsign) m->SetCallsign(callsign);
        if (AddMember(uuid, m) == MAX_IN_SESSION) return;
    }
    if (!m->Address().Equal(sender)) return;
    m->SetPosition(ap);
}

int SessionHub::AsyncBroadcastGroupState()
{
    auto nextWakeTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(SERVER_BROADCAST_PERIOD_MS);
    while (1)
    {
        ++loopNumber;
        std::this_thread::sleep_until(nextWakeTime);
        {
            std::lock_guard<std::mutex> lock(guard);
            if (!running)
            {
                LOG_VERBOSE(verboseLogging, "exiting, session server is shutting down");
                return 0;
            }
        }
        nextWakeTime += std::chrono::milliseconds(SERVER_BROADCAST_PERIOD_MS);

        CheckLapsedMembership();
        RemoveExpiredMembership();

        std::shared_ptr<Datagram> dg = PrepareBroadcast();
        std::vector<SocketAddress> addrs;
        GetBroadcastAddresses(addrs);
        for (auto i = addrs.begin(); i != addrs.end(); ++i)
        {
            std::shared_ptr<AddressedDatagram> ad = std::make_shared<AddressedDatagram>(dg, *i);
            serviceSocket.QueueDatagram(ad, false);
        }
        serviceSocket.SendAll();
    }
    return -1;
}

std::shared_ptr<Datagram> SessionHub::PrepareBroadcast()
{
    std::shared_ptr<Datagram> dg = std::make_shared<Datagram>();
    char* p = dg->Payload();

    dg->SetCommand(Datagram::WORLDSTATE);
    dg->SetSequenceNumber(loopNumber);

    // first 16 bits are an encoding of how many records of each type follow.
    // we'll put this in after we've figured it out!

    char* q = p;
    p += (2 * sizeof(uint8_t));

    // first any expired members
    uint8_t expiredCount = 0;
    {
        std::vector<uint32_t> uuids;
        GetExpiredMembers(uuids);
        for (unsigned int i = 0; i < uuids.size(); ++i)
        {
            ++expiredCount;
            uint32_t uuid = htonl(uuids[i]);
            memcpy(p, &uuid, sizeof(uuid));
            p += sizeof(uuid);
        }
    }
    *q++ = expiredCount;

    // next any positions not previously broadcast
    uint8_t positionsCount = 0;
    {
        std::vector<std::shared_ptr<SessionMember> > forBroadcast;
        GetBroadcastUpdates(forBroadcast);
        for (unsigned int i = 0; i < forBroadcast.size(); ++i)
        {
            ++positionsCount;
            uint32_t uuid = htonl(forBroadcast[i]->Uuid());
            memcpy(p, &uuid, sizeof(uuid));
            p += sizeof(uuid);
            p += EncodeAircraftPosition(forBroadcast[i]->Position(), p);
        }
    }
    *q++ = positionsCount;

    std::shared_ptr<SessionMember> broadcastNameOf = NextNameBroadcast();
    if (broadcastNameOf)
    {
        uint32_t uuid = htonl(broadcastNameOf->Uuid());
        memcpy(p, &uuid, sizeof(uuid));
        p += sizeof(uuid);

        unsigned int pl = MAX_PAYLOAD_LEN - ((unsigned int)(p - dg->Payload()));
        std::string name = broadcastNameOf->Name();
        strcpy_s(p, pl, name.c_str());
        p += name.size() + 1;
        pl -= (unsigned int)(name.size() + 1);
        std::string callsign = broadcastNameOf->Callsign();
        strcpy_s(p, pl, callsign.c_str());
        p += callsign.size() + 1;
        pl -= (unsigned int)(callsign.size() + 1);
    }

    dg->SetPayloadLength((unsigned long)(p - dg->Payload()));
    return dg;
}

}

