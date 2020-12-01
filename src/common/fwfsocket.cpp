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

#include "fwfsocket.h"
#include "ilogger.h"
#include "fwfporting.h"

namespace fwf {

//static const bool infoLogging = true;
//static const bool verboseLogging = false;
static const bool debugLogging = false;

IPv4Address::IPv4Address()
:   address(0)
{
}

IPv4Address::IPv4Address(int a0, int a1, int a2, int a3)
{
    Set(a0, a1, a2, a3);
}

IPv4Address::IPv4Address(uint32_t sockaddr)
:   address(sockaddr)
{
}

IPv4Address::IPv4Address(const char * addrStr)
{
    Set(addrStr);
}

void IPv4Address::Reset()
{
    address = 0;
}

int IPv4Address::Get(unsigned int i) const
{
    if (i >= 4) return 0;
    return  ((address >> (8*(3-i))) & 0xff);
}

std::string IPv4Address::GetQuad() const
{
    if (!address) return std::string("-.-.-.-");
    char astr[16];
    SPRINTF(astr, "%d.%d.%d.%d", Get(0), Get(1), Get(2), Get(3));
    return std::string(astr);
}

void IPv4Address::Set(const char * addrStr)
{
    LOG_DEBUG(debugLogging,"addrStr=%s",addrStr);
    int a0,a1,a2,a3;
    if (SSCANF(addrStr, "%d.%d.%d.%d", &a0, &a1, &a2, &a3) == 4)
    {
        Set(a0, a1, a2, a3);
    }
}

void IPv4Address::Set(int a0, int a1, int a2, int a3)
{
    LOG_DEBUG(debugLogging,"a0=%d, a1=%d, a2=%d, a3=%d",a0,a1,a2,a3);
    address = ((a0 & 0xff) << 24) | ((a1 & 0xff) << 16)| ((a2 & 0xff) << 8) | (a3 & 0xff);
}

SocketAddress::SocketAddress()
:   IPv4Address(),
    port(0)
{
}

SocketAddress::SocketAddress(IPv4Address a, int p)
:   IPv4Address(a),
    port(p)
{
}

SocketAddress::SocketAddress(int a0, int a1, int a2, int a3, int p)
:   IPv4Address(a0, a1, a2, a3),
    port(p)
{
}

SocketAddress::SocketAddress(uint32_t hostaddr, int p)
:   IPv4Address(hostaddr),
    port(p)
{
}

void SocketAddress::Reset()
{
    IPv4Address::Reset();
    port = 0;
}

bool SocketAddress::Equal(const SocketAddress & x) const
{
    LOG_DEBUG(debugLogging,"this->IsValid()=%d, x.IsValid()=%d", IsValid(), x.IsValid());
    LOG_DEBUG(debugLogging,"this->GetAsUInt32()=%u, x.GetAsUInt32()=%u", GetAsUInt32(), x.GetAsUInt32());
    LOG_DEBUG(debugLogging,"this->port=%d, x.port=%d", port, x.port);
    return (IsValid() && x.IsValid() && (GetAsUInt32() == x.GetAsUInt32()) && (port == x.port));
}

bool SocketAddress::operator<(const SocketAddress& x) const
{
    if (address < x.address) return true;
    if (address > x.address) return false;
    if (port < x.port) return true;
    return false;
}

std::string SocketAddress::GetAsString() const
{
    char ps[8];
    SPRINTF(ps, "%u", port);
    std::string s = GetQuad() + ":" + std::string(ps);
    return s;
}


}

