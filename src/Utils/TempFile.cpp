// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2014 - TortoiseSVN

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
#include "TempFile.h"
#include "PathUtils.h"
#include "DirFileEnum.h"
#include "SmartHandle.h"

CTempFiles::CTempFiles(void)
{
}

CTempFiles::~CTempFiles(void)
{
    m_TempFileList.DeleteAllPaths(false, false, NULL);
}

CTempFiles& CTempFiles::Instance()
{
    static CTempFiles instance;
    return instance;
}

CTSVNPath CTempFiles::ConstructTempPath(const CTSVNPath& path, const SVNRev& revision) const
{
    DWORD len = ::GetTempPath(0, NULL);
    std::unique_ptr<TCHAR[]> temppath (new TCHAR[len+1]);
    std::unique_ptr<TCHAR[]> tempF (new TCHAR[len+50]);
    ::GetTempPath (len+1, temppath.get());
    CTSVNPath tempfile;
    CString possibletempfile;
    if (path.IsEmpty())
    {
        ::GetTempFileName (temppath.get(), L"svn", 0, tempF.get());
        tempfile = CTSVNPath (tempF.get());
    }
    else
    {
        int i=0;
        do
        {
            // use the UI path, which does unescaping for urls
            CString filename = path.GetUIFileOrDirectoryName();
            // remove illegal chars which could be present in a temp filename
            filename.Remove('?');
            filename.Remove('*');
            filename.Remove('<');
            filename.Remove('>');
            filename.Remove('|');
            filename.Remove('"');
            filename.Remove(':');
            filename.Remove('\'');
            // the inner loop assures that the resulting path is < MAX_PATH
            // if that's not possible without reducing the 'filename' to less than 5 chars, use a path
            // that's longer than MAX_PATH (in that case, we can't really do much to avoid longer paths)
            do
            {
                if (revision.IsValid())
                {
                    possibletempfile.Format(L"%s%s-rev%s.svn%3.3x.tmp%s", temppath.get(), (LPCTSTR)filename, (LPCTSTR)revision.ToString(), i, (LPCTSTR)path.GetFileExtension());
                }
                else
                {
                    possibletempfile.Format(L"%s%s.svn%3.3x.tmp%s", temppath.get(), (LPCTSTR)filename, i, (LPCTSTR)path.GetFileExtension());
                }
                tempfile.SetFromWin(possibletempfile);
                filename = filename.Left(filename.GetLength()-1);
            } while (   (filename.GetLength() > 4)
                     && (tempfile.GetWinPathString().GetLength() >= MAX_PATH));
            i++;
        } while (PathFileExists(tempfile.GetWinPath()));
    }

    // caller has to actually grab the file path

    return tempfile;
}

CTSVNPath CTempFiles::CreateTempPath (bool bRemoveAtEnd, const CTSVNPath& path, const SVNRev& revision, bool directory)
{
    bool succeeded = false;
    for (int retryCount = 0; retryCount < MAX_RETRIES; ++retryCount)
    {
        CTSVNPath tempfile = ConstructTempPath (path, revision);

        // now create the temp file / directory, so that subsequent calls to GetTempFile() return
        // different filenames.
        // Handle races, i.e. name collisions.

        if (directory)
        {
            DeleteFile(tempfile.GetWinPath());
            if (CreateDirectory (tempfile.GetWinPath(), NULL) == FALSE)
            {
                if (GetLastError() != ERROR_ALREADY_EXISTS)
                    return CTSVNPath();
            }
            else
                succeeded = true;
        }
        else
        {
            CAutoFile hFile = CreateFile(tempfile.GetWinPath(), GENERIC_READ, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
            if (!hFile)
            {
                if (GetLastError() != ERROR_ALREADY_EXISTS)
                    return CTSVNPath();
            }
            else
            {
                succeeded = true;
            }
        }

        // done?

        if (succeeded)
        {
            if (bRemoveAtEnd)
                m_TempFileList.AddPath(tempfile);

            return tempfile;
        }
    }

    // give up

    return CTSVNPath();
}

CTSVNPath CTempFiles::GetTempFilePath(bool bRemoveAtEnd, const CTSVNPath& path /* = CTSVNPath() */, const SVNRev& revision /* = SVNRev() */)
{
    return CreateTempPath (bRemoveAtEnd, path, revision, false);
}

CString CTempFiles::GetTempFilePathString()
{
    return CreateTempPath (true, CTSVNPath(), SVNRev(), false).GetWinPathString();
}

CTSVNPath CTempFiles::GetTempDirPath(bool bRemoveAtEnd, const CTSVNPath& path /* = CTSVNPath() */, const SVNRev& revision /* = SVNRev() */)
{
    return CreateTempPath (bRemoveAtEnd, path, revision, true);
}

void CTempFiles::DeleteOldTempFiles(LPCTSTR wildCard)
{
    DWORD len = ::GetTempPath(0, NULL);
    std::unique_ptr<TCHAR[]> path(new TCHAR[len + 100]);
    len = ::GetTempPath (len+100, path.get());
    if (len == 0)
        return;

    CSimpleFileFind finder = CSimpleFileFind(path.get(), wildCard);
    FILETIME systime_;
    ::GetSystemTimeAsFileTime(&systime_);
    __int64 systime = (__int64)systime_.dwLowDateTime | (__int64)systime_.dwHighDateTime << 32LL;
    while (finder.FindNextFileNoDirectories())
    {
        CString filepath = finder.GetFilePath();

        FILETIME createtime_ = finder.GetCreateTime();
        __int64 createtime = (__int64)createtime_.dwLowDateTime | (__int64)createtime_.dwHighDateTime << 32LL;
        createtime += 864000000000LL;      //only delete files older than a day
        if (createtime < systime)
        {
            ::SetFileAttributes(filepath, FILE_ATTRIBUTE_NORMAL);
            ::DeleteFile(filepath);
        }
    }
}

