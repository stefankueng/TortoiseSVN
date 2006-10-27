// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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
#include "SVNRev.h"


/**
 * \ingroup SVN
 * SVNUrl is a specialized CString class which is aware of Subversion URLs.
 * In addition to the normal conventions for URLs, SVNUrls may include a
 * revision information in the form "PATH?REV".
 */
class SVNUrl : public CString
{
public:
	SVNUrl();
	SVNUrl(const CString& svn_url, bool bAlreadyUnescaped = false);
	SVNUrl(const CString& path, const CString& revision, bool bAlreadyUnescaped = false);
	SVNUrl(const CString& path, const SVNRev& revision, bool bAlreadyUnescaped = false);

	SVNUrl& operator=(const CString& svn_url);

public:
	/**
	 *  Sets the \a path part of the SVNUrl.
	 */
	void SetPath(const CString& path);
	/**
	 * Returns the path part of the SVNUrl.
	 */
	CString GetPath() const;

	/**
	 * Sets the \a revision part of the pathname. If \a escaped is set to TRUE,
	 * the escaped form of the URL is returned. Otherwise, the unescaped form
	 * of the URL is returned.
	 */
	void SetRevision(const SVNRev& revision);
	/**
	 * Returns the revision part of the pathname.
	 */
	SVNRev GetRevision() const;
	/**
	 * Returns the textual representation of the revision part of the SVNUrl.
	 */
	CString GetRevisionText() const;

	/**
	 * Returns true if the SVNUrl is a root URL, e.g. http://svn.collab.net
	 * Note: A SVN repository's root can \b not be determined with this function!
	 */
	bool IsRoot() const;
	/**
	 * Returns the root of the current SVNUrl's path,
	 */
	CString GetRootPath() const;
	/**
	 * Returns the parent of the current SVNUrl's path. If the SVNUrl is a
	 * root URL, this function returns an empty string.
	 */
	CString GetParentPath() const;
	/**
	 * Returns the name part of the current SVNUrl's path. If the SVNUrl is a
	 * root URL, this function returns the full path.
	 */
	CString GetName() const;

public:
	/**
	 * Returns the unescaped form of \a url (this may also be a SVNUrl).
	 * Backslashes are converted to forward slashes, and trailing slashes
	 * are removed.
	 */
	static CString Unescape(const CString& url);
	/**
	 * Returns the textual representation of \a revision.
	 */
	static CString GetTextFromRev(const SVNRev& revision);

};

