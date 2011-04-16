// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2011 - TortoiseSVN

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
#include "SVNProperties.h"
#include "MessageBox.h"
#include "TortoiseProc.h"
#include "EditPropertiesDlg.h"
#include "UnicodeUtils.h"
#include "PathUtils.h"
#include "AppUtils.h"
#include "StringUtils.h"
#include "ProgressDlg.h"
#include "InputLogDlg.h"
#include "auto_buffer.h"
#include "JobScheduler.h"
#include "AsyncCall.h"
#include "IconMenu.h"

#include "EditPropertyValueDlg.h"
#include "EditPropExecutable.h"
#include "EditPropNeedsLock.h"
#include "EditPropMimeType.h"
#include "EditPropBugtraq.h"
#include "EditPropEOL.h"
#include "EditPropKeywords.h"
#include "EditPropExternals.h"
#include "EditPropTSVNSizes.h"
#include "EditPropTSVNLang.h"


#define ID_CMD_PROP_SAVEVALUE   1
#define ID_CMD_PROP_REMOVE      2
#define ID_CMD_PROP_EDIT        3


IMPLEMENT_DYNAMIC(CEditPropertiesDlg, CResizableStandAloneDialog)

CEditPropertiesDlg::CEditPropertiesDlg(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CEditPropertiesDlg::IDD, pParent)
    , m_bRecursive(FALSE)
    , m_bChanged(false)
    , m_revision(SVNRev::REV_WC)
    , m_bRevProps(false)
    , m_pProjectProperties(NULL)
{
}

CEditPropertiesDlg::~CEditPropertiesDlg()
{
}

void CEditPropertiesDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDITPROPLIST, m_propList);
    DDX_Control(pDX, IDC_PROPPATH, m_PropPath);
    DDX_Control(pDX, IDC_ADDPROPS, m_btnNew);
    DDX_Control(pDX, IDC_EDITPROPS, m_btnEdit);
}


BEGIN_MESSAGE_MAP(CEditPropertiesDlg, CResizableStandAloneDialog)
    ON_BN_CLICKED(IDHELP, &CEditPropertiesDlg::OnBnClickedHelp)
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_EDITPROPLIST, &CEditPropertiesDlg::OnNMCustomdrawEditproplist)
    ON_BN_CLICKED(IDC_REMOVEPROPS, &CEditPropertiesDlg::OnBnClickedRemoveProps)
    ON_BN_CLICKED(IDC_EDITPROPS, &CEditPropertiesDlg::OnBnClickedEditprops)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_EDITPROPLIST, &CEditPropertiesDlg::OnLvnItemchangedEditproplist)
    ON_NOTIFY(NM_DBLCLK, IDC_EDITPROPLIST, &CEditPropertiesDlg::OnNMDblclkEditproplist)
    ON_BN_CLICKED(IDC_SAVEPROP, &CEditPropertiesDlg::OnBnClickedSaveprop)
    ON_BN_CLICKED(IDC_ADDPROPS, &CEditPropertiesDlg::OnBnClickedAddprops)
    ON_BN_CLICKED(IDC_EXPORT, &CEditPropertiesDlg::OnBnClickedExport)
    ON_BN_CLICKED(IDC_IMPORT, &CEditPropertiesDlg::OnBnClickedImport)
    ON_WM_SETCURSOR()
    ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

void CEditPropertiesDlg::OnBnClickedHelp()
{
    OnHelp();
}

