// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2011, 2021 - TortoiseSVN

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
#include <string>

/**
 * Sets the Task ID (Win7) for the process according to settings
 * in the registry:
 * HKCU\\Software\\TortoiseSVN\\GroupTaskbarIconsPerRepo
 * determines how this is done. The Task ID is used by the windows
 * taskbar to determine how the app icons on the taskbar are grouped
 * together.
 *
 * GroupTaskbarIconsPerRepo = 0 : this is the default. Icons are
 *                                grouped by application.
 * GroupTaskbarIconsPerRepo = 1 : Icons are grouped by repository uuid, so
 *                                each TSVN dialog gets grouped according
 *                                to the repository/wc it is used for.
 *                                Each TSVN app is grouped separately, i.e.,
 *                                TortoiseMerge icons won't get grouped together
 *                                with TortoiseProc icons.
 * GroupTaskbarIconsPerRepo = 2 : The same as 1, but all TSVN apps are treated
 *                                as one, e.g., a TortoiseMerge instance showing
 *                                a diff from repo X is grouped together with
 *                                a log dialog instance for repo X.
 *
 * The repository uuid is used by examining the command line of the process:
 * it must be set with /groupuuid:"uuid".
 */
void setTaskIDPerUuid();

/**
 * Returns the App ID string. See \ref SetTaskIDPerUUID() for details.
 */
std::wstring getTaskIDPerUuid(LPCWSTR uuid = nullptr);

/**
 * Sets a different overlay icon for the taskbar icon on Win7 for each
 * repository uuid. This allows to 'mark' the uuid-grouped icons on the
 * taskbar to make them more distinguishable.
 * Call this function from the OnTaskbarButtonCreated() message handler.
 * To receive this message, you must first register it:
 * \code
 * const UINT TaskBarButtonCreated = RegisterWindowMessage(L"TaskbarButtonCreated");
 * \endcode
 *
 * The repository uuid is used by examining the command line of the process:
 * it must be set with /groupuuid:"uuid".
 */
void setUuidOverlayIcon(HWND hWnd);
