// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2008-2009 - TortoiseSVN

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
#include "SetBugTraq.h"
#include "SetBugTraqAdv.h"
#include "XPTheme.h"

IMPLEMENT_DYNAMIC(CSetBugTraq, ISettingsPropPage)

CSetBugTraq::CSetBugTraq()
	: ISettingsPropPage(CSetBugTraq::IDD)
{
}

CSetBugTraq::~CSetBugTraq()
{
}

void CSetBugTraq::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUGTRAQLIST, m_cBugTraqList);
}


BEGIN_MESSAGE_MAP(CSetBugTraq, ISettingsPropPage)
	ON_BN_CLICKED(IDC_BUGTRAQREMOVEBUTTON, &CSetBugTraq::OnBnClickedRemovebutton)
	ON_BN_CLICKED(IDC_BUGTRAQEDITBUTTON, &CSetBugTraq::OnBnClickedEditbutton)
	ON_BN_CLICKED(IDC_BUGTRAQADDBUTTON, &CSetBugTraq::OnBnClickedAddbutton)
	ON_BN_CLICKED(IDC_BUGTRAQCOPYBUTTON, &CSetBugTraq::OnBnClickedBugTraqcopybutton)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_BUGTRAQLIST, &CSetBugTraq::OnLvnItemchangedBugTraqlist)
	ON_NOTIFY(NM_DBLCLK, IDC_BUGTRAQLIST, &CSetBugTraq::OnNMDblclkBugTraqlist)
END_MESSAGE_MAP()

BOOL CSetBugTraq::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	m_associations.Load();

	m_cBugTraqList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);

	// clear all previously set header columns
	m_cBugTraqList.DeleteAllItems();
	int c = ((CHeaderCtrl*)(m_cBugTraqList.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_cBugTraqList.DeleteColumn(c--);

	// now set up the requested columns
	CString temp;

	temp.LoadString(IDS_SETTINGS_BUGTRAQ_PATHCOL);
	m_cBugTraqList.InsertColumn(0, temp);
	temp.LoadString(IDS_SETTINGS_BUGTRAQ_PROVIDERCOL);
	m_cBugTraqList.InsertColumn(1, temp);
	temp.LoadString(IDS_SETTINGS_BUGTRAQ_PARAMETERSCOL);
	m_cBugTraqList.InsertColumn(2, temp);

	CXPTheme theme;
	theme.SetWindowTheme(m_cBugTraqList.GetSafeHwnd(), L"Explorer", NULL);

	RebuildBugTraqList();

	return TRUE;
}

void CSetBugTraq::RebuildBugTraqList()
{
	m_cBugTraqList.SetRedraw(false);
	m_cBugTraqList.DeleteAllItems();

	// fill the list control with all the hooks
	for (CBugTraqAssociations::const_iterator it = m_associations.begin(); it != m_associations.end(); ++it)
	{
		int pos = m_cBugTraqList.InsertItem(m_cBugTraqList.GetItemCount(), (*it)->GetPath().GetWinPathString());
		m_cBugTraqList.SetItemText(pos, 1, (*it)->GetProviderName());
		m_cBugTraqList.SetItemText(pos, 2, (*it)->GetParameters());
		m_cBugTraqList.SetItemData(pos, (DWORD_PTR)*it);
	}

	int maxcol = ((CHeaderCtrl*)(m_cBugTraqList.GetDlgItem(0)))->GetItemCount()-1;
	for (int col = 0; col <= maxcol; col++)
	{
		m_cBugTraqList.SetColumnWidth(col, LVSCW_AUTOSIZE_USEHEADER);
	}
	m_cBugTraqList.SetRedraw(true);
}

void CSetBugTraq::OnBnClickedRemovebutton()
{
	// traversing from the end to the beginning so that the indices are not skipped
	int index = m_cBugTraqList.GetItemCount()-1;
	while (index >= 0)
	{
		if (m_cBugTraqList.GetItemState(index, LVIS_SELECTED) & LVIS_SELECTED)
		{
			CTSVNPath path = CTSVNPath(m_cBugTraqList.GetItemText(index, 0));
			m_cBugTraqList.DeleteItem(index);
			m_associations.RemoveByPath(path);
			SetModified();
		}
		index--;
	}
}

void CSetBugTraq::OnBnClickedEditbutton()
{
	if (m_cBugTraqList.GetSelectedCount() != 1)
		return;

	int index = m_cBugTraqList.GetNextItem(-1, LVNI_SELECTED);
	if (index == -1)
		return;

	CBugTraqAssociation *assoc = (CBugTraqAssociation *)m_cBugTraqList.GetItemData(index);
	if (!assoc)
		return;

	CSetBugTraqAdv dlg(*assoc);
	if (dlg.DoModal() == IDOK)
	{
		m_associations.RemoveByPath(assoc->GetPath());
		m_associations.Add(dlg.GetAssociation());
		RebuildBugTraqList();
		SetModified();
	}
}

void CSetBugTraq::OnBnClickedAddbutton()
{
	CSetBugTraqAdv dlg;
	if (dlg.DoModal() == IDOK)
	{
		m_associations.Add(dlg.GetAssociation());
		RebuildBugTraqList();
		SetModified();
	}
}

void CSetBugTraq::OnLvnItemchangedBugTraqlist(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	UINT count = m_cBugTraqList.GetSelectedCount();
	DialogEnableWindow(IDC_BUGTRAQREMOVEBUTTON, count > 0);
	DialogEnableWindow(IDC_BUGTRAQEDITBUTTON, count == 1);
	DialogEnableWindow(IDC_BUGTRAQCOPYBUTTON, count == 1);
	*pResult = 0;
}

void CSetBugTraq::OnNMDblclkBugTraqlist(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	OnBnClickedEditbutton();
	*pResult = 0;
}

BOOL CSetBugTraq::OnApply()
{
	UpdateData();
	m_associations.Save();
	SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}

void CSetBugTraq::OnBnClickedBugTraqcopybutton()
{
	if (m_cBugTraqList.GetSelectedCount() != 1)
		return;

	int index = m_cBugTraqList.GetNextItem(-1, LVNI_SELECTED);
	if (index == -1)
		return;

	CBugTraqAssociation *assoc = (CBugTraqAssociation *)m_cBugTraqList.GetItemData(index);
	if (!assoc)
		return;

	CSetBugTraqAdv dlg(*assoc);
	if (dlg.DoModal() == IDOK)
	{
		m_associations.Add(dlg.GetAssociation());
		RebuildBugTraqList();
		SetModified();
	}
}
