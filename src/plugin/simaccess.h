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
#include "fwfconsts.h"
#include "XPLMDataAccess.h"

namespace fwf {

class SimAccess : public ISimData
{
public:
    SimAccess();
    virtual ~SimAccess();

    void UpdatePositions();

    // Implementation of ISimData
    void GetUserAircraftPosition(AircraftPosition& ap) override;
    void SetOtherAircraftPosition(unsigned int id, AircraftPosition& ap) override;

protected:
private:
    // bitfield indicating which planes we have claimed
    unsigned int    ownedOthers;

    // vector datarefs to read[0] for user and to write[1..19] for other/AI aircraft
    XPLMDataRef     allRudderV, allElevatorV, allAileronV;
    XPLMDataRef     allGearV;

    // scalar datarefs to read for user aircraft
    XPLMDataRef     userLatitude;
    XPLMDataRef     userLongitude;
    XPLMDataRef     userAltitude;
    XPLMDataRef     userPitch;
    XPLMDataRef     userRoll;
    XPLMDataRef     userHeading;
    XPLMDataRef     userSpeedbrake;     // sim/cockpit2/controls/speedbrake_ratio [0..1]
    XPLMDataRef     userFlaps;          // sim/cockpit2/controls/flap_ratio [0..1]
    XPLMDataRef     userBeacon;         // sim/cockpit2/switches/beacon_on [0,1]
    XPLMDataRef     userStrobe;         // sim/cockpit2/switches/strobe_lights_on [0,1]
    XPLMDataRef     userNav;            // sim/cockpit2/switches/navigation_lights_on [0,1]
    XPLMDataRef     userTaxi;           // sim/cockpit2/switches/taxi_light_on [0,1]
    XPLMDataRef     userLanding;        // sim/cockpit2/switches/landing_lights_on [0,1]

    // arrays of scalar datarefs to write for other/AI aircraft
    XPLMDataRef     otherX[MAX_IN_SESSION];
    XPLMDataRef     otherY[MAX_IN_SESSION];
    XPLMDataRef     otherZ[MAX_IN_SESSION];
    XPLMDataRef     otherPitch[MAX_IN_SESSION];
    XPLMDataRef     otherRoll[MAX_IN_SESSION];
    XPLMDataRef     otherHeading[MAX_IN_SESSION];
    XPLMDataRef     otherSpeedbrake[MAX_IN_SESSION];
    XPLMDataRef     otherFlaps1[MAX_IN_SESSION], otherFlaps2[MAX_IN_SESSION];
    XPLMDataRef     otherBeacon[MAX_IN_SESSION], otherStrobe[MAX_IN_SESSION];
    XPLMDataRef     otherNavlight[MAX_IN_SESSION], otherTaxilight[MAX_IN_SESSION], otherLandlight[MAX_IN_SESSION];
};

}

