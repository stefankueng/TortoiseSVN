// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2014 - TortoiseSVN

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
#include "Globals.h"
#include "ShellUpdater.h"
#include "AppUtils.h"
#include "StringUtils.h"
#include "SetLookAndFeelPage.h"

IMPLEMENT_DYNAMIC(CSetLookAndFeelPage, ISettingsPropPage)
CSetLookAndFeelPage::CSetLookAndFeelPage()
    : ISettingsPropPage(CSetLookAndFeelPage::IDD)
    , m_bGetLockTop(FALSE)
    , m_bBlock(false)
    , m_bHideMenus(false)
    , m_bModified(false)
{
    m_regTopmenu = CRegDWORD(L"Software\\TortoiseSVN\\ContextMenuEntries", MENUCHECKOUT | MENUUPDATE | MENUCOMMIT);
    m_regTopmenuhigh = CRegDWORD(L"Software\\TortoiseSVN\\ContextMenuEntrieshigh", 0);

    m_topmenu = unsigned __int64(DWORD(m_regTopmenuhigh))<<32;
    m_topmenu |= unsigned __int64(DWORD(m_regTopmenu));

    m_regGetLockTop = CRegDWORD(L"Software\\TortoiseSVN\\GetLockTop", TRUE);
    m_bGetLockTop = m_regGetLockTop;
    m_regHideMenus = CRegDWORD(L"Software\\TortoiseSVN\\HideMenusForUnversionedItems", FALSE);
    m_bHideMenus = m_regHideMenus;

    m_regNoContextPaths = CRegString(L"Software\\TortoiseSVN\\NoContextPaths", L"");
    m_sNoContextPaths = m_regNoContextPaths;
    m_sNoContextPaths.Replace(L"\n", L"\r\n");

    m_regEnableDragContextMenu = CRegDWORD(L"Software\\TortoiseSVN\\EnableDragContextMenu", TRUE);
    m_bEnableDragContextMenu = m_regEnableDragContextMenu;
}

CSetLookAndFeelPage::~CSetLookAndFeelPage()
{
}

void CSetLookAndFeelPage::DoDataExchange(CDataExchange* pDX)
{
    ISettingsPropPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_MENULIST, m_cMenuList);
    DDX_Check(pDX, IDC_GETLOCKTOP, m_bGetLockTop);
    DDX_Check(pDX, IDC_HIDEMENUS, m_bHideMenus);
    DDX_Text(pDX, IDC_NOCONTEXTPATHS, m_sNoContextPaths);
    DDX_Check(pDX, IDC_ENABLEDRAGCONTEXTMENU, m_bEnableDragContextMenu);
}


BEGIN_MESSAGE_MAP(CSetLookAndFeelPage, ISettingsPropPage)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_MENULIST, OnLvnItemchangedMenulist)
    ON_BN_CLICKED(IDC_GETLOCKTOP, OnChange)
    ON_BN_CLICKED(IDC_HIDEMENUS, OnChange)
    ON_EN_CHANGE(IDC_NOCONTEXTPATHS, &CSetLookAndFeelPage::OnEnChangeNocontextpaths)
    ON_BN_CLICKED(IDC_ENABLEDRAGCONTEXTMENU, OnChange)
END_MESSAGE_MAP()


