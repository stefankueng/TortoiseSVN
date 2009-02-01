﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2009 - TortoiseSVN

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
#include "resource.h"
#include "TortoiseProc.h"
#include "PathUtils.h"
#include "AppUtils.h"
#include "SVNProperties.h"
#include "StringUtils.h"
#include "MessageBox.h"
#include "Registry.h"
#include "TSVNPath.h"
#include "SVN.h"
#include "RepositoryBrowser.h"
#include "BrowseFolder.h"


CAppUtils::CAppUtils(void)
{
}

CAppUtils::~CAppUtils(void)
{
}

bool CAppUtils::GetMimeType(const CTSVNPath& file, CString& mimetype)
{
	SVNProperties props(file, SVNRev::REV_WC, false);
	for (int i = 0; i < props.GetCount(); ++i)
	{
		if (props.GetItemName(i).compare(_T("svn:mime-type"))==0)
		{
			mimetype = props.GetItemValue(i).c_str();
			return true;
		}
	}
	return false;
}

BOOL CAppUtils::StartExtMerge(const MergeFlags& flags,
	const CTSVNPath& basefile, const CTSVNPath& theirfile, const CTSVNPath& yourfile, const CTSVNPath& mergedfile,
	const CString& basename, const CString& theirname, const CString& yourname, const CString& mergedname)
{

	CRegString regCom = CRegString(_T("Software\\TortoiseSVN\\Merge"));
	CString ext = mergedfile.GetFileExtension();
	CString com = regCom;
	bool bInternal = false;

	CString mimetype;
	if (ext != "")
	{
		// is there an extension specific merge tool?
		CRegString mergetool(_T("Software\\TortoiseSVN\\MergeTools\\") + ext.MakeLower());
		if (CString(mergetool) != "")
		{
			com = mergetool;
		}
	}
	if (GetMimeType(yourfile, mimetype) || GetMimeType(theirfile, mimetype) || GetMimeType(basefile, mimetype))
	{
		// is there a mime type specific merge tool?
		CRegString mergetool(_T("Software\\TortoiseSVN\\MergeTools\\") + mimetype);
		if (CString(mergetool) != "")
		{
			com = mergetool;
		}
	}
	
	if ((flags.bAlternativeTool)&&(!com.IsEmpty()))
	{
		if (com.Left(1).Compare(_T("#"))==0)
			com.Delete(0);
		else
			com.Empty();
	}

	if (com.IsEmpty()||(com.Left(1).Compare(_T("#"))==0))
	{
		// use TortoiseMerge
		bInternal = true;
		com = CPathUtils::GetAppDirectory() + _T("TortoiseMerge.exe");
		if (com.IsEmpty())
		{
			com = CPathUtils::GetAppDirectory();
			com += _T("TortoiseMerge.exe");
		}
		com = _T("\"") + com + _T("\"");
		com = com + _T(" /base:%base /theirs:%theirs /mine:%mine /merged:%merged");
		com = com + _T(" /basename:%bname /theirsname:%tname /minename:%yname /mergedname:%mname");
	}
	// check if the params are set. If not, just add the files to the command line
	if ((com.Find(_T("%merged"))<0)&&(com.Find(_T("%base"))<0)&&(com.Find(_T("%theirs"))<0)&&(com.Find(_T("%mine"))<0))
	{
		com += _T(" \"")+basefile.GetWinPathString()+_T("\"");
		com += _T(" \"")+theirfile.GetWinPathString()+_T("\"");
		com += _T(" \"")+yourfile.GetWinPathString()+_T("\"");
		com += _T(" \"")+mergedfile.GetWinPathString()+_T("\"");
	}
	if (basefile.IsEmpty())
	{
		com.Replace(_T("/base:%base"), _T(""));
		com.Replace(_T("%base"), _T(""));		
	}
	else
		com.Replace(_T("%base"), _T("\"") + basefile.GetWinPathString() + _T("\""));
	if (theirfile.IsEmpty())
	{
		com.Replace(_T("/theirs:%theirs"), _T(""));
		com.Replace(_T("%theirs"), _T(""));
	}
	else
		com.Replace(_T("%theirs"), _T("\"") + theirfile.GetWinPathString() + _T("\""));
	if (yourfile.IsEmpty())
	{
		com.Replace(_T("/mine:%mine"), _T(""));
		com.Replace(_T("%mine"), _T(""));
	}
	else
		com.Replace(_T("%mine"), _T("\"") + yourfile.GetWinPathString() + _T("\""));
	if (mergedfile.IsEmpty())
	{
		com.Replace(_T("/merged:%merged"), _T(""));
		com.Replace(_T("%merged"), _T(""));
	}
	else
		com.Replace(_T("%merged"), _T("\"") + mergedfile.GetWinPathString() + _T("\""));
	if (basename.IsEmpty())
	{
		if (basefile.IsEmpty())
		{
			com.Replace(_T("/basename:%bname"), _T(""));
			com.Replace(_T("%bname"), _T(""));
		}
		else
		{
			com.Replace(_T("%bname"), _T("\"") + basefile.GetUIFileOrDirectoryName() + _T("\""));
		}
	}
	else
		com.Replace(_T("%bname"), _T("\"") + basename + _T("\""));
	if (theirname.IsEmpty())
	{
		if (theirfile.IsEmpty())
		{
			com.Replace(_T("/theirsname:%tname"), _T(""));
			com.Replace(_T("%tname"), _T(""));
		}
		else
		{
			com.Replace(_T("%tname"), _T("\"") + theirfile.GetUIFileOrDirectoryName() + _T("\""));
		}
	}
	else
		com.Replace(_T("%tname"), _T("\"") + theirname + _T("\""));
	if (yourname.IsEmpty())
	{
		if (yourfile.IsEmpty())
		{
			com.Replace(_T("/minename:%yname"), _T(""));
			com.Replace(_T("%yname"), _T(""));
		}
		else
		{
			com.Replace(_T("%yname"), _T("\"") + yourfile.GetUIFileOrDirectoryName() + _T("\""));
		}
	}
	else
		com.Replace(_T("%yname"), _T("\"") + yourname + _T("\""));
	if (mergedname.IsEmpty())
	{
		if (mergedfile.IsEmpty())
		{
			com.Replace(_T("/mergedname:%mname"), _T(""));
			com.Replace(_T("%mname"), _T(""));
		}
		else
		{
			com.Replace(_T("%mname"), _T("\"") + mergedfile.GetUIFileOrDirectoryName() + _T("\""));
		}
	}
	else
		com.Replace(_T("%mname"), _T("\"") + mergedname + _T("\""));

	if ((flags.bReadOnly)&&(bInternal))
		com += _T(" /readonly");

	if(!LaunchApplication(com, IDS_ERR_EXTMERGESTART, false))
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CAppUtils::StartExtPatch(const CTSVNPath& patchfile, const CTSVNPath& dir, const CString& sOriginalDescription, const CString& sPatchedDescription, BOOL bReversed, BOOL bWait)
{
	CString viewer;
	// use TortoiseMerge
	viewer = CPathUtils::GetAppDirectory();
	viewer += _T("TortoiseMerge.exe");

	viewer = _T("\"") + viewer + _T("\"");
	viewer = viewer + _T(" /diff:\"") + patchfile.GetWinPathString() + _T("\"");
	viewer = viewer + _T(" /patchpath:\"") + dir.GetWinPathString() + _T("\"");
	if (bReversed)
		viewer += _T(" /reversedpatch");
	if (!sOriginalDescription.IsEmpty())
		viewer = viewer + _T(" /patchoriginal:\"") + sOriginalDescription + _T("\"");
	if (!sPatchedDescription.IsEmpty())
		viewer = viewer + _T(" /patchpatched:\"") + sPatchedDescription + _T("\"");
	if(!LaunchApplication(viewer, IDS_ERR_DIFFVIEWSTART, !!bWait))
	{
		return FALSE;
	}
	return TRUE;
}

CString CAppUtils::PickDiffTool(const CTSVNPath& file1, const CTSVNPath& file2)
{
	// Is there a mime type specific diff tool?
	CString mimetype;
	if (GetMimeType(file1, mimetype) ||  GetMimeType(file2, mimetype))
	{
		CString difftool = CRegString(_T("Software\\TortoiseSVN\\DiffTools\\") + mimetype);
		if (!difftool.IsEmpty())
			return difftool;
	}
	
	// Is there an extension specific diff tool?
	CString ext = file2.GetFileExtension().MakeLower();
	if (!ext.IsEmpty())
	{
		CString difftool = CRegString(_T("Software\\TortoiseSVN\\DiffTools\\") + ext);
		if (!difftool.IsEmpty())
			return difftool;
		// Maybe we should use TortoiseIDiff?
		if ((ext == _T(".jpg")) || (ext == _T(".jpeg")) ||
			(ext == _T(".bmp")) || (ext == _T(".gif"))  ||
			(ext == _T(".png")) || (ext == _T(".ico"))  ||
			(ext == _T(".dib")) || (ext == _T(".emf")))
		{
			return
				_T("\"") + CPathUtils::GetAppDirectory() + _T("TortoiseIDiff.exe") + _T("\"") +
				_T(" /left:%base /right:%mine /lefttitle:%bname /righttitle:%yname");
		}
	}
	
	// Finally, pick a generic external diff tool
	CString difftool = CRegString(_T("Software\\TortoiseSVN\\Diff"));
	return difftool;
}

bool CAppUtils::StartExtDiff(
	const CTSVNPath& file1, const CTSVNPath& file2,
	const CString& sName1, const CString& sName2, const DiffFlags& flags)
{
	CString viewer;

	CRegDWORD blamediff(_T("Software\\TortoiseSVN\\DiffBlamesWithTortoiseMerge"), FALSE);
	if (!flags.bBlame || !(DWORD)blamediff)
	{
		viewer = PickDiffTool(file1, file2);
		// If registry entry for a diff program is commented out, use TortoiseMerge.
		bool bCommentedOut = viewer.Left(1) == _T("#");
		if (flags.bAlternativeTool)
		{
			// Invert external vs. internal diff tool selection.
			if (bCommentedOut)
				viewer.Delete(0); // uncomment
			else
				viewer = "";
		}
		else if (bCommentedOut)
			viewer = "";
	}

	bool bInternal = viewer.IsEmpty();
	if (bInternal)
	{
		viewer =
			_T("\"") + CPathUtils::GetAppDirectory() + _T("TortoiseMerge.exe") + _T("\"") +
			_T(" /base:%base /mine:%mine /basename:%bname /minename:%yname");
		if (flags.bBlame)
			viewer += _T(" /blame");
	}
	// check if the params are set. If not, just add the files to the command line
	if ((viewer.Find(_T("%base"))<0)&&(viewer.Find(_T("%mine"))<0))
	{
		viewer += _T(" \"")+file1.GetWinPathString()+_T("\"");
		viewer += _T(" \"")+file2.GetWinPathString()+_T("\"");
	}
	if (viewer.Find(_T("%base")) >= 0)
	{
		viewer.Replace(_T("%base"),  _T("\"")+file1.GetWinPathString()+_T("\""));
	}
	if (viewer.Find(_T("%mine")) >= 0)
	{
		viewer.Replace(_T("%mine"),  _T("\"")+file2.GetWinPathString()+_T("\""));
	}

	if (sName1.IsEmpty())
		viewer.Replace(_T("%bname"), _T("\"") + file1.GetUIFileOrDirectoryName() + _T("\""));
	else
		viewer.Replace(_T("%bname"), _T("\"") + sName1 + _T("\""));

	if (sName2.IsEmpty())
		viewer.Replace(_T("%yname"), _T("\"") + file2.GetUIFileOrDirectoryName() + _T("\""));
	else
		viewer.Replace(_T("%yname"), _T("\"") + sName2 + _T("\""));

	if (flags.bReadOnly && bInternal)
		viewer += _T(" /readonly");

	return LaunchApplication(viewer, IDS_ERR_EXTDIFFSTART, flags.bWait);
}

BOOL CAppUtils::StartExtDiffProps(const CTSVNPath& file1, const CTSVNPath& file2, const CString& sName1, const CString& sName2, BOOL bWait, BOOL bReadOnly)
{
	CRegString diffpropsexe(_T("Software\\TortoiseSVN\\DiffProps"));
	CString viewer = diffpropsexe;
	bool bInternal = false;
	if (viewer.IsEmpty()||(viewer.Left(1).Compare(_T("#"))==0))
	{
		//no registry entry (or commented out) for a diff program
		//use TortoiseMerge
		bInternal = true;
		viewer = CPathUtils::GetAppDirectory();
		viewer += _T("TortoiseMerge.exe");
		viewer = _T("\"") + viewer + _T("\"");
		viewer = viewer + _T(" /base:%base /mine:%mine /basename:%bname /minename:%yname");
	}
	// check if the params are set. If not, just add the files to the command line
	if ((viewer.Find(_T("%base"))<0)&&(viewer.Find(_T("%mine"))<0))
	{
		viewer += _T(" \"")+file1.GetWinPathString()+_T("\"");
		viewer += _T(" \"")+file2.GetWinPathString()+_T("\"");
	}
	if (viewer.Find(_T("%base")) >= 0)
	{
		viewer.Replace(_T("%base"),  _T("\"")+file1.GetWinPathString()+_T("\""));
	}
	if (viewer.Find(_T("%mine")) >= 0)
	{
		viewer.Replace(_T("%mine"),  _T("\"")+file2.GetWinPathString()+_T("\""));
	}

	if (sName1.IsEmpty())
		viewer.Replace(_T("%bname"), _T("\"") + file1.GetUIFileOrDirectoryName() + _T("\""));
	else
		viewer.Replace(_T("%bname"), _T("\"") + sName1 + _T("\""));

	if (sName2.IsEmpty())
		viewer.Replace(_T("%yname"), _T("\"") + file2.GetUIFileOrDirectoryName() + _T("\""));
	else
		viewer.Replace(_T("%yname"), _T("\"") + sName2 + _T("\""));

	if ((bReadOnly)&&(bInternal))
		viewer += _T(" /readonly");

	if(!LaunchApplication(viewer, IDS_ERR_EXTDIFFSTART, !!bWait))
	{
		return FALSE;
	}
	return TRUE;
}

BOOL CAppUtils::StartUnifiedDiffViewer(const CTSVNPath& patchfile, const CString& title, BOOL bWait)
{
	CString viewer;
	CRegString v = CRegString(_T("Software\\TortoiseSVN\\DiffViewer"));
	viewer = v;
	if (viewer.IsEmpty() || (viewer.Left(1).Compare(_T("#"))==0))
	{
		// use TortoiseUDiff
		viewer = CPathUtils::GetAppDirectory();
		viewer += _T("TortoiseUDiff.exe");
		// enquote the path to TortoiseUDiff
		viewer = _T("\"") + viewer + _T("\"");
		// add the params
		viewer = viewer + _T(" /patchfile:%1 /title:\"%title\"");

	}
	if (viewer.Find(_T("%1"))>=0)
	{
		if (viewer.Find(_T("\"%1\"")) >= 0)
			viewer.Replace(_T("%1"), patchfile.GetWinPathString());
		else
			viewer.Replace(_T("%1"), _T("\"") + patchfile.GetWinPathString() + _T("\""));
	}
	else
		viewer += _T(" \"") + patchfile.GetWinPathString() + _T("\"");
	if (viewer.Find(_T("%title")) >= 0)
	{
		viewer.Replace(_T("%title"), title);
	}

	if(!LaunchApplication(viewer, IDS_ERR_DIFFVIEWSTART, !!bWait))
	{
		return FALSE;
	}
	return TRUE;
}

BOOL CAppUtils::StartTextViewer(CString file)
{
	CString viewer;
	CRegString txt = CRegString(_T(".txt\\"), _T(""), FALSE, HKEY_CLASSES_ROOT);
	viewer = txt;
	viewer = viewer + _T("\\Shell\\Open\\Command\\");
	CRegString txtexe = CRegString(viewer, _T(""), FALSE, HKEY_CLASSES_ROOT);
	viewer = txtexe;

	DWORD len = ExpandEnvironmentStrings(viewer, NULL, 0);
	TCHAR * buf = new TCHAR[len+1];
	ExpandEnvironmentStrings(viewer, buf, len);
	viewer = buf;
	delete [] buf;
	len = ExpandEnvironmentStrings(file, NULL, 0);
	buf = new TCHAR[len+1];
	ExpandEnvironmentStrings(file, buf, len);
	file = buf;
	delete [] buf;
	file = _T("\"")+file+_T("\"");
	if (viewer.IsEmpty())
	{
		OPENFILENAME ofn = {0};				// common dialog box structure
		TCHAR szFile[MAX_PATH] = {0};		// buffer for file name. Explorer can't handle paths longer than MAX_PATH.
		// Initialize OPENFILENAME
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = NULL;
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
		CString sFilter;
		sFilter.LoadString(IDS_PROGRAMSFILEFILTER);
		TCHAR * pszFilters = new TCHAR[sFilter.GetLength()+4];
		_tcscpy_s (pszFilters, sFilter.GetLength()+4, sFilter);
		// Replace '|' delimiters with '\0's
		TCHAR *ptr = pszFilters + _tcslen(pszFilters);  //set ptr at the NULL
		while (ptr != pszFilters)
		{
			if (*ptr == '|')
				*ptr = '\0';
			ptr--;
		}
		ofn.lpstrFilter = pszFilters;
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		CString temp;
		temp.LoadString(IDS_UTILS_SELECTTEXTVIEWER);
		CStringUtils::RemoveAccelerators(temp);
		ofn.lpstrTitle = temp;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

		// Display the Open dialog box. 

		if (GetOpenFileName(&ofn)==TRUE)
		{
			delete [] pszFilters;
			viewer = CString(ofn.lpstrFile);
		}
		else
		{
			delete [] pszFilters;
			return FALSE;
		}
	}
	if (viewer.Find(_T("\"%1\"")) >= 0)
	{
		viewer.Replace(_T("\"%1\""), file);
	}
	else if (viewer.Find(_T("%1")) >= 0)
	{
		viewer.Replace(_T("%1"),  file);
	}
	else
	{
		viewer += _T(" ");
		viewer += file;
	}

	if(!LaunchApplication(viewer, IDS_ERR_TEXTVIEWSTART, false))
	{
		return FALSE;
	}
	return TRUE;
}

BOOL CAppUtils::CheckForEmptyDiff(const CTSVNPath& sDiffPath)
{
	DWORD length = 0;
	HANDLE hFile = ::CreateFile(sDiffPath.GetWinPath(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return TRUE;
	length = ::GetFileSize(hFile, NULL);
	::CloseHandle(hFile);
	if (length < 4)
		return TRUE;
	return FALSE;

}

void CAppUtils::CreateFontForLogs(CFont& fontToCreate)
{
	LOGFONT logFont;
	HDC hScreenDC = ::GetDC(NULL);
	logFont.lfHeight         = -MulDiv((DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\LogFontSize"), 8), GetDeviceCaps(hScreenDC, LOGPIXELSY), 72);
	::ReleaseDC(NULL, hScreenDC);
	logFont.lfWidth          = 0;
	logFont.lfEscapement     = 0;
	logFont.lfOrientation    = 0;
	logFont.lfWeight         = FW_NORMAL;
	logFont.lfItalic         = 0;
	logFont.lfUnderline      = 0;
	logFont.lfStrikeOut      = 0;
	logFont.lfCharSet        = DEFAULT_CHARSET;
	logFont.lfOutPrecision   = OUT_DEFAULT_PRECIS;
	logFont.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
	logFont.lfQuality        = DRAFT_QUALITY;
	logFont.lfPitchAndFamily = FF_DONTCARE | FIXED_PITCH;
	_tcscpy_s(logFont.lfFaceName, 32, (LPCTSTR)(CString)CRegString(_T("Software\\TortoiseSVN\\LogFontName"), _T("Courier New")));
	VERIFY(fontToCreate.CreateFontIndirect(&logFont));
}

bool CAppUtils::LaunchApplication(const CString& sCommandLine, UINT idErrMessageFormat, bool bWaitForStartup)
{
	STARTUPINFO startup;
	PROCESS_INFORMATION process;
	memset(&startup, 0, sizeof(startup));
	startup.cb = sizeof(startup);
	memset(&process, 0, sizeof(process));

	CString cleanCommandLine(sCommandLine);

	if (CreateProcess(NULL, const_cast<TCHAR*>((LPCTSTR)cleanCommandLine), NULL, NULL, FALSE, 0, 0, sOrigCWD, &startup, &process)==0)
	{
		if(idErrMessageFormat != 0)
		{
			LPVOID lpMsgBuf;
			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM | 
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL 
				);
			CString temp;
			temp.Format(idErrMessageFormat, lpMsgBuf);
			CMessageBox::Show(NULL, temp, _T("TortoiseSVN"), MB_OK | MB_ICONINFORMATION);
			LocalFree( lpMsgBuf );
		}
		return false;
	}

	if (bWaitForStartup)
	{
		WaitForInputIdle(process.hProcess, 10000);
	}

	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);
	return true;
}

/**
* Launch the external blame viewer
*/
bool CAppUtils::LaunchTortoiseBlame(const CString& sBlameFile, const CString& sLogFile, const CString& sOriginalFile, const CString& sParams, const SVNRev& startrev, const SVNRev& endrev)
{
	CString viewer = CPathUtils::GetAppDirectory();
	viewer += _T("TortoiseBlame.exe");
	viewer += _T(" \"") + sBlameFile + _T("\"");
	viewer += _T(" \"") + sLogFile + _T("\"");
	viewer += _T(" \"") + sOriginalFile + _T("\"");
	viewer += _T(" ")+sParams;
	if (startrev.IsValid() && endrev.IsValid())
		viewer += _T(" /revrange:\"") + startrev.ToString() + _T("-") + endrev.ToString() + _T("\"");
	
	return LaunchApplication(viewer, IDS_ERR_EXTDIFFSTART, false);
}

void CAppUtils::ResizeAllListCtrlCols(CListCtrl * pListCtrl)
{
	int maxcol = ((CHeaderCtrl*)(pListCtrl->GetDlgItem(0)))->GetItemCount()-1;
	int nItemCount = pListCtrl->GetItemCount();
	TCHAR textbuf[MAX_PATH];
	CHeaderCtrl * pHdrCtrl = (CHeaderCtrl*)(pListCtrl->GetDlgItem(0));
	if (pHdrCtrl)
	{
		for (int col = 0; col <= maxcol; col++)
		{
			HDITEM hdi = {0};
			hdi.mask = HDI_TEXT;
			hdi.pszText = textbuf;
			hdi.cchTextMax = sizeof(textbuf);
			pHdrCtrl->GetItem(col, &hdi);
			int cx = pListCtrl->GetStringWidth(hdi.pszText)+20; // 20 pixels for col separator and margin
			for (int index = 0; index<nItemCount; ++index)
			{
				// get the width of the string and add 14 pixels for the column separator and margins
				int linewidth = pListCtrl->GetStringWidth(pListCtrl->GetItemText(index, col)) + 14;
				if (index == 0)
				{
					// add the image size
					CImageList * pImgList = pListCtrl->GetImageList(LVSIL_SMALL);
					if ((pImgList)&&(pImgList->GetImageCount()))
					{
						IMAGEINFO imginfo;
						pImgList->GetImageInfo(0, &imginfo);
						linewidth += (imginfo.rcImage.right - imginfo.rcImage.left);
						linewidth += 3;	// 3 pixels between icon and text
					}
				}
				if (cx < linewidth)
					cx = linewidth;
			}
			pListCtrl->SetColumnWidth(col, cx);

		}
	}
}

bool CAppUtils::FormatTextInRichEditControl(CWnd * pWnd)
{
	CString sText;
	if (pWnd == NULL)
		return false;
	bool bStyled = false;
	pWnd->GetWindowText(sText);
	// the rich edit control doesn't count the CR char!
	// to be exact: CRLF is treated as one char.
	sText.Replace(_T("\r"), _T(""));

	// style each line separately
	int offset = 0;
	int nNewlinePos;
	do 
	{
		nNewlinePos = sText.Find('\n', offset);
		CString sLine = sText.Mid(offset);
		if (nNewlinePos>=0)
			sLine = sLine.Left(nNewlinePos-offset);
		int start = 0;
		int end = 0;
		while (FindStyleChars(sLine, '*', start, end))
		{
			CHARRANGE range = {(LONG)start+offset, (LONG)end+offset};
			pWnd->SendMessage(EM_EXSETSEL, NULL, (LPARAM)&range);
			CHARFORMAT2 format;
			SecureZeroMemory(&format, sizeof(CHARFORMAT2));
			format.cbSize = sizeof(CHARFORMAT2);
			format.dwMask = CFM_BOLD;
			format.dwEffects = CFE_BOLD;
			pWnd->SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&format);
			bStyled = true;
			start = end;
		}
		start = 0;
		end = 0;
		while (FindStyleChars(sLine, '^', start, end))
		{
			CHARRANGE range = {(LONG)start+offset, (LONG)end+offset};
			pWnd->SendMessage(EM_EXSETSEL, NULL, (LPARAM)&range);
			CHARFORMAT2 format;
			SecureZeroMemory(&format, sizeof(CHARFORMAT2));
			format.cbSize = sizeof(CHARFORMAT2);
			format.dwMask = CFM_ITALIC;
			format.dwEffects = CFE_ITALIC;
			pWnd->SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&format);
			bStyled = true;
			start = end;
		}
		start = 0;
		end = 0;
		while (FindStyleChars(sLine, '_', start, end))
		{
			CHARRANGE range = {(LONG)start+offset, (LONG)end+offset};
			pWnd->SendMessage(EM_EXSETSEL, NULL, (LPARAM)&range);
			CHARFORMAT2 format;
			SecureZeroMemory(&format, sizeof(CHARFORMAT2));
			format.cbSize = sizeof(CHARFORMAT2);
			format.dwMask = CFM_UNDERLINE;
			format.dwEffects = CFE_UNDERLINE;
			pWnd->SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&format);
			bStyled = true;
			start = end;
		}
		offset = nNewlinePos+1;
	} while(nNewlinePos>=0);
	return bStyled;	
}

