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

#include "prefs.h"
#include "ilogger.h"
#include "nlohmann/json.hpp"
#include <filesystem>
#include <fstream>
#include <iomanip>

namespace fwf {

std::shared_ptr<IPrefs> IPrefs::New(const char * f)
{
    LOG_INFO(1,"factory");
    return std::make_shared<PreferencesFile>(f);
}

PreferencesFile::PreferencesFile(const char * f)
:   filePath(f)
{
    LOG_INFO(1,"constructor, path=%s", f);
    prefs = std::make_shared<nlohmann::json>();
    LOG_INFO(1,"created json", f);
    Init();
    LOG_INFO(1,"inited json", f);
    Load();
    LOG_INFO(1,"loaded json", f);
}

PreferencesFile::~PreferencesFile()
{
    try {
        Save();
    } catch (...) {
        // can't rescue the process at this point
    }
}

void PreferencesFile::Init()
{
    // full init not required, just defaults that aren't zero/false/empty
    *prefs = { { "general", { { "prefs_version", 1 } } } };
}

void PreferencesFile::Load()
{
    nlohmann::json filedata;
    try {
        std::ifstream fin(filePath);
        fin >> filedata;
    } catch (const std::exception &) {
        LOG_WARN("Could not load user settings from %s, using defaults", filePath.c_str());
    }
    for (const auto &i : filedata.items()) {
        (*prefs)[i.key()] = i.value();
    }
}

void PreferencesFile::Save()
{
    try {
        std::ofstream fout(filePath);
        fout << std::setw(4) << *prefs;
    } catch (const std::exception &) {
        LOG_ERROR("Could not save user settings to %s", filePath.c_str());
    }
}

template<typename DATATYPE>
DATATYPE PreferencesFile::Get(const std::string &ptr, DATATYPE def) const
{
    return (*prefs).value(nlohmann::json::json_pointer(ptr), def);
}

template<typename DATATYPE>
void PreferencesFile::Set(const std::string &id, const DATATYPE value)
{
    (*prefs)[nlohmann::json::json_pointer(id)] = value;
}

std::string PreferencesFile::Name() const
{
    return Get("/dialog/pilot/name", std::string());
}

void PreferencesFile::Name(std::string & name)
{
    Set("/dialog/pilot/name", name);
}

std::string PreferencesFile::Callsign() const
{
    return Get("/dialog/pilot/callsign", std::string());
}

void PreferencesFile::Callsign(std::string & cs)
{
    Set("/dialog/pilot/callsign", cs);
}

int PreferencesFile::HostingPort() const
{
    return Get("/dialog/hosting/port", 1024);
}

void PreferencesFile::HostingPort(int p)
{
    Set("/dialog/hosting/port", p);
}

std::string PreferencesFile::ServerAddr() const
{
    return Get("/dialog/session/ipv4", std::string("1.2.3.4"));
}

void PreferencesFile::ServerAddr(std::string & a)
{
    Set("/dialog/session/ipv4", a);
}

int PreferencesFile::ServerPort() const
{
    return Get("/dialog/session/port", 1024);
}

void PreferencesFile::ServerPort(int p)
{
    Set("/dialog/session/port", p);
}

std::string PreferencesFile::Passcode() const
{
    return Get("/dialog/session/passcode", std::string("12345678"));
}

void PreferencesFile::Passcode(std::string & pc)
{
    Set("/dialog/session/passcode", pc);
}




}

