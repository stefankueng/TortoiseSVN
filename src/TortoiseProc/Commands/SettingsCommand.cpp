// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008, 2012-2013, 2015, 2018, 2020-2021 - TortoiseSVN

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
#include "stdafx.h"
#include "SettingsCommand.h"

#include "../Settings/Settings.h"
#include "AppUtils.h"
#include "DPIAware.h"

bool SettingsCommand::Execute()
{
    CSettings dlg(IDS_PROC_SETTINGS_TITLE);
    dlg.SetTreeViewMode(TRUE, TRUE, TRUE);

    int dpiX = CDPIAware::Instance().GetDPI(nullptr);
    dlg.SetTreeWidth(220 * dpiX / 96); // need to adjust manually if text gets longer
    long page = parser.GetLongVal(L"page");
    dlg.SetActivePage(page);
    dlg.DoModal();
    dlg.HandleRestart();

    CString sCmd = L" /command:sync";
    CAppUtils::RunTortoiseProc(sCmd);

    return true;
}