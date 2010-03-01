// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007, 2009-2010 - TortoiseSVN

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
#include "MergeAllDlg.h"


IMPLEMENT_DYNAMIC(CMergeAllDlg, CStandAloneDialog)

CMergeAllDlg::CMergeAllDlg(CWnd* pParent /*=NULL*/)
	: CStandAloneDialog(CMergeAllDlg::IDD, pParent)
	, m_depth(svn_depth_unknown)
	, m_bIgnoreEOL(FALSE)
	, m_bIgnoreAncestry(FALSE)
	, m_bForce(FALSE)
{

}

CMergeAllDlg::~CMergeAllDlg()
{
}

void CMergeAllDlg::DoDataExchange(CDataExchange* pDX)
{
	CStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_DEPTH, m_depthCombo);
	DDX_Check(pDX, IDC_IGNOREANCESTRY, m_bIgnoreAncestry);
	DDX_Control(pDX, IDC_DEPTH, m_depthCombo);
	DDX_Check(pDX, IDC_IGNOREEOL, m_bIgnoreEOL);
	DDX_Check(pDX, IDC_FORCE, m_bForce);
}


BEGIN_MESSAGE_MAP(CMergeAllDlg, CStandAloneDialog)
	ON_BN_CLICKED(IDHELP, &CMergeAllDlg::OnBnClickedHelp)
END_MESSAGE_MAP()


// CMergeAllDlg message handlers

void CMergeAllDlg::OnBnClickedHelp()
{
	OnHelp();
}

BOOL CMergeAllDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();

	ExtendFrameIntoClientArea(IDC_MERGEOPTIONS, IDC_MERGEOPTIONS, IDC_MERGEOPTIONS, IDC_MERGEOPTIONS);
	m_aeroControls.SubclassOkCancelHelp(this);

	m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_WORKING)));
	m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_INFINITE)));
	m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_IMMEDIATE)));
	m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_FILES)));
	m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_EMPTY)));
	switch (m_depth)
	{
	case svn_depth_unknown:
		m_depthCombo.SetCurSel(0);
		break;
	case svn_depth_infinity:
		m_depthCombo.SetCurSel(1);
		break;
	case svn_depth_immediates:
		m_depthCombo.SetCurSel(2);
		break;
	case svn_depth_files:
		m_depthCombo.SetCurSel(3);
		break;
	case svn_depth_empty:
		m_depthCombo.SetCurSel(4);
		break;
	default:
		m_depthCombo.SetCurSel(0);
		break;
	}

	CheckRadioButton(IDC_COMPAREWHITESPACES, IDC_IGNOREALLWHITESPACES, IDC_COMPAREWHITESPACES);

	if ((m_pParentWnd==NULL)&&(hWndExplorer))
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	return TRUE;
}

void CMergeAllDlg::OnOK()
{
	switch (m_depthCombo.GetCurSel())
	{
	case 0:
		m_depth = svn_depth_unknown;
		break;
	case 1:
		m_depth = svn_depth_infinity;
		break;
	case 2:
		m_depth = svn_depth_immediates;
		break;
	case 3:
		m_depth = svn_depth_files;
		break;
	case 4:
		m_depth = svn_depth_empty;
		break;
	default:
		m_depth = svn_depth_empty;
		break;
	}

	int rb = GetCheckedRadioButton(IDC_COMPAREWHITESPACES, IDC_IGNOREALLWHITESPACES);
	switch (rb)
	{
	case IDC_IGNOREWHITESPACECHANGES:
		m_IgnoreSpaces = svn_diff_file_ignore_space_change;
		break;
	case IDC_IGNOREALLWHITESPACES:
		m_IgnoreSpaces = svn_diff_file_ignore_space_all;
		break;
	case IDC_COMPAREWHITESPACES:
	default:
		m_IgnoreSpaces = svn_diff_file_ignore_space_none;
		break;
	}

	CStandAloneDialog::OnOK();
}