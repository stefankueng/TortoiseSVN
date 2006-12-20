// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006 - Stefan Kueng

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
#include "SetColorPage.h"
#include "DiffData.h"


// CSetColorPage dialog
#define INLINEADDED_COLOR			RGB(255, 255, 150)
#define INLINEREMOVED_COLOR			RGB(200, 100, 100)
#define MODIFIED_COLOR				RGB(220, 220, 255)

IMPLEMENT_DYNAMIC(CSetColorPage, CPropertyPage)
CSetColorPage::CSetColorPage()
	: CPropertyPage(CSetColorPage::IDD)
	, m_bReloadNeeded(FALSE)
	, m_regInlineAdded(_T("Software\\TortoiseMerge\\InlineAdded"), INLINEADDED_COLOR)
	, m_regInlineRemoved(_T("Software\\TortoiseMerge\\InlineRemoved"), INLINEREMOVED_COLOR)
	, m_regModifiedBackground(_T("Software\\TortoiseMerge\\Colors\\ColorModifiedB"), MODIFIED_COLOR)
{
}

CSetColorPage::~CSetColorPage()
{
	m_bInit = FALSE;
}

void CSetColorPage::SaveData()
{
	if (m_bInit)
	{
		CDiffData diffdata;
		COLORREF cBk;
		COLORREF cFg;

		cFg = ::GetSysColor(COLOR_WINDOWTEXT);

		cBk = m_cBkNormal.GetColor(TRUE);
		diffdata.SetColors(CDiffData::DIFFSTATE_NORMAL, cBk, cFg);
		diffdata.SetColors(CDiffData::DIFFSTATE_UNKNOWN, cBk, cFg);

		cBk = m_cBkRemoved.GetColor(TRUE);
		diffdata.SetColors(CDiffData::DIFFSTATE_REMOVED, cBk, cFg);
		diffdata.SetColors(CDiffData::DIFFSTATE_IDENTICALREMOVED, cBk, cFg);
		diffdata.SetColors(CDiffData::DIFFSTATE_THEIRSREMOVED, cBk, cFg);
		diffdata.SetColors(CDiffData::DIFFSTATE_YOURSREMOVED, cBk, cFg);

		cBk = m_cBkAdded.GetColor(TRUE);
		diffdata.SetColors(CDiffData::DIFFSTATE_ADDED, cBk, cFg);
		diffdata.SetColors(CDiffData::DIFFSTATE_IDENTICALADDED, cBk, cFg);
		diffdata.SetColors(CDiffData::DIFFSTATE_THEIRSADDED, cBk, cFg);
		diffdata.SetColors(CDiffData::DIFFSTATE_YOURSADDED, cBk, cFg);

		if ((DWORD)m_regInlineAdded != (DWORD)m_cBkInlineAdded.GetColor(TRUE))
			m_bReloadNeeded = true;
		m_regInlineAdded = m_cBkInlineAdded.GetColor(TRUE);
		if ((DWORD)m_regInlineRemoved != (DWORD)m_cBkInlineRemoved.GetColor(TRUE))
			m_bReloadNeeded = true;
		m_regInlineRemoved = m_cBkInlineRemoved.GetColor(TRUE);
		if ((DWORD)m_regModifiedBackground != (DWORD)m_cBkModified.GetColor(TRUE))
			m_bReloadNeeded = true;
		m_regModifiedBackground = m_cBkModified.GetColor(TRUE);

		cBk = m_cBkEmpty.GetColor(TRUE);
		diffdata.SetColors(CDiffData::DIFFSTATE_EMPTY, cBk, cFg);

		// there are three different colors for conflicted lines
		// conflicted, conflicted added, conflicted removed
		// but only one colorchooser for all three of them
		// so try to adjust the conflicted added and conflicted removed
		// colors a little so they look different.
		cBk = m_cBkConflict.GetColor(TRUE);
		diffdata.SetColors(CDiffData::DIFFSTATE_CONFLICTED, cBk, cFg);
		COLORREF adjustedcolor = cBk;
		if (GetRValue(cBk)-155 > 0)
			adjustedcolor = RGB(GetRValue(cBk), GetRValue(cBk)-155, 0);
		diffdata.SetColors(CDiffData::DIFFSTATE_CONFLICTADDED, adjustedcolor, cFg);
		if (GetRValue(cBk)-205 > 0)
			adjustedcolor = RGB(GetRValue(cBk), GetRValue(cBk)-205, GetRValue(cBk)-205);
		diffdata.SetColors(CDiffData::DIFFSTATE_CONFLICTEMPTY, adjustedcolor, cFg);
		
	}
}

void CSetColorPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BKNORMAL, m_cBkNormal);
	DDX_Control(pDX, IDC_BKREMOVED, m_cBkRemoved);
	DDX_Control(pDX, IDC_BKADDED, m_cBkAdded);
	DDX_Control(pDX, IDC_BKWHITESPACES, m_cBkInlineAdded);
	DDX_Control(pDX, IDC_BKWHITESPACEDIFF, m_cBkInlineRemoved);
	DDX_Control(pDX, IDC_BKMODIFIED, m_cBkModified);
	DDX_Control(pDX, IDC_BKEMPTY, m_cBkEmpty);
	DDX_Control(pDX, IDC_BKCONFLICTED, m_cBkConflict);
}


BEGIN_MESSAGE_MAP(CSetColorPage, CPropertyPage)
	ON_MESSAGE(CPN_SELENDOK, OnSelEndOK)
END_MESSAGE_MAP()


// CSetColorPage message handlers

LRESULT CSetColorPage::OnSelEndOK(WPARAM /*lParam*/, LPARAM /*wParam*/)
{
	SetModified();
	return 0;
}


BOOL CSetColorPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	CDiffData diffdata;
	COLORREF cBk;
	COLORREF cFg;

	CString sDefaultText, sCustomText;
	sDefaultText.LoadString(IDS_COLOURPICKER_DEFAULTTEXT);
	sCustomText.LoadString(IDS_COLOURPICKER_CUSTOMTEXT);

	diffdata.GetColors(CDiffData::DIFFSTATE_NORMAL, cBk, cFg);
	m_cBkNormal.SetDefaultColor(DIFFSTATE_NORMAL_DEFAULT_BG);
	m_cBkNormal.SetColor(cBk);
	m_cBkNormal.SetDefaultText(sDefaultText);
	m_cBkNormal.SetCustomText(sCustomText);

	diffdata.GetColors(CDiffData::DIFFSTATE_REMOVED, cBk, cFg);
	m_cBkRemoved.SetDefaultColor(DIFFSTATE_REMOVED_DEFAULT_BG);
	m_cBkRemoved.SetColor(cBk);
	m_cBkRemoved.SetDefaultText(sDefaultText);
	m_cBkRemoved.SetCustomText(sCustomText);

	diffdata.GetColors(CDiffData::DIFFSTATE_ADDED, cBk, cFg);
	m_cBkAdded.SetDefaultColor(DIFFSTATE_ADDED_DEFAULT_BG);
	m_cBkAdded.SetColor(cBk);
	m_cBkAdded.SetDefaultText(sDefaultText);
	m_cBkAdded.SetCustomText(sCustomText);

	m_cBkInlineAdded.SetDefaultColor(INLINEADDED_COLOR);
	m_cBkInlineAdded.SetColor((DWORD)m_regInlineAdded);
	m_cBkInlineAdded.SetDefaultText(sDefaultText);
	m_cBkInlineAdded.SetCustomText(sCustomText);

	m_cBkInlineRemoved.SetDefaultColor(INLINEREMOVED_COLOR);
	m_cBkInlineRemoved.SetColor((DWORD)m_regInlineRemoved);
	m_cBkInlineRemoved.SetDefaultText(sDefaultText);
	m_cBkInlineRemoved.SetCustomText(sCustomText);

	m_cBkModified.SetDefaultColor(MODIFIED_COLOR);
	m_cBkModified.SetColor((DWORD)m_regModifiedBackground);
	m_cBkModified.SetDefaultText(sDefaultText);
	m_cBkModified.SetCustomText(sCustomText);

	diffdata.GetColors(CDiffData::DIFFSTATE_EMPTY, cBk, cFg);
	m_cBkEmpty.SetDefaultColor(DIFFSTATE_EMPTY_DEFAULT_BG);
	m_cBkEmpty.SetColor(cBk);
	m_cBkEmpty.SetDefaultText(sDefaultText);
	m_cBkEmpty.SetCustomText(sCustomText);

	diffdata.GetColors(CDiffData::DIFFSTATE_CONFLICTED, cBk, cFg);
	m_cBkConflict.SetDefaultColor(DIFFSTATE_CONFLICTED_DEFAULT_BG);
	m_cBkConflict.SetColor(cBk);
	m_cBkConflict.SetDefaultText(sDefaultText);
	m_cBkConflict.SetCustomText(sCustomText);

	m_bInit = TRUE;

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSetColorPage::OnApply()
{
	UpdateData();
	SaveData();
	SetModified(FALSE);
	return CPropertyPage::OnApply();
}

