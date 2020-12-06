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
#include "fwfporting.h"
#include <ctime>
#include <sstream>
#include <iomanip>

namespace fwf {

static const bool infoLogging = true;

std::shared_ptr<IEngine> IEngine::New(std::shared_ptr<fwf::ISimData> si, std::string& dir)
{
    return std::make_shared<Engine>(si, dir);
}

Engine::Engine(std::shared_ptr<ISimData> si, std::string& dir)
:   workDirPath(dir),
    simulation(si),
    findDelay(0),
    frameNumber(0)
{
}

Engine::~Engine()
{
    StopRecording();
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

void Engine::StartSessionServer(const int p, const char * passcode, bool logging)
{
    if (!sessionHub)
    {
        if (logging)
        {
            sessionHub = std::make_unique<SessionHub>(p, passcode, workDirPath.c_str());

        }
        else
        {
            sessionHub = std::make_unique<SessionHub>(p, passcode);
        }
    }
}

void Engine::StopSessionServer()
{
    LOG_INFO(infoLogging,"stop session server");
    if (sessionHub) sessionHub->Stop();
    sessionHub.reset();
}

bool Engine::JoinSession(const char * addr, const int port, const char * name, const char * callsign, const char * passcode, bool logging)
{
    // called from UI/control when user wants to join session
    IPv4Address a(addr);
    if (!a.IsValid()) return false;
    try
    {
        if (logging)
        {
            clientLink = std::make_unique<ClientLink>(a, port, name, callsign, passcode, workDirPath.c_str());
        }
        else
        {
            clientLink = std::make_unique<ClientLink>(a, port, name, callsign, passcode);
        }
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

bool Engine::IsSessionActive()
{
    if (!clientLink) return false;
    return !(clientLink->Inactive());
}

void Engine::StartRecording()
{
    if (recording || !clientLink) return;

    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    struct tm timeinfo;
    LOCALTIME(&in_time_t, &timeinfo);
    std::stringstream ss;
    ss << workDirPath << std::put_time(&timeinfo, "FlightLog-%Y-%m-%d-%H-%M.fwf");
    recording = std::make_unique<std::ofstream>(ss.str().c_str());
    (*recording) << "FWFA" << std::endl;
    (*recording) << "Recorded Flight" << std::endl;
    (*recording) << "ROBOT" << std::endl;
    (*recording) << (1000.0f / CLIENT_UPDATE_PERIOD_MS) << std::endl;
}

void Engine::StopRecording()
{
    if (!recording) return;
    recording.reset();
}

float Engine::DoFlightLoop()
{
    ++frameNumber;

    // if no current session then next call in 10s
    if (!clientLink) return 10.0;

    // is it time to update the server with our latest position etc?
    if (std::chrono::steady_clock::now() >= nextNetworkUpdateTime)
    {
        nextNetworkUpdateTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(CLIENT_UPDATE_PERIOD_MS);

        AircraftPosition position;
        simulation->GetUserAircraftPosition(position);
        clientLink->SetCurrentPosition(position);
        if (recording)
        {
            std::stringstream ss;
            ss << "A," << std::setprecision(16)
                << position.latitude << ',' << position.longitude << ',' << position.altitude << ','
                << position.heading << ',' << position.pitch << ',' << position.roll << ','
                << position.gear << ',' << position.flap << ',' << position.spoiler << ','
                << position.speedBrake << ',' << position.slat << ',' << position.sweep;
            (*recording) << ss.str() << std::endl;
        }
    }

    // get the latest (integrated) positions of the other aircraft
    unsigned int activeOthers = clientLink->ActiveAircraftCount();
    if (activeOthers > 0)
    {
        AircraftPosition others[MAX_IN_SESSION];
        unsigned int active = clientLink->GetCurrentAircraftPositions(others);
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
    return ((clientLink->ActiveAircraftCount() > 0) || (recording)) ? NEXT_FRAME : IDLE_PERIOD;
}

bool Engine::IsConnected(std::string& desc)
{
    bool connected = clientLink->Connected(desc);
    return connected;
}

unsigned int Engine::CountOtherFliers()
{
    return clientLink->ActiveAircraftCount();
}

bool Engine::OtherFlier(unsigned int id, std::string& nameCS, float& distance, unsigned int& bearing)
{
    return clientLink->OtherFlier(id, nameCS, distance, bearing);
}

}
