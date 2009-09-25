// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007,2009 - TortoiseSVN

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
#include "SVN.h"

#include <map>

using namespace std;

/**
 * \ingroup SVN
 * helper class for retrieving log messages without the log dialog.
 * 
 * Can find the copy from revision of a tag/branch and the corresponding copy from URL.
 * Can also save log messages to a temp file for later use.
 */
class SVNLogHelper : public SVN
{
public:
    /**
	 * Finds the copy from revision and URL of a branch/tag URL.
	 * \param url			the url of the branch/tag to find the copy from data for (input)
	 * \param pegrev		the peg revision to use to find the copy from data (input)
	 * \param copyfromURL	the url the branch/tag was copied from (output)
	 * \return				the copy from revision
	 */
	SVNRev GetCopyFromRev(CTSVNPath url, SVNRev pegrev, CString& copyfromURL);

};