BOOL CEditPropertiesDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();

    ExtendFrameIntoClientArea(IDC_GROUP, IDC_GROUP, IDC_GROUP, IDC_GROUP);
    m_aeroControls.SubclassControl(this, IDOK);
    m_aeroControls.SubclassControl(this, IDHELP);

    // fill in the path edit control
    if (m_pathlist.GetCount() == 1)
    {
        if (m_pathlist[0].IsUrl())
            SetDlgItemText(IDC_PROPPATH, m_pathlist[0].GetSVNPathString());
        else
            SetDlgItemText(IDC_PROPPATH, m_pathlist[0].GetWinPathString());
    }
    else
    {
        CString sTemp;
        sTemp.Format(IDS_EDITPROPS_NUMPATHS, m_pathlist.GetCount());
        SetDlgItemText(IDC_PROPPATH, sTemp);
    }

    SetWindowTheme(m_propList.GetSafeHwnd(), L"Explorer", NULL);

    // initialize the property list control
    m_propList.DeleteAllItems();
    DWORD exStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER;
    m_propList.SetExtendedStyle(exStyle);
    int c = ((CHeaderCtrl*)(m_propList.GetDlgItem(0)))->GetItemCount()-1;
    while (c>=0)
        m_propList.DeleteColumn(c--);
    CString temp;
    temp.LoadString(IDS_PROPPROPERTY);
    m_propList.InsertColumn(0, temp);
    temp.LoadString(IDS_PROPVALUE);
    m_propList.InsertColumn(1, temp);
    m_propList.SetRedraw(false);

    if (m_bRevProps)
    {
        GetDlgItem(IDC_IMPORT)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_EXPORT)->ShowWindow(SW_HIDE);
    }

    m_newMenu.LoadMenu(IDR_PROPNEWMENU);
    m_btnNew.m_hMenu = m_newMenu.GetSubMenu(0)->GetSafeHmenu();
    m_btnNew.m_bOSMenu = TRUE;
    m_btnNew.m_bRightArrow = TRUE;

    m_editMenu.LoadMenu(IDR_PROPEDITMENU);
    m_btnEdit.m_hMenu = m_editMenu.GetSubMenu(0)->GetSafeHmenu();
    m_btnEdit.m_bOSMenu = TRUE;
    m_btnEdit.m_bRightArrow = TRUE;
    m_btnEdit.m_bDefaultClick = TRUE;

    m_tooltips.Create(this);
    m_tooltips.AddTool(IDC_IMPORT, IDS_PROP_TT_IMPORT);
    m_tooltips.AddTool(IDC_EXPORT,  IDS_PROP_TT_EXPORT);
    m_tooltips.AddTool(IDC_SAVEPROP,  IDS_PROP_TT_SAVE);
    m_tooltips.AddTool(IDC_REMOVEPROPS,  IDS_PROP_TT_REMOVE);
    m_tooltips.AddTool(IDC_EDITPROPS,  IDS_PROP_TT_EDIT);
    m_tooltips.AddTool(IDC_ADDPROPS,  IDS_PROP_TT_ADD);

    AddAnchor(IDC_GROUP, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_PROPPATH, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_EDITPROPLIST, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_IMPORT, BOTTOM_RIGHT);
    AddAnchor(IDC_EXPORT, BOTTOM_RIGHT);
    AddAnchor(IDC_SAVEPROP, BOTTOM_RIGHT);
    AddAnchor(IDC_REMOVEPROPS, BOTTOM_RIGHT);
    AddAnchor(IDC_EDITPROPS, BOTTOM_RIGHT);
    AddAnchor(IDC_ADDPROPS, BOTTOM_RIGHT);
    AddAnchor(IDOK, BOTTOM_RIGHT);
    AddAnchor(IDHELP, BOTTOM_RIGHT);
    if (GetExplorerHWND())
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    EnableSaveRestore(_T("EditPropertiesDlg"));

    InterlockedExchange(&m_bThreadRunning, TRUE);
    if (AfxBeginThread(PropsThreadEntry, this)==NULL)
    {
        InterlockedExchange(&m_bThreadRunning, FALSE);
        OnCantStartThread();
    }
    GetDlgItem(IDC_EDITPROPLIST)->SetFocus();

    return FALSE;
}

void CEditPropertiesDlg::Refresh()
{
    if (m_bThreadRunning)
        return;
    m_propList.DeleteAllItems();
    InterlockedExchange(&m_bThreadRunning, TRUE);
    if (AfxBeginThread(PropsThreadEntry, this)==NULL)
    {
        InterlockedExchange(&m_bThreadRunning, FALSE);
        OnCantStartThread();
    }
}

UINT CEditPropertiesDlg::PropsThreadEntry(LPVOID pVoid)
{
    return ((CEditPropertiesDlg*)pVoid)->PropsThread();
}

void CEditPropertiesDlg::ReadProperties (int first, int last)
{
    SVNProperties props (m_revision, m_bRevProps);
    for (int i=first; i < last; ++i)
    {
        props.SetFilePath(m_pathlist[i]);
        for (int p=0; p<props.GetCount(); ++p)
        {
            std::string prop_str = props.GetItemName(p);
            std::string prop_value = props.GetItemValue(p);

            async::CCriticalSectionLock lock (m_mutex);

            IT it = m_properties.find(prop_str);
            if (it != m_properties.end())
            {
                it->second.count++;
                if (it->second.value.compare(prop_value)!=0)
                    it->second.allthesamevalue = false;
            }
            else
            {
                it = m_properties.insert(it, std::make_pair(prop_str, PropValue()));
                tstring value = CUnicodeUtils::StdGetUnicode(prop_value);
                it->second.value = prop_value;
                CString stemp = value.c_str();
                stemp.Replace('\n', ' ');
                stemp.Remove(_T('\r'));
                it->second.value_without_newlines = tstring((LPCTSTR)stemp);
                it->second.count = 1;
                it->second.allthesamevalue = true;
                if (SVNProperties::IsBinary(prop_value))
                    it->second.isbinary = true;
            }
        }
    }
}

