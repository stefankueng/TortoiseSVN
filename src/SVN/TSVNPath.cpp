// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2010 - TortoiseSVN

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
#include "TSVNPath.h"
#include "UnicodeUtils.h"
#include "SVNAdminDir.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "svn_dirent_uri.h"
#include <regex>
#include "auto_buffer.h"

#if defined(_MFC_VER)
#include "MessageBox.h"
#include "AppUtils.h"
#include "StringUtils.h"
#endif

using namespace std;


CTSVNPath::CTSVNPath(void) :
	m_bDirectoryKnown(false),
	m_bIsDirectory(false),
	m_bIsURL(false),
	m_bURLKnown(false),
	m_bHasAdminDirKnown(false),
	m_bHasAdminDir(false),
	m_bIsValidOnWindowsKnown(false),
	m_bIsReadOnly(false),
	m_bIsAdminDirKnown(false),
	m_bIsAdminDir(false),
	m_bExists(false),
	m_bExistsKnown(false),
	m_bLastWriteTimeKnown(0),
	m_lastWriteTime(0),
	m_customData(NULL),
	m_bIsSpecialDirectoryKnown(false),
	m_bIsSpecialDirectory(false)
{
}

CTSVNPath::~CTSVNPath(void)
{
}
// Create a TSVNPath object from an unknown path type (same as using SetFromUnknown)
CTSVNPath::CTSVNPath(const CString& sUnknownPath) :
	m_bDirectoryKnown(false),
	m_bIsDirectory(false),
	m_bIsURL(false),
	m_bURLKnown(false),
	m_bHasAdminDirKnown(false),
	m_bHasAdminDir(false),
	m_bIsValidOnWindowsKnown(false),
	m_bIsReadOnly(false),
	m_bIsAdminDirKnown(false),
	m_bIsAdminDir(false),
	m_bExists(false),
	m_bExistsKnown(false),
	m_bLastWriteTimeKnown(0),
	m_lastWriteTime(0),
	m_fileSize(0),
	m_customData(NULL),
	m_bIsSpecialDirectoryKnown(false),
	m_bIsSpecialDirectory(false)
{
	SetFromUnknown(sUnknownPath);
}

void CTSVNPath::SetFromSVN(const char* pPath)
{
	Reset();
	if (pPath == NULL)
		return;
	int len = MultiByteToWideChar(CP_UTF8, 0, pPath, -1, NULL, 0);
	if (len)
	{
		len = MultiByteToWideChar(CP_UTF8, 0, pPath, -1, m_sFwdslashPath.GetBuffer(len+1), len+1);
		m_sFwdslashPath.ReleaseBuffer(len-1);
	}
	SanitizeRootPath(m_sFwdslashPath, true);
}

void CTSVNPath::SetFromSVN(const char* pPath, bool bIsDirectory)
{
	SetFromSVN(pPath);
	m_bDirectoryKnown = true;
	m_bIsDirectory = bIsDirectory;
}

void CTSVNPath::SetFromSVN(const CString& sPath)
{
	Reset();
	m_sFwdslashPath = sPath;
	SanitizeRootPath(m_sFwdslashPath, true);
}

void CTSVNPath::SetFromWin(LPCTSTR pPath)
{
	Reset();
	m_sBackslashPath = pPath;
	SanitizeRootPath(m_sBackslashPath, false);
	ATLASSERT(m_sBackslashPath.Find('/')<0);
}
void CTSVNPath::SetFromWin(const CString& sPath)
{
	Reset();
	m_sBackslashPath = sPath;
	SanitizeRootPath(m_sBackslashPath, false);
}
void CTSVNPath::SetFromWin(const CString& sPath, bool bIsDirectory)
{
	Reset();
	m_sBackslashPath = sPath;
	m_bIsDirectory = bIsDirectory;
	m_bDirectoryKnown = true;
	SanitizeRootPath(m_sBackslashPath, false);
}
void CTSVNPath::SetFromUnknown(const CString& sPath)
{
	Reset();
	// Just set whichever path we think is most likely to be used
	SetFwdslashPath(sPath);
}

LPCTSTR CTSVNPath::GetWinPath() const
{
	if(IsEmpty())
	{
		return _T("");
	}
	if(m_sBackslashPath.IsEmpty())
	{
		SetBackslashPath(m_sFwdslashPath);
	}
	if(m_sBackslashPath.GetLength() >= 248)
	{
		m_sLongBackslashPath = _T("\\\\?\\") + m_sBackslashPath;
		return m_sLongBackslashPath;
	}
	return m_sBackslashPath;
}

const CString& CTSVNPath::GetWinPathString() const
{
	if(m_sBackslashPath.IsEmpty())
	{
		SetBackslashPath(m_sFwdslashPath);
	}
	return m_sBackslashPath;
}

const CString& CTSVNPath::GetSVNPathString() const
{
	if(m_sFwdslashPath.IsEmpty())
	{
		SetFwdslashPath(m_sBackslashPath);
	}
	return m_sFwdslashPath;
}


const char* CTSVNPath::GetSVNApiPath(apr_pool_t *pool) const
{
	// This funny-looking 'if' is to avoid a subtle problem with empty paths, whereby
	// each call to GetSVNApiPath returns a different pointer value.
	// If you made multiple calls to GetSVNApiPath on the same string, only the last
	// one would give you a valid pointer to an empty string, because each 
	// call would invalidate the previous call's return. 
	if(IsEmpty())
	{
		return "";
	}
	if(m_sFwdslashPath.IsEmpty())
	{
		SetFwdslashPath(m_sBackslashPath);
	}
	if(m_sUTF8FwdslashPath.IsEmpty())
	{
		SetUTF8FwdslashPath(m_sFwdslashPath);
	}
	if (svn_path_is_url(m_sUTF8FwdslashPath))
	{
		m_sUTF8FwdslashPathEscaped = CPathUtils::PathEscape(m_sUTF8FwdslashPath);
		m_sUTF8FwdslashPathEscaped.Replace("file:////", "file://");
		m_sUTF8FwdslashPathEscaped = svn_path_canonicalize(m_sUTF8FwdslashPathEscaped, pool);
		return m_sUTF8FwdslashPathEscaped;
	}
	else
	{
		m_sUTF8FwdslashPath = svn_dirent_canonicalize(m_sUTF8FwdslashPath, pool);
		if ((m_sUTF8FwdslashPath.GetLength() == 3)  
			&& ( ((m_sUTF8FwdslashPath[0] >= 'A') && (m_sUTF8FwdslashPath[0] <= 'Z')) || ((m_sUTF8FwdslashPath[0] >= 'a') && (m_sUTF8FwdslashPath[0] <= 'z')) )
			&& (m_sUTF8FwdslashPath[1] == ':') && (m_sUTF8FwdslashPath[2] == '/'))
		{
			m_sUTF8FwdslashPath = m_sUTF8FwdslashPath.Left(2);
		}
	}

	return m_sUTF8FwdslashPath;
}

const CString& CTSVNPath::GetUIPathString() const
{
	if (m_sUIPath.IsEmpty())
	{
#if defined(_MFC_VER)
		//BUGBUG HORRIBLE!!! - CPathUtils::IsEscaped doesn't need to be MFC-only
		if (IsUrl())
		{
			m_sUIPath = CPathUtils::PathUnescape(GetSVNPathString());
			m_sUIPath.Replace(_T("file:////"), _T("file://"));

		}
		else
#endif 
		{
			m_sUIPath = GetWinPathString();
		}
	}
	return m_sUIPath;
}

