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
#include "ExportDlg.h"
#include "RepositoryBrowser.h"
#include "PathUtils.h"
#include "BrowseFolder.h"
#include "AppUtils.h"
#include "ClipboardHelper.h"


IMPLEMENT_DYNAMIC(CExportDlg, CResizableStandAloneDialog)
CExportDlg::CExportDlg(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CExportDlg::IDD, pParent)
    , Revision(L"HEAD")
    , m_strExportDirectory(L"")
    , m_sExportDirOrig(L"")
    , m_bNoExternals(CRegDWORD(L"Software\\TortoiseSVN\\noext"))
    , m_bNoKeywords(FALSE)
    , m_pLogDlg(NULL)
    , m_blockPathAdjustments(FALSE)
    , m_bAutoCreateTargetName(false)
    , m_depth(svn_depth_unknown)
{
}

CExportDlg::~CExportDlg()
{
    delete m_pLogDlg;
}

void CExportDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_URLCOMBO, m_URLCombo);
    DDX_Control(pDX, IDC_REVISION_NUM, m_editRevision);
    DDX_Control(pDX, IDC_BROWSE, m_butBrowse);
    DDX_Text(pDX, IDC_REVISION_NUM, m_sRevision);
    DDX_Text(pDX, IDC_CHECKOUTDIRECTORY, m_strExportDirectory);
    DDX_Check(pDX, IDC_NOEXTERNALS, m_bNoExternals);
    DDX_Check(pDX, IDC_IGNOREKEYWORDS, m_bNoKeywords);
    DDX_Control(pDX, IDC_CHECKOUTDIRECTORY, m_cCheckoutEdit);
    DDX_Control(pDX, IDC_EOLCOMBO, m_eolCombo);
    DDX_Control(pDX, IDC_DEPTH, m_depthCombo);
}


BEGIN_MESSAGE_MAP(CExportDlg, CResizableStandAloneDialog)
    ON_REGISTERED_MESSAGE(WM_REVSELECTED, OnRevSelected)
    ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
    ON_BN_CLICKED(IDC_CHECKOUTDIRECTORY_BROWSE, OnBnClickedCheckoutdirectoryBrowse)
    ON_EN_CHANGE(IDC_CHECKOUTDIRECTORY, OnEnChangeCheckoutdirectory)
    ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
    ON_BN_CLICKED(IDC_SHOW_LOG, OnBnClickedShowlog)
    ON_EN_CHANGE(IDC_REVISION_NUM, &CExportDlg::OnEnChangeRevisionNum)
    ON_CBN_SELCHANGE(IDC_EOLCOMBO, &CExportDlg::OnCbnSelchangeEolcombo)
    ON_CBN_EDITCHANGE(IDC_URLCOMBO, &CExportDlg::OnCbnEditchangeUrlcombo)
END_MESSAGE_MAP()