UINT CEditPropertiesDlg::PropsThread()
{
    enum {CHUNK_SIZE = 100};

    // get all properties in multiple threads
    async::CJobScheduler jobs (0, async::CJobScheduler::GetHWThreadCount());

    m_properties.clear();
    for (int i=0; i < m_pathlist.GetCount(); i += CHUNK_SIZE)
        new async::CAsyncCall ( this
                              , &CEditPropertiesDlg::ReadProperties
                              , i
                              , min (m_pathlist.GetCount(), i + CHUNK_SIZE)
                              , &jobs);

    jobs.WaitForEmptyQueue();

    // fill the property list control with the gathered information
    int index=0;
    m_propList.SetRedraw(FALSE);
    for (IT it = m_properties.begin(); it != m_properties.end(); ++it)
    {
        m_propList.InsertItem(index, CUnicodeUtils::StdGetUnicode(it->first).c_str());
        if (it->second.isbinary)
        {
            m_propList.SetItemText(index, 1, CString(MAKEINTRESOURCE(IDS_EDITPROPS_BINVALUE)));
            m_propList.SetItemData(index, FALSE);
        }
        else if (it->second.count != m_pathlist.GetCount())
        {
            // if the property values are the same for all paths they're set
            // but they're not set for *all* paths, then we show the entry grayed out
            m_propList.SetItemText(index, 1, it->second.value_without_newlines.c_str());
            m_propList.SetItemData(index, FALSE);
        }
        else if (it->second.allthesamevalue)
        {
            // if the property values are the same for all paths,
            // we show the value
            m_propList.SetItemText(index, 1, it->second.value_without_newlines.c_str());
            m_propList.SetItemData(index, TRUE);
        }
        else
        {
            // if the property values aren't the same for all paths
            // then we show "values are different" instead of the value
            CString sTemp(MAKEINTRESOURCE(IDS_EDITPROPS_DIFFERENTPROPVALUES));
            m_propList.SetItemText(index, 1, sTemp);
            m_propList.SetItemData(index, FALSE);
        }
        if (index == 0)
        {
            // select the first entry
            m_propList.SetItemState(index, LVIS_SELECTED, LVIS_SELECTED);
            m_propList.SetSelectionMark(index);
        }
        index++;
    }
    int maxcol = ((CHeaderCtrl*)(m_propList.GetDlgItem(0)))->GetItemCount()-1;
    for (int col = 0; col <= maxcol; col++)
    {
        m_propList.SetColumnWidth(col, LVSCW_AUTOSIZE_USEHEADER);
    }

    InterlockedExchange(&m_bThreadRunning, FALSE);
    m_propList.SetRedraw(TRUE);
    if (!PathIsDirectory(m_pathlist[0].GetWinPath()))
    {
        // properties for one or more files:
        // remove the menu entries which are only useful for folders
        RemoveMenu(m_btnNew.m_hMenu, ID_NEW_EXTERNALS, MF_BYCOMMAND);
        RemoveMenu(m_btnNew.m_hMenu, ID_NEW_LOGSIZES, MF_BYCOMMAND);
        RemoveMenu(m_btnNew.m_hMenu, ID_NEW_BUGTRAQ, MF_BYCOMMAND);
        RemoveMenu(m_btnNew.m_hMenu, ID_NEW_LANGUAGES, MF_BYCOMMAND);
    }
    return 0;
}

void CEditPropertiesDlg::OnNMCustomdrawEditproplist(NMHDR *pNMHDR, LRESULT *pResult)
{
    NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );
    // Take the default processing unless we set this to something else below.
    *pResult = CDRF_DODEFAULT;

    if (m_bThreadRunning)
        return;

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

        if ((int)pLVCD->nmcd.dwItemSpec > m_propList.GetItemCount())
            return;

        COLORREF crText = GetSysColor(COLOR_WINDOWTEXT);
        if (m_propList.GetItemData (static_cast<int>(pLVCD->nmcd.dwItemSpec))==FALSE)
            crText = GetSysColor(COLOR_GRAYTEXT);
        // Store the color back in the NMLVCUSTOMDRAW struct.
        pLVCD->clrText = crText;
    }

}

void CEditPropertiesDlg::OnLvnItemchangedEditproplist(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
    *pResult = 0;
    // disable the "remove" button if nothing
    // is selected, enable it otherwise
    int selCount = m_propList.GetSelectedCount();
    DialogEnableWindow(IDC_EXPORT, selCount >= 1);
    DialogEnableWindow(IDC_REMOVEPROPS, selCount >= 1);
    DialogEnableWindow(IDC_SAVEPROP, selCount == 1);
    DialogEnableWindow(IDC_EDITPROPS, selCount == 1);

    *pResult = 0;
}

void CEditPropertiesDlg::OnBnClickedRemoveProps()
{
    RemoveProps();
}

void CEditPropertiesDlg::OnNMDblclkEditproplist(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
    if (m_propList.GetSelectedCount() == 1)
        EditProps(true);

    *pResult = 0;
}

void CEditPropertiesDlg::OnBnClickedEditprops()
{
    switch (m_btnEdit.m_nMenuResult)
    {
    case ID_EDIT_ADVANCED:
        EditProps(false);
        break;
    default:
        EditProps(true);
        break;
    }
}

