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

#include "ilogger.h"
#include "iengine.h"
#include "isimdata.h"
#include "iprefs.h"
#include "fwfporting.h"
#include "uimanager.h"
#include "XPLMPlugin.h"
#include "XPLMProcessing.h"
#include "XPLMUtilities.h"
#include "XPLMMenus.h"
#include <memory>

#ifndef XPLM300
#error This is made to be compiled against the XPLM300 SDK
#endif

static std::unique_ptr<fwf::ILogger>    logger;
static std::unique_ptr<fwf::UIManager>  uimanager;
static std::shared_ptr<fwf::IPrefs>     prefs;
static std::shared_ptr<fwf::IEngine>    engine;
static std::shared_ptr<fwf::ISimData>   simdata;

static int pluginsMenuFWF;
static XPLMMenuID fwfMenu;
static int fwfSubMenuHost;
static int fwfSubMenuJoin;
static int fwfSubMenuLeave;
static int fwfSubMenuToggleStatus;
static int fwfSubMenuReload;

static XPLMCommandRef fwfCommandSessionHost;
static XPLMCommandRef fwfCommandSessionJoin;
static XPLMCommandRef fwfCommandRecordingStart;
static XPLMCommandRef fwfCommandRecordingStop;
static XPLMCommandRef fwfCommandRecordingToggle;

static XPLMFlightLoopID fwfFlightLoop;

static const bool infoLogging = true;
static const bool verboseLogging = true;
//static const bool debugLogging = false;

