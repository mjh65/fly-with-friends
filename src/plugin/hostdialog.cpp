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

#include "hostdialog.h"
#include "uimanager.h"
#include "Widgets/XPWidgets.h"
#include "Widgets/XPStandardWidgets.h"

namespace fwf {

HostingDialog::HostingDialog(UIManager * m)
:   Dialog(m, 320, 170, "Fly-With-Friends - Host Session")
{
}

HostingDialog::~HostingDialog()
{
}

void HostingDialog::CreateDialogWidgets()
{
    int x = left;
    int y = top;

    // Add Close Box decorations to the Main Widget
    XPSetWidgetProperty(dialog, xpProperty_MainWindowHasCloseBoxes, 1);

    // User's name
    nameCaption = XPCreateWidget(x + 10, y - 20, x + 90, y - 42,
        1, "Name", 0, dialog,
        xpWidgetClass_Caption);
    nameEdit = XPCreateWidget(x + 100, y - 20, x + 310, y - 42,
        1, "", 0, dialog,
        xpWidgetClass_TextField);
    XPSetWidgetProperty(nameEdit, xpProperty_TextFieldType, xpTextEntryField);
    XPSetWidgetProperty(nameEdit, xpProperty_Enabled, 1);

    // User's callsign
    callsignCaption = XPCreateWidget(x + 10, y - 50, x + 90, y - 72,
        1, "Callsign", 0, dialog,
        xpWidgetClass_Caption);
    callsignEdit = XPCreateWidget(x + 100, y - 50, x + 310, y - 72,
        1, "", 0, dialog,
        xpWidgetClass_TextField);
    XPSetWidgetProperty(callsignEdit, xpProperty_TextFieldType, xpTextEntryField);
    XPSetWidgetProperty(callsignEdit, xpProperty_Enabled, 1);

    // Rendezvous server address and port
    serverCaption = XPCreateWidget(x + 10, y - 80, x + 90, y - 102,
        1, "Session host", 0, dialog,
        xpWidgetClass_Caption);
    serverText = XPCreateWidget(x + 100, y - 80, x + 220, y - 102,
        1, "-.-.-.-", 0, dialog,
        xpWidgetClass_Caption);
    portEdit = XPCreateWidget(x + 230, y - 80, x + 310, y - 102,
        1, "6886", 0, dialog,
        xpWidgetClass_TextField);
    XPSetWidgetProperty(portEdit, xpProperty_TextFieldType, xpTextEntryField);
    XPSetWidgetProperty(portEdit, xpProperty_Enabled, 1);

    // Session passcode
    passcodeCaption = XPCreateWidget(x + 10, y - 110, x + 90, y - 132,
        1, "Passcode", 0, dialog,
        xpWidgetClass_Caption);
    passcodeEdit = XPCreateWidget(x + 100, y - 110, x + 310, y - 132,
        1, "1234567890abcdef", 0, dialog,
        xpWidgetClass_TextField);
    XPSetWidgetProperty(passcodeEdit, xpProperty_TextFieldType, xpTextEntryField);
    XPSetWidgetProperty(passcodeEdit, xpProperty_Enabled, 1);

    // Packet logging button
    loggingCaption = XPCreateWidget(x + 10, y - 140, x + 60, y - 162,
        1, "Logging", 0, dialog,
        xpWidgetClass_Caption);
    loggingButton = XPCreateWidget(x + 70, y - 140, x + 92, y - 162,
        1, "", 0, dialog,
        xpWidgetClass_Button);
    XPSetWidgetProperty(loggingButton, xpProperty_ButtonType, xpRadioButton);
    XPSetWidgetProperty(loggingButton, xpProperty_ButtonBehavior, xpButtonBehaviorCheckBox);

    // Hosting activation button
    hostButton = XPCreateWidget(x + 210, y - 140, x + 310, y - 162,
        1, " Host session", 0, dialog,
        xpWidgetClass_Button);
    XPSetWidgetProperty(hostButton, xpProperty_ButtonType, xpPushButton);
}

int HostingDialog::HandleMessage(XPWidgetMessage msg, XPWidgetID id, intptr_t p1, intptr_t p2)
{
    if ((msg == xpMsg_PushButtonPressed) && (p1 == (intptr_t)hostButton))
    {
        char name[32], callsign[32], addr[64], port[32], passcode[64];
        XPGetWidgetDescriptor(nameEdit, name, 32);
        name[31] = 0;
        XPGetWidgetDescriptor(callsignEdit, callsign, 32);
        callsign[31] = 0;
        XPGetWidgetDescriptor(passcodeEdit, passcode, 64);
        passcode[63] = 0;
        XPGetWidgetDescriptor(serverText, addr, 64);
        addr[63] = 0;
        XPGetWidgetDescriptor(portEdit, port, 32);
        port[31] = 0;
        bool logging = (XPGetWidgetProperty(loggingButton, xpProperty_ButtonState, 0) != 0);
        uiMgr->StartSession(addr, port, name, callsign, passcode, logging);
        return 1;
    }

    return 0;
}

void HostingDialog::SetHostingAddress(const char *a)
{
    XPSetWidgetDescriptor(serverText, a);
}

void HostingDialog::SetName(const char *a)
{
    XPSetWidgetDescriptor(nameEdit, a);
}

void HostingDialog::SetCallsign(const char *a)
{
    XPSetWidgetDescriptor(callsignEdit, a);
}

void HostingDialog::SetHostingPort(const char *a)
{
    XPSetWidgetDescriptor(portEdit, a);
}

void HostingDialog::SetPasscode(const char *a)
{
    XPSetWidgetDescriptor(passcodeEdit, a);
}


}
