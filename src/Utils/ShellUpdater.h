// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2005 - Stefan Kueng

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

#include "TSVNPath.h"

/**
* \ingroup Utils
* This singleton class contains a list of items which require a shell-update notification
* This update is done lazily at the end of a run of SVN operations
*
* \par requirements
* win95 or later
* winNT4 or later
* MFC
*
* \version 1.0
* first version
*
* \date JAN-2002
*
* \author Will Dean
*
*/
class CShellUpdater
{
private:
	CShellUpdater(void);
	~CShellUpdater(void);
public:
	static CShellUpdater& Instance();

public:
	/** 
	 * Add a single path for updating.
	 * The update will happen at some suitable time in the future
	 */
	void AddPathForUpdate(const CTSVNPath& path);
	/** 
	 * Add a list of paths for updating.
	 * The update will happen at some suitable time in the future
	 */
	void AddPathsForUpdate(const CTSVNPathList& pathList);
	/**
	 * Do the update, and clear the list of items waiting
	 */
	void Flush();
	
	static void RebuildIcons();

private:
	void UpdateShell();

private:
	// The list of paths which will need updating
	CTSVNPathList m_pathsForUpdating;
	// A handle to an event which, when set, tells the ShellExtension to purge its status cache
	HANDLE			m_hInvalidationEvent;
};
