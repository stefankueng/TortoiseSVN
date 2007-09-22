// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007 - TortoiseSVN

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

#include "MessageBox.h"

/**
 * \ingroup TortoiseProc
 * crashes the application to test the crashhandler.
 */
class CrashCommand : public Command
{
public:
	/**
	 * Executes the command.
	 */
	virtual bool			Execute()
	{
		CMessageBox::Show(NULL, _T("You are testing the crashhandler.\n<ct=0x0000FF>Do NOT send the crashreport!!!!</ct>"), _T("TortoiseSVN"), MB_ICONINFORMATION);
		CrashProgram();
		CMessageBox::Show(NULL, IDS_ERR_NOCOMMAND, IDS_APPNAME, MB_ICONERROR);
		return true;
	}

	void CrashProgram()
	{
		// this function is to test the crash reporting utility
		int * a;
		a = NULL;
		*a = 7;
	}
};


