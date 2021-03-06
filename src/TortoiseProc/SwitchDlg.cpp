// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2015, 2020-2021 - TortoiseSVN

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
#include "SwitchDlg.h"
#include "RepositoryBrowser.h"
#include "TSVNPath.h"
#include "AppUtils.h"
#include "PathUtils.h"

IMPLEMENT_DYNAMIC(CSwitchDlg, CResizableStandAloneDialog)
CSwitchDlg::CSwitchDlg(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CSwitchDlg::IDD, pParent)
    , m_bFolder(false)
    , m_pLogDlg(nullptr)
    , m_revision(L"HEAD")
    , m_bNoExternals(CRegDWORD(L"Software\\TortoiseSVN\\noext"))
    , m_bStickyDepth(FALSE)
    , m_bIgnoreAncestry(FALSE)
    , m_depth(svn_depth_unknown)
{
}

CSwitchDlg::~CSwitchDlg()
{
    delete m_pLogDlg;
}

void CSwitchDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_URLCOMBO, m_urlCombo);
    DDX_Text(pDX, IDC_REVISION_NUM, m_rev);
    DDX_Control(pDX, IDC_SWITCHPATH, m_switchPath);
    DDX_Control(pDX, IDC_DESTURL, m_destUrl);
    DDX_Control(pDX, IDC_SRCURL, m_srcUrl);
    DDX_Check(pDX, IDC_NOEXTERNALS, m_bNoExternals);
    DDX_Check(pDX, IDC_STICKYDEPTH, m_bStickyDepth);
    DDX_Check(pDX, IDC_IGNOREANCESTRY, m_bIgnoreAncestry);
    DDX_Control(pDX, IDC_DEPTH, m_depthCombo);
}

BEGIN_MESSAGE_MAP(CSwitchDlg, CResizableStandAloneDialog)
    ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
    ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
    ON_EN_CHANGE(IDC_REVISION_NUM, &CSwitchDlg::OnEnChangeRevisionNum)
    ON_BN_CLICKED(IDC_LOG, &CSwitchDlg::OnBnClickedLog)
    ON_REGISTERED_MESSAGE(WM_REVSELECTED, &CSwitchDlg::OnRevSelected)
    ON_CBN_EDITCHANGE(IDC_URLCOMBO, &CSwitchDlg::OnCbnEditchangeUrlcombo)
END_MESSAGE_MAP()

BOOL CSwitchDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);
    BlockResize(DIALOG_BLOCKVERTICAL);

    ExtendFrameIntoClientArea(IDC_REVGROUP);
    m_aeroControls.SubclassControl(this, IDC_IGNOREANCESTRY);
    m_aeroControls.SubclassOkCancelHelp(this);

    CTSVNPath svnPath(m_path);
    SetDlgItemText(IDC_SWITCHPATH, m_path);
    m_bFolder = svnPath.IsDirectory();
    SVN svn;

    m_repoRoot = svn.GetRepositoryRootAndUUID(svnPath, true, m_sUuid);
    m_repoRoot.TrimRight('/');
    CString url     = svn.GetURLFromPath(svnPath);
    CString destUrl = url;
    if (!m_url.IsEmpty())
    {
        destUrl = m_url;
    }
    m_urlCombo.LoadHistory(L"Software\\TortoiseSVN\\History\\repoPaths\\" + m_sUuid, L"url");
    m_urlCombo.SetCurSel(0);
    if (!url.IsEmpty())
    {
        CString relPath = destUrl.Mid(m_repoRoot.GetLength());
        relPath         = CPathUtils::PathUnescape(relPath);
        relPath.Replace('\\', '/');
        m_urlCombo.AddString(relPath, 0);
        m_urlCombo.SelectString(-1, relPath);
        m_url = destUrl;

        SetDlgItemText(IDC_SRCURL, CTSVNPath(url).GetUIPathString());
        SetDlgItemText(IDC_DESTURL, CTSVNPath(CPathUtils::CombineUrls(m_repoRoot, relPath)).GetUIPathString());
    }

    CString   sRegOptionIgnoreAncestry = L"Software\\TortoiseSVN\\Merge\\IgnoreAncestry_" + m_sUuid;
    CRegDWORD regIgnoreAncestryOpt(sRegOptionIgnoreAncestry, FALSE);
    m_bIgnoreAncestry = static_cast<DWORD>(regIgnoreAncestryOpt);

    GetWindowText(m_sTitle);
    CAppUtils::SetWindowTitle(m_hWnd, m_path, m_sTitle);

    // set head revision as default revision
    SetRevision(SVNRev::REV_HEAD);
    if (m_revision.IsValid())
    {
        SetRevision(m_revision);
    }

    m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_WORKING)));
    m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_INFINITE)));
    m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_IMMEDIATE)));
    m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_FILES)));
    m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_EMPTY)));
    m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_EXCLUDE)));
    m_depthCombo.SetCurSel(0);

    m_tooltips.AddTool(IDC_STICKYDEPTH, IDS_SWITCH_STICKYDEPTH_TT);

    SetTheme(CTheme::Instance().IsDarkTheme());

    UpdateData(FALSE);

    AddAnchor(IDC_SWITCHLABEL, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_SWITCHPATH, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_URLLABEL, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_URLCOMBO, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_BROWSE, TOP_RIGHT);
    AddAnchor(IDC_SRCLABEL, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_SRCURL, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_DESTLABEL, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_DESTURL, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_REVGROUP, TOP_LEFT);
    AddAnchor(IDC_REVISION_HEAD, TOP_LEFT);
    AddAnchor(IDC_REVISION_N, TOP_LEFT);
    AddAnchor(IDC_REVISION_NUM, TOP_LEFT);
    AddAnchor(IDC_LOG, TOP_LEFT);
    AddAnchor(IDC_GROUPMIDDLE, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_DEPTH, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_STICKYDEPTH, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_NOEXTERNALS, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_IGNOREANCESTRY, BOTTOM_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDOK, BOTTOM_RIGHT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);
    AddAnchor(IDHELP, BOTTOM_RIGHT);

    if ((m_pParentWnd == nullptr) && (GetExplorerHWND()))
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    EnableSaveRestore(L"SwitchDlg");
    return TRUE;
}

