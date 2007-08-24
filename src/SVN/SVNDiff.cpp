// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - TortoiseSVN

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

#include "StdAfx.h"
#include "resource.h"
#include "..\TortoiseShell\resource.h"
#include "AppUtils.h"
#include "TempFile.h"
#include "SVNStatus.h"
#include "SVNInfo.h"
#include "SVNProperties.h"
#include "UnicodeUtils.h"
#include "registry.h"
#include "MessageBox.h"
#include "FileDiffDlg.h"
#include "ProgressDlg.h"
#include ".\svndiff.h"
#include "Blame.h"
#include "svn_types.h"

SVNDiff::SVNDiff(SVN * pSVN /* = NULL */, HWND hWnd /* = NULL */, bool bRemoveTempFiles /* = false */) :
	m_bDeleteSVN(false),
	m_pSVN(NULL),
	m_hWnd(NULL),
	m_bRemoveTempFiles(false),
	m_headPeg(SVNRev::REV_HEAD)
{
	if (pSVN)
		m_pSVN = pSVN;
	else
	{
		m_pSVN = new SVN;
		m_bDeleteSVN = true;
	}
	m_hWnd = hWnd;
	m_bRemoveTempFiles = bRemoveTempFiles;
}

SVNDiff::~SVNDiff(void)
{
	if (m_bDeleteSVN)
		delete m_pSVN;
}

