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

#include "aircraft.h"
#include "datagrams.h"
#include "ilogger.h"
#include <cmath>

namespace fwf {

//static const bool infoLogging = true;
//static const bool verboseLogging = false;
static const bool debugLogging = false;

void AircraftPosition::Reset()
{
    latitude = longitude = altitude = 0.0f;
    heading = pitch = roll = 0.0f;
    rudder = elevator = aileron = 0.0f;
    speedbrake = flaps = 0.0f;
    gear = beacon = strobe = navlight = taxilight = landlight = false;
}

double AircraftPosition::DistanceTo(double lat, double lon)
{
    double p = 0.017453292519943295f;    // pi/180
    double a = 0.5f - cos((lat - latitude) * p) / 2 + cos(latitude * p) * cos(lat * p) * (1 - cos((lon - longitude) * p)) / 2;
    return 12742.0f * asin(sqrt(a)); // 2 * R; R = 6371 km
}

unsigned int AircraftPosition::EncodeTo(char* buffer)
{
    char* p = buffer;

    // timestamp
    uint32_t ts = htonl(msTimestamp);
    memcpy(p, &ts, sizeof(ts));
    p += sizeof(ts);

    // position
    int32_t i32 = htonl(static_cast<int32_t>(latitude * (1 << 23))); // convert to 9.23 fixed point
    memcpy(p, &i32, sizeof(i32));
    p += sizeof(i32);
    i32 = htonl(static_cast<int32_t>(longitude * (1 << 23))); // convert to 9.23 fixed point
    memcpy(p, &i32, sizeof(i32));
    p += sizeof(i32);
    i32 = htonl(static_cast<int32_t>(altitude * (1 << 8))); // convert to 24.8 fixed point
    memcpy(p, &i32, sizeof(i32));
    p += sizeof(i32);

    // orientation
    uint16_t u16 = htons(static_cast<uint16_t>(heading * (1 << 7))); // convert to 9.7 fixed point
    memcpy(p, &u16, sizeof(u16));
    p += sizeof(u16);
    int16_t i16 = htons(static_cast<int16_t>(pitch * (1 << 7))); // convert to 9.7 fixed point
    memcpy(p, &i16, sizeof(i16));
    p += sizeof(i16);
    i16 = htons(static_cast<int16_t>(roll * (1 << 7))); // convert to 9.7 fixed point
    memcpy(p, &i16, sizeof(i16));
    p += sizeof(i16);

    // control surfaces
    *p++ = static_cast<int8_t>(rudder * 127); // -1.0 .. 1.0 ==> -127 .. 127
    *p++ = static_cast<int8_t>(elevator * 127); // -1.0 .. 1.0 ==> -127 .. 127
    *p++ = static_cast<int8_t>(aileron * 127); // -1.0 .. 1.0 ==> -127 .. 127
    *p++ = static_cast<uint8_t>(speedbrake * 255); // 0.0 .. 1.0 ==> 0 .. 255
    *p++ = static_cast<uint8_t>(flaps * 255); // 0.0 .. 1.0 ==> 0 .. 255

    // gear and lights
    *p++ = static_cast<uint8_t>((gear ? 0x1 : 0) | (beacon ? 0x2 : 0) | (strobe ? 0x4 : 0) | (navlight ? 0x8 : 0) | (taxilight ? 0x10 : 0) | (landlight ? 0x20 : 0));

    return (unsigned int)(p - buffer);
}

unsigned int AircraftPosition::DecodeFrom(const char* buffer)
{
    const char* p = buffer;

    // timestamp
    uint32_t ts;
    memcpy(&ts, p, sizeof(ts));
    msTimestamp = ntohl(ts);
    p += sizeof(ts);

    // position
    int32_t i32;
    memcpy(&i32, p, sizeof(i32));
    i32 = ntohl(i32);
    latitude = static_cast<double>(i32) / (1 << 23); // convert from 9.23 fixed point
    p += sizeof(i32);
    memcpy(&i32, p, sizeof(i32));
    i32 = ntohl(i32);
    longitude = static_cast<double>(i32) / (1 << 23); // convert from 9.23 fixed point
    p += sizeof(i32);
    memcpy(&i32, p, sizeof(i32));
    i32 = ntohl(i32);
    altitude = static_cast<double>(i32) / (1 << 8); // convert from 24.8 fixed point
    p += sizeof(i32);

    // orientation
    uint16_t u16;
    memcpy(&u16, p, sizeof(u16));
    u16 = ntohs(u16);
    heading = static_cast<float>(u16) / (1 << 7); // convert from 9.7 fixed point
    p += sizeof(u16);
    int16_t i16;
    memcpy(&i16, p, sizeof(i16));
    i16 = ntohs(i16);
    pitch = static_cast<float>(i16) / (1 << 7); // convert from 9.7 fixed point
    p += sizeof(i16);
    memcpy(&i16, p, sizeof(i16));
    i16 = ntohs(i16);
    roll = static_cast<float>(i16) / (1 << 7); // convert from 9.7 fixed point
    p += sizeof(i16);

    // control surfaces
    const int8_t* i8 = (const int8_t*)p;
    rudder = ((float)*i8++) / 127;
    elevator = ((float)*i8++) / 127;
    aileron = ((float)*i8++) / 127;
    const uint8_t* u8 = (const uint8_t*)i8;
    speedbrake = ((float)*u8++) / 255;
    flaps = ((float)*u8++) / 255;

    // gear and lights
    uint8_t switches = *u8++;
    gear = (switches & 0x1) != 0;
    beacon = (switches & 0x2) != 0;
    strobe = (switches & 0x4) != 0;
    navlight = (switches & 0x8) != 0;
    taxilight = (switches & 0x10) != 0;
    landlight = (switches & 0x20) != 0;

    return (unsigned int)((const char*)u8 - buffer);
}

inline unsigned int calcEpl()
{
    char b[sizeof(AircraftPosition)];
    AircraftPosition ap;
    return ap.EncodeTo(b);
}

unsigned int Aircraft::lenEncodedPosition = calcEpl();

Aircraft::Aircraft(uint32_t u)
:   counter(0),
    sessionId(0xff),
    uuid(u),
    name(),
    callsign()
{
}

void Aircraft::SetSessionId(unsigned int sid)
{
    std::lock_guard<std::mutex> lock(guard);
    sessionId = sid;
}

void Aircraft::SetName(const char* n)
{
    if (n)
    {
        std::lock_guard<std::mutex> lock(guard);
        name = std::string(n);
    }
}

void Aircraft::SetCallsign(const char* cs)
{
    if (cs)
    {
        std::lock_guard<std::mutex> lock(guard);
        callsign = std::string(cs);
    }
}

bool Aircraft::CheckCounter(unsigned int limit)
{
    std::lock_guard<std::mutex> lock(guard);
    if (++counter > limit) return true;
    return false;
}

void Aircraft::ResetCounter()
{
    std::lock_guard<std::mutex> lock(guard);
    counter = 0;
}


SessionMember::SessionMember(uint32_t u, SocketAddress& s)
    : Aircraft(u),
    address(s),
    nextIdBroadcast(0),
    pending(false)
{
}

AircraftPosition SessionMember::Position()
{
    return position;
}

uint32_t SessionMember::ExpiredUuid()
{
    std::lock_guard<std::mutex> lock(guard);
    ++counter;
    return uint32_t();
}

bool SessionMember::PendingPositionBroadcast()
{
    std::lock_guard<std::mutex> lock(guard);
    bool r = pending;
    pending = false;
    return r;
}

bool SessionMember::PendingNameBroadcast()
{
    std::lock_guard<std::mutex> lock(guard);
    bool r = false;
    if (--nextIdBroadcast < 0)
    {
        r = true;
        nextIdBroadcast = (ID_REBROADCAST_PERIOD_MS / SERVER_BROADCAST_PERIOD_MS);
    }
    return r;
}

void SessionMember::SetName(const char* n)
{
    Aircraft::SetName(n);
    std::lock_guard<std::mutex> lock(guard);
    nextIdBroadcast = 0;
}

void SessionMember::SetCallsign(const char* cs)
{
    Aircraft::SetCallsign(cs);
    std::lock_guard<std::mutex> lock(guard);
    nextIdBroadcast = 0;
}

void SessionMember::SetPosition(AircraftPosition& ap)
{
    std::lock_guard<std::mutex> lock(guard);
    position = ap;
    pending = true;
    counter = 0;
}





TrackedAircraft::TrackedAircraft(uint32_t u)
:   Aircraft(u),
    countReports(0),
    tsOffset(0)
{
}

float TrackedAircraft::Distance()
{
    std::lock_guard<std::mutex> lock(guard);
    return distance;
}

unsigned int TrackedAircraft::Bearing()
{
    return 0;
}

inline double Next(double r, double s0, double s1)
{
    return s0 + r * (s1 - s0);
}

inline double LimitedNext(double r, double s0, double s1, double l, double u)
{
    double n = Next(r, s0, s1);
    if (n < l) return l;
    if (n > u) return u;
    return n;
}

inline double WrappedNext(double r, double s0, double s1, double q1, double q2)
{
    if ((s0 < q1) && (s1 > q2))
    {
        // crossing the wraparound from below
        double range = 2.0f * (q2 - q1);
        return Next(r, s0 + range, s1) - range;
    }
    else if ((s1 < q1) && (s0 > q2))
    {
        // crossing the wraparound from above
        double range = 2.0f * (q2 - q1);
        return Next(r, s0, s1 + range) - range;
    }
    // both samples on the same side of the divide
    return Next(r, s0, s1);
}

AircraftPosition TrackedAircraft::GetPrediction(uint32_t ptime)
{
    std::lock_guard<std::mutex> lock(targetGuard); // no update to predictions while we're in here
    if ((countReports > 2) && (ptime > current.msTimestamp))
    {
        if (ptime >= target.msTimestamp)
        {
            // if we've 'overflown' the target then only update the lat/lon using the deltas
            double r = static_cast<double>(ptime - current.msTimestamp) / 1000.0f;
            current.latitude += (r * deltas.latitude);
            current.longitude += (r * deltas.longitude);
            if (current.longitude < -180.0f) current.longitude += 360.0f;
            if (current.longitude >= 180.0f) current.longitude -= 360.0f;
        }
        else
        {
            double r = static_cast<double>(ptime - current.msTimestamp) / (target.msTimestamp - current.msTimestamp);
            current.latitude = LimitedNext(r, current.latitude, target.latitude, -180.0f, 180.0f);
            current.longitude = WrappedNext(r, current.longitude, target.longitude, -90.0f, 90.0f);
            current.altitude = Next(r, current.altitude, target.altitude);
            current.heading = WrappedNext(r, current.heading, target.heading, 90.0f, 270.0f);
            current.pitch = WrappedNext(r, current.pitch, target.pitch, -90.0f, 90.0f);
            current.roll = WrappedNext(r, current.roll, target.roll, -90.0f, 90.0f);
            current.rudder = LimitedNext(r, current.rudder, target.rudder, -1.0f, 1.0f);
            current.elevator = LimitedNext(r, current.elevator, target.elevator, -1.0f, 1.0f);
            current.aileron = LimitedNext(r, current.aileron, target.aileron, -1.0f, 1.0f);
            current.speedbrake = target.speedbrake;
            current.flaps = target.flaps;
            current.gear = target.gear;
            current.beacon = target.beacon;
            current.strobe = target.strobe;
            current.navlight = target.navlight;
            current.taxilight = target.taxilight;
            current.landlight = target.landlight;
        }
        current.msTimestamp = ptime;
    }
    return current;
}



inline double Target(double r, double s0, double s1)
{
    return s1 + r * (s1 - s0);
}

inline double WrappedTarget(double r, double s0, double s1, double q1, double q2)
{
    if ((s0 < q1) && (s1 > q2))
    {
        // crossing the wraparound from below
        double range = 2.0f * (q2 - q1);
        return Target(r, s0 + range, s1) - range;
    }
    else if ((s1 < q1) && (s0 > q2))
    {
        // crossing the wraparound from above
        double range = 2.0f * (q2 - q1);
        return Target(r, s0, s1 + range) - range;
    }
    // both samples on the same side of the divide
    return Target(r, s0, s1);
}


inline double Delta(unsigned int t, double s0, double s1)
{
    return ((1000.0f * (s1 - s0)) / t);
}

inline double WrappedDelta(unsigned int t, double s0, double s1, double q1, double q2)
{
    if ((s0 < q1) && (s1 > q2))
    {
        // crossing the wraparound from below
        double range = 2.0f * (q2 - q1);
        return Delta(t, s0 + range, s1) - range;
    }
    else if ((s1 < q1) && (s0 > q2))
    {
        // crossing the wraparound from above
        double range = 2.0f * (q2 - q1);
        return Delta(t, s0, s1 + range) - range;
    }
    // both samples on the same side of the divide
    return Delta(t, s0, s1);
}

void TrackedAircraft::UpdateTracking(AircraftPosition& ap, uint32_t tsRcvd, double lat, double lon)
{
    // create a new aiming point, which will be 'targetDeltaMs' ahead of the current position
    if (countReports == 0)
    {
        // first time just initialise some stuff
        tsOffset = (int)tsRcvd - (int)ap.msTimestamp;
        ap.msTimestamp += tsOffset; // localise the timestamp
        FWF_LOG_DEBUG(debugLogging, "initial timestamp offset = %d", tsOffset);
        std::lock_guard<std::mutex> lock(targetGuard);
        reportedLast = current = ap;
    }
    else
    {
        // all other times tune the offset and recalculate the target
        if (((int)tsRcvd - (int)ap.msTimestamp) < tsOffset)
        {
            --tsOffset;
            FWF_LOG_DEBUG(debugLogging, "tuning down latency to %d", tsOffset);
        }
        ap.msTimestamp += tsOffset; // localise the timestamp
        reportedPrev = reportedLast;
        reportedLast = ap;

        // work out the scaling factor which will be applied to all pairs of prev/last samples
        uint32_t sampleDistance = reportedLast.msTimestamp - reportedPrev.msTimestamp;

        // set the lat/lon deltas in case we (temporally) overrun the target without getting any further samples
        AircraftPosition nextTarget;
        double dlat;
        double dlon;

        double lateralDistance = reportedPrev.DistanceTo(reportedLast.latitude, reportedLast.longitude);
        double speed = 1.0e6 * lateralDistance / sampleDistance;
        FWF_LOG_DEBUG(debugLogging, "sample->sample  dist=%0.5fkm, t=%ums, speed=%0.5f m/s", lateralDistance, sampleDistance, speed);
        if (speed > 1000.0)
        {
            // seems likely the aircraft has been relocated rather than flown.
            // just set the next target location to the aircraft's new location
            nextTarget.msTimestamp = tsRcvd;
            nextTarget = reportedLast;
            dlat = dlon = 0;
        }
        else
        {
            // calculate a target location based on the difference between the last 2 samples
            double r = static_cast<double>((tsRcvd + PREDICTION_INTERCEPT_MS) - reportedLast.msTimestamp) / sampleDistance;
            nextTarget.msTimestamp = tsRcvd + PREDICTION_INTERCEPT_MS;
            nextTarget.latitude = Target(r, reportedPrev.latitude, reportedLast.latitude);
            nextTarget.longitude = WrappedTarget(r, reportedPrev.longitude, reportedLast.longitude, -90.0f, 90.0f);
            nextTarget.altitude = Target(r, reportedPrev.altitude, reportedLast.altitude);
            nextTarget.heading = WrappedTarget(r, reportedPrev.heading, reportedLast.heading, 90.0f, 270.0f);
            nextTarget.pitch = WrappedTarget(r, reportedPrev.pitch, reportedLast.pitch, -90.0f, 90.0f);
            nextTarget.roll = WrappedTarget(r, reportedPrev.roll, reportedLast.roll, -90.0f, 90.0f);
            nextTarget.rudder = reportedLast.rudder;
            nextTarget.elevator = reportedLast.elevator;
            nextTarget.aileron = reportedLast.aileron;
            nextTarget.speedbrake = reportedLast.speedbrake;
            nextTarget.flaps = reportedLast.flaps;
            nextTarget.gear = reportedLast.gear;
            nextTarget.beacon = reportedLast.beacon;
            nextTarget.strobe = reportedLast.strobe;
            nextTarget.navlight = reportedLast.navlight;
            nextTarget.taxilight = reportedLast.taxilight;
            nextTarget.landlight = reportedLast.landlight;

            // set the lat/lon deltas in case we (temporally) overrun the target without getting any further samples
            dlat = Delta(sampleDistance, reportedPrev.latitude, reportedLast.latitude);
            dlon = WrappedDelta(sampleDistance, reportedPrev.longitude, reportedLast.longitude, -90.0f, 90.0f);
        }

        {
            std::lock_guard<std::mutex> lock(targetGuard);
            target = nextTarget;
            deltas.latitude = dlat;
            deltas.longitude = dlon;
        }
    }

    {
        float dUs = static_cast<float>(ap.DistanceTo(lat, lon));
        std::lock_guard<std::mutex> lock(guard);
        distance = dUs;
    }

    ++countReports;
    counter = 0;
}

}