void CTSVNPath::SetFwdslashPath(const CString& sPath) const
{
	m_sFwdslashPath = sPath;
	m_sFwdslashPath.Replace('\\', '/');

	// We don't leave a trailing /
	m_sFwdslashPath.TrimRight('/');	

	SanitizeRootPath(m_sFwdslashPath, true);

	m_sFwdslashPath.Replace(_T("file:////"), _T("file://"));

	m_sUTF8FwdslashPath.Empty();
}

void CTSVNPath::SetBackslashPath(const CString& sPath) const
{
	m_sBackslashPath = sPath;
	m_sBackslashPath.Replace('/', '\\');
	m_sBackslashPath.TrimRight('\\');
	SanitizeRootPath(m_sBackslashPath, false);
}

void CTSVNPath::SetUTF8FwdslashPath(const CString& sPath) const
{
	m_sUTF8FwdslashPath = CUnicodeUtils::GetUTF8(sPath);
}

void CTSVNPath::SanitizeRootPath(CString& sPath, bool bIsForwardPath) const
{
	// Make sure to add the trailing slash to root paths such as 'C:'
	if (sPath.GetLength() == 2 && sPath[1] == ':')
	{
		sPath += (bIsForwardPath) ? _T("/") : _T("\\");
	}
}

bool CTSVNPath::IsUrl() const
{
	if (!m_bURLKnown)
	{
		EnsureFwdslashPathSet();
		if(m_sUTF8FwdslashPath.IsEmpty())
		{
			SetUTF8FwdslashPath(m_sFwdslashPath);
		}
		m_bIsURL = !!svn_path_is_url(m_sUTF8FwdslashPath);
		m_bURLKnown = true;
	}
	return m_bIsURL;
}

bool CTSVNPath::IsDirectory() const
{
	if(!m_bDirectoryKnown)
	{
		UpdateAttributes();
	}
	return m_bIsDirectory;
}

bool CTSVNPath::Exists() const
{
	if (!m_bExistsKnown)
	{
		UpdateAttributes();
	}
	return m_bExists;
}

bool CTSVNPath::Delete(bool bTrash) const
{
	EnsureBackslashPathSet();
	::SetFileAttributes(m_sBackslashPath, FILE_ATTRIBUTE_NORMAL);
	bool bRet = false;
	if (Exists())
	{
		if ((bTrash)||(IsDirectory()))
		{
			auto_buffer<TCHAR> buf(m_sBackslashPath.GetLength()+2);
			_tcscpy_s(buf, m_sBackslashPath.GetLength()+2, m_sBackslashPath);
			buf[m_sBackslashPath.GetLength()] = 0;
			buf[m_sBackslashPath.GetLength()+1] = 0;
			bRet = CTSVNPathList::DeleteViaShell(buf, bTrash);
		}
		else
		{
			bRet = !!::DeleteFile(m_sBackslashPath);
		}
	}
	m_bExists = false;
	m_bExistsKnown = true;
	return bRet;
}

__int64  CTSVNPath::GetLastWriteTime() const
{
	if(!m_bLastWriteTimeKnown)
	{
		UpdateAttributes();
	}
	return m_lastWriteTime;
}

__int64 CTSVNPath::GetFileSize() const
{
	if(!m_bDirectoryKnown)
	{
		UpdateAttributes();
	}
	return m_fileSize;
}

bool CTSVNPath::IsReadOnly() const
{
	if(!m_bLastWriteTimeKnown)
	{
		UpdateAttributes();
	}
	return m_bIsReadOnly;
}