void CSwitchDlg::OnBnClickedBrowse()
{
    UpdateData();
    SVNRev rev;
    if (GetCheckedRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N) == IDC_REVISION_HEAD)
    {
        rev = SVNRev::REV_HEAD;
    }
    else
        rev = SVNRev(m_rev);
    if (!rev.IsValid())
        rev = SVNRev::REV_HEAD;
    CAppUtils::BrowseRepository(m_repoRoot, m_urlCombo, this, rev);
    SetRevision(rev);
}

void CSwitchDlg::OnCancel()
{
    if (::IsWindow(m_pLogDlg->GetSafeHwnd()) && (m_pLogDlg->IsWindowVisible()))
    {
        m_pLogDlg->SendMessage(WM_CLOSE);
        return;
    }

    CResizableStandAloneDialog::OnCancel();
}

void CSwitchDlg::OnOK()
{
    if (!UpdateData(TRUE))
        return; // don't dismiss dialog (error message already shown by MFC framework)

    if (::IsWindow(m_pLogDlg->GetSafeHwnd()) && (m_pLogDlg->IsWindowVisible()))
    {
        m_pLogDlg->SendMessage(WM_CLOSE);
        return;
    }

    // if head revision, set revision as HEAD
    if (GetCheckedRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N) == IDC_REVISION_HEAD)
    {
        m_rev = L"HEAD";
    }
    m_revision = SVNRev(m_rev);
    if (!m_revision.IsValid())
    {
        ShowEditBalloon(IDC_REVISION_NUM, IDS_ERR_INVALIDREV, IDS_ERR_ERROR, TTI_ERROR);
        return;
    }

    m_urlCombo.SaveHistory();
    m_url = CPathUtils::CombineUrls(m_repoRoot, m_urlCombo.GetString());

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
        case 5:
            m_depth = svn_depth_exclude;
            break;
        default:
            m_depth = svn_depth_empty;
            break;
    }

    UpdateData(FALSE);

    CRegDWORD regNoExt(L"Software\\TortoiseSVN\\noext");
    regNoExt = m_bNoExternals;

    CString   sRegOptionIgnoreAncestry = L"Software\\TortoiseSVN\\Merge\\IgnoreAncestry_" + m_sUuid;
    CRegDWORD regIgnoreAncestryOpt(sRegOptionIgnoreAncestry, FALSE);
    regIgnoreAncestryOpt = m_bIgnoreAncestry;

    CResizableStandAloneDialog::OnOK();
}

void CSwitchDlg::OnBnClickedHelp()
{
    OnHelp();
}

void CSwitchDlg::OnEnChangeRevisionNum()
{
    UpdateData();
    if (m_rev.IsEmpty())
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
    else
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
}

void CSwitchDlg::SetRevision(const SVNRev& rev)
{
    if (rev.IsHead())
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
    else
    {
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
        m_rev = rev.ToString();
        UpdateData(FALSE);
    }
}

void CSwitchDlg::OnBnClickedLog()
{
    UpdateData(TRUE);
    if (::IsWindow(m_pLogDlg->GetSafeHwnd()) && (m_pLogDlg->IsWindowVisible()))
        return;
    m_url = CPathUtils::CombineUrls(m_repoRoot, m_urlCombo.GetString());
    if (!m_url.IsEmpty())
    {
        delete m_pLogDlg;
        m_pLogDlg = new CLogDlg();
        m_pLogDlg->SetSelect(true);
        m_pLogDlg->m_pNotifyWindow = this;
        m_pLogDlg->m_wParam        = 0;
        m_pLogDlg->SetParams(CTSVNPath(m_url), SVNRev::REV_HEAD, SVNRev::REV_HEAD, 1, TRUE);
        m_pLogDlg->ContinuousSelection(true);
        m_pLogDlg->SetProjectPropertiesPath(CTSVNPath(m_path));
        m_pLogDlg->Create(IDD_LOGMESSAGE, this);
        m_pLogDlg->ShowWindow(SW_SHOW);
    }
    AfxGetApp()->DoWaitCursor(-1);
}

LPARAM CSwitchDlg::OnRevSelected(WPARAM /*wParam*/, LPARAM lParam)
{
    CString temp;
    temp.Format(L"%Id", lParam);
    SetDlgItemText(IDC_REVISION_NUM, temp);
    CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
    return 0;
}

void CSwitchDlg::OnCbnEditchangeUrlcombo()
{
    SetDlgItemText(IDC_DESTURL, CTSVNPath(CPathUtils::CombineUrls(m_repoRoot, m_urlCombo.GetWindowString())).GetUIPathString());
}
