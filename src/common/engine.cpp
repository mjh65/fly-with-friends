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
    nextNetworkUpdateTime = std::chrono::steady_clock::now();
    return true;
}

void Engine::LeaveSession()
{
    if (clientLink) clientLink->LeaveSession();
}

void Engine::StartRecording()
{
}

void Engine::StopRecording()
{
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

float Engine::DoFlightLoop()
{
    ++frameNumber;

    // if no current session then next call in 10s
    if (!clientLink) return 10.0;
    if (clientLink->Inactive())
    {
        clientLink.reset();
        return 10.0;
    }

    uint32_t msnow = (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() & 0xffffffff);

    // is it time to update the server with our latest position etc?
    if (std::chrono::steady_clock::now() >= nextNetworkUpdateTime)
    {
        AircraftPosition position;
        simulation->GetUserAircraftPosition(position);
        position.msTimestamp = msnow;
        clientLink->SendOurAircraftData(position);
        nextNetworkUpdateTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(CLIENT_UPDATE_PERIOD_MS);
    }

    // get the latest (integrated) positions of the other aircraft
    unsigned int activeOthers = clientLink->ActiveAircraftCount();
    if (activeOthers > 0)
    {
        AircraftPosition others[MAX_IN_SESSION];
        unsigned int active = clientLink->GetActiveMemberPositions(others, msnow);
        for (unsigned int i = 0; i < MAX_IN_SESSION; ++i)
        {
            if (active & (1 << i))
            {
                simulation->SetOtherAircraftPosition(i + 1, *(others + i));
            }
        }
    }

    // if there are other aircraft in the session we want to update every frame,
    // otherwise we only need to do it enough to keep the connection alive
    const float NEXT_FRAME = -1.0;
    const float IDLE_PERIOD = 1000.0f / SERVER_BROADCAST_PERIOD_MS;
    return (clientLink->ActiveAircraftCount() > 0) ? NEXT_FRAME : IDLE_PERIOD;
}

}
