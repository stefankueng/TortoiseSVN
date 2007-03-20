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
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "stdafx.h"
#include "TortoiseProc.h"
#include "SetMisc.h"
#include "MessageBox.h"

IMPLEMENT_DYNAMIC(CSetMisc, CPropertyPage)

CSetMisc::CSetMisc()
	: CPropertyPage(CSetMisc::IDD)
	, m_bUnversionedRecurse(FALSE)
	, m_bAutocompletion(FALSE)
	, m_dwAutocompletionTimeout(0)
	, m_bSpell(TRUE)
	, m_bCheckRepo(FALSE)
	, m_dwMaxHistory(25)
	, m_bSortNumerical(FALSE)
	, m_bCommitReopen(FALSE)
{
	m_regUnversionedRecurse = CRegDWORD(_T("Software\\TortoiseSVN\\UnversionedRecurse"), TRUE);
	m_bUnversionedRecurse = (DWORD)m_regUnversionedRecurse;
	m_regAutocompletion = CRegDWORD(_T("Software\\TortoiseSVN\\Autocompletion"), TRUE);
	m_bAutocompletion = (DWORD)m_regAutocompletion;
	m_regAutocompletionTimeout = CRegDWORD(_T("Software\\TortoiseSVN\\AutocompleteParseTimeout"), 5);
	m_dwAutocompletionTimeout = (DWORD)m_regAutocompletionTimeout;
	m_regSpell = CRegDWORD(_T("Software\\TortoiseSVN\\Spellchecker"), FALSE);
	m_bSpell = (DWORD)m_regSpell;
	m_regCheckRepo = CRegDWORD(_T("Software\\TortoiseSVN\\CheckRepo"), FALSE);
	m_bCheckRepo = (DWORD)m_regCheckRepo;
	m_regMaxHistory = CRegDWORD(_T("Software\\TortoiseSVN\\MaxHistoryItems"), 25);
	m_dwMaxHistory = (DWORD)m_regMaxHistory;
	m_regSortNumerical = CRegDWORD(_T("Software\\TortoiseSVN\\SortNumerical"), TRUE);
	m_bSortNumerical = (BOOL)(DWORD)m_regSortNumerical;
	m_regCommitReopen = CRegDWORD(_T("Software\\TortoiseSVN\\CommitReopen"), FALSE);
	m_bCommitReopen = (BOOL)(DWORD)m_regCommitReopen;
}

CSetMisc::~CSetMisc()
{
}

