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

#include "fwfsocket.h"
#include "isimdata.h"
#include <stdint.h>
#include <mutex>
#include <list>

namespace fwf {

// Aircraft position, oriententation (and other state?)

struct AircraftPosition
{
    uint32_t        msTimestamp;
    double          latitude, longitude, altitude;
    double          heading, pitch, roll;
    double          gear, flap, spoiler, speedBrake, slat, sweep;
    static const unsigned int ENCODED_SIZE = sizeof(uint32_t) + (3 * sizeof(int32_t)) + (3 * sizeof(int16_t)) + (6 * sizeof(uint8_t));

    unsigned int EncodeTo(char* buffer);
    unsigned int DecodeFrom(const char* buffer);
};

class Aircraft
{
public:
    static unsigned int EncodedPositionLength() { return lenEncodedPosition; }

    Aircraft(uint32_t uuid);
    virtual ~Aircraft() = default;

    unsigned int SessionId() const { return sessionId; }
    uint32_t Uuid() const { return uuid; }
    std::string Name() const { return name; }
    std::string Callsign() const { return callsign; }

    void SetSessionId(unsigned int sid);
    virtual void SetName(const char* name);
    virtual void SetCallsign(const char* callsign);

    bool CheckCounter(unsigned int limit);
    void ResetCounter();

protected:
    std::mutex              guard;
    unsigned int            counter;

private:
    static unsigned int     lenEncodedPosition;

private:
    unsigned int            sessionId;
    uint32_t                uuid;
    std::string             name;
    std::string             callsign;

};


class SessionMember : public Aircraft
{
public:
    SessionMember(uint32_t uuid, SocketAddress& skt);

    const SocketAddress& Address() const { return address; }
    AircraftPosition Position();

    bool PendingPositionBroadcast();
    bool PendingNameBroadcast();
    uint32_t ExpiredUuid();

    void SetName(const char* name) override;
    void SetCallsign(const char* callsign) override;
    void SetPosition(AircraftPosition& ap);

protected:
private:
    SocketAddress       address;
    int                 nextIdBroadcast;
    bool                pending;
    AircraftPosition    position;

};


class TrackedAircraft : public Aircraft
{
public:
    TrackedAircraft(uint32_t uuid);

    AircraftPosition GetPrediction(uint32_t ts);
    void UpdateTracking(AircraftPosition& ap, uint32_t ts);

protected:
private:
    unsigned int                countReports;
    int                         tsOffset;
    AircraftPosition            reportedPrev;
    AircraftPosition            reportedLast;
    std::mutex                  targetGuard;
    AircraftPosition            current;
    AircraftPosition            target;
    AircraftPosition            deltas;
};




}
