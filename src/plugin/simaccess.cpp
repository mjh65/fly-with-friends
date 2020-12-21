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
#include "aircraft.h"
#include "XPLMGraphics.h"
#include "XPLMPlanes.h"
#include <cassert>
#include <string>

namespace fwf {

static const bool infoLogging = true;
//static const bool verboseLogging = false;
static const bool debugLogging = false;

std::shared_ptr<ISimData> ISimData::New()
{
    return std::make_shared<SimAccess>();
}

SimAccess::SimAccess()
:   ownedOthers(0)
{
    allRudderV = XPLMFindDataRef("sim/multiplayer/controls/yoke_heading_ratio");
    assert(allRudderV);
    allElevatorV = XPLMFindDataRef("sim/multiplayer/controls/yoke_pitch_ratio");
    assert(allElevatorV);
    allAileronV = XPLMFindDataRef("sim/multiplayer/controls/yoke_roll_ratio");
    assert(allAileronV);
    allGearV = XPLMFindDataRef("sim/multiplayer/controls/gear_request");
    assert(allGearV);

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
    userSpeedbrake = XPLMFindDataRef("sim/cockpit2/controls/speedbrake_ratio");
    assert(userSpeedbrake);
    userFlaps = XPLMFindDataRef("sim/cockpit2/controls/flap_ratio");
    assert(userFlaps);
    userBeacon = XPLMFindDataRef("sim/cockpit2/switches/beacon_on");
    assert(userBeacon);
    userStrobe = XPLMFindDataRef("sim/cockpit2/switches/strobe_lights_on");
    assert(userStrobe);
    userNav = XPLMFindDataRef("sim/cockpit2/switches/navigation_lights_on");
    assert(userNav);
    userTaxi = XPLMFindDataRef("sim/cockpit2/switches/taxi_light_on");
    assert(userTaxi);
    userLanding = XPLMFindDataRef("sim/cockpit2/switches/landing_lights_on");
    assert(userLanding);

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
        otherSpeedbrake[i] = XPLMFindDataRef((b0 + d + "_speedbrake_ratio").c_str());
        assert(otherSpeedbrake[i]);
        otherFlaps1[i] = XPLMFindDataRef((b0 + d + "_flap_ratio").c_str());
        assert(otherFlaps1[i]);
        otherFlaps2[i] = XPLMFindDataRef((b0 + d + "_flap_ratio2").c_str());
        assert(otherFlaps2[i]);
        otherBeacon[i] = XPLMFindDataRef((b0 + d + "_beacon_lights_on").c_str());
        assert(otherBeacon[i]);
        otherStrobe[i] = XPLMFindDataRef((b0 + d + "_strobe_lights_on").c_str());
        assert(otherStrobe[i]);
        otherNavlight[i] = XPLMFindDataRef((b0 + d + "_nav_lights_on").c_str());
        assert(otherNavlight[i]);
        otherTaxilight[i] = XPLMFindDataRef((b0 + d + "_taxi_light_on").c_str());
        assert(otherTaxilight[i]);
        otherLandlight[i] = XPLMFindDataRef((b0 + d + "_landing_lights_on").c_str());
        assert(otherLandlight[i]);
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
        otherSpeedbrake[i] = XPLMFindDataRef((b1 + d + "_speedbrake_ratio").c_str());
        assert(otherSpeedbrake[i]);
        otherFlaps1[i] = XPLMFindDataRef((b1 + d + "_flap_ratio").c_str());
        assert(otherFlaps1[i]);
        otherFlaps2[i] = XPLMFindDataRef((b1 + d + "_flap_ratio2").c_str());
        assert(otherFlaps2[i]);
        otherBeacon[i] = XPLMFindDataRef((b1 + d + "_beacon_lights_on").c_str());
        assert(otherBeacon[i]);
        otherStrobe[i] = XPLMFindDataRef((b1 + d + "_strobe_lights_on").c_str());
        assert(otherStrobe[i]);
        otherNavlight[i] = XPLMFindDataRef((b1 + d + "_nav_lights_on").c_str());
        assert(otherNavlight[i]);
        otherTaxilight[i] = XPLMFindDataRef((b1 + d + "_taxi_light_on").c_str());
        assert(otherTaxilight[i]);
        otherLandlight[i] = XPLMFindDataRef((b1 + d + "_landing_lights_on").c_str());
        assert(otherLandlight[i]);
    }
}

SimAccess::~SimAccess()
{
}

void SimAccess::GetUserAircraftPosition(AircraftPosition& ap)
{
    int i;
    float f;

    ap.latitude = XPLMGetDatad(userLatitude);
    ap.longitude = XPLMGetDatad(userLongitude);
    ap.altitude = XPLMGetDatad(userAltitude);
    ap.heading = XPLMGetDataf(userHeading);
    ap.pitch = XPLMGetDataf(userPitch);
    ap.roll = XPLMGetDataf(userRoll);

    XPLMGetDatavf(allRudderV, &f, 0, 1);
    ap.rudder = f;
    XPLMGetDatavf(allElevatorV, &f, 0, 1);
    ap.elevator = f;
    XPLMGetDatavf(allAileronV, &f, 0, 1);
    ap.aileron = f;

    ap.speedbrake = XPLMGetDataf(userSpeedbrake);
    ap.flaps = XPLMGetDataf(userFlaps);
    XPLMGetDatavi(allGearV, &i, 0, 1);
    ap.gear = (i != 0);

    ap.beacon = (XPLMGetDatai(userBeacon) != 0);
    ap.strobe = (XPLMGetDatai(userStrobe) != 0);
    ap.navlight = (XPLMGetDatai(userNav) != 0);
    ap.taxilight = (XPLMGetDatai(userTaxi) != 0);
    ap.landlight = (XPLMGetDatai(userLanding) != 0);

#ifndef NDEBUG
    LOG_DEBUG(debugLogging, "A,  %f,%f,%f,   %f,%f,%f,   %f,%f,%f,   %f,%f,  %d,   %d,%d,%d,%d,%d",
        ap.latitude, ap.longitude, ap.altitude,
        ap.heading, ap.pitch, ap.roll,
        ap.rudder, ap.elevator, ap.aileron,
        ap.speedbrake, ap.flaps, ap.gear,
        ap.beacon, ap.strobe, ap.navlight, ap.taxilight, ap.landlight);
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

    float f;
    f = static_cast<float>(ap.rudder);
    XPLMSetDatavf(allRudderV, &f, id, 1);
    f = static_cast<float>(ap.elevator);
    XPLMSetDatavf(allElevatorV, &f, id, 1);
    f = static_cast<float>(ap.aileron);
    XPLMSetDatavf(allAileronV, &f, id, 1);

    XPLMSetDataf(otherSpeedbrake[id], static_cast<float>(ap.speedbrake));
    XPLMSetDataf(otherFlaps1[id], static_cast<float>(ap.flaps));
    XPLMSetDataf(otherFlaps2[id], static_cast<float>(ap.flaps));

    int i;
    i = (ap.gear ? 1 : 0);
    XPLMSetDatavi(allGearV, &i, id, 1);

    XPLMSetDatai(otherBeacon[id], (ap.beacon ? 1 : 0));
    XPLMSetDatai(otherStrobe[id], (ap.strobe ? 1 : 0));
    XPLMSetDatai(otherNavlight[id], (ap.navlight ? 1 : 0));
    XPLMSetDatai(otherTaxilight[id], (ap.taxilight ? 1 : 0));
    XPLMSetDatai(otherLandlight[id], (ap.landlight ? 1 : 0));
}

}