BOOL CExportDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    ExtendFrameIntoClientArea(IDC_REVISIONGROUP);
    m_aeroControls.SubclassOkCancelHelp(this);

    m_sExportDirOrig = m_strExportDirectory;
    m_bAutoCreateTargetName = !(PathIsDirectoryEmpty(m_sExportDirOrig) || !PathFileExists(m_sExportDirOrig));

    AdjustControlSize(IDC_NOEXTERNALS);
    AdjustControlSize(IDC_IGNOREKEYWORDS);
    AdjustControlSize(IDC_REVISION_HEAD);
    AdjustControlSize(IDC_REVISION_N);

    AddAnchor(IDC_REPOGROUP, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_REPOLABEL, TOP_LEFT);
    AddAnchor(IDC_URLCOMBO, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_BROWSE, TOP_RIGHT);
    AddAnchor(IDC_EXPORT_CHECKOUTDIR, TOP_LEFT);
    AddAnchor(IDC_CHECKOUTDIRECTORY, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_CHECKOUTDIRECTORY_BROWSE, TOP_RIGHT);
    AddAnchor(IDC_DEPTH, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_NOEXTERNALS, TOP_LEFT);
    AddAnchor(IDC_IGNOREKEYWORDS, TOP_LEFT);
    AddAnchor(IDC_EOLLABEL, TOP_LEFT);
    AddAnchor(IDC_EOLCOMBO, TOP_LEFT);

    AddAnchor(IDC_REVISIONGROUP, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_REVISION_HEAD, TOP_LEFT);
    AddAnchor(IDC_REVISION_N, TOP_LEFT);
    AddAnchor(IDC_REVISION_NUM, TOP_LEFT);
    AddAnchor(IDC_SHOW_LOG, TOP_LEFT);

    AddAnchor(IDOK, BOTTOM_RIGHT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);
    AddAnchor(IDHELP, BOTTOM_RIGHT);

    // save the provided url since the url combo
    // will change that below
    CString origurl = m_URL;
    m_URLCombo.SetURLHistory(true, true);
    m_URLCombo.LoadHistory(L"Software\\TortoiseSVN\\History\\repoURLS", L"url");
    m_URLCombo.SetCurSel(0);

    m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_INFINITE)));
    m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_IMMEDIATE)));
    m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_FILES)));
    m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_EMPTY)));
    m_depthCombo.SetCurSel(0);

    // set radio buttons according to the revision
    SetRevision(Revision);

    m_editRevision.SetWindowText(L"");

    if (!origurl.IsEmpty())
        m_URLCombo.SetWindowText(origurl);
    else
    {
        // if there is an url on the clipboard, use that url as the default.
        CAppUtils::AddClipboardUrlToWindow(m_URLCombo.GetSafeHwnd());
    }
    GetDlgItem(IDC_BROWSE)->EnableWindow(!m_URLCombo.GetString().IsEmpty());

    m_tooltips.Create(this);
    m_tooltips.AddTool(IDC_CHECKOUTDIRECTORY, IDS_CHECKOUT_TT_DIR);
    m_tooltips.AddTool(IDC_EOLCOMBO, IDS_EXPORT_TT_EOL);

    SHAutoComplete(GetDlgItem(IDC_CHECKOUTDIRECTORY)->m_hWnd, SHACF_FILESYSTEM);

    // fill the combobox with the choices of eol styles
    m_eolCombo.AddString(L"default");
    m_eolCombo.AddString(L"CRLF");
    m_eolCombo.AddString(L"LF");
    m_eolCombo.AddString(L"CR");
    m_eolCombo.SelectString(0, L"default");

    if (!Revision.IsHead())
    {
        // if the revision is not HEAD, change the radio button and
        // fill in the revision in the edit box
        CString temp;
        temp.Format(L"%ld", (LONG)Revision);
        m_editRevision.SetWindowText(temp);
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
    }

    DialogEnableWindow(IDOK, !m_strExportDirectory.IsEmpty());

    if ((m_pParentWnd==NULL)&&(GetExplorerHWND()))
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    EnableSaveRestore(L"ExportDlg");
    return TRUE;
}

void CExportDlg::OnCancel()
{
    if (::IsWindow(m_pLogDlg->GetSafeHwnd())&&(m_pLogDlg->IsWindowVisible()))
    {
        m_pLogDlg->SendMessage(WM_CLOSE);
        return;
    }

    CResizableStandAloneDialog::OnCancel();
}