void CEditPropertiesDlg::OnBnClickedAddprops()
{
    switch (m_btnNew.m_nMenuResult)
    {
    case ID_NEW_EXECUTABLE:
        EditProps(true, SVN_PROP_EXECUTABLE, true);
        break;
    case ID_NEW_NEEDSLOCK:
        EditProps(true, SVN_PROP_NEEDS_LOCK, true);
        break;
    case ID_NEW_MIMETYPE:
        EditProps(true, SVN_PROP_MIME_TYPE, true);
        break;
    case ID_NEW_BUGTRAQ:
        EditProps(true, "bugtraq:", true);
        break;
    case ID_NEW_EOL:
        EditProps(true, SVN_PROP_EOL_STYLE, true);
        break;
    case ID_NEW_KEYWORDS:
        EditProps(true, SVN_PROP_KEYWORDS, true);
        break;
    case ID_NEW_EXTERNALS:
        EditProps(true, SVN_PROP_EXTERNALS, true);
        break;
    case ID_NEW_LOGSIZES:
        EditProps(true, "tsvn:log", true);
        break;
    case ID_NEW_LANGUAGES:
        EditProps(true, "tsvn:lang", true);
        break;
    case ID_NEW_ADVANCED:
    default:
        EditProps(false, "", true);
        break;
    }
}

EditPropBase * CEditPropertiesDlg::GetPropDialog(bool bDefault, const std::string& sName)
{
    if (!bDefault)
    {
        return new CEditPropertyValueDlg(this);
    }

    EditPropBase * dlg = NULL;
    if (sName.compare(SVN_PROP_EXECUTABLE) == 0)
        dlg = new CEditPropExecutable(this);
    else if (sName.compare(SVN_PROP_NEEDS_LOCK) == 0)
        dlg = new CEditPropNeedsLock(this);
    else if (sName.compare(SVN_PROP_MIME_TYPE) == 0)
        dlg = new CEditPropMimeType(this);
    else if (sName.substr(0, 8).compare("bugtraq:") == 0)
        dlg = new CEditPropBugtraq(this);
    else if (sName.compare(SVN_PROP_EOL_STYLE) == 0)
        dlg = new CEditPropEOL(this);
    else if (sName.compare(SVN_PROP_KEYWORDS) == 0)
        dlg = new CEditPropKeywords(this);
    else if (sName.compare(SVN_PROP_EXTERNALS) == 0)
        dlg = new CEditPropExternals(this);
    else if ((sName.compare(PROJECTPROPNAME_LOGMINSIZE) == 0) ||
        (sName.compare(PROJECTPROPNAME_LOCKMSGMINSIZE) == 0) ||
        (sName.compare(PROJECTPROPNAME_LOGWIDTHLINE) == 0) ||
        (sName.compare("tsvn:log") == 0))
        dlg = new CEditPropTSVNSizes(this);
    else if ((sName.compare(PROJECTPROPNAME_LOGFILELISTLANG) == 0) ||
        (sName.compare(PROJECTPROPNAME_PROJECTLANGUAGE) == 0) ||
        (sName.compare("tsvn:lang") == 0))
        dlg = new CEditPropTSVNLang(this);
    else
        dlg = new CEditPropertyValueDlg(this);

    return dlg;
}

