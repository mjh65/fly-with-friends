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

#include "flightreplay.h"
#include "ilogger.h"
#include "fwfporting.h"
#include <cstring>

namespace fwf
{

//static const bool infoLogging = true;
static const bool verboseLogging = true;
//static const bool debugLogging = true;

inline std::string NextLine(char*& buffer)
{
    const char* eolChars = "\r\n";
    char* eol = strpbrk(buffer, eolChars);
    if (!eol) throw - 2;
    *eol = 0;
    std::string line(buffer);
    buffer = eol + 1;
    while (*buffer)
    {
        eol = strpbrk(buffer, eolChars);
        if (eol != buffer) break;
        ++buffer;
    }
    return line;
}

FlightReplay::FlightReplay(const char* p, unsigned int l)
:   path(p),
    fs(p, std::ios::in | std::ios::binary),
    loops(l),
    name(),
    callsign(),
    periodMs(100),
    samplesStartOffset(0),
    nextSampleOffset(0),
    clientLink(0),
    running(true),
    deltasLeft(0)
{
    if (!fs.is_open()) throw -1;

    buffer = std::make_unique<char[]>(bufferSize + 1);
    char* b = buffer.get();
    fs.read(b, bufferSize);
    unsigned int n = (unsigned int)fs.gcount();
    *(b + n) = 0; // make sure strpbrk() will have a terminating null

    fileSignature = NextLine(b);
    if ((fileSignature != "FWFA") && (fileSignature != "FWFB")) throw -3;

    name = NextLine(b).substr(0,31);
    callsign = NextLine(b).substr(0, 31);

    std::string feedRateString(NextLine(b));
    float rate;
    if (SSCANF(feedRateString.c_str(), "%f", &rate) != 1) throw - 4;
    periodMs = static_cast<unsigned int>((float)1000 / rate + 0.5);

    nextSampleOffset = samplesStartOffset = (unsigned int)(b - buffer.get());
}

FlightReplay::~FlightReplay()
{
    while ((replayDone.valid()) && (replayDone.wait_for(std::chrono::milliseconds(10)) != std::future_status::ready)) {}
}

void FlightReplay::Start(std::shared_ptr<fwf::ClientLink> cl)
{
    clientLink = cl;
    replayDone = std::async(std::launch::async, &FlightReplay::AsyncReplayFlight, this);
}

void FlightReplay::Stop()
{
    running = false;
}

bool FlightReplay::Completed()
{
    return (!replayDone.valid()) || (replayDone.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready);
}

const char* FlightReplay::Name() const
{
    return name.c_str();
}

const char* FlightReplay::Callsign() const
{
    return callsign.c_str();
}

bool FlightReplay::AsyncReplayFlight()
{
    auto nextWakeTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(periodMs);
    while (running && Next())
    {
        std::this_thread::sleep_until(nextWakeTime);
        position.msTimestamp = TIMENOWMS32();
        LOG_VERBOSE(verboseLogging, "send position, ts=%u, lat=%3.5f, lon=%3.5f", position.msTimestamp, position.latitude, position.longitude);
        clientLink->SendOurAircraftData(position);
        nextWakeTime += std::chrono::milliseconds(periodMs);
    }
    clientLink->LeaveSession();
    return false;
}

bool FlightReplay::Next()
{
    if (deltasLeft)
    {
        ApplyDelta();
        --deltasLeft;
        return true;
    }
    bool result = false;
    char* b = buffer.get();
    unsigned int bytesRead = 0;
    while (!bytesRead)
    {
        fs.clear();
        fs.seekg(nextSampleOffset);
        fs.read(b, bufferSize);
        bytesRead = (unsigned int)fs.gcount();
        if (!bytesRead)
        {
            if (--loops == 0) return false;
            nextSampleOffset = samplesStartOffset;
        }
    }

    if (fileSignature == "FWFA")
    {
        // position items are ascii CSV records
        *(b + bytesRead) = 0; // make sure strpbrk() will have a terminating null
        std::string csv;
        try
        {
            csv = NextLine(b);
        }
        catch (...)
        {
            return false;
        }
        result = DecodeAsciiCSV(csv);
        nextSampleOffset += (unsigned int)(b - buffer.get());
    }
    else if (fileSignature == "FWFB")
    {
        // posiiton items are native binary format
        nextSampleOffset += sizeof(AircraftPosition);
    }
    else if (fileSignature == "FWFX")
    {
        // posiiton items are hex encoded binary format
        nextSampleOffset += 2 * sizeof(AircraftPosition);
    }

    return result;
}

void FlightReplay::ApplyDelta()
{
    position.latitude += delta.latitude;
    position.longitude += delta.longitude;
    position.altitude += delta.altitude;
    position.heading += delta.heading;
    position.pitch += delta.pitch;
    position.roll += delta.roll;
    position.gear += delta.gear;
    position.flap += delta.flap;
    position.spoiler += delta.spoiler;
    position.speedBrake += delta.speedBrake;
    position.slat += delta.slat;
    position.sweep += delta.sweep;
}

bool FlightReplay::DecodeAsciiCSV(std::string& csv)
{
    const char* p = csv.c_str();
    const char* psep = strchr(p, ',');
    if (!psep || (psep == p)) return false;
    if ((psep - p) > 1)
    {
        unsigned int repeats = 0;
        if ((SSCANF(p + 1, "%u", &repeats) == 1) && (repeats > 1))
        {
            delta.latitude = delta.longitude = delta.altitude = 0.0;
            delta.heading = delta.pitch = delta.roll = 0.0;
            delta.gear = delta.flap = delta.spoiler = 0.0;
            delta.speedBrake = delta.slat = delta.sweep = 0.0;
            deltasLeft = repeats - 1;
        }
    }

    if (*p == 'A')
    {
        // absolute location and state
        double x, y, z;
        float i, j, k;
        float a, b, c, d, e, f;
        int nFields = SSCANF(psep + 1, "%lf,%lf,%lf,%f,%f,%f,%f,%f,%f,%f,%f,%f",
            &x, &y, &z, &i, &j, &k, &a, &b, &c, &d, &e, &f);
        if (nFields != 12) return false;
        position.latitude = x;
        position.longitude = y;
        position.altitude = z;
        position.heading = i; 
        position.pitch = j;
        position.roll = k;
        position.gear = a;
        position.flap = b;
        position.spoiler = c;
        position.speedBrake = d;
        position.slat = e;
        position.sweep = f;
    }
    else if (*p == 'R')
    {
        // relative location only
        double x, y, z;
        int nFields = SSCANF(psep + 1, "%lf,%lf,%lf", &x, &y, &z);
        if (nFields != 3) return false;
        delta.latitude = x;
        delta.longitude = y;
        delta.altitude = z;
        ApplyDelta();
    }
    else if (*p == 'Q')
    {
        // relative location and orientation
        double x, y, z;
        float i, j, k;
        int nFields = SSCANF(psep + 1, "%lf,%lf,%lf,%f,%f,%f", &x, &y, &z, &i, &j, &k);
        if (nFields != 6) return false;
        delta.latitude = x;
        delta.longitude = y;
        delta.altitude = z;
        delta.heading = i;
        delta.pitch = j;
        delta.roll = k;
        ApplyDelta();
    }
    else
    {
        return false;
    }
    return true;
}

}
