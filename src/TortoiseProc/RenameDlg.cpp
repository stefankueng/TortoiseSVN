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
#include "stdafx.h"
#include "TortoiseProc.h"
#include "MessageBox.h"
#include "TSVNPath.h"
#include "RenameDlg.h"
#include ".\renamedlg.h"


IMPLEMENT_DYNAMIC(CRenameDlg, CResizableStandAloneDialog)
CRenameDlg::CRenameDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CRenameDlg::IDD, pParent)
	, m_name(_T(""))
{
}

CRenameDlg::~CRenameDlg()
{
}

void CRenameDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_NAME, m_name);
}


BEGIN_MESSAGE_MAP(CRenameDlg, CResizableStandAloneDialog)
	ON_WM_SIZING()
	ON_EN_CHANGE(IDC_NAME, OnEnChangeName)
END_MESSAGE_MAP()

BOOL CRenameDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	ExtendFrameIntoClientArea(IDC_DWM);
	m_aeroControls.SubclassOkCancel(this);

	SHAutoComplete(GetDlgItem(IDC_NAME)->m_hWnd, SHACF_DEFAULT);

	if (!m_windowtitle.IsEmpty())
		this->SetWindowText(m_windowtitle);
	if (!m_label.IsEmpty())
		SetDlgItemText(IDC_LABEL, m_label);
	AddAnchor(IDC_LABEL, TOP_LEFT);
	AddAnchor(IDC_NAME, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_DWM, TOP_LEFT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("RenameDlg"));
	GetDlgItem(IDOK)->EnableWindow(FALSE);
	return TRUE;
}

void CRenameDlg::OnOK()
{
	UpdateData();
	m_name.Trim();
	CTSVNPath path(m_name);
	if (!path.IsValidOnWindows())
	{
		if (CMessageBox::Show(GetSafeHwnd(), IDS_WARN_NOVALIDPATH, IDS_APPNAME, MB_ICONWARNING | MB_OKCANCEL)==IDCANCEL)
			return;
	}
	CResizableDialog::OnOK();
}

void CRenameDlg::OnSizing(UINT fwSide, LPRECT pRect)
{
	// don't allow the dialog to be changed in height
	CRect rcWindowRect;
	GetWindowRect(&rcWindowRect);
	switch (fwSide)
	{
	case WMSZ_BOTTOM:
	case WMSZ_BOTTOMLEFT:
	case WMSZ_BOTTOMRIGHT:
		pRect->bottom = pRect->top + rcWindowRect.Height();
		break;
	case WMSZ_TOP:
	case WMSZ_TOPLEFT:
	case WMSZ_TOPRIGHT:
		pRect->top = pRect->bottom - rcWindowRect.Height();
		break;
	}
	CResizableStandAloneDialog::OnSizing(fwSide, pRect);
}

void CRenameDlg::OnEnChangeName()
{
	UpdateData();
	GetDlgItem(IDOK)->EnableWindow(!m_name.IsEmpty());
}