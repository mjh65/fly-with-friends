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

#include "iprefs.h"
#include "nlohmann/json_fwd.hpp"

namespace fwf {

class PreferencesFile : public IPrefs
{
public:
    PreferencesFile(const char * filepath);
    virtual ~PreferencesFile();

    std::string Name() const override;
    void Name(std::string &) override;
    std::string Callsign() const override;
    void Callsign(std::string &) override;
    int HostingPort() const override;
    void HostingPort(int) override;
    std::string ServerAddr() const override;
    void ServerAddr(std::string &) override;
    int ServerPort() const override;
    void ServerPort(int) override;
    std::string Passcode() const override;
    void Passcode(std::string &) override;

protected:
    void Init();
    void Load();
    void Save();

    template<typename DATATYPE>
    DATATYPE Get(const std::string &ptr, DATATYPE def) const;

    template<typename DATATYPE>
    void Set(const std::string &id, const DATATYPE value);

private:
    const std::string                   filePath;
    std::shared_ptr<nlohmann::json>     prefs;
#if 0
    std::string                         name;
    std::string                         callsign;
    int                                 hostingPort;
    std::string                         serverAddr;
    int                                 serverPort;
    std::string                         passcode;
#endif
};

}