void CExportDlg::OnOK()
{
    if (!UpdateData(TRUE))
        return; // don't dismiss dialog (error message already shown by MFC framework)

    if (::IsWindow(m_pLogDlg->GetSafeHwnd())&&(m_pLogDlg->IsWindowVisible()))
    {
        m_pLogDlg->SendMessage(WM_CLOSE);
        return;
    }

    // check it the export path is a valid windows path
    CTSVNPath ExportDirectory;
    if (::PathIsRelative(m_strExportDirectory))
    {
        ExportDirectory = CTSVNPath(sOrigCWD);
        ExportDirectory.AppendPathString(L"\\" + m_strExportDirectory);
        m_strExportDirectory = ExportDirectory.GetWinPathString();
    }
    else
        ExportDirectory = CTSVNPath(m_strExportDirectory);
    if (!ExportDirectory.IsValidOnWindows())
    {
        ShowEditBalloon(IDC_CHECKOUTDIRECTORY, IDS_ERR_NOVALIDPATH, IDS_ERR_ERROR, TTI_ERROR);
        return;
    }

    // check if the specified revision is valid
    if (GetCheckedRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N) == IDC_REVISION_HEAD)
    {
        Revision = SVNRev(L"HEAD");
    }
    else
        Revision = SVNRev(m_sRevision);
    if (!Revision.IsValid())
    {
        ShowEditBalloon(IDC_REVISION_NUM, IDS_ERR_INVALIDREV, IDS_ERR_ERROR, TTI_ERROR);
        return;
    }
    bool bAutoCreateTargetName = m_bAutoCreateTargetName;
    m_bAutoCreateTargetName = false;

    m_URLCombo.SaveHistory();
    m_URL = m_URLCombo.GetString();

    // we need an url to export from - local paths won't work
    if (!SVN::PathIsURL(CTSVNPath(m_URL)))
    {
        m_tooltips.ShowBalloon(IDC_URLCOMBO, IDS_ERR_MUSTBEURL, IDS_ERR_ERROR, TTI_ERROR);
        m_bAutoCreateTargetName = bAutoCreateTargetName;
        return;
    }

    if (m_strExportDirectory.IsEmpty())
    {
        m_bAutoCreateTargetName = bAutoCreateTargetName;
        return;
    }

    // if the export directory does not exist, where should we export to?
    // We ask if the directory should be created...
    if (!PathFileExists(m_strExportDirectory))
    {
        CPathUtils::MakeSureDirectoryPathExists(m_strExportDirectory);
    }

    // if the directory we should export to is not empty, show a warning:
    // maybe the user doesn't want to overwrite the existing files.
    if (!PathIsDirectoryEmpty(m_strExportDirectory))
    {
        CString message;
        message.Format(CString(MAKEINTRESOURCE(IDS_WARN_FOLDERNOTEMPTY)),(LPCTSTR)m_strExportDirectory);
        CTaskDialog taskdlg(message,
                            CString(MAKEINTRESOURCE(IDS_WARN_FOLDERNOTEMPTY_TASK2)),
                            L"TortoiseSVN",
                            0,
                            TDF_ENABLE_HYPERLINKS|TDF_USE_COMMAND_LINKS|TDF_ALLOW_DIALOG_CANCELLATION|TDF_POSITION_RELATIVE_TO_WINDOW);
        taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_WARN_FOLDERNOTEMPTY_TASK3_1)));
        taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_WARN_FOLDERNOTEMPTY_TASK4)));
        taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
        taskdlg.SetDefaultCommandControl(2);
        taskdlg.SetMainIcon(TD_WARNING_ICON);
        bool doIt = (taskdlg.DoModal(GetExplorerHWND()) == 1);

        if (!doIt)
        {
            m_bAutoCreateTargetName = bAutoCreateTargetName;
            return;     //don't dismiss the dialog
        }
    }
    m_eolCombo.GetWindowText(m_eolStyle);
    if (m_eolStyle.Compare(L"default")==0)
        m_eolStyle.Empty();

    switch (m_depthCombo.GetCurSel())
    {
    case 0:
        m_depth = svn_depth_infinity;
        break;
    case 1:
        m_depth = svn_depth_immediates;
        break;
    case 2:
        m_depth = svn_depth_files;
        break;
    case 3:
        m_depth = svn_depth_empty;
        break;
    default:
        m_depth = svn_depth_empty;
        break;
    }

    UpdateData(FALSE);

    CRegDWORD regNoExt(L"Software\\TortoiseSVN\\noext");
    regNoExt = m_bNoExternals;

    CResizableStandAloneDialog::OnOK();
}

void CExportDlg::OnBnClickedBrowse()
{
    m_tooltips.Pop();   // hide the tooltips
    SVNRev rev;
    UpdateData();
    if (GetCheckedRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N) == IDC_REVISION_HEAD)
    {
        rev = SVNRev::REV_HEAD;
    }
    else
        rev = SVNRev(m_sRevision);
    if (!rev.IsValid())
        rev = SVNRev::REV_HEAD;
    CAppUtils::BrowseRepository(m_URLCombo, this, rev);
    SetRevision(rev);
    DialogEnableWindow(IDOK, !m_strExportDirectory.IsEmpty());
}