BOOL CSetLookAndFeelPage::OnInitDialog()
{
    ISettingsPropPage::OnInitDialog();

    m_tooltips.Create(this);
    m_tooltips.AddTool(IDC_MENULIST, IDS_SETTINGS_MENULAYOUT_TT);
    m_tooltips.AddTool(IDC_GETLOCKTOP, IDS_SETTINGS_GETLOCKTOP_TT);
    m_tooltips.AddTool(IDC_HIDEMENUS, IDS_SETTINGS_HIDEMENUS_TT);
    m_tooltips.AddTool(IDC_NOCONTEXTPATHS, IDS_SETTINGS_EXCLUDECONTEXTLIST_TT);
    m_tooltips.AddTool(IDC_ENABLEDRAGCONTEXTMENU, IDS_SETTINGS_ENABLEDRAGCONTEXTMENU_TT);

    m_cMenuList.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

    m_cMenuList.DeleteAllItems();
    int c = ((CHeaderCtrl*)(m_cMenuList.GetDlgItem(0)))->GetItemCount()-1;
    while (c>=0)
        m_cMenuList.DeleteColumn(c--);
    m_cMenuList.InsertColumn(0, L"");

    SetWindowTheme(m_cMenuList.GetSafeHwnd(), L"Explorer", NULL);

    m_cMenuList.SetRedraw(false);

    m_imgList.Create(16, 16, ILC_COLOR16 | ILC_MASK, 4, 1);

    m_bBlock = true;

    InsertItem(IDS_MENUCHECKOUT, IDI_CHECKOUT, MENUCHECKOUT);
    InsertItem(IDS_MENUUPDATE, IDI_UPDATE, MENUUPDATE);
    InsertItem(IDS_MENUCOMMIT, IDI_COMMIT, MENUCOMMIT);
    InsertItem(IDS_MENUDIFF, IDI_DIFF, MENUDIFF);
    InsertItem(IDS_MENUDIFFLATER, IDI_DIFF, MENUDIFFLATER);
    InsertItem(IDS_MENUDIFFNOW, IDI_DIFF, MENUDIFFNOW);
    InsertItem(IDS_MENUPREVDIFF, IDI_DIFF, MENUPREVDIFF);
    InsertItem(IDS_MENUURLDIFF, IDI_DIFF, MENUURLDIFF);
    InsertItem(IDS_MENULOG, IDI_LOG, MENULOG);
    InsertItem(IDS_MENUREPOBROWSE, IDI_REPOBROWSE, MENUREPOBROWSE);
    InsertItem(IDS_MENUSHOWCHANGED, IDI_SHOWCHANGED, MENUSHOWCHANGED);
    InsertItem(IDS_MENUREVISIONGRAPH, IDI_REVISIONGRAPH, MENUREVISIONGRAPH);
    InsertItem(IDS_MENUCONFLICT, IDI_CONFLICT, MENUCONFLICTEDITOR);
    InsertItem(IDS_MENURESOLVE, IDI_RESOLVE, MENURESOLVE);
    InsertItem(IDS_MENUUPDATEEXT, IDI_UPDATE, MENUUPDATEEXT);
    InsertItem(IDS_MENURENAME, IDI_RENAME, MENURENAME);
    InsertItem(IDS_MENUREMOVE, IDI_DELETE, MENUREMOVE);
    InsertItem(IDS_MENUREVERT, IDI_REVERT, MENUREVERT);
    InsertItem(IDS_MENUDELUNVERSIONED, IDI_DELUNVERSIONED, MENUDELUNVERSIONED);
    InsertItem(IDS_MENUCLEANUP, IDI_CLEANUP, MENUCLEANUP);
    InsertItem(IDS_MENU_LOCK, IDI_LOCK, MENULOCK);
    InsertItem(IDS_MENU_UNLOCK, IDI_UNLOCK, MENUUNLOCK);
    InsertItem(IDS_MENUBRANCH, IDI_COPY, MENUCOPY);
    InsertItem(IDS_MENUSWITCH, IDI_SWITCH, MENUSWITCH);
    InsertItem(IDS_MENUMERGE, IDI_MERGE, MENUMERGE);
    InsertItem(IDS_MENUMERGEALL, IDI_MERGE, MENUMERGEALL);
    InsertItem(IDS_MENUEXPORT, IDI_EXPORT, MENUEXPORT);
    InsertItem(IDS_MENURELOCATE, IDI_RELOCATE, MENURELOCATE);
    InsertItem(IDS_MENUCREATEREPOS, IDI_CREATEREPOS, MENUCREATEREPOS);
    InsertItem(IDS_MENUADD, IDI_ADD, MENUADD);
    InsertItem(IDS_MENUIMPORT, IDI_IMPORT, MENUIMPORT);
    InsertItem(IDS_MENUBLAME, IDI_BLAME, MENUBLAME);
    InsertItem(IDS_MENUCOPYURL, IDI_COPYURL, MENUCOPYURL);
    InsertItem(IDS_MENUIGNORE, IDI_IGNORE, MENUIGNORE);
    InsertItem(IDS_MENUCREATEPATCH, IDI_CREATEPATCH, MENUCREATEPATCH);
    InsertItem(IDS_MENUAPPLYPATCH, IDI_PATCH, MENUAPPLYPATCH);
    InsertItem(IDS_MENUPROPERTIES, IDI_PROPERTIES, MENUPROPERTIES);
    InsertItem(IDS_MENUCLIPPASTE, IDI_CLIPPASTE, MENUCLIPPASTE);
    m_bBlock = false;

    m_cMenuList.SetImageList(&m_imgList, LVSIL_SMALL);
    int mincol = 0;
    int maxcol = ((CHeaderCtrl*)(m_cMenuList.GetDlgItem(0)))->GetItemCount()-1;
    int col;
    for (col = mincol; col <= maxcol; col++)
    {
        m_cMenuList.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
    }
    m_cMenuList.SetRedraw(true);

    UpdateData(FALSE);

    return TRUE;
}