bool CAppUtils::FindStyleChars(const CString& sText, TCHAR stylechar, int& start, int& end)
{
	int i=start;
	bool bFoundMarker = false;
	// find a starting marker
	while (sText[i] != 0)
	{
		if (sText[i] == stylechar)
		{
			if (((i+1)<sText.GetLength())&&(IsCharAlphaNumeric(sText[i+1])) &&
				(((i>0)&&(!IsCharAlphaNumeric(sText[i-1])))||(i==0)))
			{
				start = i+1;
				i++;
				bFoundMarker = true;
				break;
			}
		}
		i++;
	}
	if (!bFoundMarker)
		return false;
	// find ending marker
	bFoundMarker = false;
	while (sText[i] != 0)
	{
		if (sText[i] == stylechar)
		{
			if ((IsCharAlphaNumeric(sText[i-1])) &&
				((((i+1)<sText.GetLength())&&(!IsCharAlphaNumeric(sText[i+1])))||(i+1)==sText.GetLength()))
			{
				end = i;
				i++;
				bFoundMarker = true;
				break;
			}
		}
		i++;
	}
	return bFoundMarker;
}

bool CAppUtils::BrowseRepository(CHistoryCombo& combo, CWnd * pParent, SVNRev& rev)
{
	CString strUrl;
	combo.GetWindowText(strUrl);
	strUrl.Replace('\\', '/');
	strUrl.Replace(_T("%"), _T("%25"));
	strUrl = CUnicodeUtils::GetUnicode(CPathUtils::PathEscape(CUnicodeUtils::GetUTF8(strUrl)));
	if (strUrl.Left(7) == _T("file://"))
	{
		CString strFile(strUrl);
		SVN::UrlToPath(strFile);

		SVN svn;
		if (svn.IsRepository(CTSVNPath(strFile)))
		{
			// browse repository - show repository browser
			SVN::preparePath(strUrl);
			CRepositoryBrowser browser(strUrl, rev, pParent);
			if (browser.DoModal() == IDOK)
			{
				combo.SetCurSel(-1);
				combo.SetWindowText(browser.GetPath());
				rev = browser.GetRevision();
				return true;
			}
		}
		else
		{
			// browse local directories
			CBrowseFolder folderBrowser;
			folderBrowser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
			// remove the 'file:///' so the shell can recognize the local path
			SVN::UrlToPath(strUrl);
			if (folderBrowser.Show(pParent->GetSafeHwnd(), strUrl) == CBrowseFolder::OK)
			{
				SVN::PathToUrl(strUrl);

				combo.SetCurSel(-1);
				combo.SetWindowText(strUrl);
				return true;
			}
		}
	}
	else if ((strUrl.Left(7) == _T("http://")
		||(strUrl.Left(8) == _T("https://"))
		||(strUrl.Left(6) == _T("svn://"))
		||(strUrl.Left(4) == _T("svn+"))) && strUrl.GetLength() > 6)
	{
		// browse repository - show repository browser
		CRepositoryBrowser browser(strUrl, rev, pParent);
		if (browser.DoModal() == IDOK)
		{
			combo.SetCurSel(-1);
			combo.SetWindowText(browser.GetPath());
			rev = browser.GetRevision();
			return true;
		}
	}
	return false;
}

