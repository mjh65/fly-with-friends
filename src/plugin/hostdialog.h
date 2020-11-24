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

#include "fwfdialog.h"
#include "XPLMDisplay.h"
#include "Widgets/XPWidgetDefs.h"
#include <memory>

namespace fwf {

class UIManager;

class HostingDialog : public Dialog
{
public:
    HostingDialog(UIManager * owner);
    ~HostingDialog();

    void CreateDialogWidgets() override;
    int HandleMessage(XPWidgetMessage msg, XPWidgetID id, intptr_t p1, intptr_t p2) override;

    void SetHostingAddress(const char *a);
    void SetName(const char *a);
    void SetCallsign(const char *a);
    void SetHostingPort(const char *a);
    void SetPasscode(const char *a);

protected:
private:
    XPWidgetID                  nameCaption, nameEdit;
    XPWidgetID                  callsignCaption, callsignEdit;
    XPWidgetID                  serverCaption, serverText, portEdit;
    XPWidgetID                  passcodeCaption, passcodeEdit;
    XPWidgetID                  hostButton;
};

}
