// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010-2016, 2020-2021 - TortoiseSVN

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
#include "EditPropExternalsValue.h"
#include "AppUtils.h"
#include "UnicodeUtils.h"
#include "PathUtils.h"

// CEditPropExternalsValue dialog

IMPLEMENT_DYNAMIC(CEditPropExternalsValue, CResizableStandAloneDialog)

CEditPropExternalsValue::CEditPropExternalsValue(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CEditPropExternalsValue::IDD, pParent)
    , m_pLogDlg(nullptr)
{
}

CEditPropExternalsValue::~CEditPropExternalsValue()
{
    delete m_pLogDlg;
}

void CEditPropExternalsValue::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_URLCOMBO, m_urlCombo);
    DDX_Text(pDX, IDC_WCPATH, m_sWCPath);
    DDX_Text(pDX, IDC_REVISION_NUM, m_sRevision);
    DDX_Text(pDX, IDC_PEGREV, m_sPegRev);
}

BEGIN_MESSAGE_MAP(CEditPropExternalsValue, CResizableStandAloneDialog)
    ON_BN_CLICKED(IDC_BROWSE, &CEditPropExternalsValue::OnBnClickedBrowse)
    ON_BN_CLICKED(IDC_SHOW_LOG, &CEditPropExternalsValue::OnBnClickedShowLog)
    ON_REGISTERED_MESSAGE(WM_REVSELECTED, &CEditPropExternalsValue::OnRevSelected)
    ON_EN_CHANGE(IDC_REVISION_NUM, &CEditPropExternalsValue::OnEnChangeRevisionNum)
    ON_EN_CHANGE(IDC_PEGREV, &CEditPropExternalsValue::OnEnChangeRevisionNum)
    ON_BN_CLICKED(IDHELP, &CEditPropExternalsValue::OnBnClickedHelp)
END_MESSAGE_MAP()

// CEditPropExternalsValue message handlers

BOOL CEditPropExternalsValue::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);
    BlockResize(DIALOG_BLOCKVERTICAL);

    ExtendFrameIntoClientArea(IDC_GROUPBOTTOM);
    m_aeroControls.SubclassOkCancelHelp(this);

    m_sWCPath = m_external.targetDir;

    SVNRev rev    = m_external.revision;
    SVNRev pegRev = SVNRev(m_external.pegRevision);

    if ((pegRev.IsValid() && !pegRev.IsHead()) || (rev.IsValid() && !rev.IsHead()))
    {
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);

        if (m_external.revision.value.number == m_external.pegRevision.value.number)
        {
            m_sPegRev = pegRev.ToString();
        }
        else
        {
            m_sRevision = rev.ToString();
            m_sPegRev   = pegRev.ToString();
        }
    }
    else
    {
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
    }

    m_urlCombo.LoadHistory(L"Software\\TortoiseSVN\\History\\repoURLS", L"url");
    m_urlCombo.SetURLHistory(true, false);
    m_urlCombo.SetWindowText(CPathUtils::PathUnescape(m_external.url));

    UpdateData(false);

    CString sWindowTitle;
    GetWindowText(sWindowTitle);
    CAppUtils::SetWindowTitle(m_hWnd, m_pathList.GetCommonRoot().GetUIPathString(), sWindowTitle);

    SetTheme(CTheme::Instance().IsDarkTheme());

    AddAnchor(IDC_WCLABEL, TOP_LEFT);
    AddAnchor(IDC_WCPATH, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_URLLABEL, TOP_LEFT);
    AddAnchor(IDC_URLCOMBO, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_BROWSE, TOP_RIGHT);
    AddAnchor(IDC_PEGLABEL, TOP_LEFT);
    AddAnchor(IDC_OPERATIVELABEL, TOP_LEFT);
    AddAnchor(IDC_PEGREV, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_GROUPBOTTOM, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_REVISION_HEAD, TOP_LEFT);
    AddAnchor(IDC_REVISION_N, TOP_LEFT);
    AddAnchor(IDC_REVISION_NUM, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_SHOW_LOG, TOP_RIGHT);
    AddAnchor(IDOK, BOTTOM_RIGHT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);
    AddAnchor(IDHELP, BOTTOM_RIGHT);

    EnableSaveRestore(L"EditPropExternalsValue");

    return TRUE;
}

void CEditPropExternalsValue::OnCancel()
{
    if (::IsWindow(m_pLogDlg->GetSafeHwnd()) && (m_pLogDlg->IsWindowVisible()))
    {
        m_pLogDlg->SendMessage(WM_CLOSE);
        return;
    }

    CResizableStandAloneDialog::OnCancel();
}

