// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006-2007, 2011 - TortoiseSVN

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
#include "StdAfx.h"
#include "Workingfile.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "resource.h"
#include "SmartHandle.h"

CWorkingFile::CWorkingFile(void)
{
    m_bHaveData = false;
}

CWorkingFile::~CWorkingFile(void)
{
}

void CWorkingFile::SetFileName(const CString& newFilename)
{
    m_sFilename = newFilename;
    m_sFilename.Replace('/', '\\');
    m_sDescriptiveName.Empty();
}

void CWorkingFile::SetDescriptiveName(const CString& newDescName)
{
    m_sDescriptiveName = newDescName;
}

CString CWorkingFile::GetDescriptiveName()
{
    if (m_sDescriptiveName.IsEmpty())
    {
        CString sDescriptiveName = CPathUtils::GetFileNameFromPath(m_sFilename);
        if (sDescriptiveName.GetLength() < 20)
            return sDescriptiveName;
    }
    return m_sDescriptiveName;
}
//
// Make an empty file with this name
void CWorkingFile::CreateEmptyFile()
{
    CAutoFile hFile = CreateFile(m_sFilename, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
}

// Move the details of the specified file to the current one, and then mark the specified file
// as out of use
void CWorkingFile::TransferDetailsFrom(CWorkingFile& rightHandFile)
{
    // We don't do this to files which are already in use
    ASSERT(!InUse());

    m_sFilename = rightHandFile.m_sFilename;
    m_sDescriptiveName = rightHandFile.m_sDescriptiveName;
    rightHandFile.SetOutOfUse();

    m_bHaveData = rightHandFile.m_bHaveData; 
    m_nFilesize = rightHandFile.m_nFilesize;
    m_timeCreation = rightHandFile.m_timeCreation;
    m_timeLastWrite = rightHandFile.m_timeLastWrite;
}

CString CWorkingFile::GetWindowName() const
{
    CString sErrMsg = _T("");
    // TortoiseMerge allows non-existing files to be used in a merge
    // Inform the user (in a non-intrusive way) if a file is absent
    if (! this->Exists())
    {
        sErrMsg = CString(MAKEINTRESOURCE(IDS_NOTFOUNDVIEWTITLEINDICATOR));
    }

    if(m_sDescriptiveName.IsEmpty())
    {
        // We don't have a proper name - use the filename part of the path
        // return the filename part of the path.
        return CPathUtils::GetFileNameFromPath(m_sFilename) + _T(" ") + sErrMsg;
    }
    else if (sErrMsg.IsEmpty())
    {
        return m_sDescriptiveName;
    }
    return m_sDescriptiveName + _T(" ") + sErrMsg;
}

bool CWorkingFile::Exists() const
{
    return (!!PathFileExists(m_sFilename));
}

void CWorkingFile::SetOutOfUse() {
    m_sFilename.Empty();
    m_sDescriptiveName.Empty();
    m_bHaveData=false;
}

static bool FileTimesEqual(const FILETIME& first, const FILETIME& second)
{
    return first.dwLowDateTime == second.dwLowDateTime &&
        first.dwHighDateTime == second.dwHighDateTime;
}

bool CWorkingFile::IsSourceFileChanged() const
{
    if (!InUse())
    {
        return false;
    }
    CAutoFile hFile = CreateFile(m_sFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
    if (!hFile || !m_bHaveData)
    {
        return hFile || m_bHaveData;
    }

    LARGE_INTEGER nFilesize;
    nFilesize.QuadPart = -1;
    GetFileSizeEx(hFile, &nFilesize);
    if (nFilesize.QuadPart != m_nFilesize.QuadPart)
        return true;

    const FILETIME timeEmpty = {0};
    FILETIME timeCreation = timeEmpty;
    FILETIME timeLastWrite = timeEmpty;

    GetFileTime(hFile, &timeCreation, NULL, &timeLastWrite);
    return !(FileTimesEqual(timeCreation, m_timeCreation)
            && FileTimesEqual(timeLastWrite, m_timeLastWrite));
}

void CWorkingFile::StoreFileAttributes() {
    m_nFilesize.QuadPart = -1;
    const FILETIME timeEmpty = {0};
    m_timeCreation = timeEmpty;
    m_timeLastWrite = timeEmpty;

    CAutoFile hFile = CreateFile(m_sFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
    m_bHaveData = hFile;
    if (hFile)
    {
        GetFileSizeEx(hFile, &m_nFilesize);
        GetFileTime(hFile, &m_timeCreation, NULL, &m_timeLastWrite);
    }
}