bool CAppUtils::FileOpenSave(CString& path, int * filterindex, UINT title, UINT filter, bool bOpen, HWND hwndOwner)
{
	OPENFILENAME ofn = {0};				// common dialog box structure
	TCHAR szFile[MAX_PATH] = {0};		// buffer for file name. Explorer can't handle paths longer than MAX_PATH.
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwndOwner;
	_tcscpy_s(szFile, MAX_PATH, (LPCTSTR)path);
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
	CString sFilter;
	TCHAR * pszFilters = NULL;
	if (filter)
	{
		sFilter.LoadString(filter);
		pszFilters = new TCHAR[sFilter.GetLength()+4];
		_tcscpy_s (pszFilters, sFilter.GetLength()+4, sFilter);
		// Replace '|' delimiters with '\0's
		TCHAR *ptr = pszFilters + _tcslen(pszFilters);  //set ptr at the NULL
		while (ptr != pszFilters)
		{
			if (*ptr == '|')
				*ptr = '\0';
			ptr--;
		}
		ofn.lpstrFilter = pszFilters;
	}
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	CString temp;
	if (title)
	{
		temp.LoadString(title);
		CStringUtils::RemoveAccelerators(temp);
	}
	ofn.lpstrTitle = temp;
	if (bOpen)
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER;
	else
		ofn.Flags = OFN_OVERWRITEPROMPT | OFN_EXPLORER;


	// Display the Open dialog box. 
	bool bRet = false;
	if (bOpen)
	{
		bRet = !!GetOpenFileName(&ofn);
	}
	else
	{
		bRet = !!GetSaveFileName(&ofn);
	}
	if (bRet)
	{
		if (pszFilters)
			delete [] pszFilters;
		path = CString(ofn.lpstrFile);
		if (filterindex)
			*filterindex = ofn.nFilterIndex;
		return true;
	}
	if (pszFilters)
		delete [] pszFilters;
	return false;
}

