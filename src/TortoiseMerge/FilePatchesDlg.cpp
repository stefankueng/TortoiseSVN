// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2004 - Stefan Kueng

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
#include "TortoiseMerge.h"
#include "FilePatchesDlg.h"
#include ".\filepatchesdlg.h"


IMPLEMENT_DYNAMIC(CFilePatchesDlg, CDialog)
CFilePatchesDlg::CFilePatchesDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFilePatchesDlg::IDD, pParent)
{
	m_ImgList.Create(16, 16, ILC_COLOR16 | ILC_MASK, 4, 1);
	m_bMinimized = FALSE;
}

CFilePatchesDlg::~CFilePatchesDlg()
{
}

void CFilePatchesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FILELIST, m_cFileList);
}

BOOL CFilePatchesDlg::SetFileStatusAsPatched(CString sPath)
{
	for (int i=0; i<m_arFileStates.GetCount(); i++)
	{
		if (sPath.CompareNoCase(GetFullPath(i))==0)
		{
			m_arFileStates.SetAt(i, FPDLG_FILESTATE_PATCHED);
			Invalidate();
			return TRUE;
		} // if (sPath.CompareNoCase(GetFullPath(i))==0) 
	} // for (int i=0; i<m_arFileStates.GetCount(); i++) 
	return FALSE;
}

CString CFilePatchesDlg::GetFullPath(int nIndex)
{
	CString temp = m_pPatch->GetFilename(nIndex);
	temp.Replace('/', '\\');
	//temp = temp.Mid(temp.Find('\\')+1);
	temp = m_sPath + temp;
	return temp;
}

BOOL CFilePatchesDlg::Init(CPatch * pPatch, CPatchFilesDlgCallBack * pCallBack, CString sPath)
{
	if ((pCallBack==NULL)||(pPatch==NULL))
	{
		return FALSE;
	}
	m_arFileStates.RemoveAll();
	m_pPatch = pPatch;
	m_pCallBack = pCallBack;
	m_sPath = sPath;
	if (m_sPath.Right(1).Compare(_T("\\"))==0)
		m_sPath = m_sPath.Left(m_sPath.GetLength()-1);

	m_sPath = m_sPath + _T("\\");
	for (int i=m_ImgList.GetImageCount();i>0;i--)
	{
		m_ImgList.Remove(0);
	}

	m_cFileList.SetExtendedStyle(LVS_EX_INFOTIP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	m_cFileList.DeleteAllItems();
	int c = ((CHeaderCtrl*)(m_cFileList.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_cFileList.DeleteColumn(c--);
	m_cFileList.InsertColumn(0, _T(""));

	m_cFileList.SetRedraw(false);

	for(int i=0; i<m_pPatch->GetNumberOfFiles(); i++)
	{
		CString sFile = m_pPatch->GetFilename(i);
		sFile.Replace('/', '\\');
		sFile = sFile.Mid(sFile.ReverseFind('\\')+1);
		DWORD state;
		if (m_pPatch->PatchFile(GetFullPath(i)))
			state = FPDLG_FILESTATE_GOOD;
		else
			state = FPDLG_FILESTATE_CONFLICTED;
		m_arFileStates.Add(state);
		SHFILEINFO    sfi;
		SHGetFileInfo(
			GetFullPath(i), 
			FILE_ATTRIBUTE_NORMAL,
			&sfi, 
			sizeof(SHFILEINFO), 
			SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
		m_cFileList.InsertItem(i, sFile, m_ImgList.Add(sfi.hIcon));

	} // for(int i=0; i<m_pPatch->GetNumberOfFiles(); i++) 
	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(m_cFileList.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		m_cFileList.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}

	m_cFileList.SetImageList(&m_ImgList, LVSIL_SMALL);
	m_cFileList.SetRedraw(true);

	RECT windowrect;
	GetWindowRect(&windowrect);
	m_nWindowHeight = windowrect.bottom - windowrect.top;
	return TRUE;
}

BEGIN_MESSAGE_MAP(CFilePatchesDlg, CDialog)
	ON_WM_SIZE()
	ON_NOTIFY(LVN_GETINFOTIP, IDC_FILELIST, OnLvnGetInfoTipFilelist)
	ON_NOTIFY(NM_DBLCLK, IDC_FILELIST, OnNMDblclkFilelist)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_FILELIST, OnNMCustomdrawFilelist)
	ON_NOTIFY(NM_RCLICK, IDC_FILELIST, OnNMRclickFilelist)
	ON_WM_NCLBUTTONDBLCLK()
END_MESSAGE_MAP()

void CFilePatchesDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);
	if (this->IsWindowVisible())
	{
		CRect rect;
		GetClientRect(rect);
		GetDlgItem(IDC_FILELIST)->MoveWindow(rect.left, rect.top , cx, cy);
		m_cFileList.SetColumnWidth(0, cx);
	} // if (this->IsWindowVisible()) 
}

void CFilePatchesDlg::OnLvnGetInfoTipFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVGETINFOTIP pGetInfoTip = reinterpret_cast<LPNMLVGETINFOTIP>(pNMHDR);

	CString temp = GetFullPath(pGetInfoTip->iItem);
	_tcsncpy(pGetInfoTip->pszText, temp, pGetInfoTip->cchTextMax);
	*pResult = 0;
}

