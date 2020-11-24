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

#include "engine.h"
#include "isimdata.h"
#include "ilogger.h"
#include <ctime>

namespace fwf {

static const bool infoLogging = true;
static const bool verboseLogging = false;

std::shared_ptr<IEngine> IEngine::New(std::shared_ptr<fwf::ISimData> si)
{
    return std::make_shared<Engine>(si);
}

Engine::Engine(std::shared_ptr<ISimData> si)
:   simulation(si),
    findDelay(0),
    loopNumber(0),
    frameNumber(0)
{
}

Engine::~Engine()
{
    LeaveSession();
    StopSessionServer();
    if (clientLink)
    { 
        for (size_t i=0; i<10; ++i)
        {
            if (clientLink->Inactive())
            {
                clientLink.reset();
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
}

bool Engine::GetPublicIPAddress(std::string & ipaddr)
{
    // called from UI/control for display in various windows
    bool result = false;
    std::string astr;
    if (publicInetAddr.IsValid())
    {
        ipaddr = publicInetAddr.GetQuad();
        result = true;
    }
    if (addrFinder && (astr = addrFinder->Finished()).size())
    {
        addrFinder.reset();
        findDelay = 1000;
        publicInetAddr.Set(astr.c_str());
    }
    if (findDelay)
    {
        --findDelay;
    }
    if (!result && !addrFinder && !findDelay)
    {
        addrFinder = std::make_unique<IPAddrFinder>();
    }
    return result;
}

void Engine::StartSessionServer(const int p, const char * passcode)
{
    if (!sessionHub)
    {
        sessionHub = std::make_unique<SessionHub>(p, passcode);
    }
}

void Engine::StopSessionServer()
{
    LOG_INFO(infoLogging,"stop session server");
    if (sessionHub) sessionHub->Stop();
    sessionHub.reset();
}

bool Engine::JoinSession(const char * addr, const int port, const char * name, const char * callsign, const char * passcode)
{
    // called from UI/control when user wants to join session
    IPv4Address a(addr);
    if (!a.IsValid()) return false;
    try
    {
        clientLink = std::make_unique<ClientLink>(a, port, name, callsign, passcode);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

void Engine::LeaveSession()
{
    if (clientLink) clientLink->LeaveSession();
}

std::string Engine::StatusSummary()
{
    char status[120];

    std::string addr;
    int port;
    unsigned int peerCount;
    bool connected = clientLink->Connected(addr, port, peerCount);

    if (connected)
    {
        sprintf_s(status, "Session active [%s:%d] %u other flier%s", addr.c_str(), port, peerCount, ((peerCount == 1) ? "" : "s"));
    }
    else
    {
        sprintf_s(status, "Connecting ...");
    }
    return std::string(status);
}

std::string Engine::StatusDetail(unsigned int i)
{
    std::string detail("< n/c >");
    unsigned int peerCount;
    bool connected = clientLink->Connected(peerCount);

    if (connected)
    {
        std::string name, callsign;
        if (clientLink->GetFlierIdentifiers(i, name, callsign))
        {
            detail = name + " / " + callsign; 
        }
    }
    return detail;
}

float Engine::DoFlightLoop(unsigned int n, unsigned int ms)
{
    static unsigned int lastTimeInfo = 0;
    float nextCall = (float)0.1; // might want to reduce this to get per-frame updating
    loopNumber = n;
    milliSeconds = ms;
    ++frameNumber;
#ifndef _MSC_VER // TODO - sort out the localtime stuff for MSVC
    if (infoLogging)
    {
        if (ms > (lastTimeInfo + 5000))
        {
            std::time_t time_now = std::time(nullptr);
            char timeBuffer[32];
            if (!strftime(timeBuffer, 32, "%H:%M:%S", std::localtime(&time_now))) strcpy(timeBuffer,"--:--:--");
            LOG_INFO(infoLogging,"+++ %s +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++",timeBuffer);
            LOG_INFO(infoLogging,"%d ms since previous log anchor");
            lastTimeInfo = ms;
        }
    }
#endif
    LOG_VERBOSE(verboseLogging,"flight loop %u - %u - %ums", frameNumber, loopNumber, milliSeconds);

    // if no current session then next call in 10s
    if (!clientLink) return 10.0;
    if (clientLink->Inactive())
    {
        clientLink.reset();
        return 10.0;
    }

    AircraftPosition us;
    simulation->GetUserAircraftPosition(us);
    AircraftPosition others[MAX_IN_SESSION];
    unsigned int active = 0;
    nextCall = clientLink->RunCycle(us, others, &active);
    for (unsigned int i = 0; i < MAX_IN_SESSION; ++i)
    {
        if (active & (1 << i))
        {
            simulation->SetOtherAircraftPosition(i+1, *(others + i));
        }
    }

    unsigned int peerCount;
    if (!clientLink->Connected(peerCount)) return (float)1.0;

    return nextCall;
}

unsigned int Engine::LoopNumber()
{
    std::lock_guard<std::mutex> lock(guard);
    return loopNumber;
}

unsigned int Engine::Milliseconds()
{
    std::lock_guard<std::mutex> lock(guard);
    return milliSeconds;
}

}