void CEditPropertiesDlg::EditProps(bool bDefault, const std::string& propName /* = "" */, bool bAdd /* = false*/)
{
    m_tooltips.Pop();   // hide the tooltips
    int selIndex = m_propList.GetSelectionMark();

    EditPropBase * dlg = NULL;
    std::string sName = propName;

    if ((!bAdd)&&(selIndex >= 0)&&(m_propList.GetSelectedCount()))
    {
        sName = CUnicodeUtils::StdGetUTF8((LPCTSTR)m_propList.GetItemText(selIndex, 0));
        dlg = GetPropDialog(bDefault, sName);
        dlg->SetProperties(m_properties);
        PropValue& prop = m_properties[sName];
        dlg->SetPropertyName(sName);
        if (prop.allthesamevalue)
            dlg->SetPropertyValue(prop.value);
        dlg->SetPathList(m_pathlist);
        dlg->SetDialogTitle(CString(MAKEINTRESOURCE(IDS_EDITPROPS_EDITTITLE)));
    }
    else
    {
        dlg = GetPropDialog(bDefault, propName);
        dlg->SetProperties(m_properties);
        if (propName.size())
            dlg->SetPropertyName(sName);
        dlg->SetPathList(m_pathlist);
        if (m_properties.find(sName) != m_properties.end())
        {
            // the property already exists: switch to "edit" instead of "add"
            PropValue& prop = m_properties[sName];
            dlg->SetPropertyName(sName);
            if (prop.allthesamevalue)
                dlg->SetPropertyValue(prop.value);
        }
        dlg->SetDialogTitle(CString(MAKEINTRESOURCE(IDS_EDITPROPS_ADDTITLE)));
    }

    if (m_pathlist.GetCount() > 1)
        dlg->SetMultiple();
    else if (m_pathlist.GetCount() == 1)
    {
        if (PathIsDirectory(m_pathlist[0].GetWinPath()))
            dlg->SetFolder();
    }

    dlg->RevProps(m_bRevProps);
    if ( dlg->DoModal()==IDOK )
    {
        if(dlg->IsChanged() || dlg->GetRecursive())
        {
            sName = dlg->GetPropertyName();
            TProperties dlgprops = dlg->GetProperties();
            if (!sName.empty() || (dlg->HasMultipleProperties()&&dlgprops.size()))
            {
                CString sMsg;
                bool bDoIt = true;
                if (!m_bRevProps&&(m_pathlist.GetCount())&&(m_pathlist[0].IsUrl()))
                {
                    CInputLogDlg input(this);
                    input.SetUUID(m_sUUID);
                    input.SetProjectProperties(m_pProjectProperties, PROJECTPROPNAME_LOGTEMPLATEPROPSET);
                    CString sHint;
                    sHint.FormatMessage(IDS_INPUT_EDITPROP, sName.c_str(), (LPCTSTR)(m_pathlist[0].GetSVNPathString()));
                    input.SetActionText(sHint);
                    if (input.DoModal() == IDOK)
                    {
                        sMsg = input.GetLogMessage();
                    }
                    else
                        bDoIt = false;
                }
                if (bDoIt)
                {
                    CProgressDlg prog;
                    CString sTemp;
                    sTemp.LoadString(IDS_SETPROPTITLE);
                    prog.SetTitle(sTemp);
                    sTemp.LoadString(IDS_PROPWAITCANCEL);
                    prog.SetCancelMsg(sTemp);
                    prog.SetTime(TRUE);
                    prog.SetShowProgressBar(TRUE);
                    prog.ShowModeless(m_hWnd);
                    for (int i=0; i<m_pathlist.GetCount(); ++i)
                    {
                        prog.SetLine(1, m_pathlist[i].GetWinPath(), true);
                        SVNProperties props(m_pathlist[i], m_revision, m_bRevProps);
                        props.SetProgressDlg(&prog);
                        if (dlg->HasMultipleProperties())
                        {
                            for (IT propsit = dlgprops.begin(); propsit != dlgprops.end(); ++propsit)
                            {
                                prog.SetLine(1, CUnicodeUtils::StdGetUnicode(propsit->first).c_str());
                                BOOL ret = FALSE;
                                if (propsit->second.remove)
                                    ret = props.Remove(propsit->first, dlg->GetRecursive() ? svn_depth_infinity : svn_depth_empty, sMsg);
                                else
                                    ret = props.Add(propsit->first, SVNProperties::IsBinary(propsit->second.value) ? propsit->second.value : propsit->second.value.c_str(),
                                                    false, dlg->GetRecursive() ? svn_depth_infinity : svn_depth_empty, sMsg);
                                if (!ret)
                                {
                                    prog.Stop();
                                    props.ShowErrorDialog(m_hWnd, props.GetPath().GetDirectory());
                                    if (props.GetSVNError()->apr_err == SVN_ERR_CANCELLED)
                                        break;
                                    prog.ShowModeless(m_hWnd);
                                }
                                else
                                {
                                    m_bChanged = true;
                                    // bump the revision number since we just did a commit
                                    if (!m_bRevProps && m_revision.IsNumber())
                                        m_revision = LONG(m_revision)+1;
                                }
                            }
                        }
                        else
                        {
                            bool bRemove = false;
                            if ((sName.substr(0, 4).compare("svn:") == 0) ||
                                (sName.substr(0, 5).compare("tsvn:") == 0) ||
                                (sName.substr(0, 10).compare("webviewer:") == 0))
                            {
                                if (dlg->GetPropertyValue().size() == 0)
                                    bRemove = true;
                            }
                            BOOL ret = FALSE;
                            if (bRemove)
                                ret = props.Remove(sName, dlg->GetRecursive() ? svn_depth_infinity : svn_depth_empty, sMsg);
                            else
                                ret = props.Add(sName, dlg->IsBinary() ? dlg->GetPropertyValue() : dlg->GetPropertyValue().c_str(),
                                                false, dlg->GetRecursive() ? svn_depth_infinity : svn_depth_empty, sMsg);
                            if (!ret)
                            {
                                prog.Stop();
                                props.ShowErrorDialog(m_hWnd, props.GetPath().GetDirectory());
                                prog.ShowModeless(m_hWnd);
                            }
                            else
                            {
                                m_bChanged = true;
                                // bump the revision number since we just did a commit
                                if (!m_bRevProps && m_revision.IsNumber())
                                    m_revision = LONG(m_revision)+1;
                            }
                        }
                    }
                    prog.Stop();
                    Refresh();
                }
            }
        }
    }

    delete dlg;
}