extern "C" {

static void MenuHandler(void *, void *);
static int CommandHandler(XPLMCommandRef, XPLMCommandPhase, void*);
static float FlightLoop(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void * inRefcon);

PLUGIN_API int XPluginStart(char * outName, char * outSig, char * outDesc)
{
    STRCPY(outName, 256, "Fly-With-Friends");
    STRCPY(outSig, 256, "hasling.michael.flywithfriends");
    STRCPY(outDesc, 256, "A lightweight plugin for group flying");

    XPLMEnableFeature("XPLM_USE_NATIVE_PATHS",1);
    XPLMEnableFeature("XPLM_USE_NATIVE_WIDGET_WINDOWS",1);

    XPLMPluginID ourId = XPLMGetMyID();
    char path[1024];
    XPLMGetPluginInfo(ourId, nullptr, path, nullptr, nullptr);
    char *filePart = XPLMExtractFileAndPath(path);
    if (filePart > path)
    {
        STRCPY(filePart-1, (1024-(filePart-path)), "/../");
    }
    else
    {
        XPLMGetSystemPath(path);
    }
    std::string dirPath(path);

    STRCAT(path, "fwf.log");
    std::string logFile(path);

    XPLMGetPrefsPath(path);
    filePart = XPLMExtractFileAndPath(path);
    STRCPY(filePart-1, (1024 - (filePart - path)), "/flywithfriends.prf");
    std::string prefsFile(path);

    if (!(logger = std::unique_ptr<fwf::ILogger>(fwf::ILogger::New(logFile.c_str()))))
    {
        return false;
    }
    LOG_INFO(infoLogging, "starting ...");

    int xPlaneVersion;
    int xplmVersion;
    XPLMHostApplicationID hostID;
    XPLMGetVersions(&xPlaneVersion, &xplmVersion, &hostID);
    if (xPlaneVersion < 1150)
    {
        LOG_ERROR("Fly-With-Friends needs X-Plane 11.5 or later");
        logger.reset();
        return false;
    }

    // create all the main subsystems, and then check and share interfaces
    auto p = fwf::IPrefs::New(prefsFile.c_str()); prefs = p;
    auto s = fwf::ISimData::New(); simdata = s;
    auto e = fwf::IEngine::New(simdata, dirPath); engine = e;
    uimanager = std::unique_ptr<fwf::UIManager>(new fwf::UIManager(prefs, engine));

    if ((!uimanager) || (!simdata) || (!engine) || (!prefs))
    {
        LOG_ERROR("failed to create subsystems");
        uimanager.reset();
        engine.reset();
        simdata.reset();
        prefs.reset();
        return false;
    }
    
    // Create menus
    pluginsMenuFWF = XPLMAppendMenuItem(XPLMFindPluginsMenu(), "FlyWithFriends", NULL, 1);
    fwfMenu = XPLMCreateMenu("FlyWithFriends", XPLMFindPluginsMenu(), pluginsMenuFWF, MenuHandler, NULL);
    fwfSubMenuHost = XPLMAppendMenuItem(fwfMenu, "Host new session ...", (void *)"host", 1);
    fwfSubMenuJoin = XPLMAppendMenuItem(fwfMenu, "Join session ...", (void *)"join", 1);
    fwfSubMenuLeave = XPLMAppendMenuItem(fwfMenu, "Leave session", (void*)"leave", 1);
    fwfSubMenuToggleStatus = XPLMAppendMenuItem(fwfMenu, "Toggle status window", (void*)"toggle", 1);
    fwfSubMenuReload = XPLMAppendMenuItem(fwfMenu, "Reload plugins", (void *)"reload", 1);

    // Create commands and register handlers
    
    fwfCommandSessionHost = XPLMCreateCommand("FWF/Session/Host", "Host FWF session ...");
    fwfCommandSessionJoin = XPLMCreateCommand("FWF/Session/Join", "Join FWF session ...");
    fwfCommandRecordingStart = XPLMCreateCommand("FWF/Recording/Start", "Start FWF flight recording");
    fwfCommandRecordingStop = XPLMCreateCommand("FWF/Recording/Stop", "Stop FWF recording");
    fwfCommandRecordingToggle = XPLMCreateCommand("FWF/Recording/Toggle", "Toggle FWF recording mode");
    XPLMRegisterCommandHandler(fwfCommandSessionHost, CommandHandler, true, 0);
    XPLMRegisterCommandHandler(fwfCommandSessionJoin, CommandHandler, true, 0);
    XPLMRegisterCommandHandler(fwfCommandRecordingStart, CommandHandler, true, 0);
    XPLMRegisterCommandHandler(fwfCommandRecordingStop, CommandHandler, true, 0);
    XPLMRegisterCommandHandler(fwfCommandRecordingToggle, CommandHandler, true, 0);

    // Register flight loop callback
    XPLMCreateFlightLoop_t fl = { sizeof(XPLMCreateFlightLoop_t), xplm_FlightLoop_Phase_AfterFlightModel, FlightLoop, 0 };
    fwfFlightLoop = XPLMCreateFlightLoop(&fl);
    XPLMScheduleFlightLoop(fwfFlightLoop, 5.0, true);

    LOG_INFO(infoLogging, "started");
    
    return true;
}

PLUGIN_API void XPluginStop(void)
{
    XPLMDestroyFlightLoop(fwfFlightLoop);
    LOG_INFO(infoLogging, "stopping ...");
    engine.reset();
    simdata.reset();
    uimanager.reset();
    prefs.reset();
    LOG_INFO(infoLogging, "stopped");
    logger.reset();
}

PLUGIN_API int  XPluginEnable(void)
{ 
    LOG_INFO(infoLogging, "enabled");
    return 1;
}

PLUGIN_API void XPluginDisable(void)
{
    LOG_INFO(infoLogging, "disabled");
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void * inParam)
{
    LOG_VERBOSE(verboseLogging, "receive message");
    if ((inFrom == XPLM_PLUGIN_XPLANE) && (inMsg == XPLM_MSG_ENTERED_VR))
    {
        uimanager->EnteredVR();
    }
    else if ((inFrom == XPLM_PLUGIN_XPLANE) && (inMsg == XPLM_MSG_EXITING_VR))
    {
        uimanager->LeavingVR();
    }
}

static void MenuHandler(void * mRef, void * iRef)
{
    std::string cmd((const char *)iRef);
    LOG_VERBOSE(verboseLogging, "menu callback %s", cmd.c_str());

    if (cmd == "reload")
    {
        XPLMReloadPlugins();
    }
    else if (cmd == "host")
    {
        uimanager->ConfigureHosting();
    }
    else if (cmd == "join")
    {
        uimanager->ConfigureJoining();
    }
    else if (cmd == "leave")
    {
        uimanager->LeaveSession();
    }
    else if (cmd == "toggle")
    {
        uimanager->ToggleStatus();
    }
}

static int CommandHandler(XPLMCommandRef cmd, XPLMCommandPhase phase, void*)
{
    if (cmd == fwfCommandSessionHost)
    {
        uimanager->ConfigureHosting();
        return 0;
    }
    else if (cmd == fwfCommandSessionJoin)
    {
        uimanager->ConfigureJoining();
        return 0;
    }
    else if (cmd == fwfCommandRecordingStart)
    {
        uimanager->StartRecording();
        return 0;
    }
    else if (cmd == fwfCommandRecordingStop)
    {
        uimanager->StopRecording();
        return 0;
    }
    else if (cmd == fwfCommandRecordingToggle)
    {
        if (uimanager->IsRecording())
        {
            uimanager->StopRecording();
        }
        else
        {
            uimanager->StartRecording();
        }
        return 0;
    }
    return 1;
}

static float FlightLoop(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void * inRefcon)
{
    static unsigned int elapsedMilliseconds = 0; // not accurate!
    elapsedMilliseconds += static_cast<unsigned int>(inElapsedSinceLastCall * 1000);
    LOG_VERBOSE(0,"X-Plane Flight Loop nunber %d, %0.3f seconds since previous call",inCounter,inElapsedSinceLastCall);
    float t1 = uimanager->UpdateDialogs();
    float t2 = engine->DoFlightLoop();
    return std::min<float>(t1,t2);
}

} // extern "C"

