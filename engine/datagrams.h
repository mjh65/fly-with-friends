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
#include "fwfsocket.h"
#include "isimdata.h"
#include <memory>
#include <cassert>
#include <chrono>

namespace fwf {

inline uint64_t TIMENOWMS() { return (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()); }

// Wrapper for UDP datagrams.

class Datagram
{
public:
    enum CommandCode {
        REPORT,             // client->server, send a report of position and identity
        LEAVING,            // client->server, graceful sign out of session
        WORLDSTATE,         // server->client, share all positions, and periodic single identities
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
    std::string LogString();
protected:
private:
    std::shared_ptr<Datagram>   datagram;
    SocketAddress               address;
};

}

