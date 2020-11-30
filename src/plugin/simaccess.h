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
    unsigned int    ownedOthers;

    XPLMDataRef     userLatitude;
    XPLMDataRef     userLongitude;
    XPLMDataRef     userAltitude;
    XPLMDataRef     userPitch;
    XPLMDataRef     userRoll;
    XPLMDataRef     userHeading;
    XPLMDataRef     userGear;
    // TODO - extend with aircraft state

    XPLMDataRef     otherX[MAX_IN_SESSION];
    XPLMDataRef     otherY[MAX_IN_SESSION];
    XPLMDataRef     otherZ[MAX_IN_SESSION];
    XPLMDataRef     otherPitch[MAX_IN_SESSION];
    XPLMDataRef     otherRoll[MAX_IN_SESSION];
    XPLMDataRef     otherHeading[MAX_IN_SESSION];
    XPLMDataRef     otherGear[MAX_IN_SESSION];
    // TODO - extend with orientation and state
};

}