void CExportDlg::OnBnClickedCheckoutdirectoryBrowse()
{
    m_tooltips.Pop();   // hide the tooltips
    //
    // Create a folder browser dialog. If the user selects OK, we should update
    // the local data members with values from the controls, copy the checkout
    // directory from the browse folder, then restore the local values into the
    // dialog controls.
    //
    CBrowseFolder browseFolder;
    browseFolder.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
    CString strCheckoutDirectory = m_strExportDirectory;
    if (browseFolder.Show(GetSafeHwnd(), strCheckoutDirectory) == CBrowseFolder::OK)
    {
        UpdateData(TRUE);
        m_strExportDirectory = strCheckoutDirectory;
        m_sExportDirOrig = m_strExportDirectory;
        m_bAutoCreateTargetName = !(PathIsDirectoryEmpty(m_sExportDirOrig) || !PathFileExists(m_sExportDirOrig));
        UpdateData(FALSE);
        DialogEnableWindow(IDOK, !m_strExportDirectory.IsEmpty());
    }
}

BOOL CExportDlg::PreTranslateMessage(MSG* pMsg)
{
    m_tooltips.RelayEvent(pMsg);
    return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

void CExportDlg::OnEnChangeCheckoutdirectory()
{
    UpdateData(TRUE);
    DialogEnableWindow(IDOK, !m_strExportDirectory.IsEmpty());
}

void CExportDlg::OnBnClickedHelp()
{
    OnHelp();
}

void CExportDlg::OnBnClickedShowlog()
{
    m_tooltips.Pop();   // hide the tooltips
    UpdateData(TRUE);
    m_URL = m_URLCombo.GetString();
    if ((m_pLogDlg)&&(m_pLogDlg->IsWindowVisible()))
        return;
    AfxGetApp()->DoWaitCursor(1);
    //now show the log dialog for working copy
    if (!m_URL.IsEmpty())
    {
        delete m_pLogDlg;
        m_pLogDlg = new CLogDlg();
        m_pLogDlg->SetParams(CTSVNPath(m_URL), SVNRev::REV_HEAD, SVNRev::REV_HEAD, 1);
        m_pLogDlg->m_wParam = 1;
        m_pLogDlg->SetSelect(true);
        m_pLogDlg->m_pNotifyWindow = this;
        m_pLogDlg->Create(IDD_LOGMESSAGE, this);
        m_pLogDlg->ShowWindow(SW_SHOW);
    }
    AfxGetApp()->DoWaitCursor(-1);
}

LPARAM CExportDlg::OnRevSelected(WPARAM /*wParam*/, LPARAM lParam)
{
    CString temp;
    temp.Format(L"%Id", lParam);
    SetDlgItemText(IDC_REVISION_NUM, temp);
    CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
    return 0;
}

void CExportDlg::OnEnChangeRevisionNum()
{
    UpdateData();
    if (m_sRevision.IsEmpty())
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
    else
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
}

void CExportDlg::OnCbnSelchangeEolcombo()
{
}

void CExportDlg::SetRevision(const SVNRev& rev)
{
    if (rev.IsHead())
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
    else
    {
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
        CString sRev;
        sRev.Format(L"%ld", (LONG)rev);
        SetDlgItemText(IDC_REVISION_NUM, sRev);
    }
}

void CExportDlg::OnCbnEditchangeUrlcombo()
{
    // find out what to use as the checkout directory name
    UpdateData();
    m_URLCombo.GetWindowText(m_URL);
    if (m_URL.IsEmpty())
    {
        DialogEnableWindow(IDC_BROWSE, FALSE);
        return;
    }
    DialogEnableWindow(IDC_BROWSE, TRUE);
    if (!m_bAutoCreateTargetName)
        return;
    if ((m_sExportDirOrig.IsEmpty())||(m_blockPathAdjustments))
        return;
    CString name = CAppUtils::GetProjectNameFromURL(m_URL);
    m_strExportDirectory = m_sExportDirOrig+'\\'+name;
    m_strExportDirectory.Replace(L":\\\\", L":\\");
    UpdateData(FALSE);
}
