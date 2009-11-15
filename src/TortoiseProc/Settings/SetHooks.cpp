// TortoiseSVN - a Windows shell extension for easy version control

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
#include "stdafx.h"
#include "TortoiseProc.h"
#include "SetHooks.h"
#include "SetHooksAdv.h"
#include "Hooks.h"
#include "XPTheme.h"

IMPLEMENT_DYNAMIC(CSetHooks, ISettingsPropPage)

CSetHooks::CSetHooks()
	: ISettingsPropPage(CSetHooks::IDD)
{

}

CSetHooks::~CSetHooks()
{
}

void CSetHooks::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HOOKLIST, m_cHookList);
}


BEGIN_MESSAGE_MAP(CSetHooks, ISettingsPropPage)
	ON_BN_CLICKED(IDC_HOOKREMOVEBUTTON, &CSetHooks::OnBnClickedRemovebutton)
	ON_BN_CLICKED(IDC_HOOKEDITBUTTON, &CSetHooks::OnBnClickedEditbutton)
	ON_BN_CLICKED(IDC_HOOKADDBUTTON, &CSetHooks::OnBnClickedAddbutton)
	ON_BN_CLICKED(IDC_HOOKCOPYBUTTON, &CSetHooks::OnBnClickedHookcopybutton)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_HOOKLIST, &CSetHooks::OnLvnItemchangedHooklist)
	ON_NOTIFY(NM_DBLCLK, IDC_HOOKLIST, &CSetHooks::OnNMDblclkHooklist)
END_MESSAGE_MAP()

