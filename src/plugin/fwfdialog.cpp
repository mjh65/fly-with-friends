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

#include "fwfdialog.h"
#include "uimanager.h"
#include "ilogger.h"
#include "Widgets/XPWidgets.h"
#include "Widgets/XPStandardWidgets.h"

namespace fwf {

extern "C" {
static int DialogHandler(XPWidgetMessage msg, XPWidgetID id, intptr_t p1, intptr_t p2)
{
    int exists = 0;
    intptr_t p = XPGetWidgetProperty(id, xpProperty_UserStart, &exists);
    if (exists && p)
    {
        return reinterpret_cast<Dialog*>(p)->MsgDespatcher(msg, id, p1, p2);
    }
    return 0; // not handled
}
}

Dialog::Dialog(UIManager* m, int w, int h, const char* n)
:   uiMgr(m),
    width(w),
    height(h),
    title(n),
    window(0)
{
    // Work out screen centre and thus left and top coordinates
    int gl, gt, gr, gb;
    XPLMGetScreenBoundsGlobal(&gl, &gt, &gr, &gb);
    left = ((gl + gr) / 2) - (width / 2);
    top = ((gt + gb) / 2) + (height / 2);
    LOG_INFO(1, "dialog '%s' created left=%d top=%d", n, left, top);

    // Create the Main Widget window
    dialog = XPCreateWidget(left, top, (left + width), (top - height), 1, title.c_str(), 1, 0, xpWidgetClass_MainWindow);
}

Dialog::~Dialog()
{
    if (window)
    {
        XPLMDestroyWindow(window);
    }
}

void Dialog::Activate()
{
    // Register our widget handler
    XPSetWidgetProperty(dialog, xpProperty_UserStart, reinterpret_cast<intptr_t>(this));
    XPAddWidgetCallback(dialog, DialogHandler);

    // Get the native backing window handle
    window = XPGetWidgetUnderlyingWindow(dialog);

    MakeVisible(true);
}

bool Dialog::IsVisible()
{
    return isActive;
}

void Dialog::MakeVisible(bool v)
{
    if (v)
    {
        if (uiMgr->IsVRenabled())
        {
            XPLMSetWindowPositioningMode(window, xplm_WindowVR, -1);
        }
        else {
            XPLMSetWindowPositioningMode(window, xplm_WindowPositionFree, -1);
            XPLMSetWindowGeometry(window, left, top, left + width, top - height);
        }
    }
    else
    {
        if (!uiMgr->IsVRenabled())
        {
            int r, b;
            XPLMGetWindowGeometry(window, &left, &top, &r, &b);
        }
    }
    XPLMSetWindowIsVisible(window, v);
    isActive = v;
}

void Dialog::EnterVR()
{
    XPLMSetWindowPositioningMode(window, xplm_WindowVR, -1);
}

void Dialog::ExitVR()
{
    if (XPLMWindowIsInVR(window))
    {
        XPLMSetWindowPositioningMode(window, xplm_WindowPositionFree, 0);
        XPLMSetWindowGeometry(window, left, top, left + width, top - height);
    }
}

int Dialog::MsgDespatcher(XPWidgetMessage msg, XPWidgetID id, intptr_t p1, intptr_t p2)
{
    if (msg == xpMessage_CloseButtonPushed)
    {
        if (isActive)
        {
            MakeVisible(false);
        }
        return 1;
    }

    return HandleMessage(msg, id, p1, p2);
}

}
