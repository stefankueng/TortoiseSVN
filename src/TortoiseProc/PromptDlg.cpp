// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2009-2010 - TortoiseSVN

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
#include "PromptDlg.h"


IMPLEMENT_DYNAMIC(CPromptDlg, CStandAloneDialog)
CPromptDlg::CPromptDlg(CWnd* pParent /*=NULL*/)
	: CStandAloneDialog(CPromptDlg::IDD, pParent)
	, m_info(_T(""))
	, m_sPass(_T(""))
	, m_saveCheck(FALSE)
	, m_hide(FALSE)
	, m_hParentWnd(NULL)
{
}

CPromptDlg::~CPromptDlg()
{
}

void CPromptDlg::DoDataExchange(CDataExchange* pDX)
{
	CStandAloneDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_INFOTEXT, m_info);
	DDX_Text(pDX, IDC_PASSEDIT, m_sPass);
	DDX_Control(pDX, IDC_PASSEDIT, m_pass);
	DDX_Check(pDX, IDC_SAVECHECK, m_saveCheck);
}

void CPromptDlg::SetHide(BOOL hide)
{
	m_hide = hide;
}

BEGIN_MESSAGE_MAP(CPromptDlg, CStandAloneDialog)
END_MESSAGE_MAP()


BOOL CPromptDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();

	ExtendFrameIntoClientArea(IDC_PASSEDIT);
	m_aeroControls.SubclassControl(this, IDC_SAVECHECK);
	m_aeroControls.SubclassOkCancel(this);

	if (m_hide)
	{
		m_pass.SetPasswordChar('*');
		GetDlgItem(IDC_SAVECHECK)->ShowWindow(SW_SHOW);
	}
	else
	{
		m_pass.SetPasswordChar('\0');
		GetDlgItem(IDC_SAVECHECK)->ShowWindow(SW_HIDE);
	}
	
	m_pass.SetFocus();
	if ((m_hParentWnd==NULL)&&(hWndExplorer))
		CenterWindow(CWnd::FromHandle(m_hParentWnd));
	return FALSE;
}

