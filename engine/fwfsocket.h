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

#include <stdint.h>
#include <string>

namespace fwf {

class IPv4Address
{
public:
    IPv4Address();
    IPv4Address(int a0, int a1, int a2, int a3);
    IPv4Address(uint32_t hostaddr);
    IPv4Address(const char * addrStr);
    void Reset();
    bool IsValid() const { return (address != 0); }
    uint32_t GetAsUInt32() const { return address; }
    int Get(unsigned int i) const;
    std::string GetQuad() const;
    void Set(const char * astr);
    void Set(int a0, int a1, int a2, int a3);

protected:
    uint32_t address;
};

class SocketAddress : public IPv4Address
{
public:
    SocketAddress();
    SocketAddress(IPv4Address a, int p);
    SocketAddress(int a0, int a1, int a2, int a3, int p);
    SocketAddress(uint32_t hostaddr, int p);
    void Reset();
    bool Equal(const SocketAddress & x) const;
    bool operator<(const SocketAddress& x) const;
    int GetPort() const { return port; }
    std::string GetAsString() const;

private:
    int port;
};

}

