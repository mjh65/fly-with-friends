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

#ifdef TARGET_WINDOWS
#include <WinSock2.h>
#else
#endif
#include "fwfconsts.h"
#include "isimdata.h"
#include <memory>
#include <cassert>

namespace fwf {

// Wrapper for UDP datagrams.

class Datagram
{
public:
    enum CommandCode {
        REPORT,             // client->server, send a report of position and identity
        LEAVING,            // client->server, graceful sign out of session
        WORLDSTATE,         // server->client, share all positions, and periodic single identities
        IDENTIFY,   // obsolete
        MEMBER      // obsolete
    };
    Datagram() : length(0) { }
    virtual ~Datagram() { }
    char            * Buffer() { return buffer; }
    char            * Payload() { return buffer+8; }
    bool            ValidPayload() const { return (PayloadLength() <= (length-8)); }
    unsigned int    Length() const { return length; }
    uint32_t        SequenceNumber() const;
    CommandCode     Command() const;
    uint16_t        PayloadLength() const;
    void            SetLength(unsigned int l) { length = l; }
    void            SetSequenceNumber(uint32_t n);
    void            SetCommand(CommandCode c);
    void            SetPayloadLength(unsigned int l);
protected:
private:
    unsigned int    length;
    char            buffer[MAX_DATAGRAM_LEN];
};

inline uint32_t Datagram::SequenceNumber() const
{
    uint32_t sn;
    memcpy(&sn, buffer, sizeof(sn));
    return ntohl(sn);
}
inline Datagram::CommandCode Datagram::Command() const
{
    uint16_t cmd;
    memcpy(&cmd, buffer+sizeof(uint32_t), sizeof(cmd));
    return static_cast<CommandCode>(ntohs(cmd));
}
inline uint16_t Datagram::PayloadLength() const
{
    uint16_t pl;
    memcpy(&pl, buffer+sizeof(uint32_t)+sizeof(uint16_t), sizeof(pl));
    return ntohs(pl);
}
inline void Datagram::SetSequenceNumber(uint32_t n)
{
    uint32_t sn = htonl(n);
    memcpy(buffer, &sn, sizeof(sn));
}
inline void Datagram::SetCommand(Datagram::CommandCode c)
{
    uint16_t cn = htons(c);
    memcpy(buffer+sizeof(uint32_t), &cn, sizeof(cn));
}
inline void Datagram::SetPayloadLength(unsigned int l)
{
    assert(l < MAX_PAYLOAD_LEN);
    uint16_t ln = htons((uint16_t)l);
    memcpy(buffer+sizeof(uint32_t)+sizeof(uint16_t), &ln, sizeof(ln));
    length = l+sizeof(uint32_t)+sizeof(uint16_t)+sizeof(uint16_t);
}

// A datagram with one or more addresses

class AddressedDatagram
{
public:
    AddressedDatagram() { }
    AddressedDatagram(std::shared_ptr<Datagram> dg) : datagram(dg) { }
    AddressedDatagram(std::shared_ptr<Datagram> dg, SocketAddress sa) : datagram(dg), address(sa) { }
    AddressedDatagram(std::shared_ptr<Datagram> dg, IPv4Address a, int p) : datagram(dg), address(a,p) { }
    void SetDatagram(std::shared_ptr<Datagram> dg) { datagram = dg; }
    void SetAddress(SocketAddress s) { address = s; }
    Datagram * Data() { return datagram.get(); }
    SocketAddress & Address() { return address; }
protected:
private:
    std::shared_ptr<Datagram>   datagram;
    SocketAddress               address;
};


inline unsigned int EncodeAircraftPosition(const AircraftPosition &ap, char* buffer)
{
    char* p = buffer;
    uint32_t ts = htonl(ap.msTimestamp);
    memcpy(p, &ts, sizeof(ts));
    p += sizeof(ts);
    int32_t i32 = htonl(static_cast<int32_t>(ap.latitude * (1 << 23))); // convert to 9.23 fixed point
    memcpy(p, &i32, sizeof(i32));
    p += sizeof(i32);
    i32 = htonl(static_cast<int32_t>(ap.longitude * (1 << 23))); // convert to 9.23 fixed point
    memcpy(p, &i32, sizeof(i32));
    p += sizeof(i32);
    i32 = htonl(static_cast<int32_t>(ap.altitude * (1 << 8))); // convert to 24.8 fixed point
    memcpy(p, &i32, sizeof(i32));
    p += sizeof(i32);
    uint16_t u16 = htons(static_cast<uint16_t>(ap.heading * (1 << 7))); // convert to 9.7 fixed point
    memcpy(p, &u16, sizeof(u16));
    p += sizeof(u16);
    int16_t i16 = htons(static_cast<int16_t>(ap.pitch * (1 << 7))); // convert to 9.7 fixed point
    memcpy(p, &i16, sizeof(i16));
    p += sizeof(i16);
    i16 = htons(static_cast<int16_t>(ap.roll * (1 << 7))); // convert to 9.7 fixed point
    memcpy(p, &i16, sizeof(i16));
    p += sizeof(i16);
    *p++ = static_cast<uint8_t>(ap.gear * 255);
    *p++ = static_cast<uint8_t>(ap.flap * 255);
    *p++ = static_cast<uint8_t>(ap.spoiler * 255);
    *p++ = static_cast<uint8_t>(ap.speedBrake * 255);
    *p++ = static_cast<uint8_t>(ap.slat * 255);
    *p++ = static_cast<uint8_t>(ap.sweep * 255);
    return (unsigned int)(p - buffer);
}

inline unsigned int DecodeAircraftPosition(AircraftPosition& ap, const char* buffer)
{
    const char* p = buffer;
    uint32_t ts;
    memcpy(&ts, p, sizeof(ts));
    ap.msTimestamp = ntohl(ts);
    p += sizeof(ts);
    int32_t i32;
    memcpy(&i32, p, sizeof(i32));
    i32 = ntohl(i32);
    ap.latitude = static_cast<double>(i32) / (1 << 23); // convert from 9.23 fixed point
    p += sizeof(i32);
    memcpy(&i32, p, sizeof(i32));
    i32 = ntohl(i32);
    ap.longitude = static_cast<double>(i32) / (1 << 23); // convert from 9.23 fixed point
    p += sizeof(i32);
    memcpy(&i32, p, sizeof(i32));
    i32 = ntohl(i32);
    ap.altitude = static_cast<double>(i32) / (1 << 8); // convert from 24.8 fixed point
    p += sizeof(i32);
    uint16_t u16;
    memcpy(&u16, p, sizeof(u16));
    u16 = ntohs(u16);
    ap.heading = static_cast<float>(u16) / (1 << 7); // convert from 9.7 fixed point
    p += sizeof(u16);
    int16_t i16;
    memcpy(&i16, p, sizeof(i16));
    i16 = ntohs(i16);
    ap.pitch = static_cast<float>(i16) / (1 << 7); // convert from 9.7 fixed point
    p += sizeof(i16);
    memcpy(&i16, p, sizeof(i16));
    i16 = ntohs(i16);
    ap.roll = static_cast<float>(i16) / (1 << 7); // convert from 9.7 fixed point
    p += sizeof(i16);
    const unsigned char* q = (const unsigned char*)p;
    ap.gear = ((float)*q++) / 255;
    ap.flap = ((float)*q++) / 255;
    ap.spoiler = ((float)*q++) / 255;
    ap.speedBrake = ((float)*q++) / 255;
    ap.slat = ((float)*q++) / 255;
    ap.sweep = ((float)*q++) / 255;
    return (unsigned int)((const char*)q - buffer);
}


}