void CEditPropertiesDlg::RemoveProps()
{
    CString sLogMsg;
    POSITION pos = m_propList.GetFirstSelectedItemPosition();
    while ( pos )
    {
        int selIndex = m_propList.GetNextSelectedItem(pos);

        bool bRecurse = false;
        std::string sName = CUnicodeUtils::StdGetUTF8((LPCTSTR)m_propList.GetItemText(selIndex, 0));
        tstring sUName = CUnicodeUtils::StdGetUnicode(sName);
        if (m_pathlist[0].IsUrl())
        {
            CInputLogDlg input(this);
            input.SetUUID(m_sUUID);
            input.SetProjectProperties(m_pProjectProperties, PROJECTPROPNAME_LOGTEMPLATEPROPSET);
            CString sHint;
            sHint.FormatMessage(IDS_INPUT_REMOVEPROP, sUName.c_str(), (LPCTSTR)(m_pathlist[0].GetSVNPathString()));
            input.SetActionText(sHint);
            if (input.DoModal() != IDOK)
                return;
            sLogMsg = input.GetLogMessage();
        }
        CString sQuestion;
        sQuestion.Format(IDS_EDITPROPS_RECURSIVEREMOVEQUESTION, sUName.c_str());
        CString sRecursive(MAKEINTRESOURCE(IDS_EDITPROPS_RECURSIVE));
        CString sNotRecursive(MAKEINTRESOURCE(IDS_EDITPROPS_NOTRECURSIVE));
        CString sCancel(MAKEINTRESOURCE(IDS_EDITPROPS_CANCEL));

        if ((m_pathlist.GetCount()>1)||((m_pathlist.GetCount()==1)&&(PathIsDirectory(m_pathlist[0].GetWinPath()))))
        {
            int ret = TSVNMessageBox(m_hWnd, sQuestion, _T("TortoiseSVN"), MB_DEFBUTTON1|MB_ICONQUESTION, sRecursive, sNotRecursive, sCancel);
            if (ret == IDCUSTOM1)
                bRecurse = true;
            else if (ret == IDCUSTOM2)
                bRecurse = false;
            else
                break;
        }

        CProgressDlg prog;
        CString sTemp;
        sTemp.LoadString(IDS_SETPROPTITLE);
        prog.SetTitle(sTemp);
        sTemp.LoadString(IDS_PROPWAITCANCEL);
        prog.SetCancelMsg(sTemp);
        prog.SetTime(TRUE);
        prog.SetShowProgressBar(TRUE);
        prog.ShowModeless(m_hWnd);
        for (int i=0; i<m_pathlist.GetCount(); ++i)
        {
            prog.SetLine(1, m_pathlist[i].GetWinPath(), true);
            SVNProperties props(m_pathlist[i], m_revision, m_bRevProps);
            props.SetProgressDlg(&prog);
            if (!props.Remove(sName, bRecurse ? svn_depth_infinity : svn_depth_empty, (LPCTSTR)sLogMsg))
            {
                props.ShowErrorDialog(m_hWnd, props.GetPath().GetDirectory());
            }
            else
            {
                m_bChanged = true;
                if (m_revision.IsNumber())
                    m_revision = LONG(m_revision)+1;
            }
        }
        prog.Stop();
    }
    DialogEnableWindow(IDC_REMOVEPROPS, FALSE);
    DialogEnableWindow(IDC_SAVEPROP, FALSE);
    Refresh();
}

void CEditPropertiesDlg::OnOK()
{
    if (m_bThreadRunning)
        return;
    CStandAloneDialogTmpl<CResizableDialog>::OnOK();
}

void CEditPropertiesDlg::OnCancel()
{
    if (m_bThreadRunning)
        return;
    CStandAloneDialogTmpl<CResizableDialog>::OnCancel();
}

BOOL CEditPropertiesDlg::PreTranslateMessage(MSG* pMsg)
{
    m_tooltips.RelayEvent(pMsg);

    if (pMsg->message == WM_KEYDOWN)
    {
        switch (pMsg->wParam)
        {
        case VK_F5:
            {
                if (m_bThreadRunning)
                    return __super::PreTranslateMessage(pMsg);
                Refresh();
            }
            break;
        case VK_RETURN:
            if (OnEnterPressed())
                return TRUE;
            break;
        case VK_DELETE:
            {
                if (m_bThreadRunning)
                    return __super::PreTranslateMessage(pMsg);
                PostMessage(WM_COMMAND, MAKEWPARAM(IDC_REMOVEPROPS,BN_CLICKED));
            }
            break;
        default:
            break;
        }
    }

    return __super::PreTranslateMessage(pMsg);
}

