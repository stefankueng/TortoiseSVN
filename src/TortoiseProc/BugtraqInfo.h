// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Stefan Kueng

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

#pragma once
#include "svnproperties.h"

#define BUGTRAQPROPNAME_LABEL             _T("bugtraq:label")
#define BUGTRAQPROPNAME_MESSAGE           _T("bugtraq:message")
#define BUGTRAQPROPNAME_NUMBER            _T("bugtraq:number")
#define BUGTRAQPROPNAME_URL               _T("bugtraq:url")

/**
 * \ingroup TortoiseProc
 * Provides methods for retrieving information about bug/issuetrackers
 * associated with a Subversion repository/working copy.
 *
 *
 * \version 1.0
 * first version
 *
 * \date 08-21-2004
 *
 * \author Stefan Kueng
 *
 * \par license
 * This code is absolutely free to use and modify. The code is provided "as is" with
 * no expressed or implied warranty. The author accepts no liability if it causes
 * any damage to your computer, causes your pet to fall ill, increases baldness
 * or makes your car start emitting strange noises when you start it up.
 * This code has no bugs, just undocumented features!
 */
class BugtraqInfo
{
public:
	BugtraqInfo(void);
	~BugtraqInfo(void);

	/**
	 * Reads the bugtracking properties from a path. If the path is a file
	 * then the properties are read from the parent folder of that file.
	 * \param path path to a file or a folder
	 */
	BOOL ReadProps(CString path);
	/**
	 * Reads the bugtracking properties all paths found in the tempfile.
	 * This method calls ReadProps() for each path found in the tempfile.
	 * \param path path to the tempfile
	 */
	BOOL ReadPropsTempfile(CString path);

	/**
	 * Searches for the BugID inside a log message. If one is found,
	 * the method returns TRUE.
	 * \param msg the log message
	 * \param offset1 offset of the first char of the BugID inside the logmessage
	 * \param offset2 offset of the last char of the BugID inside the logmessage
	 */
	BOOL FindBugID(const CString& msg, int& offset1, int& offset2);
	/**
	 * Returns the URL pointing to the Issue in the issuetracker. The URL is
	 * created from the bugtraq:url property and the BugID found in the logmessage.
	 * \param msg the log message
	 */
	CString GetBugIDUrl(const CString& msg);

public:
	/* The label to show in the commit dialog where the issue number/bug id
	 * is entered. Example: "Bug-ID: " or "Issue-No.:". Default is "Bug-ID :" */
	CString		sLabel;

	/* The message string to add below the log message the user entered.
	 * It must contain the string "%BUGID%" which gets replaced by the client 
	 * with the issue number / bug id the user entered. */
	CString		sMessage;

	/* If this is set, then the bug-id / issue number must be a number, no text */
	BOOL		bNumber;

	/* The url pointing to the issue tracker. If the url contains the string
	 * "%BUGID% the client has to replace it with the issue number / bug id
	 * the user entered. */
	CString		sUrl;
};