bool CAppUtils::SetListCtrlBackgroundImage(HWND hListCtrl, UINT nID, int width /* = 128 */, int height /* = 128 */)
{
	ListView_SetTextBkColor(hListCtrl, CLR_NONE);
	COLORREF bkColor = ListView_GetBkColor(hListCtrl);
	// create a bitmap from the icon
	HICON hIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(nID), IMAGE_ICON, width, height, LR_DEFAULTCOLOR);
	if (!hIcon)
		return false;

	RECT rect = {0};
	rect.right = width;
	rect.bottom = height;
	HBITMAP bmp = NULL;

	HWND desktop = ::GetDesktopWindow();
	if (desktop)
	{
		HDC screen_dev = ::GetDC(desktop);
		if (screen_dev)
		{
			// Create a compatible DC
			HDC dst_hdc = ::CreateCompatibleDC(screen_dev);
			if (dst_hdc)
			{
				// Create a new bitmap of icon size
				bmp = ::CreateCompatibleBitmap(screen_dev, rect.right, rect.bottom);
				if (bmp)
				{
					// Select it into the compatible DC
					HBITMAP old_dst_bmp = (HBITMAP)::SelectObject(dst_hdc, bmp);
					// Fill the background of the compatible DC with the given color
					::SetBkColor(dst_hdc, bkColor);
					::ExtTextOut(dst_hdc, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);

					// Draw the icon into the compatible DC
					::DrawIconEx(dst_hdc, 0, 0, hIcon, rect.right, rect.bottom, 0, NULL, DI_NORMAL);
					::SelectObject(dst_hdc, old_dst_bmp);
				}
				::DeleteDC(dst_hdc);
			}
		}
		::ReleaseDC(desktop, screen_dev); 
	}

	// Restore settings
	DestroyIcon(hIcon);

	if (bmp == NULL)
		return false;

	LVBKIMAGE lv;
	lv.ulFlags = LVBKIF_TYPE_WATERMARK;
	lv.hbm = bmp;
	lv.xOffsetPercent = 100;
	lv.yOffsetPercent = 100;
	ListView_SetBkImage(hListCtrl, &lv);
	return true;
}