void CTSVNPath::UpdateAttributes() const
{
	EnsureBackslashPathSet();
	WIN32_FILE_ATTRIBUTE_DATA attribs;
	if (m_sBackslashPath.GetLength() >= 248)
		m_sLongBackslashPath = _T("\\\\?\\") + m_sBackslashPath;
	if(GetFileAttributesEx(m_sBackslashPath.GetLength() >= 248 ? m_sLongBackslashPath : m_sBackslashPath, GetFileExInfoStandard, &attribs))
	{
		m_bIsDirectory = !!(attribs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
		m_lastWriteTime = *(__int64*)&attribs.ftLastWriteTime;
		if (m_bIsDirectory)
		{
			m_fileSize = 0;
		}
		else
		{
			m_fileSize = ((INT64)( (DWORD)(attribs.nFileSizeLow) ) | ( (INT64)( (DWORD)(attribs.nFileSizeHigh) )<<32 ));
		}
		m_bIsReadOnly = !!(attribs.dwFileAttributes & FILE_ATTRIBUTE_READONLY);
		m_bExists = true;
	}
	else
	{
		DWORD err = GetLastError();
		if ((err == ERROR_FILE_NOT_FOUND)||(err == ERROR_PATH_NOT_FOUND)||(err == ERROR_INVALID_NAME))
		{
			m_bIsDirectory = false;
			m_lastWriteTime = 0;
			m_fileSize = 0;
			m_bExists = false;
		}
		else
		{
			m_bIsDirectory = false;
			m_lastWriteTime = 0;
			m_fileSize = 0;
			m_bExists = true;
			return;
		}
	}
	m_bDirectoryKnown = true;
	m_bLastWriteTimeKnown = true;
	m_bExistsKnown = true;
}


void CTSVNPath::EnsureBackslashPathSet() const
{
	if(m_sBackslashPath.IsEmpty())
	{
		SetBackslashPath(m_sFwdslashPath);
		ATLASSERT(IsEmpty() || !m_sBackslashPath.IsEmpty());
	}
}
void CTSVNPath::EnsureFwdslashPathSet() const
{
	if(m_sFwdslashPath.IsEmpty())
	{
		SetFwdslashPath(m_sBackslashPath);
		ATLASSERT(IsEmpty() || !m_sFwdslashPath.IsEmpty());
	}
}


// Reset all the caches
void CTSVNPath::Reset()
{
	m_bDirectoryKnown = false;
	m_bURLKnown = false;
	m_bLastWriteTimeKnown = false;
	m_bHasAdminDirKnown = false;
	m_bIsValidOnWindowsKnown = false;
	m_bIsAdminDirKnown = false;
	m_bExistsKnown = false;
	m_bIsSpecialDirectoryKnown = false;
	m_bIsSpecialDirectory = false;

	m_sBackslashPath.Empty();
	m_sLongBackslashPath.Empty();
	m_sFwdslashPath.Empty();
	m_sUTF8FwdslashPath.Empty();
	m_sUIPath.Empty();
	ATLASSERT(IsEmpty());
}

CTSVNPath CTSVNPath::GetDirectory() const
{
	if ((IsDirectory())||(!Exists()))
	{
		return *this;
	}
	return GetContainingDirectory();
}

CTSVNPath CTSVNPath::GetContainingDirectory() const
{
	EnsureBackslashPathSet();

	CString sDirName = m_sBackslashPath.Left(m_sBackslashPath.ReverseFind('\\'));
	if(sDirName.GetLength() == 2 && sDirName[1] == ':')
	{
		// This is a root directory, which needs a trailing slash
		sDirName += '\\';
		if(sDirName == m_sBackslashPath)
		{
			// We were clearly provided with a root path to start with - we should return nothing now
			sDirName.Empty();
		}
	}
	if(sDirName.GetLength() == 1 && sDirName[0] == '\\')
	{
		// We have an UNC path and we already are the root
		sDirName.Empty();
	}
	CTSVNPath retVal;
	retVal.SetFromWin(sDirName);
	return retVal;
}

CString CTSVNPath::GetRootPathString() const
{
	EnsureBackslashPathSet();
	CString workingPath = m_sBackslashPath;
	LPTSTR pPath = workingPath.GetBuffer(MAX_PATH);		// MAX_PATH ok here.
	ATLVERIFY(::PathStripToRoot(pPath));
	workingPath.ReleaseBuffer();
	return workingPath;
}


CString CTSVNPath::GetFilename() const
{
	ATLASSERT(!IsDirectory());
	return GetFileOrDirectoryName();
}

CString CTSVNPath::GetFileOrDirectoryName() const
{
	EnsureBackslashPathSet();
	return m_sBackslashPath.Mid(m_sBackslashPath.ReverseFind('\\')+1);
}

CString CTSVNPath::GetUIFileOrDirectoryName() const
{
	GetUIPathString();
	CString sUIPath = m_sUIPath;
	sUIPath.Replace('\\', '/');
	return sUIPath.Mid(sUIPath.ReverseFind('/')+1);
}

CString CTSVNPath::GetFileExtension() const
{
	if(!IsDirectory())
	{
		EnsureBackslashPathSet();
		int dotPos = m_sBackslashPath.ReverseFind('.');
		int slashPos = m_sBackslashPath.ReverseFind('\\');
		if (dotPos > slashPos)
			return m_sBackslashPath.Mid(dotPos);
	}
	return CString();
}


bool CTSVNPath::ArePathStringsEqual(const CString& sP1, const CString& sP2)
{
	int length = sP1.GetLength();
	if(length != sP2.GetLength())
	{
		// Different lengths
		return false;
	}
	// We work from the end of the strings, because path differences
	// are more likely to occur at the far end of a string
	LPCTSTR pP1Start = sP1;
	LPCTSTR pP1 = pP1Start+(length-1);
	LPCTSTR pP2 = ((LPCTSTR)sP2)+(length-1);
	while(length-- > 0)
	{
		if(_totlower(*pP1--) != _totlower(*pP2--))
		{
			return false;
		}
	}
	return true;
}

bool CTSVNPath::ArePathStringsEqualWithCase(const CString& sP1, const CString& sP2)
{
	int length = sP1.GetLength();
	if(length != sP2.GetLength())
	{
		// Different lengths
		return false;
	}
	// We work from the end of the strings, because path differences
	// are more likely to occur at the far end of a string
	LPCTSTR pP1Start = sP1;
	LPCTSTR pP1 = pP1Start+(length-1);
	LPCTSTR pP2 = ((LPCTSTR)sP2)+(length-1);
	while(length-- > 0)
	{
		if((*pP1--) != (*pP2--))
		{
			return false;
		}
	}
	return true;
}

bool CTSVNPath::IsEmpty() const
{
	// Check the backward slash path first, since the chance that this
	// one is set is higher. In case of a 'false' return value it's a little
	// bit faster.
	return m_sBackslashPath.IsEmpty() && m_sFwdslashPath.IsEmpty();
}

// Test if both paths refer to the same item
// Ignores slash direction
bool CTSVNPath::IsEquivalentTo(const CTSVNPath& rhs) const
{
	// Try and find a slash direction which avoids having to convert
	// both filenames
	if(!m_sBackslashPath.IsEmpty())
	{
		// *We've* got a \ path - make sure that the RHS also has a \ path
		rhs.EnsureBackslashPathSet();
		return ArePathStringsEqualWithCase(m_sBackslashPath, rhs.m_sBackslashPath);
	}
	else
	{
		// Assume we've got a fwdslash path and make sure that the RHS has one
		rhs.EnsureFwdslashPathSet();
		return ArePathStringsEqualWithCase(m_sFwdslashPath, rhs.m_sFwdslashPath);
	}
}

// Test if both paths refer to the same item
// Ignores case and slash direction
bool CTSVNPath::IsEquivalentToWithoutCase(const CTSVNPath& rhs) const
{
	// Try and find a slash direction which avoids having to convert
	// both filenames
	if(!m_sBackslashPath.IsEmpty())
	{
		// *We've* got a \ path - make sure that the RHS also has a \ path
		rhs.EnsureBackslashPathSet();
		return ArePathStringsEqual(m_sBackslashPath, rhs.m_sBackslashPath);
	}
	else
	{
		// Assume we've got a fwdslash path and make sure that the RHS has one
		rhs.EnsureFwdslashPathSet();
		return ArePathStringsEqual(m_sFwdslashPath, rhs.m_sFwdslashPath);
	}
}

bool CTSVNPath::IsAncestorOf(const CTSVNPath& possibleDescendant) const
{
	possibleDescendant.EnsureBackslashPathSet();
	EnsureBackslashPathSet();

	bool bPathStringsEqual = ArePathStringsEqual(m_sBackslashPath, possibleDescendant.m_sBackslashPath.Left(m_sBackslashPath.GetLength()));
	if (m_sBackslashPath.GetLength() >= possibleDescendant.GetWinPathString().GetLength())
	{
		return bPathStringsEqual;		
	}
	
	return (bPathStringsEqual && 
			((possibleDescendant.m_sBackslashPath[m_sBackslashPath.GetLength()] == '\\')||
			(m_sBackslashPath.GetLength()==3 && m_sBackslashPath[1]==':')));
}

// Get a string representing the file path, optionally with a base 
// section stripped off the front.
LPCTSTR CTSVNPath::GetDisplayString(const CTSVNPath* pOptionalBasePath /* = NULL*/) const
{
	EnsureFwdslashPathSet();
	if(pOptionalBasePath != NULL)
	{
		// Find the length of the base-path without having to do an 'ensure' on it
		int baseLength = max(pOptionalBasePath->m_sBackslashPath.GetLength(), pOptionalBasePath->m_sFwdslashPath.GetLength());

		// Now, chop that baseLength of the front of the path
		LPCTSTR result = m_sFwdslashPath;
		result += baseLength;
		while (*result == '/')
			++result;

		return result;
	}
	return m_sFwdslashPath;
}

int CTSVNPath::Compare(const CTSVNPath& left, const CTSVNPath& right)
{
	left.EnsureBackslashPathSet();
	right.EnsureBackslashPathSet();
	return CStringUtils::FastCompareNoCase (left.m_sBackslashPath, right.m_sBackslashPath);
}

bool operator<(const CTSVNPath& left, const CTSVNPath& right)
{
	return CTSVNPath::Compare(left, right) < 0;
}

bool CTSVNPath::PredLeftEquivalentToRight(const CTSVNPath& left, const CTSVNPath& right)
{
	return left.IsEquivalentTo(right);
}

bool CTSVNPath::PredLeftSameWCPathAsRight(const CTSVNPath& left, const CTSVNPath& right)
{
	if (left.IsAdminDir() && right.IsAdminDir())
	{
		CTSVNPath l = left;
		CTSVNPath r = right;
		do 
		{
			l = l.GetContainingDirectory();
		} while(l.HasAdminDir());
		do 
		{
			r = r.GetContainingDirectory();
		} while(r.HasAdminDir());
		return l.GetContainingDirectory().IsEquivalentTo(r.GetContainingDirectory());
	}
	return left.GetDirectory().IsEquivalentTo(right.GetDirectory());
}

bool CTSVNPath::CheckChild(const CTSVNPath &parent, const CTSVNPath& child)
{
	return parent.IsAncestorOf(child);
}

void CTSVNPath::AppendRawString(const CString& sAppend)
{
	EnsureFwdslashPathSet();
	CString strCopy = m_sFwdslashPath += sAppend;
	SetFromUnknown(strCopy);
}

void CTSVNPath::AppendPathString(const CString& sAppend)
{
	EnsureBackslashPathSet();
	CString cleanAppend(sAppend);
	cleanAppend.Replace('/', '\\');
	cleanAppend.TrimLeft('\\');
	m_sBackslashPath.TrimRight('\\');
	CString strCopy = m_sBackslashPath + _T("\\") + cleanAppend;
	SetFromWin(strCopy);
}

bool CTSVNPath::HasAdminDir() const
{
	if (m_bHasAdminDirKnown)
		return m_bHasAdminDir;

	EnsureBackslashPathSet();
	m_bHasAdminDir = g_SVNAdminDir.HasAdminDir(m_sBackslashPath, IsDirectory());
	m_bHasAdminDirKnown = true;
	return m_bHasAdminDir;
}

bool CTSVNPath::IsAdminDir() const
{
	if (m_bIsAdminDirKnown)
		return m_bIsAdminDir;
	
	EnsureBackslashPathSet();
	m_bIsAdminDir = g_SVNAdminDir.IsAdminDirPath(m_sBackslashPath);
	m_bIsAdminDirKnown = true;
	return m_bIsAdminDir;
}

bool CTSVNPath::IsValidOnWindows() const
{
	if (m_bIsValidOnWindowsKnown)
		return m_bIsValidOnWindows;

// TODO: when VS2010 is out of beta, check whether the
// regex works again properly.
#if _MSC_VER >= 1600
	m_bIsValidOnWindows = true;
	EnsureBackslashPathSet();
	wstring checkPath = m_sBackslashPath;
	if (IsUrl())
	{
		checkPath = m_sBackslashPath.Mid(m_sBackslashPath.Find('\\', m_sBackslashPath.Find(_T(":\\\\"))+3)+1);
	}
	try
	{
		// now check for illegal filenames
		tr1::wregex rx2(_T("(\\\\(lpt\\d|com\\d|aux|nul|prn|con)(\\\\|$))|\\*|[^\\\\]\\?|\\||<|>|\\:[^\\\\]"), tr1::regex_constants::icase | tr1::regex_constants::ECMAScript);
		if (tr1::regex_search(checkPath, rx2, tr1::regex_constants::match_default))
			m_bIsValidOnWindows = false;
	}
	catch (exception) {}

	m_bIsValidOnWindowsKnown = true;
	return m_bIsValidOnWindows;
#else
	m_bIsValidOnWindows = false;
	EnsureBackslashPathSet();
	CString sMatch = m_sBackslashPath + _T("\r\n");
	wstring sPattern;

    // commonly used sub-patterns
    wstring innerCharPattern = _T("[^\\\\/:\\*\\?\"\\|<>]");
    wstring endCharPattern = _T("[^\\\\/:\\*\\?\"\\|<>\\. ]");
    wstring filePattern = _T("(") + innerCharPattern + _T("*") + endCharPattern + _T(")?"); 
    wstring relPathPattern = _T("(((\\.)|(\\.\\.)|(") + filePattern + _T("))\\\\)*") + filePattern + _T("$");
    wstring fullPathPattern = _T("^(\\\\\\\\\\?\\\\)?(([a-zA-Z]:|\\\\)\\\\)?") + relPathPattern;

    // the 'file://' URL is just a normal windows path:
	if (sMatch.Left(7).CompareNoCase(_T("file:\\\\"))==0)
	{
		sMatch = sMatch.Mid(7);
		sMatch.TrimLeft(_T("\\"));
		sPattern = fullPathPattern;
	}
	else if (IsUrl())
	{
		sPattern = _T("^((http|https|svn|svn\\+ssh|file)\\:\\\\+([^\\\\@\\:]+\\:[^\\\\@\\:]+@)?\\\\[^\\\\]+(\\:\\d+)?)?") + relPathPattern;
	}
	else
	{
		sPattern = fullPathPattern;
	}

	try
	{
		tr1::wregex rx(sPattern, tr1::regex_constants::icase | tr1::regex_constants::ECMAScript);
		tr1::wsmatch match;

		wstring rmatch = wstring((LPCTSTR)sMatch);
		if (tr1::regex_match(rmatch, match, rx))
		{
			// the check for _Mycont to be != 0 is required since the regex_match returns
			// sometimes matches that have 'matched == true) but the iterators are actually null
			// which results without that check in a debug assertion (debug mode) or an abort() (!!!) (release mode)
			if ((match[0].matched)&&(match[0].first._Mycont != 0)&&(wstring(match[0]).compare((LPCTSTR)sMatch)==0))
				m_bIsValidOnWindows = true;
		}
		if (m_bIsValidOnWindows)
		{
			// now check for illegal filenames
			tr1::wregex rx2(_T("\\\\(lpt\\d|com\\d|aux|nul|prn|con)(\\\\|$)"), tr1::regex_constants::icase | tr1::regex_constants::ECMAScript);
			rmatch = m_sBackslashPath;
			if (tr1::regex_search(rmatch, rx2, tr1::regex_constants::match_default))
				m_bIsValidOnWindows = false;
		}
	}
	catch (exception) {}

	m_bIsValidOnWindowsKnown = true;
	return m_bIsValidOnWindows;
#endif
}

bool CTSVNPath::IsSpecialDirectory() const
{
	if (m_bIsSpecialDirectoryKnown)
		return m_bIsSpecialDirectory;

	static LPCTSTR specialDirectories[]
		= { _T("trunk"), _T("tags"), _T("branches") };

	for (int i=0 ; i<(sizeof(specialDirectories) / sizeof(specialDirectories[0])) ; ++i)
	{
		CString name = GetFileOrDirectoryName();
		if (0 == name.CompareNoCase(specialDirectories[i]))
		{
			m_bIsSpecialDirectory = true;
			break;
		}
	}

	m_bIsSpecialDirectoryKnown = true;

	return m_bIsSpecialDirectory;
}

//////////////////////////////////////////////////////////////////////////

CTSVNPathList::CTSVNPathList()
{

}

// A constructor which allows a path list to be easily built which one initial entry in
CTSVNPathList::CTSVNPathList(const CTSVNPath& firstEntry)
{
	AddPath(firstEntry);
}

void CTSVNPathList::AddPath(const CTSVNPath& newPath)
{
	m_paths.push_back(newPath);
	m_commonBaseDirectory.Reset();
}
int CTSVNPathList::GetCount() const
{
	return (int)m_paths.size();
}
void CTSVNPathList::Clear()
{
	m_paths.clear();
	m_commonBaseDirectory.Reset();
}

CTSVNPath& CTSVNPathList::operator[](INT_PTR index)
{
	ATLASSERT(index >= 0 && index < (INT_PTR)m_paths.size());
	m_commonBaseDirectory.Reset();
	return m_paths[index];
}

const CTSVNPath& CTSVNPathList::operator[](INT_PTR index) const
{
	ATLASSERT(index >= 0 && index < (INT_PTR)m_paths.size());
	return m_paths[index];
}

bool CTSVNPathList::AreAllPathsFiles() const
{
	// Look through the vector for any directories - if we find them, return false
	return std::find_if(m_paths.begin(), m_paths.end(), std::mem_fun_ref(&CTSVNPath::IsDirectory)) == m_paths.end();
}


#if defined(_MFC_VER)

bool CTSVNPathList::LoadFromFile(const CTSVNPath& filename)
{
	Clear();
	try
	{
		CString strLine;
		CStdioFile file(filename.GetWinPath(), CFile::typeBinary | CFile::modeRead | CFile::shareDenyWrite);

		// for every selected file/folder
		CTSVNPath path;
		while (file.ReadString(strLine))
		{
			path.SetFromUnknown(strLine);
			AddPath(path);
		}
		file.Close();
	}
	catch (CFileException* pE)
	{
		TRACE("CFileException loading target file list\n");
		TCHAR error[10000] = {0};
		pE->GetErrorMessage(error, 10000);
		CMessageBox::Show(NULL, error, _T("TortoiseSVN"), MB_ICONERROR);
		pE->Delete();
		return false;
	}
	return true;
}

bool CTSVNPathList::WriteToFile(const CString& sFilename, bool bANSI /* = false */) const
{
	try
	{
		if (bANSI)
		{
			CStdioFile file(sFilename, CFile::typeText | CFile::modeReadWrite | CFile::modeCreate);
			PathVector::const_iterator it;
			for(it = m_paths.begin(); it != m_paths.end(); ++it)
			{
				CStringA line = CStringA(it->GetSVNPathString()) + '\n';
				file.Write(line, line.GetLength());
			} 
			file.Close();
		}
		else
		{
			CStdioFile file(sFilename, CFile::typeBinary | CFile::modeReadWrite | CFile::modeCreate);
			PathVector::const_iterator it;
			for(it = m_paths.begin(); it != m_paths.end(); ++it)
			{
				file.WriteString(it->GetSVNPathString()+_T("\n"));
			} 
			file.Close();
		}
	}
	catch (CFileException* pE)
	{
		TRACE("CFileException in writing temp file\n");
		pE->Delete();
		return false;
	}
	return true;
}
#endif // _MFC_VER


void CTSVNPathList::LoadFromAsteriskSeparatedString(const CString& sPathString)
{
	if (GetCount() > 0)
		Clear();

	int pos = 0;
	CString temp;
	for(;;)
	{
		temp = sPathString.Tokenize(_T("*"),pos);
		if(temp.IsEmpty())
		{
			break;
		}
		AddPath(CTSVNPath(CPathUtils::GetLongPathname(temp)));
	} 
}

CString CTSVNPathList::CreateAsteriskSeparatedString() const
{
	CString sRet;
	PathVector::const_iterator it;
	for(it = m_paths.begin(); it != m_paths.end(); ++it)
	{
		if (!sRet.IsEmpty())
			sRet += _T("*");
		sRet += it->GetWinPathString();
	}
	return sRet;
}

bool 
CTSVNPathList::AreAllPathsFilesInOneDirectory() const
{
	// Check if all the paths are files and in the same directory
	PathVector::const_iterator it;
	m_commonBaseDirectory.Reset();
	for(it = m_paths.begin(); it != m_paths.end(); ++it)
	{
		if(it->IsDirectory())
		{
			return false;
		}
		const CTSVNPath& baseDirectory = it->GetDirectory();
		if(m_commonBaseDirectory.IsEmpty())
		{
			m_commonBaseDirectory = baseDirectory;
		}
		else if(!m_commonBaseDirectory.IsEquivalentTo(baseDirectory))
		{
			// Different path
			m_commonBaseDirectory.Reset();
			return false;
		}
	}
	return true;
}

CTSVNPath CTSVNPathList::GetCommonDirectory() const
{
	if (m_commonBaseDirectory.IsEmpty())
	{
		PathVector::const_iterator it;
		for(it = m_paths.begin(); it != m_paths.end(); ++it)
		{
			const CTSVNPath& baseDirectory = it->GetDirectory();
			if(m_commonBaseDirectory.IsEmpty())
			{
				m_commonBaseDirectory = baseDirectory;
			}
			else if(!m_commonBaseDirectory.IsEquivalentTo(baseDirectory))
			{
				// Different path
				m_commonBaseDirectory.Reset();
				break;
			}
		}
	}
	// since we only checked strings, not paths,
	// we have to make sure now that we really return a *path* here
	PathVector::const_iterator iter;
	for(iter = m_paths.begin(); iter != m_paths.end(); ++iter)
	{
		if (!m_commonBaseDirectory.IsAncestorOf(*iter))
		{
			m_commonBaseDirectory = m_commonBaseDirectory.GetContainingDirectory();
			break;
		}
	}	
	return m_commonBaseDirectory;
}

CTSVNPath CTSVNPathList::GetCommonRoot() const
{
	if (GetCount() == 0)
		return CTSVNPath();
	if (GetCount() == 1)
		return m_paths[0];

	// first entry is common root for itself
	// (add trailing '\\' to detect partial matches of the last path element)

	CString root = m_paths[0].GetWinPathString() + _T('\\');
	int rootLength = root.GetLength();

	// determine common path string prefix

	for ( PathVector::const_iterator it = m_paths.begin()+1
		; it != m_paths.end()
		; ++it)
	{
		CString path = it->GetWinPathString() + _T('\\');

		int newLength = CStringUtils::GetMatchingLength (root, path);
		if (newLength != rootLength)
		{
			root.Delete (newLength, rootLength);
			rootLength = newLength;
		}
	}

	// remove the last (partial) path element

	if (rootLength > 0)
		root.Delete (root.ReverseFind (_T('\\')), rootLength);

	// done

	return CTSVNPath (root);
}

void CTSVNPathList::SortByPathname(bool bReverse /*= false*/)
{
	std::sort(m_paths.begin(), m_paths.end());
	if (bReverse)
		std::reverse(m_paths.begin(), m_paths.end());
}

void CTSVNPathList::DeleteAllPaths(bool bTrash, bool bFilesOnly)
{
	PathVector::const_iterator it;
	SortByPathname (true); // nested ones first

	CString sPaths;
	for (it = m_paths.begin(); it != m_paths.end(); ++it)
	{
		if ((it->Exists())&&(it->IsDirectory() != bFilesOnly))
		{
			if (!it->IsDirectory())
				::SetFileAttributes(it->GetWinPath(), FILE_ATTRIBUTE_NORMAL);

			sPaths += it->GetWinPath();
			sPaths += '\0';
		}
	}
	sPaths += '\0';
	sPaths += '\0';
	DeleteViaShell((LPCTSTR)sPaths, bTrash); 
	Clear();
}

void CTSVNPathList::RemoveDuplicates()
{
	SortByPathname();
	// Remove the duplicates
	// (Unique moves them to the end of the vector, then erase chops them off)
	m_paths.erase(std::unique(m_paths.begin(), m_paths.end(), &CTSVNPath::PredLeftEquivalentToRight), m_paths.end());
}

void CTSVNPathList::RemoveAdminPaths()
{
	PathVector::iterator it;
	for(it = m_paths.begin(); it != m_paths.end(); )
	{
		if (it->IsAdminDir())
		{
			m_paths.erase(it);
			it = m_paths.begin();
		}
		else
			++it;
	}
}

void CTSVNPathList::RemovePath(const CTSVNPath& path)
{
	PathVector::iterator it;
	for(it = m_paths.begin(); it != m_paths.end(); ++it)
	{
		if (it->IsEquivalentTo(path))
		{
			m_paths.erase(it);
			return;
		}
	}
}

void CTSVNPathList::RemoveChildren()
{
	SortByPathname();
	m_paths.erase(std::unique(m_paths.begin(), m_paths.end(), &CTSVNPath::CheckChild), m_paths.end());
}

bool CTSVNPathList::IsEqual(const CTSVNPathList& list)
{
	if (list.GetCount() != GetCount())
		return false;
	for (int i=0; i<list.GetCount(); ++i)
	{
		if (!list[i].IsEquivalentTo(m_paths[i]))
			return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////

apr_array_header_t * CTSVNPathList::MakePathArray (apr_pool_t *pool) const
{
	apr_array_header_t *targets = apr_array_make (pool, GetCount(), sizeof(const char *));

	for(int nItem = 0; nItem < GetCount(); nItem++)
	{
		const char * target = m_paths[nItem].GetSVNApiPath(pool);
		(*((const char **) apr_array_push (targets))) = target;
	}

	return targets;
}

bool CTSVNPathList::DeleteViaShell(LPCTSTR path, bool bTrash)
{
	SHFILEOPSTRUCT shop = {0};
	shop.wFunc = FO_DELETE;
	shop.pFrom = path;
	shop.fFlags = FOF_NOCONFIRMATION|FOF_NOERRORUI|FOF_SILENT|FOF_NO_CONNECTED_ELEMENTS;
	if (bTrash)
		shop.fFlags |= FOF_ALLOWUNDO;
	const bool bRet = (SHFileOperation(&shop) == 0);
	return bRet;
}

//////////////////////////////////////////////////////////////////////////


#if defined(_DEBUG)
// Some test cases for these classes
static class CTSVNPathTests
{
public:
	CTSVNPathTests()
	{
		apr_initialize();
		pool = svn_pool_create(NULL);
		GetDirectoryTest();
		AdminDirTest();
		SortTest();
		RawAppendTest();
		PathAppendTest();
		RemoveDuplicatesTest();
		RemoveChildrenTest();
		ContainingDirectoryTest();
		AncestorTest();
		SubversionPathTest();
		GetCommonRootTest();
#if defined(_MFC_VER)
		ValidPathAndUrlTest();
		ListLoadingTest();
#endif
		apr_terminate();
	}

private:
	apr_pool_t * pool;
	void GetDirectoryTest()
	{
		// Bit tricky, this test, because we need to know something about the file
		// layout on the machine which is running the test
		TCHAR winDir[MAX_PATH+1];
		GetWindowsDirectory(winDir, MAX_PATH);
		CString sWinDir(winDir);

		CTSVNPath testPath;
		// This is a file which we know will always be there
		testPath.SetFromUnknown(sWinDir + _T("\\win.ini"));
		ATLASSERT(!testPath.IsDirectory());
		ATLASSERT(testPath.GetDirectory().GetWinPathString() == sWinDir);
		ATLASSERT(testPath.GetContainingDirectory().GetWinPathString() == sWinDir);

		// Now do the test on the win directory itself - It's hard to be sure about the containing directory
		// but we know it must be different to the directory itself
		testPath.SetFromUnknown(sWinDir);
		ATLASSERT(testPath.IsDirectory());
		ATLASSERT(testPath.GetDirectory().GetWinPathString() == sWinDir);
		ATLASSERT(testPath.GetContainingDirectory().GetWinPathString() != sWinDir);
		ATLASSERT(testPath.GetContainingDirectory().GetWinPathString().GetLength() < sWinDir.GetLength());

		// Try a root path
		testPath.SetFromUnknown(_T("C:\\"));
		ATLASSERT(testPath.IsDirectory());
		ATLASSERT(testPath.GetDirectory().GetWinPathString().CompareNoCase(_T("C:\\"))==0);
		ATLASSERT(testPath.GetContainingDirectory().IsEmpty());
		// Try a root UNC path
		testPath.SetFromUnknown(_T("\\MYSTATION"));
		ATLASSERT(testPath.GetContainingDirectory().IsEmpty());

		// test the UI path methods
		testPath.SetFromUnknown(_T("c:\\testing%20test"));
		ATLASSERT(testPath.GetUIFileOrDirectoryName().CompareNoCase(_T("testing%20test")) == 0);
#ifdef _MFC_VER
		testPath.SetFromUnknown(_T("http://server.com/testing%20special%20chars%20%c3%a4%c3%b6%c3%bc"));
		ATLASSERT(testPath.GetUIFileOrDirectoryName().CompareNoCase(_T("testing special chars \344\366\374")) == 0);
#endif
	}

	void AdminDirTest()
	{
		CTSVNPath testPath;
		testPath.SetFromUnknown(_T("c:\\.svndir"));
		ATLASSERT(!testPath.IsAdminDir());
		testPath.SetFromUnknown(_T("c:\\test.svn"));
		ATLASSERT(!testPath.IsAdminDir());
		testPath.SetFromUnknown(_T("c:\\.svn"));
		ATLASSERT(testPath.IsAdminDir());
		testPath.SetFromUnknown(_T("c:\\.svndir\\test"));
		ATLASSERT(!testPath.IsAdminDir());
		testPath.SetFromUnknown(_T("c:\\.svn\\test"));
		ATLASSERT(testPath.IsAdminDir());
		
		CTSVNPathList pathList;
		pathList.AddPath(CTSVNPath(_T("c:\\.svndir")));
		pathList.AddPath(CTSVNPath(_T("c:\\.svn")));
		pathList.AddPath(CTSVNPath(_T("c:\\.svn\\test")));
		pathList.AddPath(CTSVNPath(_T("c:\\test")));
		pathList.RemoveAdminPaths();
		ATLASSERT(pathList.GetCount()==2);
		pathList.Clear();
		pathList.AddPath(CTSVNPath(_T("c:\\test")));
		pathList.RemoveAdminPaths();
		ATLASSERT(pathList.GetCount()==1);
	}
	
	void SortTest()
	{
		CTSVNPathList testList;
		CTSVNPath testPath;
		testPath.SetFromUnknown(_T("c:/Z"));
		testList.AddPath(testPath);
		testPath.SetFromUnknown(_T("c:/B"));
		testList.AddPath(testPath);
		testPath.SetFromUnknown(_T("c:\\a"));
		testList.AddPath(testPath);
		testPath.SetFromUnknown(_T("c:/Test"));
		testList.AddPath(testPath);

		testList.SortByPathname();

		ATLASSERT(testList[0].GetWinPathString() == _T("c:\\a"));
		ATLASSERT(testList[1].GetWinPathString() == _T("c:\\B"));
		ATLASSERT(testList[2].GetWinPathString() == _T("c:\\Test"));
		ATLASSERT(testList[3].GetWinPathString() == _T("c:\\Z"));
	}

	void RawAppendTest()
	{
		CTSVNPath testPath(_T("c:/test/"));
		testPath.AppendRawString(_T("/Hello"));
		ATLASSERT(testPath.GetWinPathString() == _T("c:\\test\\Hello"));

		testPath.AppendRawString(_T("\\T2"));
		ATLASSERT(testPath.GetWinPathString() == _T("c:\\test\\Hello\\T2"));

		CTSVNPath testFilePath(_T("C:\\windows\\win.ini"));
		CTSVNPath testBasePath(_T("c:/temp/myfile.txt"));
		testBasePath.AppendRawString(testFilePath.GetFileExtension());
		ATLASSERT(testBasePath.GetWinPathString() == _T("c:\\temp\\myfile.txt.ini"));
	}

	void PathAppendTest()
	{
		CTSVNPath testPath(_T("c:/test/"));
		testPath.AppendPathString(_T("/Hello"));
		ATLASSERT(testPath.GetWinPathString() == _T("c:\\test\\Hello"));

		testPath.AppendPathString(_T("T2"));
		ATLASSERT(testPath.GetWinPathString() == _T("c:\\test\\Hello\\T2"));

		CTSVNPath testFilePath(_T("C:\\windows\\win.ini"));
		CTSVNPath testBasePath(_T("c:/temp/myfile.txt"));
		// You wouldn't want to do this in real life - you'd use append-raw
		testBasePath.AppendPathString(testFilePath.GetFileExtension());
		ATLASSERT(testBasePath.GetWinPathString() == _T("c:\\temp\\myfile.txt\\.ini"));
	}

	void RemoveDuplicatesTest()
	{
		CTSVNPathList list;
		list.AddPath(CTSVNPath(_T("Z")));
		list.AddPath(CTSVNPath(_T("A")));
		list.AddPath(CTSVNPath(_T("E")));
		list.AddPath(CTSVNPath(_T("E")));

		ATLASSERT(list[2].IsEquivalentTo(list[3]));
		ATLASSERT(list[2]==list[3]);
		
		ATLASSERT(list.GetCount() == 4);

		list.RemoveDuplicates();

		ATLASSERT(list.GetCount() == 3);

		ATLASSERT(list[0].GetWinPathString() == _T("A"));
		ATLASSERT(list[1].GetWinPathString().Compare(_T("E")) == 0);
		ATLASSERT(list[2].GetWinPathString() == _T("Z"));
	}
	
	void RemoveChildrenTest()
	{
		CTSVNPathList list;
		list.AddPath(CTSVNPath(_T("c:\\test")));
		list.AddPath(CTSVNPath(_T("c:\\test\\file")));
		list.AddPath(CTSVNPath(_T("c:\\testfile")));
		list.AddPath(CTSVNPath(_T("c:\\parent")));
		list.AddPath(CTSVNPath(_T("c:\\parent\\child")));
		list.AddPath(CTSVNPath(_T("c:\\parent\\child1")));
		list.AddPath(CTSVNPath(_T("c:\\parent\\child2")));
		
		ATLASSERT(list.GetCount() == 7);

		list.RemoveChildren();
		
		ATLTRACE("count = %d\n", list.GetCount());
		ATLASSERT(list.GetCount() == 3);

		list.SortByPathname();

		ATLASSERT(list[0].GetWinPathString().Compare(_T("c:\\parent")) == 0);
		ATLASSERT(list[1].GetWinPathString().Compare(_T("c:\\test")) == 0);
		ATLASSERT(list[2].GetWinPathString().Compare(_T("c:\\testfile")) == 0);
	}

#if defined(_MFC_VER)
	void ListLoadingTest()
	{
		TCHAR buf[MAX_PATH];
		GetCurrentDirectory(MAX_PATH, buf);
		CString sPathList(_T("Path1*c:\\path2 with spaces and stuff*\\funnypath\\*"));
		CTSVNPathList testList;
		testList.LoadFromAsteriskSeparatedString(sPathList);

		ATLASSERT(testList.GetCount() == 3);
		ATLASSERT(testList[0].GetWinPathString() == CString(buf) + _T("\\Path1"));
		ATLASSERT(testList[1].GetWinPathString() == _T("c:\\path2 with spaces and stuff"));
		ATLASSERT(testList[2].GetWinPathString() == _T("\\funnypath"));
		
		ATLASSERT(testList.GetCommonRoot().GetWinPathString() == _T(""));
		sPathList = _T("c:\\path2 with spaces and stuff*c:\\funnypath\\*");
		testList.LoadFromAsteriskSeparatedString(sPathList);
		ATLASSERT(testList.GetCommonRoot().GetWinPathString() == _T("c:\\"));
	}
#endif 

	void ContainingDirectoryTest()
	{

		CTSVNPath testPath;
		testPath.SetFromWin(_T("c:\\a\\b\\c\\d\\e"));
		CTSVNPath dir;
		dir = testPath.GetContainingDirectory();
		ATLASSERT(dir.GetWinPathString() == _T("c:\\a\\b\\c\\d"));
		dir = dir.GetContainingDirectory();
		ATLASSERT(dir.GetWinPathString() == _T("c:\\a\\b\\c"));
		dir = dir.GetContainingDirectory();
		ATLASSERT(dir.GetWinPathString() == _T("c:\\a\\b"));
		dir = dir.GetContainingDirectory();
		ATLASSERT(dir.GetWinPathString() == _T("c:\\a"));
		dir = dir.GetContainingDirectory();
		ATLASSERT(dir.GetWinPathString() == _T("c:\\"));
		dir = dir.GetContainingDirectory();
		ATLASSERT(dir.IsEmpty());
		ATLASSERT(dir.GetWinPathString() == _T(""));
	}
	
	void AncestorTest()
	{
		CTSVNPath testPath;
		testPath.SetFromWin(_T("c:\\windows"));
		ATLASSERT(testPath.IsAncestorOf(CTSVNPath(_T("c:\\")))==false);
		ATLASSERT(testPath.IsAncestorOf(CTSVNPath(_T("c:\\windows"))));
		ATLASSERT(testPath.IsAncestorOf(CTSVNPath(_T("c:\\windowsdummy")))==false);
		ATLASSERT(testPath.IsAncestorOf(CTSVNPath(_T("c:\\windows\\test.txt"))));
		ATLASSERT(testPath.IsAncestorOf(CTSVNPath(_T("c:\\windows\\system32\\test.txt"))));
	}

	void SubversionPathTest()
	{
		CTSVNPath testPath;
		testPath.SetFromWin(_T("c:\\"));
		ATLASSERT(strcmp(testPath.GetSVNApiPath(pool), "c:") == 0);
		testPath.SetFromWin(_T("c:\\folder"));
		ATLASSERT(strcmp(testPath.GetSVNApiPath(pool), "c:/folder") == 0);
		testPath.SetFromWin(_T("c:\\a\\b\\c\\d\\e"));
		ATLASSERT(strcmp(testPath.GetSVNApiPath(pool), "c:/a/b/c/d/e") == 0);
		testPath.SetFromUnknown(_T("http://testing/"));
		ATLASSERT(strcmp(testPath.GetSVNApiPath(pool), "http://testing") == 0);
		testPath.SetFromSVN(NULL);
		ATLASSERT(strlen(testPath.GetSVNApiPath(pool))==0);
		testPath.SetFromWin(_T("\\\\a\\b\\c\\d\\e"));
		ATLASSERT(strcmp(testPath.GetSVNApiPath(pool), "//a/b/c/d/e") == 0);
#if defined(_MFC_VER)
		testPath.SetFromUnknown(_T("http://testing again"));
		ATLASSERT(strcmp(testPath.GetSVNApiPath(pool), "http://testing%20again") == 0);
		testPath.SetFromUnknown(_T("http://testing%20again"));
		ATLASSERT(strcmp(testPath.GetSVNApiPath(pool), "http://testing%20again") == 0);
		testPath.SetFromUnknown(_T("http://testing special chars \344\366\374"));
		ATLASSERT(strcmp(testPath.GetSVNApiPath(pool), "http://testing%20special%20chars%20%c3%a4%c3%b6%c3%bc") == 0);		
#endif
	}

	void GetCommonRootTest()
	{
		CTSVNPath pathA (_T("C:\\Development\\LogDlg.cpp"));
		CTSVNPath pathB (_T("C:\\Development\\LogDlg.h"));
		CTSVNPath pathC (_T("C:\\Development\\SomeDir\\LogDlg.h"));
		
		CTSVNPathList list;
		list.AddPath(pathA);
		ATLASSERT(list.GetCommonRoot().GetWinPathString().CompareNoCase(_T("C:\\Development\\LogDlg.cpp"))==0);
		list.AddPath(pathB);
		ATLASSERT(list.GetCommonRoot().GetWinPathString().CompareNoCase(_T("C:\\Development"))==0);
		list.AddPath(pathC);
		ATLASSERT(list.GetCommonRoot().GetWinPathString().CompareNoCase(_T("C:\\Development"))==0);
#ifdef _MFC_VER
		CString sPathList = _T("D:\\Development\\StExBar\\StExBar\\src\\setup\\Setup64.wxs*D:\\Development\\StExBar\\StExBar\\src\\setup\\Setup.wxs*D:\\Development\\StExBar\\SKTimeStamp\\src\\setup\\Setup.wxs*D:\\Development\\StExBar\\SKTimeStamp\\src\\setup\\Setup64.wxs");
		list.LoadFromAsteriskSeparatedString(sPathList);
		ATLASSERT(list.GetCommonRoot().GetWinPathString().CompareNoCase(_T("D:\\Development\\StExBar"))==0);

		sPathList = _T("c:\\windows\\explorer.exe*c:\\windows");
		list.LoadFromAsteriskSeparatedString(sPathList);
		ATLASSERT(list.GetCommonRoot().GetWinPathString().CompareNoCase(_T("c:\\windows"))==0);

		sPathList = _T("c:\\windows\\*c:\\windows");
		list.LoadFromAsteriskSeparatedString(sPathList);
		ATLASSERT(list.GetCommonRoot().GetWinPathString().CompareNoCase(_T("c:\\windows"))==0);

		sPathList = _T("c:\\windows\\system32*c:\\windows\\system");
		list.LoadFromAsteriskSeparatedString(sPathList);
		ATLASSERT(list.GetCommonRoot().GetWinPathString().CompareNoCase(_T("c:\\windows"))==0);

		sPathList = _T("c:\\windowsdummy*c:\\windows");
		list.LoadFromAsteriskSeparatedString(sPathList);
		ATLASSERT(list.GetCommonRoot().GetWinPathString().CompareNoCase(_T("c:\\"))==0);
#endif
	}
	
	void ValidPathAndUrlTest()
	{
		CTSVNPath testPath;
		testPath.SetFromWin(_T("c:\\a\\b\\c.test.txt"));
		ATLASSERT(testPath.IsValidOnWindows());
		testPath.SetFromWin(_T("c:\\"));
		ATLASSERT(testPath.IsValidOnWindows());
		testPath.SetFromWin(_T("D:\\.Net\\SpindleSearch\\"));
		ATLASSERT(testPath.IsValidOnWindows());
		testPath.SetFromWin(_T("c"));
		ATLASSERT(testPath.IsValidOnWindows());
		testPath.SetFromWin(_T("c:\\test folder\\file"));
		ATLASSERT(testPath.IsValidOnWindows());
		testPath.SetFromWin(_T("c:\\folder\\"));
		ATLASSERT(testPath.IsValidOnWindows());
		testPath.SetFromWin(_T("c:\\ext.ext.ext\\ext.ext.ext.ext"));
		ATLASSERT(testPath.IsValidOnWindows());
		testPath.SetFromWin(_T("c:\\.svn"));
		ATLASSERT(testPath.IsValidOnWindows());
		testPath.SetFromWin(_T("c:\\com\\file"));
		ATLASSERT(testPath.IsValidOnWindows());
		testPath.SetFromWin(_T("c:\\test\\conf"));
		ATLASSERT(testPath.IsValidOnWindows());
		testPath.SetFromWin(_T("c:\\LPT"));
		ATLASSERT(testPath.IsValidOnWindows());
		testPath.SetFromWin(_T("c:\\test\\LPT"));
		ATLASSERT(testPath.IsValidOnWindows());
		testPath.SetFromWin(_T("c:\\com1test"));
		ATLASSERT(testPath.IsValidOnWindows());
		testPath.SetFromWin(_T("\\\\?\\c:\\test\\com1test"));
		ATLASSERT(testPath.IsValidOnWindows());

		testPath.SetFromWin(_T("\\\\Share\\filename"));
		ATLASSERT(testPath.IsValidOnWindows());
		testPath.SetFromWin(_T("\\\\Share\\filename.extension"));
		ATLASSERT(testPath.IsValidOnWindows());
		testPath.SetFromWin(_T("\\\\Share\\.svn"));
		ATLASSERT(testPath.IsValidOnWindows());

		// now the negative tests
		testPath.SetFromWin(_T("c:\\test:folder"));
		ATLASSERT(!testPath.IsValidOnWindows());
		testPath.SetFromWin(_T("c:\\file<name"));
		ATLASSERT(!testPath.IsValidOnWindows());
		testPath.SetFromWin(_T("c:\\something*else"));
		ATLASSERT(!testPath.IsValidOnWindows());
		testPath.SetFromWin(_T("c:\\folder\\file?nofile"));
		ATLASSERT(!testPath.IsValidOnWindows());
		testPath.SetFromWin(_T("c:\\ext.>ension"));
		ATLASSERT(!testPath.IsValidOnWindows());
		testPath.SetFromWin(_T("c:\\com1\\filename"));
		ATLASSERT(!testPath.IsValidOnWindows());
		testPath.SetFromWin(_T("c:\\com1"));
		ATLASSERT(!testPath.IsValidOnWindows());
		testPath.SetFromWin(_T("c:\\com1\\AuX"));
		ATLASSERT(!testPath.IsValidOnWindows());

		testPath.SetFromWin(_T("\\\\Share\\lpt9\\filename"));
		ATLASSERT(!testPath.IsValidOnWindows());
		testPath.SetFromWin(_T("\\\\Share\\prn"));
		ATLASSERT(!testPath.IsValidOnWindows());
		testPath.SetFromWin(_T("\\\\Share\\NUL"));
		ATLASSERT(!testPath.IsValidOnWindows());
		
		// now come some URL tests
		testPath.SetFromSVN(_T("http://myserver.com/repos/trunk"));
		ATLASSERT(testPath.IsValidOnWindows());
		testPath.SetFromSVN(_T("https://myserver.com/repos/trunk/file%20with%20spaces"));
		ATLASSERT(testPath.IsValidOnWindows());
		testPath.SetFromSVN(_T("svn://myserver.com/repos/trunk/file with spaces"));
		ATLASSERT(testPath.IsValidOnWindows());
		testPath.SetFromSVN(_T("svn+ssh://www.myserver.com/repos/trunk"));
		ATLASSERT(testPath.IsValidOnWindows());
		testPath.SetFromSVN(_T("http://localhost:90/repos/trunk"));
		ATLASSERT(testPath.IsValidOnWindows());
		testPath.SetFromSVN(_T("file:///C:/SVNRepos/Tester/Proj1/tags/t2"));
		ATLASSERT(testPath.IsValidOnWindows());
		// and some negative URL tests
		testPath.SetFromSVN(_T("https://myserver.com/rep:os/trunk/file%20with%20spaces"));
		ATLASSERT(!testPath.IsValidOnWindows());
		testPath.SetFromSVN(_T("svn://myserver.com/rep<os/trunk/file with spaces"));
		ATLASSERT(!testPath.IsValidOnWindows());
		testPath.SetFromSVN(_T("svn+ssh://www.myserver.com/repos/trunk/prn/"));
		ATLASSERT(!testPath.IsValidOnWindows());
		testPath.SetFromSVN(_T("http://localhost:90/repos/trunk/com1"));
		ATLASSERT(!testPath.IsValidOnWindows());
		
	}

} TSVNPathTestobject;
#endif