int CSetMisc::SaveData()
{
	m_regUnversionedRecurse = m_bUnversionedRecurse;
	if (m_regUnversionedRecurse.LastError != ERROR_SUCCESS)
		CMessageBox::Show(m_hWnd, m_regUnversionedRecurse.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
	m_regAutocompletion = m_bAutocompletion;
	if (m_regAutocompletion.LastError != ERROR_SUCCESS)
		CMessageBox::Show(m_hWnd, m_regAutocompletion.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
	m_regAutocompletionTimeout = m_dwAutocompletionTimeout;
	if (m_regAutocompletionTimeout.LastError != ERROR_SUCCESS)
		CMessageBox::Show(m_hWnd, m_regAutocompletionTimeout.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
	m_regSpell = m_bSpell;
	if (m_regSpell.LastError != ERROR_SUCCESS)
		CMessageBox::Show(m_hWnd, m_regSpell.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
	m_regCheckRepo = m_bCheckRepo;
	if (m_regCheckRepo.LastError != ERROR_SUCCESS)
		CMessageBox::Show(m_hWnd, m_regCheckRepo.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
	m_regMaxHistory = m_dwMaxHistory;
	if (m_regMaxHistory.LastError != ERROR_SUCCESS)
		CMessageBox::Show(m_hWnd, m_regMaxHistory.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
	m_regSortNumerical = m_bSortNumerical;
	if (m_regSortNumerical.LastError != ERROR_SUCCESS)
		CMessageBox::Show(m_hWnd, m_regSortNumerical.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
	m_regCommitReopen = m_bCommitReopen;
	if (m_regCommitReopen.LastError != ERROR_SUCCESS)
		CMessageBox::Show(m_hWnd, m_regCommitReopen.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
	return 0;
}

void CSetMisc::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_UNVERSIONEDRECURSE, m_bUnversionedRecurse);
	DDX_Check(pDX, IDC_AUTOCOMPLETION, m_bAutocompletion);
	DDX_Text(pDX, IDC_AUTOCOMPLETIONTIMEOUT, m_dwAutocompletionTimeout);
	DDV_MinMaxUInt(pDX, m_dwAutocompletionTimeout, 1, 100);
	DDX_Check(pDX, IDC_SPELL, m_bSpell);
	DDX_Check(pDX, IDC_REPOCHECK, m_bCheckRepo);
	DDX_Text(pDX, IDC_MAXHISTORY, m_dwMaxHistory);
	DDV_MinMaxUInt(pDX, m_dwMaxHistory, 1, 100);
	DDX_Check(pDX, IDC_SORTNUMERICAL, m_bSortNumerical);
	DDX_Check(pDX, IDC_REOPENCOMMIT, m_bCommitReopen);
}


BEGIN_MESSAGE_MAP(CSetMisc, CPropertyPage)
	ON_BN_CLICKED(IDC_UNVERSIONEDRECURSE, &CSetMisc::OnChanged)
	ON_BN_CLICKED(IDC_AUTOCOMPLETION, &CSetMisc::OnChanged)
	ON_EN_CHANGE(IDC_AUTOCOMPLETIONTIMEOUT, &CSetMisc::OnChanged)
	ON_EN_CHANGE(IDC_MAXHISTORY, &CSetMisc::OnChanged)
	ON_BN_CLICKED(IDC_SPELL, &CSetMisc::OnChanged)
	ON_BN_CLICKED(IDC_REPOCHECK, &CSetMisc::OnChanged)
	ON_BN_CLICKED(IDC_SORTNUMERICAL, &CSetMisc::OnChanged)
	ON_BN_CLICKED(IDC_REOPENCOMMIT, &CSetMisc::OnChanged)
END_MESSAGE_MAP()

void CSetMisc::OnChanged()
{
	SetModified();
}

BOOL CSetMisc::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_UNVERSIONEDRECURSE, IDS_SETTINGS_UNVERSIONEDRECURSE_TT);
	m_tooltips.AddTool(IDC_AUTOCOMPLETION, IDS_SETTINGS_AUTOCOMPLETION_TT);
	m_tooltips.AddTool(IDC_AUTOCOMPLETIONTIMEOUT, IDS_SETTINGS_AUTOCOMPLETIONTIMEOUT_TT);
	m_tooltips.AddTool(IDC_AUTOCOMPLETIONTIMEOUTLABEL, IDS_SETTINGS_AUTOCOMPLETIONTIMEOUT_TT);
	m_tooltips.AddTool(IDC_SPELL, IDS_SETTINGS_SPELLCHECKER_TT);
	m_tooltips.AddTool(IDC_REOPENCOMMIT, IDS_SETTINGS_COMMITREOPEN_TT);
	m_tooltips.AddTool(IDC_REPOCHECK, IDS_SETTINGS_REPOCHECK_TT);
	m_tooltips.AddTool(IDC_MAXHISTORY, IDS_SETTINGS_MAXHISTORY_TT);
	m_tooltips.AddTool(IDC_MAXHISTORYLABEL, IDS_SETTINGS_MAXHISTORY_TT);
	m_tooltips.AddTool(IDC_SORTNUMERICAL, IDS_SETTINGS_SORTNUMERICAL_TT);

	return TRUE;
}

BOOL CSetMisc::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return CPropertyPage::PreTranslateMessage(pMsg);
}
