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

#include "XPLMDisplay.h"
#include "Widgets/XPWidgetDefs.h"
#include <string>

namespace fwf {

class UIManager;

class Dialog
{
public:
    Dialog(UIManager * owner, int width, int height, const char *title);
    virtual ~Dialog();

    virtual void CreateDialogWidgets() = 0;
    virtual int HandleMessage(XPWidgetMessage msg, XPWidgetID id, intptr_t p1, intptr_t p2) = 0;

    void Activate();
    bool IsVisible();
    void MakeVisible(bool v = true);
    void EnterVR();
    void ExitVR();

    int MsgDespatcher(XPWidgetMessage msg, XPWidgetID id, intptr_t p1, intptr_t p2);

protected:
    UIManager           * const uiMgr;
    int                 left, top;
    int                 width, height;
    std::string         title;
    XPLMWindowID        window;
    XPWidgetID          dialog;
    bool                isActive;

};

}

