// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Tim Kemp and Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
#pragma once

#include "registry.h"
#include "Shlwapi.h"

/**
 * \ingroup TortoiseProc
 * An Utility class with static classes.
 *
 * \par requirements
 * win95 or later
 * winNT4 or later
 * MFC
 *
 * \version 1.0
 * first version
 *
 * \date 02-02-2003
 *
 * \author kueng
 *
 * \par license
 * This code is absolutely free to use and modify. The code is provided "as is" with
 * no expressed or implied warranty. The author accepts no liability if it causes
 * any damage to your computer, causes your pet to fall ill, increases baldness
 * or makes your car start emitting strange noises when you start it up.
 * This code has no bugs, just undocumented features!
 */
class CUtils
{
public:
	CUtils(void);
	~CUtils(void);

	/**
	 * Launches the external merge program if there is one.
	 * \return TRUE if the program could be started
	 */
	static BOOL StartExtMerge(CString basefile, CString theirfile, CString yourfile, CString mergedfile,
		CString basename = _T(""), CString theirname = _T(""), CString yourname = _T(""), CString mergedname = _T(""));

	/**
	 * Launches the diff viewer application.
	 * If dir is a directory then TortoiseMerge is started.
	 * If dir is a file then the specified diff application is started (e.g. WinMerge, TortoiseMerge, WinDiff, ...)
	 * If dir is empty then an application which can show unified diff files is started, either
	 * the specified one, the one associated with .diff files and as the last fallback the application
	 * associated with .txt files.
	 * \return TRUE if the program could be started.
	 */
	static BOOL StartDiffViewer(CString file, CString dir = _T(""), BOOL bWait = FALSE,
		CString name1 = _T(""), CString name2 = _T(""));

	/**
	 * Launches the standard text viewer/editor application which is associated
	 * with txt files.
	 * \return TRUE if the program could be started.
	 */
	static BOOL StartTextViewer(CString file);

	/**
	 * Returns a path to a temporary file
	 */
	static CString GetTempFile();

	static void Unescape(char * psz);
	static CString PathEscape(CString path);
	static CString GetVersionFromFile(const CString & p_strDateiname);
	static CString GetFileNameFromPath(CString sPath);
	static BOOL PathIsParent(CString sPath1, CString sPath2);

};
