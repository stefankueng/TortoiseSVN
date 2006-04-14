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
#include "stdafx.h"
#include "TortoiseProc.h"
#include "UnicodeUtils.h"
#include "MessageBox.h"
#include "Utils.h"
#include "TempFile.h"
#include "ProgressDlg.h"
#include "SysImageList.h"
#include "SVNProperties.h"
#include ".\filediffdlg.h"

#define ID_COMPARE 1
#define ID_BLAME 2

// CFileDiffDlg dialog

IMPLEMENT_DYNAMIC(CFileDiffDlg, CResizableStandAloneDialog)
CFileDiffDlg::CFileDiffDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CFileDiffDlg::IDD, pParent),
	m_bBlame(false)
{
}

CFileDiffDlg::~CFileDiffDlg()
{
}

void CFileDiffDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FILELIST, m_cFileList);
}


BEGIN_MESSAGE_MAP(CFileDiffDlg, CResizableStandAloneDialog)
	ON_NOTIFY(NM_DBLCLK, IDC_FILELIST, OnNMDblclkFilelist)
	ON_NOTIFY(LVN_GETINFOTIP, IDC_FILELIST, OnLvnGetInfoTipFilelist)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_FILELIST, OnNMCustomdrawFilelist)
	ON_WM_CONTEXTMENU()
	ON_WM_SETCURSOR()
	ON_EN_SETFOCUS(IDC_SECONDURL, &CFileDiffDlg::OnEnSetfocusSecondurl)
	ON_EN_SETFOCUS(IDC_FIRSTURL, &CFileDiffDlg::OnEnSetfocusFirsturl)
END_MESSAGE_MAP()


void CFileDiffDlg::SetDiff(const CTSVNPath& path, SVNRev peg, SVNRev rev1, SVNRev rev2, bool recurse, bool ignoreancestry)
{
	m_bDoPegDiff = true;
	m_path1 = path;
	m_peg = peg;
	m_rev1 = rev1;
	m_rev2 = rev2;
	m_bRecurse = recurse;
	m_bIgnoreancestry = ignoreancestry;
}

void CFileDiffDlg::SetDiff(const CTSVNPath& path1, SVNRev rev1, const CTSVNPath& path2, SVNRev rev2, bool recurse, bool ignoreancestry)
{
	m_bDoPegDiff = false;
	m_path1 = path1;
	m_path2 = path2;
	m_rev1 = rev1;
	m_rev2 = rev2;
	m_bRecurse = recurse;
	m_bIgnoreancestry = ignoreancestry;
}

