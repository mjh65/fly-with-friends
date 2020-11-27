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
#if 0
    float           heading, pitch, roll;
    float           gear, flap, spoiler, speedBrake, slat, sweep;
#else
    double          heading, pitch, roll;
    double          gear, flap, spoiler, speedBrake, slat, sweep;
#endif
    static const unsigned int ENCODED_SIZE = sizeof(uint32_t) + (3 * sizeof(int32_t)) + (3 * sizeof(int16_t)) + (6 * sizeof(uint8_t));
};

// Callbacks from the engine to access X-Plane (or other host application)

class ISimData
{
public:
    static std::shared_ptr<ISimData> New();
    virtual void GetUserAircraftPosition(AircraftPosition &ap) = 0;
    virtual void SetOtherAircraftPosition(unsigned int id, AircraftPosition& ap) = 0;
};

}

