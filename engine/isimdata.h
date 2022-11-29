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

namespace fwf {

// Aircraft position, oriententation (and other state?)

struct AircraftPosition
{
    uint32_t        msTimestamp;
    double          latitude, longitude, altitude;
    double          heading, pitch, roll;
    double          rudder, elevator, aileron;
    double          speedbrake, flaps;
    bool            gear, beacon, strobe, navlight, taxilight, landlight;

    void Reset();
    double DistanceTo(double lat, double lon);      // lateral in km
    unsigned int EncodeTo(char* buffer);
    unsigned int DecodeFrom(const char* buffer);
};


// Callbacks from the engine to interact with the simulation
class ISimData
{
public:
    virtual void GetUserAircraftPosition(AircraftPosition &ap) = 0;
    virtual void ActiveOtherAircraft(unsigned int mask) = 0;
    virtual void SetOtherAircraftPosition(unsigned int id, AircraftPosition& ap) = 0;
};

}