void CFilePatchesDlg::OnNMDblclkFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	if ((pNMLV->iItem < 0) || (pNMLV->iItem >= m_arFileStates.GetCount()))
		return;
	if ((m_pCallBack)&&(m_arFileStates.GetAt(pNMLV->iItem)!=FPDLG_FILESTATE_PATCHED))
	{
		m_pCallBack->PatchFile(GetFullPath(pNMLV->iItem), m_pPatch->GetRevision(pNMLV->iItem));
	} // if ((m_pCallBack)&&(!temp.IsEmpty())) 
}

void CFilePatchesDlg::OnNMCustomdrawFilelist(NMHDR *pNMHDR, LRESULT *pResult)
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
		// We'll cycle the colors through red, green, and light blue.

		COLORREF crText = ::GetSysColor(COLOR_WINDOWTEXT);

		if (m_arFileStates.GetCount() > (INT_PTR)pLVCD->nmcd.dwItemSpec)
		{
			if (m_arFileStates.GetAt(pLVCD->nmcd.dwItemSpec)==FPDLG_FILESTATE_CONFLICTED)
			{
				crText = RGB(200, 0, 0);
			}
			if (m_arFileStates.GetAt(pLVCD->nmcd.dwItemSpec)==FPDLG_FILESTATE_PATCHED)
			{
				crText = ::GetSysColor(COLOR_GRAYTEXT);
			}
			// Store the color back in the NMLVCUSTOMDRAW struct.
			pLVCD->clrText = crText;
		} // if (m_arFileStates.GetCount() > (INT_PTR)pLVCD->nmcd.dwItemSpec) 

		// Tell Windows to paint the control itself.
		*pResult = CDRF_DODEFAULT;
	}
}

void CFilePatchesDlg::OnNMRclickFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
	CString temp;
	CMenu popup;
	POINT point;
	GetCursorPos(&point);
	if (popup.CreatePopupMenu())
	{
		temp.LoadString(IDS_PATCH_ALL);
		popup.AppendMenu(MF_STRING | MF_ENABLED, ID_PATCHALL, temp);
		int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
		switch (cmd)
		{
		case ID_PATCHALL:
			{
				if (m_pCallBack)
				{
					for (int i=0; i<m_arFileStates.GetCount(); i++)
					{
						if (m_arFileStates.GetAt(i)!= FPDLG_FILESTATE_PATCHED)
							m_pCallBack->PatchFile(GetFullPath(i), m_pPatch->GetRevision(i), TRUE);
					} // for (int i=0; i<m_arFileStates.GetCount(); i++) 
				} // if ((m_pCallBack)&&(!temp.IsEmpty())) 
			} 
			break;
		default:
			break;
		} // switch (cmd) 
	} // if (popup.CreatePopupMenu()) 
}

void CFilePatchesDlg::OnNcLButtonDblClk(UINT nHitTest, CPoint point)
{
	if (!m_bMinimized)
	{
		RECT windowrect;
		RECT clientrect;
		GetWindowRect(&windowrect);
		GetClientRect(&clientrect);
		m_nWindowHeight = windowrect.bottom - windowrect.top;
		MoveWindow(windowrect.left, windowrect.top, 
			windowrect.right - windowrect.left,
			m_nWindowHeight - (clientrect.bottom - clientrect.top));
	}
	else
	{
		RECT windowrect;
		GetWindowRect(&windowrect);
		MoveWindow(windowrect.left, windowrect.top, windowrect.right - windowrect.left, m_nWindowHeight);
	}
	m_bMinimized = !m_bMinimized;
	CDialog::OnNcLButtonDblClk(nHitTest, point);
}