BOOL CSetHooks::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	m_cHookList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);

	// clear all previously set header columns
	m_cHookList.DeleteAllItems();
	int c = ((CHeaderCtrl*)(m_cHookList.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_cHookList.DeleteColumn(c--);

	// now set up the requested columns
	CString temp;
	// the relative path is always visible
	temp.LoadString(IDS_SETTINGS_HOOKS_TYPECOL);
	m_cHookList.InsertColumn(0, temp);
	temp.LoadString(IDS_SETTINGS_HOOKS_PATHCOL);
	m_cHookList.InsertColumn(1, temp);
	temp.LoadString(IDS_SETTINGS_HOOKS_COMMANDLINECOL);
	m_cHookList.InsertColumn(2, temp);
	temp.LoadString(IDS_SETTINGS_HOOKS_WAITCOL);
	m_cHookList.InsertColumn(3, temp);
	temp.LoadString(IDS_SETTINGS_HOOKS_SHOWCOL);
	m_cHookList.InsertColumn(4, temp);

	CXPTheme theme;
	theme.SetWindowTheme(m_cHookList.GetSafeHwnd(), L"Explorer", NULL);

	RebuildHookList();

	return TRUE;
}

void CSetHooks::RebuildHookList()
{
	m_cHookList.SetRedraw(false);
	m_cHookList.DeleteAllItems();
	// fill the list control with all the hooks
	if (CHooks::Instance().size())
	{
		for (hookiterator it = CHooks::Instance().begin(); it != CHooks::Instance().end(); ++it)
		{
			int pos = m_cHookList.InsertItem(m_cHookList.GetItemCount(), CHooks::Instance().GetHookTypeString(it->first.htype));
			m_cHookList.SetItemText(pos, 1, it->first.path.GetWinPathString());
			m_cHookList.SetItemText(pos, 2, it->second.commandline);
			m_cHookList.SetItemText(pos, 3, (it->second.bWait ? _T("true") : _T("false")));
			m_cHookList.SetItemText(pos, 4, (it->second.bShow ? _T("show") : _T("hide")));
		}
	}

	int maxcol = ((CHeaderCtrl*)(m_cHookList.GetDlgItem(0)))->GetItemCount()-1;
	for (int col = 0; col <= maxcol; col++)
	{
		m_cHookList.SetColumnWidth(col, LVSCW_AUTOSIZE_USEHEADER);
	}
	m_cHookList.SetRedraw(true);
}

void CSetHooks::OnBnClickedRemovebutton()
{
	// traversing from the end to the beginning so that the indices are not skipped
	int index = m_cHookList.GetItemCount()-1;
	while (index >= 0)
	{
		if (m_cHookList.GetItemState(index, LVIS_SELECTED) & LVIS_SELECTED)
		{
			hookkey key;
			key.htype = CHooks::GetHookType((LPCTSTR)m_cHookList.GetItemText(index, 0));
			key.path = CTSVNPath(m_cHookList.GetItemText(index, 1));
			CHooks::Instance().Remove(key);		
			m_cHookList.DeleteItem(index);
			SetModified();
		}
		index--;
	}
}

void CSetHooks::OnBnClickedEditbutton()
{
	if (m_cHookList.GetSelectedCount() > 1)
		return;
	POSITION pos = m_cHookList.GetFirstSelectedItemPosition();
	if (pos)
	{
		CSetHooksAdv dlg;
		int index = m_cHookList.GetNextSelectedItem(pos);
		dlg.key.htype = CHooks::GetHookType((LPCTSTR)m_cHookList.GetItemText(index, 0));
		dlg.key.path = CTSVNPath(m_cHookList.GetItemText(index, 1));
		dlg.cmd.commandline = m_cHookList.GetItemText(index, 2);
		dlg.cmd.bWait = (m_cHookList.GetItemText(index, 3).Compare(_T("true"))==0);
		dlg.cmd.bShow = (m_cHookList.GetItemText(index, 4).Compare(_T("show"))==0);
		hookkey key = dlg.key;
		if (dlg.DoModal() == IDOK)
		{
			CHooks::Instance().Remove(key);
			CHooks::Instance().Add(dlg.key.htype, dlg.key.path, dlg.cmd.commandline, dlg.cmd.bWait, dlg.cmd.bShow);
			RebuildHookList();
			SetModified();
		}
	}
}

void CSetHooks::OnBnClickedAddbutton()
{
	CSetHooksAdv dlg;
	if (dlg.DoModal() == IDOK)
	{
		CHooks::Instance().Add(dlg.key.htype, dlg.key.path, dlg.cmd.commandline, dlg.cmd.bWait, dlg.cmd.bShow);
		RebuildHookList();
		SetModified();
	}
}

void CSetHooks::OnLvnItemchangedHooklist(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	UINT count = m_cHookList.GetSelectedCount();
	DialogEnableWindow(IDC_HOOKREMOVEBUTTON, count > 0);
	DialogEnableWindow(IDC_HOOKEDITBUTTON, count == 1);
	DialogEnableWindow(IDC_HOOKCOPYBUTTON, count == 1);
	*pResult = 0;
}

void CSetHooks::OnNMDblclkHooklist(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	OnBnClickedEditbutton();
	*pResult = 0;
}

BOOL CSetHooks::OnApply()
{
	UpdateData();
	CHooks::Instance().Save();
	SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}

void CSetHooks::OnBnClickedHookcopybutton()
{
	if (m_cHookList.GetSelectedCount() > 1)
		return;
	POSITION pos = m_cHookList.GetFirstSelectedItemPosition();
	if (pos)
	{
		CSetHooksAdv dlg;
		int index = m_cHookList.GetNextSelectedItem(pos);
		dlg.key.htype = CHooks::GetHookType((LPCTSTR)m_cHookList.GetItemText(index, 0));
		dlg.cmd.commandline = m_cHookList.GetItemText(index, 2);
		dlg.cmd.bWait = (m_cHookList.GetItemText(index, 3).Compare(_T("true"))==0);
		dlg.cmd.bShow = (m_cHookList.GetItemText(index, 4).Compare(_T("show"))==0);
		hookkey key = dlg.key;
		if (dlg.DoModal() == IDOK)
		{
			CHooks::Instance().Add(dlg.key.htype, dlg.key.path, dlg.cmd.commandline, dlg.cmd.bWait, dlg.cmd.bShow);
			RebuildHookList();
			SetModified();
		}
	}
}
