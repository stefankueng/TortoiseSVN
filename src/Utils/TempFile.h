// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2008, 2010, 2014 - TortoiseSVN

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

#include "TSVNPath.h"
#include "SVNRev.h"

/**
* \ingroup Utils
* This singleton class handles temporary files.
* All temp files are deleted at the end of a run of SVN operations
*/
class CTempFiles
{
public:
    static CTempFiles& Instance();

    /**
     * Returns a path to a temporary file.
     * \param bRemoveAtEnd if true, the temp file is removed when this object
     *                     goes out of scope.
     * \param path         if set, the temp file will have the same file extension
     *                     as this path.
     * \param revision     if set, the temp file name will include the revision number
     */
    CTSVNPath       GetTempFilePath(bool bRemoveAtEnd, const CTSVNPath& path = CTSVNPath(), const SVNRev& revision = SVNRev());
    CString         GetTempFilePathString();

    /**
     * Returns a path to a temporary directory.
     * \param bRemoveAtEnd if true, the temp directory is removed when this object
     *                     goes out of scope.
     * \param path         if set, the temp directory will have the same file extension
     *                     as this path.
     * \param revision     if set, the temp directory name will include the revision number
     */
    CTSVNPath       GetTempDirPath(bool bRemoveAtEnd, const CTSVNPath& path = CTSVNPath(), const SVNRev& revision = SVNRev());

    // Look for temporary files left around by TortoiseMerge and
    // remove them. But only delete 'old' files
    static void     DeleteOldTempFiles(LPCTSTR wildCard);

    void            AddFileToRemove(const CString& file) { m_TempFileList.AddPath(CTSVNPath(file)); }

private:

    // try to allocate an unused temp file / dir at most MAX_RETRIES times

    enum {MAX_RETRIES = 100};

    // list of paths to delete when terminating the app

    CTSVNPathList m_TempFileList;

    // actual implementation

    CTSVNPath ConstructTempPath(const CTSVNPath& path, const SVNRev& revision) const;
    CTSVNPath CreateTempPath (bool bRemoveAtEnd, const CTSVNPath& path, const SVNRev& revision, bool directory);

    // construction / destruction

    CTempFiles(void);
    ~CTempFiles(void);
};
