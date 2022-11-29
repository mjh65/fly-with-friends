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

#include "isimdata.h"
#include "clientlink.h"
#include <string>
#include <fstream>

namespace fwf
{

class FlightReplay
{
public:
    FlightReplay(const char* path, unsigned int loops);
    ~FlightReplay();

    void Start(std::shared_ptr<fwf::ClientLink> cl);
    void Stop();
    bool Completed();

    const char* Name() const;
    const char* Callsign() const;

protected:
    bool AsyncReplayFlight();
    bool Next();
    void ApplyDelta();
    bool DecodeAsciiCSV(std::string& csv);

private:
    const std::string                   path;
    std::ifstream                       fs;
    unsigned int                        loops;
    std::string                         name;
    std::string                         callsign;
    unsigned                            periodMs;
    std::string                         fileSignature;
    unsigned int                        samplesStartOffset;
    unsigned int                        nextSampleOffset;
    static const unsigned int           bufferSize = 1024;
    std::unique_ptr<char[]>             buffer;
    std::shared_ptr<fwf::ClientLink>    clientLink;
    std::future<bool>                   replayDone;
    bool                                running;
    AircraftPosition                    position;
    unsigned int                        deltasLeft;
    AircraftPosition                    delta;

};

}