void CEditPropertiesDlg::OnBnClickedSaveprop()
{
    m_tooltips.Pop();   // hide the tooltips
    int selIndex = m_propList.GetSelectionMark();

    std::string sName;
    std::string sValue;
    if ((selIndex >= 0)&&(m_propList.GetSelectedCount()))
    {
        sName = CUnicodeUtils::StdGetUTF8 ((LPCTSTR)m_propList.GetItemText(selIndex, 0));
        PropValue& prop = m_properties[sName];
        sValue = prop.value.c_str();
        if (prop.allthesamevalue)
        {
            CString savePath;
            if (!CAppUtils::FileOpenSave(savePath, NULL, IDS_REPOBROWSE_SAVEAS, 0, false, m_hWnd))
                return;

            FILE * stream;
            errno_t err = 0;
            if ((err = _tfopen_s(&stream, (LPCTSTR)savePath, _T("wbS")))==0)
            {
                fwrite(prop.value.c_str(), sizeof(char), prop.value.size(), stream);
                fclose(stream);
            }
            else
            {
                TCHAR strErr[4096] = {0};
                _tcserror_s(strErr, 4096, err);
                ::MessageBox(m_hWnd, strErr, _T("TortoiseSVN"), MB_ICONERROR);
            }
        }
    }
}

void CEditPropertiesDlg::OnBnClickedExport()
{
    m_tooltips.Pop();   // hide the tooltips
    if (m_propList.GetSelectedCount() == 0)
        return;

    CString savePath;
    if (!CAppUtils::FileOpenSave(savePath, NULL, IDS_REPOBROWSE_SAVEAS, IDS_PROPSFILEFILTER, false, m_hWnd))
        return;

    if (CPathUtils::GetFileExtFromPath(savePath).Compare(_T(".svnprops")))
    {
        // append the default ".svnprops" extension since the user did not specify it himself
        savePath += _T(".svnprops");
    }
    // we save the list of selected properties not in a text file but in our own
    // binary format. That's because properties can be binary themselves, a text
    // or xml file just won't do it properly.
    CString sName;
    CString sValue;
    FILE * stream;
    errno_t err = 0;

    if ((err = _tfopen_s(&stream, (LPCTSTR)savePath, _T("wbS")))==0)
    {
        POSITION pos = m_propList.GetFirstSelectedItemPosition();
        int len = m_propList.GetSelectedCount();
        fwrite(&len, sizeof(int), 1, stream);                                       // number of properties
        while (pos)
        {
            int index = m_propList.GetNextSelectedItem(pos);
            sName = m_propList.GetItemText(index, 0);
            PropValue& prop = m_properties[CUnicodeUtils::StdGetUTF8((LPCTSTR)sName)];
            sValue = prop.value.c_str();
            len = sName.GetLength()*sizeof(TCHAR);
            fwrite(&len, sizeof(int), 1, stream);                                   // length of property name in bytes
            fwrite(sName, sizeof(TCHAR), sName.GetLength(), stream);                // property name
            len = static_cast<int>(prop.value.size());
            fwrite(&len, sizeof(int), 1, stream);                                   // length of property value in bytes
            fwrite(prop.value.c_str(), sizeof(char), prop.value.size(), stream);    // property value
        }
        fclose(stream);
    }
    else
    {
        TCHAR strErr[4096] = {0};
        _tcserror_s(strErr, 4096, err);
        ::MessageBox(m_hWnd, strErr, _T("TortoiseSVN"), MB_ICONERROR);
    }
}

