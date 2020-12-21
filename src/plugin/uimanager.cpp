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

#include "uimanager.h"
#include "ilogger.h"
#include "iengine.h"
#include "iprefs.h"
#include "hostdialog.h"
#include "joindialog.h"
#include "statusdialog.h"
#include "fwfporting.h"
#include "XPLMDataAccess.h"
#include "XPLMPlanes.h"

namespace fwf {

// TODO - get these from a json settings file
const int infoLogging = 1;
const int detailLogging = 0;

UIManager::UIManager(std::shared_ptr<IPrefs> p, std::shared_ptr<IEngine> e)
:   prefs(p),
    engine(e),
    activePlaneCount(-1),
    joinFS(-1),
    currentState(IDLE)
{
    char s[16];
    SPRINTF(s,"%u",prefs->HostingPort());
    hostingPort = s;
    pilotName = prefs->Name();
    pilotCallsign = prefs->Callsign();
    serverAddr = prefs->ServerAddr();
    SPRINTF(s,"%u",prefs->ServerPort());
    serverPort = s;
    sessionPasscode = prefs->Passcode();
}

UIManager::~UIManager()
{
    int pnum;
    if (SSCANF(hostingPort.c_str(), "%u", &pnum) == 1)
    {
        prefs->HostingPort(pnum);
    }
    prefs->Name(pilotName);
    prefs->Callsign(pilotCallsign);
    prefs->ServerAddr(serverAddr);
    if (SSCANF(serverPort.c_str(), "%u", &pnum) == 1)
    {
        prefs->ServerPort(pnum);
    }
    prefs->Passcode(sessionPasscode);
}

bool UIManager::AcquirePlanes()
{
    if (activePlaneCount >= 0) return true;

    int total = 0, active = 0;
    XPLMPluginID planesOwner;
    XPLMCountAircraft(&total, &active, &planesOwner);
    LOG_INFO(infoLogging, "active planes -> %d of %d total", active, total);

    if (XPLMAcquirePlanes(0, 0, 0))
    {
        LOG_INFO(infoLogging, "initial attempt to acquire planes -> successful");
        activePlaneCount = active;
        return true;
    }

    std::string stat[4];
    if (planesOwner < 0)
    {
        stat[0] = std::string("Couldn't acquire planes, but no other plugin has them.");
    }
    else
    {
        char name[256], signature[256];
        XPLMGetPluginInfo(planesOwner, name, nullptr, signature, nullptr);
        if (!strcmp(name, "JoinFS"))
        {
            // JoinFS seems to grab the planes on loading, but we assume that
            // it's fine to overrule this if our user has opened our dialog.
            joinFS = planesOwner;
            XPLMDisablePlugin(joinFS);
            if (XPLMAcquirePlanes(0, 0, 0))
            {
                LOG_INFO(infoLogging, "second attempt to acquire planes after disabling JoinFS -> successful");
                activePlaneCount = active;
                return true;
            }
            stat[0] = std::string("Couldn't acquire planes, even after disabling JoinFS.");
            XPLMEnablePlugin(joinFS);
            joinFS = -1;
        }
        else
        {
            stat[0] = std::string("Couldn't acquire planes, already owned by");
            stat[1] = std::string(name) + "[" + signature + "]";
            stat[2] = std::string("If this isn't expected try disabling the plugin.");
        }
    }

    CreateStatusDialog();
    for (int i=0; i<4; ++i) statusDialog->SetStatusText(i, stat[i].c_str());

    activePlaneCount = -1;
    return false;
}

void UIManager::ReleasePlanes()
{
    if (activePlaneCount >= 0)
    {
        XPLMReleasePlanes();
        activePlaneCount = -1;
        LOG_INFO(infoLogging, "released planes");
        if (joinFS >= 0) XPLMEnablePlugin(joinFS);
    }
}

void UIManager::UpdateAircraftCount()
{
    int total = 0, active = 0;
    XPLMPluginID planesOwner;
    XPLMCountAircraft(&total, &active, &planesOwner);
    LOG_INFO(infoLogging, "updated active planes -> now %d of %d total", active, total);
    if (activePlaneCount >= 0) activePlaneCount = active;
}

void UIManager::EnteredVR()
{
    switch (currentState)
    {
    case HOST_CONFIG_NO_ADDR:
    case HOST_CONFIG:
        hostingDialog->EnterVR();
        break;
    case JOIN_CONFIG:
        joiningDialog->EnterVR();
        break;
    case HOSTING:
    case JOINED:
        statusDialog->EnterVR();
        break;
    case IDLE:
    default:
        break;
    }
}

void UIManager::LeavingVR()
{
    switch (currentState)
    {
    case HOST_CONFIG_NO_ADDR:
    case HOST_CONFIG:
        hostingDialog->ExitVR();
        break;
    case JOIN_CONFIG:
        joiningDialog->ExitVR();
        break;
    case HOSTING:
    case JOINED:
        statusDialog->ExitVR();
        break;
    case IDLE:
    default:
        break;
    }
}

float UIManager::UpdateDialogs()
{
    float nextCall = (float)0.1;

    LOG_VERBOSE(detailLogging,"flight loop, state %d",currentState);
    switch (currentState)
    {
    case HOST_CONFIG_NO_ADDR:
        if (!hostingDialog->IsVisible())
        {
            // dialog was closed
            LOG_INFO(infoLogging,"hosting dialog was closed");
            ReleasePlanes();
            currentState = IDLE;
        }
        else if (engine->GetPublicIPAddress(hostingAddr))
        {
            hostingDialog->SetHostingAddress(hostingAddr.c_str());
            LOG_INFO(infoLogging,"public IP address is %s",hostingAddr.c_str());
            currentState = HOST_CONFIG;
        }
        break;
    case HOST_CONFIG:
        if (!hostingDialog->IsVisible())
        {
            // dialog was closed
            LOG_INFO(infoLogging,"hosting dialog was closed");
            ReleasePlanes();
            currentState = IDLE;
        }
        break;
    case JOIN_CONFIG:
        if (!joiningDialog->IsVisible())
        {
            // dialog was closed
            LOG_INFO(infoLogging,"joining dialog was closed");
            ReleasePlanes();
            currentState = IDLE;
        }
        break;
    case HOSTING:
    case JOINED:
        if (engine->IsSessionActive())
        {
            UpdateStatusDialog();
            nextCall = 1.0;
        }
        else
        {
            StopSession();
        }
        break;
    default:
        nextCall = 5.0;
        break;
    }

    return nextCall;
}

void UIManager::ConfigureHosting()
{
    LOG_INFO(infoLogging,"hosting selected (state %d)",currentState);
    if (!AcquirePlanes()) return;
    if ((currentState != IDLE) && (currentState != JOIN_CONFIG)) return;

    if (statusDialog) statusDialog->MakeVisible(false);
    if (joiningDialog && joiningDialog->IsVisible())
    {
        joiningDialog->MakeVisible(false);
    }
    if (!hostingDialog)
    {
        hostingDialog = std::make_unique<HostingDialog>(this);
        hostingDialog->CreateDialogWidgets();
        hostingDialog->SetName(pilotName.c_str());
        hostingDialog->SetCallsign(pilotCallsign.c_str());
        hostingDialog->SetHostingPort(hostingPort.c_str());
        hostingDialog->SetPasscode(sessionPasscode.c_str());
        hostingDialog->Activate();
    }
    else if (!hostingDialog->IsVisible())
    {
        hostingDialog->MakeVisible();
    }

    currentState = hostingAddr.size() ? HOST_CONFIG : HOST_CONFIG_NO_ADDR;
}

void UIManager::ConfigureJoining()
{
    LOG_INFO(infoLogging,"join session selected (state %d)",currentState);
    if (!AcquirePlanes()) return;
    if ((currentState != IDLE) && (currentState != HOST_CONFIG_NO_ADDR) && (currentState != HOST_CONFIG)) return;

    if (statusDialog) statusDialog->MakeVisible(false);
    if (hostingDialog && hostingDialog->IsVisible())
    {
        hostingDialog->MakeVisible(false);
    }
    if (!joiningDialog)
    {
        joiningDialog = std::make_unique<JoiningDialog>(this);
        joiningDialog->CreateDialogWidgets();
        joiningDialog->SetName(pilotName.c_str());
        joiningDialog->SetCallsign(pilotCallsign.c_str());
        joiningDialog->SetServerAddress(serverAddr.c_str());
        joiningDialog->SetServerPort(serverPort.c_str());
        joiningDialog->SetPasscode(sessionPasscode.c_str());
        joiningDialog->Activate();
    }
    else if (!joiningDialog->IsVisible())
    {
        joiningDialog->MakeVisible();
    }
    currentState = JOIN_CONFIG;
}

void UIManager::LeaveSession()
{
    if ((currentState == HOSTING) || (currentState == JOINED))
    {
        StopSession();
    }
}

void UIManager::ToggleStatus()
{
    if (!statusDialog)
    {
        CreateStatusDialog();
    }
    else
    {
        statusDialog->MakeVisible(!statusDialog->IsVisible());
    }
}

bool UIManager::IsVRenabled()
{
    XPLMDataRef dref = XPLMFindDataRef("sim/graphics/VR/enabled");
    int vrEnabled = dref ? XPLMGetDatai(dref) : false;
    return (vrEnabled != 0);
}

bool UIManager::IsRecording()
{
    return engine->IsRecording();
}

void UIManager::StartSession(const char *addr, const char *port, const char *name, const char *callsign, const char *passcode, bool logging)
{
    LOG_INFO(infoLogging,"start session (state %d)",currentState);
    if ((currentState != HOST_CONFIG) && (currentState != JOIN_CONFIG)) return;

    if (!strlen(name)) return;
    if (!strlen(callsign)) return;

    // convert port string to int (and back) to check validity
    int portNumber;
    if (SSCANF(port, "%u", &portNumber) != 1) return;
    char portStr[16];
    SPRINTF(portStr, "%u", portNumber);

    pilotName = name;
    pilotCallsign = callsign;
    sessionPasscode = passcode;
    if (currentState == HOST_CONFIG)
    {
        hostingPort = portStr;
    }
    else
    {
        serverAddr = addr;
        serverPort = portStr;
    }

    if (currentState == HOST_CONFIG)
    {
        engine->StartSessionServer(portNumber, passcode, logging);
        LOG_VERBOSE(detailLogging,"started session server (external app)");
    }
    if (!engine->JoinSession(addr, portNumber, name, callsign, passcode, logging)) return; // TODO - tell user why this failed
    LOG_VERBOSE(detailLogging,"started session client");

    if (hostingDialog) hostingDialog->MakeVisible(false);
    if (joiningDialog) joiningDialog->MakeVisible(false);
    LOG_VERBOSE(detailLogging,"closed dialog windows");

    CreateStatusDialog();

    currentState = (currentState == HOST_CONFIG) ? HOSTING : JOINED;
    LOG_INFO(infoLogging,"done start session (state now %d)",currentState);
}

void UIManager::StopSession()
{
    LOG_INFO(infoLogging,"stop session (state %d)",currentState);
    StopRecording();
    engine->LeaveSession();
    engine->StopSessionServer();
    statusDialog->MakeVisible(false);
    statusDialog->ResetStatusReport();
    ReleasePlanes();
    currentState = IDLE;
}

void UIManager::StartRecording()
{
    LOG_INFO(infoLogging, "start recording flight");
    engine->StartRecording();
    statusDialog->SetRecordingButton(1);
}

void UIManager::StopRecording()
{
    LOG_INFO(infoLogging, "stop recording flight");
    engine->StopRecording();
    statusDialog->SetRecordingButton(0);
}

void UIManager::CreateStatusDialog()
{
    if (!statusDialog)
    {
        LOG_VERBOSE(detailLogging, "creating status dialog");
        statusDialog = std::make_unique<StatusDialog>(this);
        statusDialog->CreateDialogWidgets();
        statusDialog->Activate();
        LOG_VERBOSE(detailLogging, "created status dialog");
    }
    else if (!statusDialog->IsVisible())
    {
        LOG_VERBOSE(detailLogging, "making status dialog visible");
        statusDialog->MakeVisible();
    }
}

void UIManager::UpdateStatusDialog()
{
    char sbuffer[64];
    std::string line;
    bool connected = engine->IsConnected(line);
    if (connected)
    {
        if (currentState == HOSTING)
        {
            line += " as " + pilotName + " to " + hostingAddr + ':' + hostingPort;
        }
        else
        {
            line += " as " + pilotName + " to " + serverAddr + ':' + serverPort;
        }
        unsigned int n = engine->CountOtherFliers();
        if (n > 1)
        {
            SPRINTF(sbuffer, "%u", n);
            line = line + " with " + sbuffer + " other fliers.";
        }
        else if (n == 1)
        {
            line += " with 1 other flier.";
        }
    }
    statusDialog->SetStatusText(0, line.c_str());

    for (unsigned int i = 1; i < 4; ++i)
    {
        if (!connected)
        {
            statusDialog->SetStatusText(i, "");
        }
        else
        {
            std::string nameCS;
            float distance;
            unsigned int bearing;
            bool active = engine->OtherFlier(i, nameCS, distance, bearing);
            SPRINTF(sbuffer, "%u: ", i);
            line = sbuffer;
            if (active)
            {
                if (distance < 1.0f)
                {
                    SPRINTF(sbuffer, " %dm", static_cast<int>(1000.0f * distance));
                }
                else if (distance < 10.0f)
                {
                    SPRINTF(sbuffer, " %0.1fkm", distance);
                }
                else
                {
                    SPRINTF(sbuffer, " %dkm", static_cast<int>(distance));
                }
                line = line + nameCS + sbuffer;
            }
            statusDialog->SetStatusText(i, line.c_str());
        }
    }
}

}
