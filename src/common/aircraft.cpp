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

namespace fwf {

//static const bool infoLogging = true;
//static const bool verboseLogging = false;
static const bool debugLogging = false;






unsigned int AircraftPosition::EncodeTo(char* buffer)
{
    char* p = buffer;
    uint32_t ts = htonl(msTimestamp);
    memcpy(p, &ts, sizeof(ts));
    p += sizeof(ts);
    int32_t i32 = htonl(static_cast<int32_t>(latitude * (1 << 23))); // convert to 9.23 fixed point
    memcpy(p, &i32, sizeof(i32));
    p += sizeof(i32);
    i32 = htonl(static_cast<int32_t>(longitude * (1 << 23))); // convert to 9.23 fixed point
    memcpy(p, &i32, sizeof(i32));
    p += sizeof(i32);
    i32 = htonl(static_cast<int32_t>(altitude * (1 << 8))); // convert to 24.8 fixed point
    memcpy(p, &i32, sizeof(i32));
    p += sizeof(i32);
    uint16_t u16 = htons(static_cast<uint16_t>(heading * (1 << 7))); // convert to 9.7 fixed point
    memcpy(p, &u16, sizeof(u16));
    p += sizeof(u16);
    int16_t i16 = htons(static_cast<int16_t>(pitch * (1 << 7))); // convert to 9.7 fixed point
    memcpy(p, &i16, sizeof(i16));
    p += sizeof(i16);
    i16 = htons(static_cast<int16_t>(roll * (1 << 7))); // convert to 9.7 fixed point
    memcpy(p, &i16, sizeof(i16));
    p += sizeof(i16);
    *p++ = static_cast<uint8_t>(gear * 255);
    *p++ = static_cast<uint8_t>(flap * 255);
    *p++ = static_cast<uint8_t>(spoiler * 255);
    *p++ = static_cast<uint8_t>(speedBrake * 255);
    *p++ = static_cast<uint8_t>(slat * 255);
    *p++ = static_cast<uint8_t>(sweep * 255);
    return (unsigned int)(p - buffer);
}

unsigned int AircraftPosition::DecodeFrom(const char* buffer)
{
    const char* p = buffer;
    uint32_t ts;
    memcpy(&ts, p, sizeof(ts));
    msTimestamp = ntohl(ts);
    p += sizeof(ts);
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
    const unsigned char* q = (const unsigned char*)p;
    gear = ((float)*q++) / 255;
    flap = ((float)*q++) / 255;
    spoiler = ((float)*q++) / 255;
    speedBrake = ((float)*q++) / 255;
    slat = ((float)*q++) / 255;
    sweep = ((float)*q++) / 255;
    return (unsigned int)((const char*)q - buffer);
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
    : Aircraft(u),
    countReports(0),
    msLatency(0)
{
#ifdef ORIG_SMOOTHING_ALGORITHM
    deltaRates.latitude = deltaRates.longitude = deltaRates.altitude = 0.0f;
    deltaRates.heading = deltaRates.pitch = deltaRates.roll = 0.0f;
    deltaRates.gear = deltaRates.flap = deltaRates.spoiler = 0.0f;
    deltaRates.speedBrake = deltaRates.slat = deltaRates.sweep = 0.0f;
#endif
}

#ifdef ORIG_SMOOTHING_ALGORITHM

AircraftPosition TrackedAircraft::GetPrediction(uint32_t ms)
{
    // this is a simple algorithm. a better one would aim towards the goal from the last prediction,
    // not from the last reported position, which would remove some of the glitches

    std::lock_guard<std::mutex> lock(guard);
    int timeSinceLastReport = (ms - msLatency) - reportedLast.msTimestamp; // can go negative while we're tuning latency
    double deltaScale = static_cast<double>(timeSinceLastReport) / 1000.0f;
    LOG_DEBUG(debugLogging, "prediction required for %u, is %d after last position @ %u, applying scale %f to deltas", ms, timeSinceLastReport, reportedLast.msTimestamp, deltaScale);

    predicted.latitude = reportedLast.latitude + (deltaScale * deltaRates.latitude);
    while (predicted.latitude <= -180.0f) predicted.latitude += 360.0f;
    while (predicted.latitude > 180.0f) predicted.latitude -= 360.0f;
    predicted.longitude = reportedLast.longitude + (deltaScale * deltaRates.longitude);
    while (predicted.longitude < -180.0f) predicted.longitude = -180.0f;
    while (predicted.longitude > 180.0f) predicted.longitude = 180.0f;
    predicted.altitude = reportedLast.altitude + (deltaScale * deltaRates.altitude);
    predicted.heading = static_cast<float>(reportedLast.heading + (deltaScale * deltaRates.heading));
    while (predicted.heading < 0.0) predicted.heading += 360.0;
    while (predicted.heading >= 360.0) predicted.heading -= 360.0;
    predicted.pitch = static_cast<float>(reportedLast.pitch + (deltaScale * deltaRates.pitch));
    predicted.roll = static_cast<float>(reportedLast.roll + (deltaScale * deltaRates.roll));
    predicted.gear = static_cast<float>(reportedLast.gear + (deltaScale * deltaRates.gear));

    return predicted;
}

void TrackedAircraft::UpdateTracking(AircraftPosition& ap, uint32_t ts)
{
    LOG_DEBUG(debugLogging, "@ %u rcvd position, ts=%u, lat=%3.5f, lon=%3.5f", ts, ap.msTimestamp, ap.latitude, ap.longitude);
    std::lock_guard<std::mutex> lock(guard);
    if (countReports == 0)
    {
        // first time just initialise some stuff
        reportedLast = ap;
        msLatency = ts - ap.msTimestamp;
        LOG_DEBUG(debugLogging, "initial latency = %u", msLatency);
    }
    else
    {
        // all other times tune the latency and recalculate the prediction deltas
        if ((ts - ap.msTimestamp) < msLatency)
        {
            --msLatency;
            LOG_DEBUG(debugLogging, "tuning down latency to %u", msLatency);
        }
        reportedPrev = reportedLast;
        reportedLast = ap;

        // this is a simple algorithm. a better one would 
        double scaleQ = 1000.0f / static_cast<double>(reportedLast.msTimestamp - reportedPrev.msTimestamp);
        LOG_DEBUG(debugLogging, "scale factor to create rate/s = %f", scaleQ);
        deltaRates.latitude = scaleQ * (reportedLast.latitude - reportedPrev.latitude);
        deltaRates.longitude = scaleQ * (reportedLast.longitude - reportedPrev.longitude);
        deltaRates.altitude = scaleQ * (reportedLast.altitude - reportedPrev.altitude);
        deltaRates.heading = static_cast<float>(scaleQ * (static_cast<double>(reportedLast.heading) - reportedPrev.heading));
        deltaRates.pitch = static_cast<float>(scaleQ * (static_cast<double>(reportedLast.pitch) - reportedPrev.pitch));
        deltaRates.roll = static_cast<float>(scaleQ * (static_cast<double>(reportedLast.roll) - reportedPrev.roll));
        deltaRates.gear = static_cast<float>(scaleQ * (static_cast<double>(reportedLast.gear) - reportedPrev.gear));
    }
    ++countReports;
    counter = 0;
}

#else

inline double Next(double r, double s0, double s1)
{
    return s0 + r * (s1 - s0);
}

inline double Next(double r, double s0, double s1, double q1, double q2)
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

AircraftPosition TrackedAircraft::GetPrediction(uint32_t ms)
{
    std::lock_guard<std::mutex> lock(targetGuard);
    // move current towards the target (always 'targetDeltaMs' away)
    if (ms > current.msTimestamp)
    {
        double r = static_cast<double>(ms - current.msTimestamp) / targetDeltaMs;
        current.msTimestamp = ms;
        current.latitude = Next(r, current.latitude, target.latitude);
        current.longitude = Next(r, current.longitude, target.longitude, -90.0f, 90.0f);
        current.altitude = Next(r, current.altitude, target.altitude);
        current.heading = Next(r, current.heading, target.heading, 90.0f, 270.0f);
        current.pitch = Next(r, current.pitch, target.pitch, -90.0f, 90.0f);
        current.roll = Next(r, current.roll, target.roll, -90.0f, 90.0f);
        current.gear = Next(r, current.gear, target.gear);
    }
    return current;
}

inline double Target(double r, double s0, double s1)
{
    return s1 + r * (s1 - s0);
}

inline double Target(double r, double s0, double s1, double q1, double q2)
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

void TrackedAircraft::UpdateTracking(AircraftPosition& ap, uint32_t ts)
{
    // create a new aiming point, which will be 'targetDeltaMs' ahead of the current position
    if (countReports == 0)
    {
        // first time just initialise some stuff
        reportedLast = current = ap;
        msLatency = ts - ap.msTimestamp;
    }
    else
    {
        // all other times tune the latency and recalculate the target
        if ((ts - ap.msTimestamp) < msLatency)
        {
            --msLatency;
            LOG_DEBUG(debugLogging, "tuning down latency to %u", msLatency);
        }
        reportedPrev = reportedLast;
        reportedLast = ap;

        // take a copy of the current location (mutexted) for the next calculations,
        AircraftPosition here;
        {
            std::lock_guard<std::mutex> lock(targetGuard);
            here = current;
        }

        // work out the scaling factor which will be applied to all pairs of prev/last samples
        // TODO - should we factor in the latency here?? it probably doesn't really matter for this version of the algorithm
        double r = static_cast<double>(static_cast<unsigned int>(here.msTimestamp) + targetDeltaMs - reportedLast.msTimestamp) / (reportedLast.msTimestamp - reportedPrev.msTimestamp);

        AircraftPosition there;
        there.latitude = Target(r, reportedPrev.latitude, reportedLast.latitude);
        there.longitude = Target(r, reportedPrev.longitude, reportedLast.longitude, -90.0f, 90.0f);
        there.altitude = Target(r, reportedPrev.altitude, reportedLast.altitude);
        there.heading = Target(r, reportedPrev.heading, reportedLast.heading, 90.0f, 270.0f);
        there.pitch = Target(r, reportedPrev.pitch, reportedLast.pitch, -90.0f, 90.0f);
        there.roll = Target(r, reportedPrev.roll, reportedLast.roll, -90.0f, 90.0f);
        there.gear = Target(r, reportedPrev.gear, reportedLast.gear);

        {
            std::lock_guard<std::mutex> lock(targetGuard);
            target = there;
        }
    }

    ++countReports;
    counter = 0;
}

#endif

}
