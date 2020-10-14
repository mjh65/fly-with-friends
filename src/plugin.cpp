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
#include "icontrol.h"
#include "isimulation.h"
#include "iengine.h"
#include "XPLM/XPLMPlugin.h"
#include "XPLM/XPLMUtilities.h"
#include <memory>


static std::shared_ptr<fwf::ILogger> logger;
static std::shared_ptr<fwf::IControl> control;
static std::shared_ptr<fwf::ISimulation> sim;
static std::shared_ptr<fwf::IEngine> engine;


PLUGIN_API int XPluginStart(char * outName, char * outSig, char * outDesc)
{
    strcpy(outName, "Fly-With-Friends");
    strcpy(outSig, "hasling.michael.flywithfriends");
    strcpy(outDesc, "A lightweight plugin for group flying");

    XPLMEnableFeature("XPLM_USE_NATIVE_PATHS",1);

    XPLMPluginID ourId = XPLMGetMyID();
    char logFile[1024];
    XPLMGetPluginInfo(ourId, nullptr, logFile, nullptr, nullptr);
    char *filePart = XPLMExtractFileAndPath(logFile);
    if (filePart > logFile)
    {
        strcpy(filePart-1, "/../fwf.log");
    }
    else
    {
        XPLMGetSystemPath(logFile);
        strcat(logFile,"fwf.log");
    }

    logger = std::shared_ptr<fwf::ILogger>(fwf::ILogger::New(logFile));
    LOG_INFO(1, "starting ...");
    control = std::shared_ptr<fwf::IControl>(fwf::IControl::New());
    sim = std::shared_ptr<fwf::ISimulation>(fwf::ISimulation::New());
    engine = std::shared_ptr<fwf::IEngine>(fwf::IEngine::New());
    
    if (!logger || !control || !sim || !engine)
    {
        LOG_INFO(1, "failed to start");
        logger.reset();
        control.reset();
        sim.reset();
        engine.reset();
        return false;
    }
    
    LOG_INFO(1, "started");
    
    return true;
}

PLUGIN_API void XPluginStop(void)
{
    LOG_INFO(1, "stopping ...");
    engine.reset();
    sim.reset();
    control.reset();
    LOG_INFO(1, "stopped");
    logger.reset();
}

PLUGIN_API int  XPluginEnable(void)
{ 
    LOG_INFO(1, "enabled");
    return 1;
}

PLUGIN_API void XPluginDisable(void)
{
    LOG_INFO(1, "disabled");
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void * inParam)
{
    LOG_INFO(1, "receive message");
}

