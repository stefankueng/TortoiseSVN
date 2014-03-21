// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008, 2010-2012, 2014 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#pragma once
#include "Command.h"

#include "AppUtils.h"
#include "CrashReport.h"

extern CCrashReport crasher;
/**
 * \ingroup TortoiseProc
 * crashes the application to test the crash handler.
 */
class CrashCommand : public Command
{
public:
    /**
     * Executes the command.
     */
    virtual bool            Execute() override
    {
        MessageBox(NULL, L"You are testing the crashhandler.\nDo NOT send the crashreport!!!!", L"TortoiseSVN", MB_ICONINFORMATION);
        CrashProgram();
        TaskDialog(GetExplorerHWND(), AfxGetResourceHandle(), MAKEINTRESOURCE(IDS_APPNAME), MAKEINTRESOURCE(IDS_ERR_ERROROCCURED), MAKEINTRESOURCE(IDS_ERR_NOCOMMAND), TDCBF_OK_BUTTON, TD_ERROR_ICON, NULL);
        return true;
    }
    virtual bool            CheckPaths() override {return true;}

    void CrashProgram()
    {
        // this function is to test the crash reporting utility
        int * a;
        a = NULL;
        *a = 7;
    }
};