void CEditPropExternalsValue::OnOK()
{
    UpdateData();
    m_sWCPath.Trim(L"\"'");
    if (!CTSVNPath(m_sWCPath).IsValidOnWindows())
    {
        ShowEditBalloon(IDC_CHECKOUTDIRECTORY, IDS_ERR_NOVALIDPATH, IDS_ERR_ERROR, TTI_ERROR);
        return;
    }

    if (::IsWindow(m_pLogDlg->GetSafeHwnd()) && (m_pLogDlg->IsWindowVisible()))
    {
        m_pLogDlg->SendMessage(WM_CLOSE);
        return;
    }

    if (GetCheckedRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N) == IDC_REVISION_HEAD)
    {
        m_external.revision.kind = svn_opt_revision_head;
        m_sPegRev.Empty();
    }
    else
    {
        SVNRev rev = m_sRevision;
        if (!rev.IsValid())
        {
            ShowEditBalloon(IDC_REVISION_N, IDS_ERR_INVALIDREV, IDS_ERR_ERROR, TTI_ERROR);
            return;
        }
        m_external.revision = *rev;
    }
    m_urlCombo.SaveHistory();
    m_url          = CTSVNPath(m_urlCombo.GetString());
    m_external.url = CUnicodeUtils::GetUnicode(CPathUtils::PathEscape(CUnicodeUtils::GetUTF8(m_url.GetSVNPathString())));
    if (m_url.GetSVNPathString().GetLength() && (m_url.GetSVNPathString()[0] == '^'))
    {
        // the ^ char must not be escaped
        m_external.url = CUnicodeUtils::GetUnicode(CPathUtils::PathEscape(CUnicodeUtils::GetUTF8(m_url.GetSVNPathString().Mid(1))));
        m_external.url = '^' + m_external.url;
    }

    if (m_sPegRev.IsEmpty())
        m_external.pegRevision = *SVNRev(L"HEAD");
    else
        m_external.pegRevision = *SVNRev(m_sPegRev);
    m_external.targetDir = m_sWCPath;

    CResizableStandAloneDialog::OnOK();
}

void CEditPropExternalsValue::OnBnClickedBrowse()
{
    SVNRev rev = SVNRev::REV_HEAD;
    CAppUtils::BrowseRepository(m_urlCombo, this, rev, false, m_repoRoot.GetSVNPathString(), m_url.GetSVNPathString());

    // if possible, create a repository-root relative url
    CString strUrLs;
    m_urlCombo.GetWindowText(strUrLs);
    if (strUrLs.IsEmpty())
        strUrLs = m_urlCombo.GetString();
    strUrLs.Replace('\\', '/');
    strUrLs.Replace(L"%", L"%25");

    CString root       = m_repoRoot.GetSVNPathString();
    int     rootlength = root.GetLength();
    if (strUrLs.Left(rootlength).Compare(root) == 0)
    {
        if ((strUrLs.GetLength() > rootlength) && (strUrLs.GetAt(rootlength) == '/'))
        {
            strUrLs = L"^/" + strUrLs.Mid(rootlength);
            strUrLs.Replace(L"^//", L"^/");
            m_urlCombo.SetWindowText(strUrLs);
        }
    }
}

void CEditPropExternalsValue::OnBnClickedShowLog()
{
    UpdateData(TRUE);
    if (::IsWindow(m_pLogDlg->GetSafeHwnd()) && (m_pLogDlg->IsWindowVisible()))
        return;
    CString   urlString = m_urlCombo.GetString();
    CTSVNPath logUrl    = m_url;
    if (urlString.GetLength() > 1)
    {
        logUrl = CTSVNPath(SVNExternals::GetFullExternalUrl(urlString, m_repoRoot.GetSVNPathString(), m_url.GetSVNPathString()));
    }
    else
    {
        logUrl = m_repoRoot;
        logUrl.AppendPathString(urlString);
    }

    if (!logUrl.IsEmpty())
    {
        delete m_pLogDlg;
        m_pLogDlg = new CLogDlg();
        m_pLogDlg->SetSelect(true);
        m_pLogDlg->m_pNotifyWindow = this;
        m_pLogDlg->m_wParam        = 0;
        m_pLogDlg->SetParams(CTSVNPath(logUrl), SVNRev::REV_HEAD, SVNRev::REV_HEAD, 1, TRUE);
        m_pLogDlg->ContinuousSelection(true);
        m_pLogDlg->Create(IDD_LOGMESSAGE, this);
        m_pLogDlg->ShowWindow(SW_SHOW);
    }
    AfxGetApp()->DoWaitCursor(-1);
}

LPARAM CEditPropExternalsValue::OnRevSelected(WPARAM /*wParam*/, LPARAM lParam)
{
    CString temp;
    temp.Format(L"%Id", lParam);
    SetDlgItemText(IDC_PEGREV, temp);
    SetDlgItemText(IDC_REVISION_NUM, CString());
    CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
    return 0;
}

void CEditPropExternalsValue::OnEnChangeRevisionNum()
{
    UpdateData();
    if (m_sRevision.IsEmpty() && m_sPegRev.IsEmpty())
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
    else
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
}

void CEditPropExternalsValue::OnBnClickedHelp()
{
    OnHelp();
}
