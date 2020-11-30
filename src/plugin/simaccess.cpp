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

#include "simaccess.h"
#include "ilogger.h"
#include "XPLMGraphics.h"
#include "XPLMPlanes.h"
#include <cassert>
#include <string>

namespace fwf {

static const bool infoLogging = true;
static const bool verboseLogging = false;
static const bool debugLogging = false;

std::shared_ptr<ISimData> ISimData::New()
{
    return std::make_shared<SimAccess>();
}

SimAccess::SimAccess()
:   ownedOthers(0)
{
    userLatitude = XPLMFindDataRef("sim/flightmodel/position/latitude");
    assert(userLatitude);
    userLongitude = XPLMFindDataRef("sim/flightmodel/position/longitude");
    assert(userLongitude);
    userAltitude = XPLMFindDataRef("sim/flightmodel/position/elevation");
    assert(userAltitude);
    userPitch = XPLMFindDataRef("sim/flightmodel/position/theta");
    assert(userPitch);
    userRoll = XPLMFindDataRef("sim/flightmodel/position/phi");
    assert(userRoll);
    userHeading = XPLMFindDataRef("sim/flightmodel/position/psi");
    assert(userHeading);
    userGear = XPLMFindDataRef("sim/flightmodel/movingparts/gear1def"); // only using this one to represent all
    assert(userGear);

    std::string b0("sim/multiplayer/position/plane");
    for (unsigned int i = 1; i < 10; ++i) {
        char d = '0' + i;
        otherX[i] = XPLMFindDataRef((b0 + d + "_x").c_str());
        assert(otherX[i]);
        otherY[i] = XPLMFindDataRef((b0 + d + "_y").c_str());
        assert(otherY[i]);
        otherZ[i] = XPLMFindDataRef((b0 + d + "_z").c_str());
        assert(otherZ[i]);
        otherPitch[i] = XPLMFindDataRef((b0 + d + "_the").c_str());
        assert(otherPitch[i]);
        otherRoll[i] = XPLMFindDataRef((b0 + d + "_phi").c_str());
        assert(otherRoll[i]);
        otherHeading[i] = XPLMFindDataRef((b0 + d + "_psi").c_str());
        assert(otherHeading[i]);
        otherGear[i] = XPLMFindDataRef((b0 + d + "_gear_deploy").c_str());
        assert(otherGear[i]);
    }
    std::string b1("sim/multiplayer/position/plane1");
    for (unsigned int i = 10; i < MAX_IN_SESSION; ++i) {
        char d = '0' + (i-10);
        otherX[i] = XPLMFindDataRef((b1 + d + "_x").c_str());
        assert(otherX[i]);
        otherY[i] = XPLMFindDataRef((b1 + d + "_y").c_str());
        assert(otherY[i]);
        otherZ[i] = XPLMFindDataRef((b1 + d + "_z").c_str());
        assert(otherZ[i]);
        otherPitch[i] = XPLMFindDataRef((b1 + d + "_the").c_str());
        assert(otherPitch[i]);
        otherRoll[i] = XPLMFindDataRef((b1 + d + "_phi").c_str());
        assert(otherRoll[i]);
        otherHeading[i] = XPLMFindDataRef((b1 + d + "_psi").c_str());
        assert(otherHeading[i]);
        otherGear[i] = XPLMFindDataRef((b1 + d + "_gear_deploy").c_str());
        assert(otherGear[i]);
    }

}

SimAccess::~SimAccess()
{
}

void SimAccess::GetUserAircraftPosition(AircraftPosition& ap)
{
    ap.latitude = XPLMGetDatad(userLatitude);
    ap.longitude = XPLMGetDatad(userLongitude);
    ap.altitude = XPLMGetDatad(userAltitude);
    ap.heading = XPLMGetDataf(userHeading);
    ap.pitch = XPLMGetDataf(userPitch);
    ap.roll = XPLMGetDataf(userRoll);
    ap.gear = XPLMGetDataf(userGear);
    ap.flap = ap.spoiler = ap.speedBrake = ap.slat = ap.sweep = 0.0f;
#ifndef NDEBUG
    LOG_DEBUG(debugLogging, "A,%f,%f,%f,   %f,%f,%f,   %f,%f,%f,%f,%f,%f",
        ap.latitude, ap.longitude, ap.altitude, ap.heading, ap.pitch, ap.roll,
        ap.gear, ap.flap, ap.spoiler, ap.speedBrake, ap.slat, ap.sweep);
#endif
}

void SimAccess::SetOtherAircraftPosition(unsigned int id, AircraftPosition& ap)
{
    if (!(ownedOthers & (1 << id)))
    {
        LOG_INFO(infoLogging, "Disabling AI for plane %d, taking control", id);
        XPLMDisableAIForPlane(id);
        ownedOthers |= (1 << id);
    }
    if ((id < 1) || (id >= MAX_IN_SESSION)) return;
#ifndef NDEBUG
    LOG_DEBUG(debugLogging, "SET(%d)  %f,%f,%f,   %f,%f,%f", id,
        ap.latitude, ap.longitude, ap.altitude, ap.heading, ap.pitch, ap.roll);
#endif
    double x, y, z;
    XPLMWorldToLocal(ap.latitude, ap.longitude, ap.altitude, &x, &y, &z);
    XPLMSetDatad(otherX[id], x);
    XPLMSetDatad(otherY[id], y);
    XPLMSetDatad(otherZ[id], z);
    XPLMSetDataf(otherHeading[id], static_cast<float>(ap.heading));
    XPLMSetDataf(otherPitch[id], static_cast<float>(ap.pitch));
    XPLMSetDataf(otherRoll[id], static_cast<float>(ap.roll));
    float gear[10];
    for (unsigned int i=0; i<10; ++i) gear[i] = static_cast<float>(ap.gear);
    XPLMSetDatavf(otherGear[id], gear, 0, 10);
}

}