bool SVNDiff::DiffWCFile(const CTSVNPath& filePath, 
						svn_wc_status_kind text_status /* = svn_wc_status_none */, 
						svn_wc_status_kind prop_status /* = svn_wc_status_none */, 
						svn_wc_status_kind remotetext_status /* = svn_wc_status_none */, 
						svn_wc_status_kind remoteprop_status /* = svn_wc_status_none */)
{
	CTSVNPath basePath;
	CTSVNPath remotePath;
	
	// first diff the remote properties against the wc props
	// TODO: should we attempt to do a three way diff with the properties too
	// if they're modified locally and remotely?
	if (remoteprop_status > svn_wc_status_normal)
	{
		DiffProps(filePath, SVNRev::REV_HEAD, SVNRev::REV_WC);
	}
	if (prop_status > svn_wc_status_normal)
	{
		DiffProps(filePath, SVNRev::REV_WC, SVNRev::REV_BASE);
	}
	if (filePath.IsDirectory())
		return true;

	if (text_status > svn_wc_status_normal)
		basePath = SVN::GetPristinePath(filePath);

	if (remotetext_status > svn_wc_status_normal)
	{
		remotePath = CTempFiles::Instance().GetTempFilePath(true, filePath, SVNRev::REV_HEAD);

		CProgressDlg progDlg;
		progDlg.SetTitle(IDS_APPNAME);
		progDlg.SetAnimation(IDR_DOWNLOAD);
		progDlg.SetTime(false);
		m_pSVN->SetAndClearProgressInfo(&progDlg, true);	// activate progress bar
		progDlg.ShowModeless(m_hWnd);
		progDlg.FormatPathLine(1, IDS_PROGRESSGETFILE, (LPCTSTR)filePath.GetUIPathString());
		if (!m_pSVN->Cat(filePath, SVNRev(SVNRev::REV_HEAD), SVNRev::REV_HEAD, remotePath))
		{
			progDlg.Stop();
			m_pSVN->SetAndClearProgressInfo((HWND)NULL);
			CMessageBox::Show(m_hWnd, m_pSVN->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			return false;
		}
		progDlg.Stop();
		m_pSVN->SetAndClearProgressInfo((HWND)NULL);
		SetFileAttributes(remotePath.GetWinPath(), FILE_ATTRIBUTE_READONLY);
	}

	CString name = filePath.GetUIFileOrDirectoryName();
	CString n1, n2, n3;
	n1.Format(IDS_DIFF_WCNAME, name);
	n2.Format(IDS_DIFF_BASENAME, name);
	n3.Format(IDS_DIFF_REMOTENAME, name);

	if ((text_status <= svn_wc_status_normal)&&(prop_status <= svn_wc_status_normal))
	{
		// Hasn't changed locally - diff remote against WC
		return !!CAppUtils::StartExtDiff(filePath, remotePath, n1, n3);
	}
	else if (remotePath.IsEmpty())
	{
		return DiffFileAgainstBase(filePath, text_status, prop_status);
	}
	else
	{
		// Three-way diff
		return !!CAppUtils::StartExtMerge(basePath, remotePath, filePath, CTSVNPath(), n2, n3, n1, CString(), true);
	}
}

bool SVNDiff::StartConflictEditor(const CTSVNPath& conflictedFilePath)
{
	CTSVNPath merge = conflictedFilePath;
	CTSVNPath directory = merge.GetDirectory();
	CTSVNPath theirs(directory);
	CTSVNPath mine(directory);
	CTSVNPath base(directory);
	bool bConflictData = false;

	// we have the conflicted file (%merged)
	// now look for the other required files
	SVNStatus stat;
	stat.GetStatus(merge);
	if ((stat.status == NULL)||(stat.status->entry == NULL))
		return false;

	if (stat.status->entry->conflict_new)
	{
		theirs.AppendPathString(CUnicodeUtils::GetUnicode(stat.status->entry->conflict_new));
		bConflictData = true;
	}
	if (stat.status->entry->conflict_old)
	{
		base.AppendPathString(CUnicodeUtils::GetUnicode(stat.status->entry->conflict_old));
		bConflictData = true;
	}
	if (stat.status->entry->conflict_wrk)
	{
		mine.AppendPathString(CUnicodeUtils::GetUnicode(stat.status->entry->conflict_wrk));
		bConflictData = true;
	}
	else
	{
		mine = merge;
	}
	if (bConflictData)
		return !!CAppUtils::StartExtMerge(base,theirs,mine,merge);
	return false;
}

bool SVNDiff::DiffFileAgainstBase(const CTSVNPath& filePath, svn_wc_status_kind text_status /* = svn_wc_status_none */, svn_wc_status_kind prop_status /* = svn_wc_status_none */)
{
	bool retvalue = false;
	if ((text_status == svn_wc_status_none)||(prop_status == svn_wc_status_none))
	{
		SVNStatus stat;
		stat.GetStatus(filePath);
		if (stat.status == NULL)
			return false;
		text_status = stat.status->text_status;
		prop_status = stat.status->prop_status;
	}
	if (prop_status > svn_wc_status_normal)
	{
		DiffProps(filePath, SVNRev::REV_WC, SVNRev::REV_BASE);
	}

	if (filePath.IsDirectory())
		return true;
	if (text_status >= svn_wc_status_normal)
	{
		CTSVNPath basePath(SVN::GetPristinePath(filePath));
		// If necessary, convert the line-endings on the file before diffing
		if ((DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\ConvertBase"), TRUE))
		{
			CTSVNPath temporaryFile = CTempFiles::Instance().GetTempFilePath(m_bRemoveTempFiles, filePath, SVNRev::REV_BASE);
			if (!m_pSVN->Cat(filePath, SVNRev(SVNRev::REV_BASE), SVNRev(SVNRev::REV_BASE), temporaryFile))
			{
				temporaryFile.Reset();
			}
			else
			{
				basePath = temporaryFile;
				SetFileAttributes(basePath.GetWinPath(), FILE_ATTRIBUTE_READONLY);
			}
		}
		// for added/deleted files, we don't have a BASE file.
		// create an empty temp file to be used.
		if (!basePath.Exists())
		{
			basePath = CTempFiles::Instance().GetTempFilePath(m_bRemoveTempFiles, filePath, SVNRev::REV_BASE);
			SetFileAttributes(basePath.GetWinPath(), FILE_ATTRIBUTE_READONLY);
		}
		CString name = filePath.GetFilename();
		CTSVNPath wcFilePath = filePath;
		if (!wcFilePath.Exists())
		{
			wcFilePath = CTempFiles::Instance().GetTempFilePath(m_bRemoveTempFiles, filePath, SVNRev::REV_BASE);
			SetFileAttributes(wcFilePath.GetWinPath(), FILE_ATTRIBUTE_READONLY);
		}
		CString n1, n2;
		n1.Format(IDS_DIFF_WCNAME, (LPCTSTR)name);
		n2.Format(IDS_DIFF_BASENAME, (LPCTSTR)name);
		retvalue = !!CAppUtils::StartExtDiff(basePath, wcFilePath, n2, n1, TRUE);
	}
	return retvalue;
}

bool SVNDiff::UnifiedDiff(CTSVNPath& tempfile, const CTSVNPath& url1, const SVNRev& rev1, const CTSVNPath& url2, const SVNRev& rev2, const SVNRev& peg /* = SVNRev() */, bool bIgnoreAncestry /* = false */)
{
	tempfile = CTempFiles::Instance().GetTempFilePath(m_bRemoveTempFiles, CTSVNPath(_T("Test.diff")));
	bool bIsUrl = !!SVN::PathIsURL(url1.GetSVNPathString());
	
	CProgressDlg progDlg;
	progDlg.SetTitle(IDS_APPNAME);
	progDlg.SetAnimation(IDR_DOWNLOAD);
	progDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROGRESS_UNIFIEDDIFF)));
	progDlg.SetTime(false);
	m_pSVN->SetAndClearProgressInfo(&progDlg);
	progDlg.ShowModeless(m_hWnd);
	if ((!url1.IsEquivalentTo(url2))||((rev1.IsWorking() || rev1.IsBase())&&(rev2.IsWorking() || rev2.IsBase())))
	{
		if (!m_pSVN->Diff(url1, rev1, url2, rev2, svn_depth_infinity, FALSE, FALSE, FALSE, _T(""), bIgnoreAncestry, tempfile))
		{
			progDlg.Stop();
			m_pSVN->SetAndClearProgressInfo((HWND)NULL);
			CMessageBox::Show(this->m_hWnd, m_pSVN->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			return false;
		}
	}
	else
	{
		if (!m_pSVN->PegDiff(url1, (peg.IsValid() ? peg : (bIsUrl ? m_headPeg : SVNRev::REV_WC)), rev1, rev2, svn_depth_infinity, FALSE, FALSE, FALSE, _T(""), tempfile))
		{
			if (!m_pSVN->Diff(url1, rev1, url2, rev2, svn_depth_infinity, FALSE, FALSE, FALSE, _T(""), false, tempfile))
			{
				progDlg.Stop();
				m_pSVN->SetAndClearProgressInfo((HWND)NULL);
				CMessageBox::Show(this->m_hWnd, m_pSVN->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
				return false;
			}
		}
	}
	if (CAppUtils::CheckForEmptyDiff(tempfile))
	{
		progDlg.Stop();
		m_pSVN->SetAndClearProgressInfo((HWND)NULL);
		CMessageBox::Show(m_hWnd, IDS_ERR_EMPTYDIFF, IDS_APPNAME, MB_ICONERROR);
		return false;
	}
	progDlg.Stop();
	m_pSVN->SetAndClearProgressInfo((HWND)NULL);
	return true;
}

bool SVNDiff::ShowUnifiedDiff(const CTSVNPath& url1, const SVNRev& rev1, const CTSVNPath& url2, const SVNRev& rev2, const SVNRev& peg /* = SVNRev() */, bool bIgnoreAncestry /* = false */)
{
	CTSVNPath tempfile;
	if (UnifiedDiff(tempfile, url1, rev1, url2, rev2, peg, bIgnoreAncestry))
	{
		CString title;
		CTSVNPathList list;
		list.AddPath(url1);
		list.AddPath(url2);
		if (url1.IsEquivalentTo(url2))
			title.Format(IDS_SVNDIFF_ONEURL, (LPCTSTR)rev1.ToString(), (LPCTSTR)rev2.ToString(), (LPCTSTR)url1.GetUIFileOrDirectoryName());
		else
		{
			CTSVNPath root = list.GetCommonRoot();
			CString u1 = url1.GetUIPathString().Mid(root.GetUIPathString().GetLength());
			CString u2 = url2.GetUIPathString().Mid(root.GetUIPathString().GetLength());
			title.Format(IDS_SVNDIFF_TWOURLS, (LPCTSTR)rev1.ToString(), (LPCTSTR)u1, (LPCTSTR)rev2.ToString(), (LPCTSTR)u2);
		}
		return !!CAppUtils::StartUnifiedDiffViewer(tempfile, title);
	}
	return false;
}

bool SVNDiff::ShowCompare(const CTSVNPath& url1, const SVNRev& rev1, 
						  const CTSVNPath& url2, const SVNRev& rev2, 
						  SVNRev peg /* = SVNRev() */,
						  bool ignoreancestry /* = false */,
						  bool blame /* = false */)
{
	CTSVNPath tempfile;
	
	CProgressDlg progDlg;
	progDlg.SetTitle(IDS_APPNAME);
	progDlg.SetAnimation(IDR_DOWNLOAD);
	progDlg.SetTime(false);
	m_pSVN->SetAndClearProgressInfo(&progDlg);

	if ((m_pSVN->PathIsURL(url1.GetSVNPathString()))||(!rev1.IsWorking())||(!url1.IsEquivalentTo(url2)))
	{
		// no working copy path!
		progDlg.ShowModeless(m_hWnd);

		tempfile = CTempFiles::Instance().GetTempFilePath(true, url1);		
		// first find out if the url points to a file or dir
		progDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROGRESS_INFO)));
		CString sRepoRoot;
		SVNInfo info;
		const SVNInfoData * data = info.GetFirstFileInfo(url1, (peg.IsValid() ? peg : m_headPeg), rev1, false);
		if (data == NULL)
		{
			data = info.GetFirstFileInfo(url1, (peg.IsValid() ? peg : rev1), rev1, false);
			if (data == NULL)
			{
				data = info.GetFirstFileInfo(url1, (peg.IsValid() ? peg : rev2), rev1, false);
				if (data == NULL)
				{
					progDlg.Stop();
					m_pSVN->SetAndClearProgressInfo((HWND)NULL);
					CMessageBox::Show(m_hWnd, info.GetLastErrorMsg(), _T("TortoiseSVN"), MB_ICONERROR);
					return false;
				}
				else
				{
					sRepoRoot = data->reposRoot;
					peg = peg.IsValid() ? peg : rev2;
				}
			}
			else
			{
				sRepoRoot = data->reposRoot;
				peg = peg.IsValid() ? peg : rev1;
			}
		}
		else
		{
			sRepoRoot = data->reposRoot;
			peg = peg.IsValid() ? peg : m_headPeg;
		}
		if (data->kind == svn_node_dir)
		{
			if (rev1.IsWorking())
			{
				if (UnifiedDiff(tempfile, url1, rev1, url2, rev2, (peg.IsValid() ? peg : SVNRev::REV_WC)))
				{
					CString sWC;
					sWC.LoadString(IDS_DIFF_WORKINGCOPY);
					m_pSVN->SetAndClearProgressInfo((HWND)NULL);
					return !!CAppUtils::StartExtPatch(tempfile, url1.GetDirectory(), sWC, url2.GetSVNPathString(), TRUE);
				}
			}
			else
			{
				progDlg.Stop();
				CFileDiffDlg fdlg;
				fdlg.DoBlame(blame);
				if (url1.IsEquivalentTo(url2))
				{
					fdlg.SetDiff(url1, (peg.IsValid() ? peg : m_headPeg), rev1, rev2, svn_depth_infinity, ignoreancestry);
					fdlg.DoModal();
				}
				else
				{
					fdlg.SetDiff(url1, rev1, url2, rev2, svn_depth_infinity, ignoreancestry);
					fdlg.DoModal();
				}
			}
		}
		else
		{
			// diffing two revs of a file, so cat two files
			CTSVNPath tempfile1 = CTempFiles::Instance().GetTempFilePath(m_bRemoveTempFiles, url1, rev1);
			CTSVNPath tempfile2 = CTempFiles::Instance().GetTempFilePath(m_bRemoveTempFiles, url2, rev2);

			m_pSVN->SetAndClearProgressInfo(&progDlg, true);	// activate progress bar
			progDlg.ShowModeless(m_hWnd);
			progDlg.FormatPathLine(1, IDS_PROGRESSGETFILEREVISION, (LPCTSTR)url1.GetUIPathString(), rev1.ToString());

			CBlame blamer;
			if (blame)
			{
				if (!blamer.BlameToFile(url1, 1, rev1, peg.IsValid() ? peg : rev1, tempfile1, _T("")))
				{
					if ((peg.IsValid())&&(blamer.Err->apr_err != SVN_ERR_CLIENT_IS_BINARY_FILE))
					{
						if (!blamer.BlameToFile(url1, 1, rev1, rev1, tempfile1, _T("")))
						{
							progDlg.Stop();
							m_pSVN->SetAndClearProgressInfo((HWND)NULL);
							CMessageBox::Show(NULL, blamer.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							return false;
						}
					}
					else
					{
						if (blamer.Err->apr_err != SVN_ERR_CLIENT_IS_BINARY_FILE)
						{
							progDlg.Stop();
							m_pSVN->SetAndClearProgressInfo((HWND)NULL);
						}
						CMessageBox::Show(NULL, blamer.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						if (blamer.Err->apr_err == SVN_ERR_CLIENT_IS_BINARY_FILE)
							blame = false;
						else
							return false;
					}
				}
			}
			if (!blame)
			{
				if (!m_pSVN->Cat(url1, peg.IsValid() ? peg : rev1, rev1, tempfile1))
				{
					if (peg.IsValid())
					{
						if (!m_pSVN->Cat(url1, rev1, rev1, tempfile1))
						{
							progDlg.Stop();
							m_pSVN->SetAndClearProgressInfo((HWND)NULL);
							CMessageBox::Show(NULL, m_pSVN->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							return false;
						}
					}
					else
					{
						progDlg.Stop();
						m_pSVN->SetAndClearProgressInfo((HWND)NULL);
						CMessageBox::Show(NULL, m_pSVN->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						return false;
					}
				}
			}
			SetFileAttributes(tempfile1.GetWinPath(), FILE_ATTRIBUTE_READONLY);
			
			progDlg.FormatPathLine(1, IDS_PROGRESSGETFILEREVISION, (LPCTSTR)url2.GetUIPathString(), rev2.ToString());
			if (blame)
			{
				if (!blamer.BlameToFile(url2, 1, rev2, peg.IsValid() ? peg : rev2, tempfile2, _T("")))
				{
					if (peg.IsValid())
					{
						if (!blamer.BlameToFile(url2, 1, rev2, rev2, tempfile2, _T("")))
						{
							progDlg.Stop();
							m_pSVN->SetAndClearProgressInfo((HWND)NULL);
							CMessageBox::Show(NULL, m_pSVN->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							return false;
						}
					}
					else
					{
						progDlg.Stop();
						m_pSVN->SetAndClearProgressInfo((HWND)NULL);
						CMessageBox::Show(NULL, m_pSVN->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						return false;
					}
				}
			}
			else
			{
				if (!m_pSVN->Cat(url2, peg.IsValid() ? peg : rev2, rev2, tempfile2))
				{
					if (peg.IsValid())
					{
						if (!m_pSVN->Cat(url2, rev2, rev2, tempfile2))
						{
							progDlg.Stop();
							m_pSVN->SetAndClearProgressInfo((HWND)NULL);
							CMessageBox::Show(NULL, m_pSVN->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							return false;
						}
					}
					else
					{
						progDlg.Stop();
						m_pSVN->SetAndClearProgressInfo((HWND)NULL);
						CMessageBox::Show(NULL, m_pSVN->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						return false;
					}
				}
			}
			SetFileAttributes(tempfile2.GetWinPath(), FILE_ATTRIBUTE_READONLY);

			progDlg.Stop();
			m_pSVN->SetAndClearProgressInfo((HWND)NULL);

			CString revname1, revname2;
			if (url1.IsEquivalentTo(url2))
			{
				revname1.Format(_T("%s Revision %s"), (LPCTSTR)url1.GetUIFileOrDirectoryName(), rev1.ToString());
				revname2.Format(_T("%s Revision %s"), (LPCTSTR)url2.GetUIFileOrDirectoryName(), rev2.ToString());
			}
			else
			{
				if (sRepoRoot.IsEmpty())
				{
					revname1.Format(_T("%s Revision %s"), (LPCTSTR)url1.GetSVNPathString(), rev1.ToString());
					revname2.Format(_T("%s Revision %s"), (LPCTSTR)url2.GetSVNPathString(), rev2.ToString());
				}
				else
				{
					if (url1.IsUrl())
						revname1.Format(_T("%s Revision %s"), (LPCTSTR)url1.GetSVNPathString().Mid(sRepoRoot.GetLength()), rev1.ToString());
					else
						revname1.Format(_T("%s Revision %s"), (LPCTSTR)url1.GetSVNPathString(), rev1.ToString());
					if (url2.IsUrl())
						revname2.Format(_T("%s Revision %s"), (LPCTSTR)url2.GetSVNPathString().Mid(sRepoRoot.GetLength()), rev2.ToString());
					else
						revname2.Format(_T("%s Revision %s"), (LPCTSTR)url2.GetSVNPathString(), rev2.ToString());
				}
			}
			m_pSVN->SetAndClearProgressInfo((HWND)NULL);
			return !!CAppUtils::StartExtDiff(tempfile1, tempfile2, revname1, revname2, FALSE, blame, true);
		}
	}
	else
	{
		// compare with working copy
		if (PathIsDirectory(url1.GetWinPath()))
		{
			if (UnifiedDiff(tempfile, url1, rev1, url1, rev2, (peg.IsValid() ? peg : SVNRev::REV_WC)))
			{
				CString sWC, sRev;
				sWC.LoadString(IDS_DIFF_WORKINGCOPY);
				sRev.Format(IDS_DIFF_REVISIONPATCHED, (LONG)rev2);
				m_pSVN->SetAndClearProgressInfo((HWND)NULL);
				return !!CAppUtils::StartExtPatch(tempfile, url1.GetDirectory(), sWC, sRev, TRUE);
			}
		}
		else
		{
			ASSERT(rev1.IsWorking());

			m_pSVN->SetAndClearProgressInfo(&progDlg, true);	// activate progress bar
			progDlg.ShowModeless(m_hWnd);
			progDlg.FormatPathLine(1, IDS_PROGRESSGETFILEREVISION, (LPCTSTR)url1.GetUIPathString(), rev2.ToString());

			tempfile = CTempFiles::Instance().GetTempFilePath(m_bRemoveTempFiles, url1, rev2);
			if (blame)
			{
				CBlame blamer;
				if (!blamer.BlameToFile(url1, 1, rev2, (peg.IsValid() ? peg : SVNRev::REV_WC), tempfile, _T("")))
				{
					if (peg.IsValid())
					{
						if (!blamer.BlameToFile(url1, 1, rev2, SVNRev::REV_WC, tempfile, _T("")))
						{
							progDlg.Stop();
							m_pSVN->SetAndClearProgressInfo((HWND)NULL);
							CMessageBox::Show(NULL, m_pSVN->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							return false;
						}
					}
					else
					{
						progDlg.Stop();
						m_pSVN->SetAndClearProgressInfo((HWND)NULL);
						CMessageBox::Show(NULL, m_pSVN->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						return false;
					}
				} 
				else
				{
					progDlg.Stop();
					m_pSVN->SetAndClearProgressInfo((HWND)NULL);
					SetFileAttributes(tempfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
					CTSVNPath tempfile2 = CTempFiles::Instance().GetTempFilePath(true, url1);
					if (!blamer.BlameToFile(url1, 1, SVNRev::REV_WC, SVNRev::REV_WC, tempfile2, _T("")))
					{
						progDlg.Stop();
						m_pSVN->SetAndClearProgressInfo((HWND)NULL);
						CMessageBox::Show(NULL, m_pSVN->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						return false;
					}
					CString revname, wcname;
					revname.Format(_T("%s Revision %ld"), (LPCTSTR)url1.GetFilename(), (LONG)rev2);
					wcname.Format(IDS_DIFF_WCNAME, (LPCTSTR)url1.GetFilename());
					m_pSVN->SetAndClearProgressInfo((HWND)NULL);
					return !!CAppUtils::StartExtDiff(tempfile, tempfile2, revname, wcname, FALSE, FALSE, TRUE);
				}
			}
			else
			{
				if (!m_pSVN->Cat(url1, (peg.IsValid() ? peg : SVNRev::REV_WC), rev2, tempfile))
				{
					if (peg.IsValid())
					{
						if (!m_pSVN->Cat(url1, SVNRev::REV_WC, rev2, tempfile))
						{
							progDlg.Stop();
							m_pSVN->SetAndClearProgressInfo((HWND)NULL);
							CMessageBox::Show(NULL, m_pSVN->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							return false;
						}
					}
					else
					{
						progDlg.Stop();
						m_pSVN->SetAndClearProgressInfo((HWND)NULL);
						CMessageBox::Show(NULL, m_pSVN->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						return false;
					}
				} 
				else
				{
					progDlg.Stop();
					m_pSVN->SetAndClearProgressInfo((HWND)NULL);
					SetFileAttributes(tempfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
					CString revname, wcname;
					revname.Format(_T("%s Revision %s"), (LPCTSTR)url1.GetFilename(), rev2.ToString());
					wcname.Format(IDS_DIFF_WCNAME, (LPCTSTR)url1.GetFilename());
					m_pSVN->SetAndClearProgressInfo((HWND)NULL);
					return !!CAppUtils::StartExtDiff(tempfile, url1, revname, wcname, FALSE, FALSE, TRUE);
				}
			}
		}
	}
	m_pSVN->SetAndClearProgressInfo((HWND)NULL);
	return false;
}

bool SVNDiff::DiffProps(const CTSVNPath& filePath, SVNRev rev1, SVNRev rev2)
{
	bool retvalue = false;
	// diff the properties
	SVNProperties propswc(filePath, rev1);
	SVNProperties propsbase(filePath, rev2);

	// check for properties that got removed
	for (int baseindex = 0; baseindex < propsbase.GetCount(); ++baseindex)
	{
		stdstring basename = propsbase.GetItemName(baseindex);
		stdstring basevalue = CUnicodeUtils::StdGetUnicode((char *)propsbase.GetItemValue(baseindex).c_str());
		bool bFound = false;
		for (int wcindex = 0; wcindex < propswc.GetCount(); ++wcindex)
		{
			if (basename.compare(propswc.GetItemName(wcindex))==0)
			{
				bFound = true;
				break;
			}
		}
		if (!bFound)
		{
			// write the old property value to temporary file
			CTSVNPath wcpropfile = CTempFiles::Instance().GetTempFilePath(true);
			CTSVNPath basepropfile = CTempFiles::Instance().GetTempFilePath(true);
			FILE * pFile;
			_tfopen_s(&pFile, wcpropfile.GetWinPath(), _T("wb"));
			if (pFile)
			{
				fclose(pFile);
				FILE * pFile;
				_tfopen_s(&pFile, basepropfile.GetWinPath(), _T("wb"));
				if (pFile)
				{
					fputs(CUnicodeUtils::StdGetUTF8(basevalue).c_str(), pFile);
					fclose(pFile);
				}
				else
					return false;
			}
			else
				return false;
			SetFileAttributes(wcpropfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
			SetFileAttributes(basepropfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
			CString n1, n2;
			bool bSwitch = false;
			if (rev1.IsWorking())
				n1.Format(IDS_DIFF_WCNAME, basename.c_str());
			if (rev1.IsBase())
				n1.Format(IDS_DIFF_BASENAME, basename.c_str());
			if (rev1.IsHead())
				n1.Format(IDS_DIFF_REMOTENAME, basename.c_str());
			if (n1.IsEmpty())
			{
				CString temp;
				temp.Format(IDS_DIFF_REVISIONPATCHED, (LONG)rev1);
				n1 = basename.c_str();
				n1 += _T(" ") + temp;
				bSwitch = true;
			}
			if (rev2.IsWorking())
				n2.Format(IDS_DIFF_WCNAME, basename.c_str());
			if (rev2.IsBase())
				n2.Format(IDS_DIFF_BASENAME, basename.c_str());
			if (rev2.IsHead())
				n2.Format(IDS_DIFF_REMOTENAME, basename.c_str());
			if (n2.IsEmpty())
			{
				CString temp;
				temp.Format(IDS_DIFF_REVISIONPATCHED, (LONG)rev2);
				n2 = basename.c_str();
				n2 += _T(" ") + temp;
				bSwitch = true;
			}
			if (bSwitch)
			{
				retvalue = !!CAppUtils::StartExtDiffProps(wcpropfile, basepropfile, n1, n2, TRUE, TRUE);
			}
			else
			{
				retvalue = !!CAppUtils::StartExtDiffProps(basepropfile, wcpropfile, n2, n1, TRUE, TRUE);
			}
		}
	}

	for (int wcindex = 0; wcindex < propswc.GetCount(); ++wcindex)
	{
		stdstring wcname = propswc.GetItemName(wcindex);
		stdstring wcvalue = CUnicodeUtils::StdGetUnicode((char *)propswc.GetItemValue(wcindex).c_str());
		stdstring basevalue;
		bool bDiffRequired = true;
		for (int baseindex = 0; baseindex < propsbase.GetCount(); ++baseindex)
		{
			if (propsbase.GetItemName(baseindex).compare(wcname)==0)
			{
				basevalue = CUnicodeUtils::StdGetUnicode((char *)propsbase.GetItemValue(baseindex).c_str());
				if (basevalue.compare(wcvalue)==0)
				{
					// name and value are identical
					bDiffRequired = false;
					break;
				}
			}
		}
		if (bDiffRequired)
		{
			// write both property values to temporary files
			CTSVNPath wcpropfile = CTempFiles::Instance().GetTempFilePath(true);
			CTSVNPath basepropfile = CTempFiles::Instance().GetTempFilePath(true);
			FILE * pFile;
			_tfopen_s(&pFile, wcpropfile.GetWinPath(), _T("wb"));
			if (pFile)
			{
				fputs(CUnicodeUtils::StdGetUTF8(wcvalue).c_str(), pFile);
				fclose(pFile);
				FILE * pFile;
				_tfopen_s(&pFile, basepropfile.GetWinPath(), _T("wb"));
				if (pFile)
				{
					fputs(CUnicodeUtils::StdGetUTF8(basevalue).c_str(), pFile);
					fclose(pFile);
				}
				else
					return false;
			}
			else
				return false;
			SetFileAttributes(wcpropfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
			SetFileAttributes(basepropfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
			CString n1, n2;
			if (rev1.IsWorking())
				n1.Format(IDS_DIFF_WCNAME, wcname.c_str());
			if (rev1.IsBase())
				n1.Format(IDS_DIFF_BASENAME, wcname.c_str());
			if (rev1.IsHead())
				n1.Format(IDS_DIFF_REMOTENAME, wcname.c_str());
			if (rev2.IsWorking())
				n2.Format(IDS_DIFF_WCNAME, wcname.c_str());
			if (rev2.IsBase())
				n2.Format(IDS_DIFF_BASENAME, wcname.c_str());
			if (rev2.IsHead())
				n2.Format(IDS_DIFF_REMOTENAME, wcname.c_str());
			retvalue = !!CAppUtils::StartExtDiffProps(basepropfile, wcpropfile, n2, n1, TRUE, TRUE);
		}
	}
	return retvalue;
}