BOOL CFileDiffDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	m_cFileList.SetRedraw(false);
	m_cFileList.DeleteAllItems();
	DWORD exStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP;
	m_cFileList.SetExtendedStyle(exStyle);

	m_nIconFolder = SYS_IMAGE_LIST().GetDirIconIndex();
	m_cFileList.SetImageList(&SYS_IMAGE_LIST(), LVSIL_SMALL);

	int c = ((CHeaderCtrl*)(m_cFileList.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_cFileList.DeleteColumn(c--);
	CString temp;
	temp.LoadString(IDS_FILEDIFF_FILE);
	m_cFileList.InsertColumn(0, temp);
	temp.LoadString(IDS_FILEDIFF_ACTION);
	m_cFileList.InsertColumn(1, temp);

	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(m_cFileList.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		m_cFileList.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}
	
	m_cFileList.SetRedraw(true);
	
	AddAnchor(IDC_DIFFSTATIC1, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_FIRSTURL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_DIFFSTATIC2, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_SECONDURL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_FILELIST, TOP_LEFT, BOTTOM_RIGHT);
	

	CString url1, url2;
	url1.Format(_T("%s : revision %ld"), m_path1.GetSVNPathString(), (LONG)m_rev1);
	url2.Format(_T("%s : revision %ld"), m_bDoPegDiff ? m_path1.GetSVNPathString() : m_path2.GetSVNPathString(), (LONG)m_rev2);

	GetDlgItem(IDC_FIRSTURL)->SetWindowText(url1);
	GetDlgItem(IDC_SECONDURL)->SetWindowText(url2);

	InterlockedExchange(&m_bThreadRunning, TRUE);
	if (AfxBeginThread(DiffThreadEntry, this)==NULL)
	{
		InterlockedExchange(&m_bThreadRunning, FALSE);
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	return TRUE;
}

svn_error_t* CFileDiffDlg::DiffSummarizeCallback(const CTSVNPath& path, 
												 svn_client_diff_summarize_kind_t kind, 
												 bool propchanged, svn_node_kind_t node)
{
	FileDiff fd;
	fd.path = path;
	fd.kind = kind;
	fd.node = node;
	fd.propchanged = propchanged;
	m_arFileList.Add(fd);
	return SVN_NO_ERROR;
}

UINT CFileDiffDlg::DiffThreadEntry(LPVOID pVoid)
{
	return ((CFileDiffDlg*)pVoid)->DiffThread();
}

UINT CFileDiffDlg::DiffThread()
{
	bool bSuccess = true;
	POINT pt;
	GetCursorPos(&pt);
	SetCursorPos(pt.x, pt.y);
	m_cFileList.ShowText(CString(MAKEINTRESOURCE(IDS_FILEDIFF_WAIT)));
	if (m_bDoPegDiff)
	{
		bSuccess = DiffSummarizePeg(m_path1, m_peg, m_rev1, m_rev2, m_bRecurse, m_bIgnoreancestry);
	}
	else
	{
		bSuccess = DiffSummarize(m_path1, m_rev1, m_path2, m_rev2, m_bRecurse, m_bIgnoreancestry);
	}
	if (!bSuccess)
	{
		m_cFileList.ShowText(GetLastErrorMessage());
		InterlockedExchange(&m_bThreadRunning, FALSE);
		return 0;
	}

	m_cFileList.SetRedraw(false);
	for (INT_PTR i=0; i<m_arFileList.GetCount(); ++i)
	{
		FileDiff fd = m_arFileList.GetAt(i);
		AddEntry(&fd);
	}

	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(m_cFileList.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		m_cFileList.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}

	m_cFileList.ClearText();
	m_cFileList.SetRedraw(true);

	InterlockedExchange(&m_bThreadRunning, FALSE);
	InvalidateRect(NULL);
	GetCursorPos(&pt);
	SetCursorPos(pt.x, pt.y);
	return 0;
}

int CFileDiffDlg::AddEntry(FileDiff * fd)
{
	int ret = -1;
	if (fd)
	{
		int index = m_cFileList.GetItemCount();

		int icon_idx = 0;
		if (fd->node == svn_node_dir)
				icon_idx = m_nIconFolder;
		else
		{
			icon_idx = SYS_IMAGE_LIST().GetPathIconIndex(fd->path);
		}

		ret = m_cFileList.InsertItem(index, fd->path.GetSVNPathString(), icon_idx);
		m_cFileList.SetItemText(index, 1, GetSummarizeActionText(fd->kind));
	}
	return ret;
}

void CFileDiffDlg::DoDiff(int selIndex, bool blame)
{
	CFileDiffDlg::FileDiff fd = m_arFileList.GetAt(selIndex);

	CTSVNPath url1 = CTSVNPath(m_path1.GetSVNPathString() + _T("/") + fd.path.GetSVNPathString());
	CTSVNPath url2 = m_bDoPegDiff ? url1 : CTSVNPath(m_path2.GetSVNPathString() + _T("/") + fd.path.GetSVNPathString());

	if (fd.propchanged)
	{
		DiffProps(selIndex);
	}
	if (fd.node == svn_node_dir)
		return;

	CTSVNPath tempfile = CTempFiles::Instance().GetTempFilePath(true, m_path1, m_rev1);
	CString sTemp;
	CProgressDlg progDlg;
	progDlg.SetTitle(IDS_PROGRESSWAIT);
	progDlg.ShowModeless(this);
	progDlg.FormatPathLine(1, IDS_PROGRESSGETFILE, (LPCTSTR)m_path1.GetUIPathString());
	progDlg.FormatNonPathLine(2, IDS_PROGRESSREVISION, (LONG)m_rev1);

	if ((fd.kind != svn_client_diff_summarize_kind_added)&&(!blame)&&(!Cat(url1, SVNRev(m_rev1), m_rev1, tempfile)))
	{
		CMessageBox::Show(NULL, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		return;
	}
	else if ((fd.kind != svn_client_diff_summarize_kind_added)&&(blame)&&(!m_blamer.BlameToFile(url1, 1, m_rev1, m_rev1, tempfile)))
	{
		CMessageBox::Show(NULL, m_blamer.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		return;
	}
	SetFileAttributes(tempfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
	progDlg.SetProgress(1, 2);
	progDlg.FormatPathLine(1, IDS_PROGRESSGETFILE, url2.GetUIPathString());
	progDlg.FormatNonPathLine(2, IDS_PROGRESSREVISION, m_rev2);
	CTSVNPath tempfile2 = CTempFiles::Instance().GetTempFilePath(true, url2);
	if ((fd.kind != svn_client_diff_summarize_kind_deleted)&&(!blame)&&(!Cat(url2, m_bDoPegDiff ? m_peg : m_rev2, m_rev2, tempfile2)))
	{
		CMessageBox::Show(NULL, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		return;
	}
	else if ((fd.kind != svn_client_diff_summarize_kind_deleted)&&(blame)&&(!m_blamer.BlameToFile(url2, 1, m_bDoPegDiff ? m_peg : m_rev2, m_rev2, tempfile2)))
	{
		CMessageBox::Show(NULL, m_blamer.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		return;
	}
	SetFileAttributes(tempfile2.GetWinPath(), FILE_ATTRIBUTE_READONLY);
	progDlg.SetProgress(2,2);
	progDlg.Stop();

	CString rev1name, rev2name;
	if (m_bDoPegDiff)
	{
		rev1name.Format(_T("%s Revision %ld"), (LPCTSTR)fd.path.GetSVNPathString(), (LONG)m_rev1);
		rev2name.Format(_T("%s Revision %ld"), (LPCTSTR)fd.path.GetSVNPathString(), (LONG)m_rev2);
	}
	else
	{
		rev1name = m_path1.GetSVNPathString() + _T("/") + fd.path.GetSVNPathString();
		rev2name = m_path2.GetSVNPathString() + _T("/") + fd.path.GetSVNPathString();
	}
	CUtils::StartExtDiff(tempfile, tempfile2, rev1name, rev2name, FALSE, blame);
}

void CFileDiffDlg::DiffProps(int selIndex)
{
	CFileDiffDlg::FileDiff fd = m_arFileList.GetAt(selIndex);

	CTSVNPath url1 = CTSVNPath(m_path1.GetSVNPathString() + _T("/") + fd.path.GetSVNPathString());
	CTSVNPath url2 = m_bDoPegDiff ? url1 : CTSVNPath(m_path2.GetSVNPathString() + _T("/") + fd.path.GetSVNPathString());

	// diff the properties
	SVNProperties propsurl1(url1, m_rev1);
	SVNProperties propsurl2(url2, m_rev2);

	for (int wcindex = 0; wcindex < propsurl1.GetCount(); ++wcindex)
	{
		stdstring wcname = propsurl1.GetItemName(wcindex);
		stdstring wcvalue = CUnicodeUtils::StdGetUnicode((char *)propsurl1.GetItemValue(wcindex).c_str());
		stdstring basevalue;
		bool bDiffRequired = true;
		for (int baseindex = 0; baseindex < propsurl2.GetCount(); ++baseindex)
		{
			if (propsurl2.GetItemName(baseindex).compare(wcname)==0)
			{
				basevalue = CUnicodeUtils::StdGetUnicode((char *)propsurl2.GetItemValue(baseindex).c_str());
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
					return;
			}
			else
				return;
			SetFileAttributes(wcpropfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
			SetFileAttributes(basepropfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
			CString n1, n2;
			if (m_rev1.IsWorking())
				n1.Format(IDS_DIFF_WCNAME, wcname.c_str());
			if (m_rev1.IsBase())
				n1.Format(IDS_DIFF_BASENAME, wcname.c_str());
			if (m_rev1.IsHead() || m_rev1.IsNumber())
			{
				if (m_bDoPegDiff)
				{
					n1.Format(_T("%s : %s Revision %ld"), wcname.c_str(), (LPCTSTR)fd.path.GetSVNPathString(), (LONG)m_rev1);
				}
				else
				{
					CString sTemp = wcname.c_str();
					sTemp += _T(" : ");
					n1 = sTemp + m_path1.GetSVNPathString() + _T("/") + fd.path.GetSVNPathString();
				}
			}
			if (m_rev2.IsWorking())
				n2.Format(IDS_DIFF_WCNAME, wcname.c_str());
			if (m_rev2.IsBase())
				n2.Format(IDS_DIFF_BASENAME, wcname.c_str());
			if (m_rev2.IsHead() || m_rev2.IsNumber())
			{
				if (m_bDoPegDiff)
				{
					n2.Format(_T("%s : %s Revision %ld"), wcname.c_str(),  (LPCTSTR)fd.path.GetSVNPathString(), (LONG)m_rev2);
				}
				else
				{
					CString sTemp = wcname.c_str();
					sTemp += _T(" : ");
					n2 = sTemp + m_path2.GetSVNPathString() + _T("/") + fd.path.GetSVNPathString();
				}
			}
			CUtils::StartExtDiff(basepropfile, wcpropfile, n2, n1, TRUE);
		}
	}
}

void CFileDiffDlg::OnNMDblclkFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	int selIndex = pNMLV->iItem;
	if (selIndex < 0)
		return;
	if (selIndex >= m_arFileList.GetCount())
		return;	
	
	DoDiff(selIndex, m_bBlame);
}

void CFileDiffDlg::OnLvnGetInfoTipFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVGETINFOTIP pGetInfoTip = reinterpret_cast<LPNMLVGETINFOTIP>(pNMHDR);
	if (pGetInfoTip->iItem >= m_arFileList.GetCount())
		return;

	CString path = m_path1.GetSVNPathString() + _T("/") + m_arFileList.GetAt(pGetInfoTip->iItem).path.GetSVNPathString();
	if (pGetInfoTip->cchTextMax > path.GetLength())
			_tcsncpy_s(pGetInfoTip->pszText, pGetInfoTip->cchTextMax, path, pGetInfoTip->cchTextMax);
	*pResult = 0;
}

void CFileDiffDlg::OnNMCustomdrawFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );
	// Take the default processing unless we set this to something else below.
	*pResult = CDRF_DODEFAULT;

	// First thing - check the draw stage. If it's the control's prepaint
	// stage, then tell Windows we want messages for every item.

	if ( CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
	}
	else if ( CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		// This is the prepaint stage for an item. Here's where we set the
		// item's text color. Our return value will tell Windows to draw the
		// item itself, but it will use the new color we set here.

		// Tell Windows to paint the control itself.
		*pResult = CDRF_DODEFAULT;

		COLORREF crText = GetSysColor(COLOR_WINDOWTEXT);

		if (m_arFileList.GetCount() > (INT_PTR)pLVCD->nmcd.dwItemSpec)
		{
			FileDiff fd = m_arFileList.GetAt(pLVCD->nmcd.dwItemSpec);
			if (fd.kind == svn_client_diff_summarize_kind_added)
				crText = m_colors.GetColor(CColors::Added);
			if (fd.kind == svn_client_diff_summarize_kind_deleted)
				crText = m_colors.GetColor(CColors::Deleted);
			if (fd.propchanged)
				crText = m_colors.GetColor(CColors::PropertyChanged);
		}
		// Store the color back in the NMLVCUSTOMDRAW struct.
		pLVCD->clrText = crText;
	}
}

void CFileDiffDlg::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	CMenu popup;
	if (popup.CreatePopupMenu())
	{
		CString temp;
		temp.LoadString(IDS_LOG_POPUP_COMPARETWO);
		popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COMPARE, temp);
		temp.LoadString(IDS_FILEDIFF_POPBLAME);
		popup.AppendMenu(MF_STRING | MF_ENABLED, ID_BLAME, temp);
		int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
		switch (cmd)
		{
		case ID_COMPARE:
			{
				POSITION pos = m_cFileList.GetFirstSelectedItemPosition();
				while (pos)
				{
					int index = m_cFileList.GetNextSelectedItem(pos);
					DoDiff(index, false);
				}					
			}
			break;
		case ID_BLAME:
			{
				POSITION pos = m_cFileList.GetFirstSelectedItemPosition();
				while (pos)
				{
					int index = m_cFileList.GetNextSelectedItem(pos);
					DoDiff(index, true);
				}					
			}
			break;
		}
	}
}

BOOL CFileDiffDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (pWnd != &m_cFileList)
		return CResizableStandAloneDialog::OnSetCursor(pWnd, nHitTest, message);
	if (m_bThreadRunning == 0)
	{
		HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
		SetCursor(hCur);
		return CResizableStandAloneDialog::OnSetCursor(pWnd, nHitTest, message);
	}
	HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT));
	SetCursor(hCur);
	return TRUE;
}

void CFileDiffDlg::OnEnSetfocusFirsturl()
{
	GetDlgItem(IDC_FIRSTURL)->HideCaret();
}

void CFileDiffDlg::OnEnSetfocusSecondurl()
{
	GetDlgItem(IDC_SECONDURL)->HideCaret();
}