CString CAppUtils::GetProjectNameFromURL(CString url)
{
	CString name;
	while ((name.IsEmpty() || (name.CompareNoCase(_T("branches"))==0) ||
		(name.CompareNoCase(_T("tags"))==0) ||
		(name.CompareNoCase(_T("trunk"))==0))&&(!url.IsEmpty()))
	{
		name = url.Mid(url.ReverseFind('/')+1);
		url = url.Left(url.ReverseFind('/'));
	}
	if ((name.Compare(_T("svn")) == 0)||(name.Compare(_T("svnroot")) == 0))
	{
		// a name of svn or svnroot indicates that it's not really the project name. In that
		// case, we try the first part of the URL
		// of course, this won't work in all cases (but it works for Google project hosting)
		url.Replace(_T("http://"), _T(""));
		url.Replace(_T("https://"), _T(""));
		url.Replace(_T("svn://"), _T(""));
		url.Replace(_T("svn+ssh://"), _T(""));
		url.TrimLeft(_T("/"));
		name = url.Left(url.Find('.'));
	}
	return name;
}

bool CAppUtils::StartShowUnifiedDiff(HWND hWnd, const CTSVNPath& url1, const SVNRev& rev1, 
									 const CTSVNPath& url2, const SVNRev& rev2, 
									 const SVNRev& peg /* = SVNRev */, const SVNRev& headpeg /* = SVNRev */,  
									 bool bAlternateDiff /* = false */, bool bIgnoreAncestry /* = false */, bool /* blame = false */)
{
	CString sCmd;
	sCmd.Format(_T("%s /command:showcompare /unified"),
		(LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")));
	sCmd += _T(" /url1:\"") + url1.GetSVNPathString() + _T("\"");
	if (rev1.IsValid())
		sCmd += _T(" /revision1:") + rev1.ToString();
	sCmd += _T(" /url2:\"") + url2.GetSVNPathString() + _T("\"");
	if (rev2.IsValid())
		sCmd += _T(" /revision2:") + rev2.ToString();
	if (peg.IsValid())
		sCmd += _T(" /pegrevision:") + peg.ToString();
	if (headpeg.IsValid())
		sCmd += _T(" /headpegrevision:") + headpeg.ToString();

	if (bAlternateDiff)
		sCmd += _T(" /alternatediff");

	if (bIgnoreAncestry)
		sCmd += _T(" /ignoreancestry");

	if (hWnd)
	{
		sCmd += _T(" /hwnd:");
		TCHAR buf[30];
		_stprintf_s(buf, 30, _T("%ld"), (DWORD)hWnd);
		sCmd += buf;
	}

	return CAppUtils::LaunchApplication(sCmd, NULL, false);
}

bool CAppUtils::StartShowCompare(HWND hWnd, const CTSVNPath& url1, const SVNRev& rev1, 
								 const CTSVNPath& url2, const SVNRev& rev2, 
								 const SVNRev& peg /* = SVNRev */, const SVNRev& headpeg /* = SVNRev */, 
								 bool bAlternateDiff /* = false */, bool bIgnoreAncestry /* = false */, bool blame /* = false */)
{
	CString sCmd;
	sCmd.Format(_T("%s /command:showcompare"),
		(LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")));
	sCmd += _T(" /url1:\"") + url1.GetSVNPathString() + _T("\"");
	if (rev1.IsValid())
		sCmd += _T(" /revision1:") + rev1.ToString();
	sCmd += _T(" /url2:\"") + url2.GetSVNPathString() + _T("\"");
	if (rev2.IsValid())
		sCmd += _T(" /revision2:") + rev2.ToString();
	if (peg.IsValid())
		sCmd += _T(" /pegrevision:") + peg.ToString();
	if (headpeg.IsValid())
		sCmd += _T(" /headpegrevision:") + headpeg.ToString();
	if (bAlternateDiff)
		sCmd += _T(" /alternatediff");
	if (bIgnoreAncestry)
		sCmd += _T(" /ignoreancestry");
	if (blame)
		sCmd += _T(" /blame");

	if (hWnd)
	{
		sCmd += _T(" /hwnd:");
		TCHAR buf[30];
		_stprintf_s(buf, 30, _T("%ld"), (DWORD)hWnd);
		sCmd += buf;
	}

	return CAppUtils::LaunchApplication(sCmd, NULL, false);
}