void CEditPropertiesDlg::OnBnClickedImport()
{
    m_tooltips.Pop();   // hide the tooltips
    CString openPath;
    if (!CAppUtils::FileOpenSave(openPath, NULL, IDS_REPOBROWSE_OPEN, IDS_PROPSFILEFILTER, true, m_hWnd))
    {
        return;
    }
    // first check the size of the file
    FILE * stream;
    _tfopen_s(&stream, openPath, _T("rbS"));
    int nProperties = 0;
    if (fread(&nProperties, sizeof(int), 1, stream) == 1)
    {
        bool bFailed = false;
        if ((nProperties < 0)||(nProperties > 4096))
        {
            TSVNMessageBox(m_hWnd, IDS_EDITPROPS_ERRIMPORTFORMAT, IDS_APPNAME, MB_ICONERROR);
            bFailed = true;
        }
        CProgressDlg prog;
        CString sTemp;
        sTemp.LoadString(IDS_SETPROPTITLE);
        prog.SetTitle(sTemp);
        sTemp.LoadString(IDS_PROPWAITCANCEL);
        prog.SetCancelMsg(sTemp);
        prog.SetTime(TRUE);
        prog.SetShowProgressBar(TRUE);
        prog.ShowModeless(m_hWnd);
        while ((nProperties-- > 0)&&(!bFailed))
        {
            int nNameBytes = 0;
            if (fread(&nNameBytes, sizeof(int), 1, stream) == 1)
            {
                if ((nNameBytes < 0)||(nNameBytes > 4096))
                {
                    prog.Stop();
                    TSVNMessageBox(m_hWnd, IDS_EDITPROPS_ERRIMPORTFORMAT, IDS_APPNAME, MB_ICONERROR);
                    bFailed = true;
                    continue;
                }
                auto_buffer<TCHAR> pNameBuf(nNameBytes/sizeof(TCHAR));
                if (fread(pNameBuf, 1, nNameBytes, stream) == (size_t)nNameBytes)
                {
                    std::string sName = CUnicodeUtils::StdGetUTF8 (tstring (pNameBuf, nNameBytes/sizeof(TCHAR)));
                    tstring sUName = CUnicodeUtils::StdGetUnicode(sName);
                    int nValueBytes = 0;
                    if (fread(&nValueBytes, sizeof(int), 1, stream) == 1)
                    {
                        auto_buffer<BYTE> pValueBuf(nValueBytes);
                        if (fread(pValueBuf, sizeof(char), nValueBytes, stream) == (size_t)nValueBytes)
                        {
                            std::string propertyvalue;
                            propertyvalue.assign((const char*)pValueBuf.get(), nValueBytes);
                            CString sMsg;
                            if (m_pathlist[0].IsUrl())
                            {
                                CInputLogDlg input(this);
                                input.SetUUID(m_sUUID);
                                input.SetProjectProperties(m_pProjectProperties, PROJECTPROPNAME_LOGTEMPLATEPROPSET);
                                CString sHint;
                                sHint.FormatMessage(IDS_INPUT_SETPROP, sUName.c_str(), (LPCTSTR)(m_pathlist[0].GetSVNPathString()));
                                input.SetActionText(sHint);
                                if (input.DoModal() == IDOK)
                                {
                                    sMsg = input.GetLogMessage();
                                }
                                else
                                    bFailed = true;
                            }

                            for (int i=0; i<m_pathlist.GetCount() && !bFailed; ++i)
                            {
                                prog.SetLine(1, m_pathlist[i].GetWinPath(), true);
                                SVNProperties props(m_pathlist[i], m_revision, m_bRevProps);
                                if (!props.Add(sName, propertyvalue, false, svn_depth_empty, (LPCTSTR)sMsg))
                                {
                                    prog.Stop();
                                    props.ShowErrorDialog(m_hWnd, props.GetPath().GetDirectory());
                                    bFailed = true;
                                }
                                else
                                {
                                    if (m_revision.IsNumber())
                                        m_revision = (LONG)m_revision + 1;
                                }
                            }
                        }
                        else
                        {
                            prog.Stop();
                            TSVNMessageBox(m_hWnd, IDS_EDITPROPS_ERRIMPORTFORMAT, IDS_APPNAME, MB_ICONERROR);
                            bFailed = true;
                        }
                    }
                    else
                    {
                        prog.Stop();
                        TSVNMessageBox(m_hWnd, IDS_EDITPROPS_ERRIMPORTFORMAT, IDS_APPNAME, MB_ICONERROR);
                        bFailed = true;
                    }
                }
                else
                {
                    prog.Stop();
                    TSVNMessageBox(m_hWnd, IDS_EDITPROPS_ERRIMPORTFORMAT, IDS_APPNAME, MB_ICONERROR);
                    bFailed = true;
                }
            }
            else
            {
                prog.Stop();
                TSVNMessageBox(m_hWnd, IDS_EDITPROPS_ERRIMPORTFORMAT, IDS_APPNAME, MB_ICONERROR);
                bFailed = true;
            }
        }
        prog.Stop();
        fclose(stream);
    }

    Refresh();
}

BOOL CEditPropertiesDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    if (m_bThreadRunning)
    {
        // only show the wait cursor over the main controls
        if (!IsCursorOverWindowBorder())
        {
            HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT));
            SetCursor(hCur);
            return TRUE;
        }
    }
    HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
    SetCursor(hCur);

    return CResizableStandAloneDialog::OnSetCursor(pWnd, nHitTest, message);
}

void CEditPropertiesDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
    if ((pWnd == &m_propList)&&(m_propList.GetSelectedCount() == 1))
    {
        int selIndex = m_propList.GetSelectionMark();
        if (selIndex < 0)
            return; // nothing selected, nothing to do with a context menu
        if ((point.x == -1) && (point.y == -1))
        {
            CRect rect;
            m_propList.GetItemRect(selIndex, &rect, LVIR_LABEL);
            m_propList.ClientToScreen(&rect);
            point = rect.CenterPoint();
        }
        CIconMenu popup;
        if (popup.CreatePopupMenu())
        {
            popup.AppendMenuIcon(ID_CMD_PROP_SAVEVALUE, IDS_PROP_SAVEVALUE);
            popup.AppendMenuIcon(ID_CMD_PROP_REMOVE, IDS_PROP_REMOVE);
            popup.AppendMenuIcon(ID_CMD_PROP_EDIT, IDS_PROP_EDIT);
            int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
            switch (cmd)
            {
            case ID_CMD_PROP_SAVEVALUE:
                OnBnClickedSaveprop();
                break;
            case ID_CMD_PROP_REMOVE:
                OnBnClickedRemoveProps();
                break;
            case ID_CMD_PROP_EDIT:
                OnBnClickedEditprops();
                break;
            }
        }
    }
}