BOOL CSetLookAndFeelPage::PreTranslateMessage(MSG* pMsg)
{
    m_tooltips.RelayEvent(pMsg);
    return ISettingsPropPage::PreTranslateMessage(pMsg);
}

BOOL CSetLookAndFeelPage::OnApply()
{
    UpdateData();
    Store ((DWORD)(m_topmenu & 0xFFFFFFFF), m_regTopmenu);
    Store ((DWORD)(m_topmenu >> 32), m_regTopmenuhigh);
    Store (m_bGetLockTop, m_regGetLockTop);
    Store (m_bHideMenus, m_regHideMenus);
    Store (m_bEnableDragContextMenu, m_regEnableDragContextMenu);

    m_sNoContextPaths.Remove('\r');
    if (m_sNoContextPaths.Right(1).Compare(L"\n")!=0)
        m_sNoContextPaths += L"\n";

    Store (m_sNoContextPaths, m_regNoContextPaths);

    m_sNoContextPaths.Replace(L"\n", L"\r\n");
    SetModified(FALSE);
    return ISettingsPropPage::OnApply();
}

void CSetLookAndFeelPage::InsertItem(UINT nTextID, UINT nIconID, unsigned __int64 dwFlags)
{
    HICON hIcon = reinterpret_cast<HICON>(::LoadImage(AfxGetResourceHandle(),
        MAKEINTRESOURCE(nIconID),
        IMAGE_ICON, 16, 16, LR_LOADTRANSPARENT ));
    int nImage = m_imgList.Add(hIcon);
    CString temp;
    temp.LoadString(nTextID);
    CStringUtils::RemoveAccelerators(temp);
    int nIndex = m_cMenuList.GetItemCount();
    m_cMenuList.InsertItem(nIndex, temp, nImage);
    m_cMenuList.SetCheck(nIndex, !!(m_topmenu & dwFlags));
}

void CSetLookAndFeelPage::OnLvnItemchangedMenulist(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
    if (m_bBlock)
        return;
    SetModified(TRUE);
    if (m_cMenuList.GetItemCount() > 0)
    {
        int i=0;
        m_topmenu = 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUCHECKOUT : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUUPDATE : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUCOMMIT : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUDIFF : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUDIFFLATER : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUDIFFNOW : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUPREVDIFF : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUURLDIFF : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENULOG : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUREPOBROWSE : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUSHOWCHANGED : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUREVISIONGRAPH : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUCONFLICTEDITOR : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENURESOLVE : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUUPDATEEXT : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENURENAME : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUREMOVE : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUREVERT : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUDELUNVERSIONED : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUCLEANUP : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENULOCK : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUUNLOCK : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUCOPY : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUSWITCH : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUMERGE : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUMERGEALL : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUEXPORT : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENURELOCATE : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUCREATEREPOS : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUADD : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUIMPORT : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUBLAME : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUCOPYURL : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUIGNORE : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUCREATEPATCH : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUAPPLYPATCH : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUPROPERTIES : 0;
        m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUCLIPPASTE : 0;
    }
    *pResult = 0;
}

void CSetLookAndFeelPage::OnChange()
{
    SetModified();
}

void CSetLookAndFeelPage::OnEnChangeNocontextpaths()
{
    SetModified();
}

