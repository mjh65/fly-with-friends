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

#include "XPLMPlugin.h"
#include <memory>
#include <string>

namespace fwf {

class IPrefs;
class IEngine;
class SimAccess;
class Dialog;
class HostingDialog;
class JoiningDialog;
class StatusDialog;


class UIManager
{
public:
    UIManager(std::shared_ptr<IPrefs> p, std::shared_ptr<IEngine> e);
    ~UIManager();

    // called from X-Plane DLL main routines
    bool AcquirePlanes();
    void ReleasePlanes();
    void EnteredVR();
    void LeavingVR();
    void ConfigureHosting();
    void ConfigureJoining();
    void LeaveSession();
    void ToggleStatus();
    float UpdateDialogs();

    // called from dialog UI handlers 
    bool IsVRenabled();
    bool IsRecording();
    void StartSession(const char *addr, const char *port, const char *name, const char *callsign, const char *passcode, bool logging);
    void StopSession();
    void StartRecording();
    void StopRecording();

protected:
    void CreateStatusDialog();
    void UpdateStatusDialog();

private:
    enum State
    {
        IDLE, HOST_CONFIG_NO_ADDR, HOST_CONFIG, HOSTING, JOIN_CONFIG, JOINED
    };

private:
    std::shared_ptr<IPrefs>         prefs;
    std::shared_ptr<IEngine>        engine;
    bool                            planesAreOwned;
    XPLMPluginID                    joinFS;
    std::unique_ptr<HostingDialog>  hostingDialog;
    std::unique_ptr<JoiningDialog>  joiningDialog;
    std::unique_ptr<StatusDialog>   statusDialog;
    std::string                     hostingAddr;
    std::string                     hostingPort;
    std::string                     pilotName;
    std::string                     pilotCallsign;
    std::string                     serverAddr;
    std::string                     serverPort;
    std::string                     sessionPasscode;
    State                           currentState;
};

}

