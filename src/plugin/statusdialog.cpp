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

#include "statusdialog.h"
#include "uimanager.h"
#include "Widgets/XPWidgets.h"
#include "Widgets/XPStandardWidgets.h"

namespace fwf {

StatusDialog::StatusDialog(UIManager * m)
:   Dialog(m, 400, 120, "Fly-With-Friends - Status")
{
}

StatusDialog::~StatusDialog()
{
}

void StatusDialog::CreateDialogWidgets()
{
    int x = left;
    int y = top;

    // Add Close Box decorations to the Main Widget
    XPSetWidgetProperty(dialog, xpProperty_MainWindowHasCloseBoxes, 1);

    // Current session status
    for (unsigned int i = 0; i < STATUS_LINES; ++i)
    {
        unsigned int yp = y - 20 - (i * 16);
        const char* t = "";
        statusText[i] = XPCreateWidget(x + 10, yp, x + 390, yp - 16,
            1, t, 0, dialog,
            xpWidgetClass_Caption);
    }

    // Leave session button
    leaveButton = XPCreateWidget(x + 150, y - 90, x + 250, y - 112,
        1, " Leave session", 0, dialog,
        xpWidgetClass_Button);
    XPSetWidgetProperty(leaveButton, xpProperty_ButtonType, xpPushButton);

#ifndef NDEBUG
    recordButton = XPCreateWidget(x + 330, y - 90, x + 390, y - 112,
        1, "rec  r  rec", 0, dialog,
        xpWidgetClass_Button);
    XPSetWidgetProperty(recordButton, xpProperty_ButtonType, xpRadioButton);
    XPSetWidgetProperty(recordButton, xpProperty_ButtonBehavior, xpButtonBehaviorCheckBox);
#endif
}

int StatusDialog::HandleMessage(XPWidgetMessage msg, XPWidgetID id, intptr_t p1, intptr_t p2)
{
    if ((msg == xpMsg_PushButtonPressed) && (p1 == (intptr_t)leaveButton))
    {
        uiMgr->StopSession();
        return 1;
    }
#ifndef NDEBUG
    if ((msg == xpMsg_ButtonStateChanged) && (p1 == (intptr_t)recordButton))
    {
        if (p2)
        {
            uiMgr->StartRecording();
        }
        else
        {
            uiMgr->StopRecording();
        }
        return 1;
    }
#endif

    return 0;
}

void StatusDialog::ResetStatusReport()
{
    for (unsigned int i = 0; i < STATUS_LINES; ++i)
    {
        XPSetWidgetDescriptor(statusText[i], i ? "" : "Not in active session");
    }
}

void StatusDialog::SetStatusText(unsigned int i, const char *a)
{
    if ((i<0) || (i>=STATUS_LINES)) return;
    XPSetWidgetDescriptor(statusText[i], a);
}

void StatusDialog::SetRecordingButton(bool state)
{
#ifndef NDEBUG
    XPSetWidgetProperty(recordButton, xpProperty_ButtonState, state ? 1 : 0);
#endif
}

}
