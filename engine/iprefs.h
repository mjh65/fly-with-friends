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

#include <memory>
#include <string>

namespace fwf {

// Interface for object providing saved user preferences

class IPrefs
{
public:
    static std::shared_ptr<IPrefs> New(const char * filepath);
    virtual ~IPrefs() {}

    virtual void Save() = 0;

    virtual std::string Name() const = 0;
    virtual void Name(std::string &) = 0;
    virtual std::string Callsign() const = 0;
    virtual void Callsign(std::string &) = 0;
    virtual int HostingPort() const = 0;
    virtual void HostingPort(int) = 0;
    virtual std::string ServerAddr() const = 0;
    virtual void ServerAddr(std::string &) = 0;
    virtual int ServerPort() const = 0;
    virtual void ServerPort(int) = 0;
    virtual std::string Passcode() const = 0;
    virtual void Passcode(std::string &) = 0;

};

}

