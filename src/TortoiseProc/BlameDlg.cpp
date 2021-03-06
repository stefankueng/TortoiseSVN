// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2012, 2014-2015, 2019, 2021 - TortoiseSVN

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
#include "BlameDlg.h"
#include "registry.h"
#include "AppUtils.h"

IMPLEMENT_DYNAMIC(CBlameDlg, CStateStandAloneDialog)
CBlameDlg::CBlameDlg(CWnd* pParent /*=NULL*/)
    : CStateStandAloneDialog(CBlameDlg::IDD, pParent)
    , m_sStartRev(L"1")
    , m_startRev(1)
    , m_endRev(0)
    , m_bTextView(FALSE)
    , m_bIgnoreEOL(TRUE)
    , m_bIncludeMerge(TRUE)
    , m_ignoreSpaces(svn_diff_file_ignore_space_none)
{
    m_regTextView         = CRegDWORD(L"Software\\TortoiseSVN\\TextBlame", FALSE);
    m_bTextView           = m_regTextView;
    m_regIncludeMerge     = CRegDWORD(L"Software\\TortoiseSVN\\BlameIncludeMerge", FALSE);
    m_bIncludeMerge       = m_regIncludeMerge;
    m_regIgnoreWhitespace = CRegDWORD(L"Software\\TortoiseSVN\\BlameIgnoreWhitespace", svn_diff_file_ignore_space_none);
    m_ignoreSpaces        = static_cast<svn_diff_file_ignore_space_t>(static_cast<DWORD>(m_regIgnoreWhitespace));
}

CBlameDlg::~CBlameDlg()
{
}

void CBlameDlg::DoDataExchange(CDataExchange* pDX)
{
    CStateStandAloneDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_REVISON_START, m_sStartRev);
    DDX_Text(pDX, IDC_REVISION_END, m_sEndRev);
    DDX_Check(pDX, IDC_USETEXTVIEWER, m_bTextView);
    DDX_Check(pDX, IDC_IGNOREEOL2, m_bIgnoreEOL);
    DDX_Check(pDX, IDC_INCLUDEMERGEINFO, m_bIncludeMerge);
}

BEGIN_MESSAGE_MAP(CBlameDlg, CStateStandAloneDialog)
    ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
    ON_EN_CHANGE(IDC_REVISION_END, &CBlameDlg::OnEnChangeRevisionEnd)
END_MESSAGE_MAP()

BOOL CBlameDlg::OnInitDialog()
{
    CStateStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    ExtendFrameIntoClientArea(IDC_DIFFGROUP);
    m_aeroControls.SubclassControl(this, IDC_USETEXTVIEWER);
    m_aeroControls.SubclassControl(this, IDC_INCLUDEMERGEINFO);
    m_aeroControls.SubclassOkCancelHelp(this);

    AdjustControlSize(IDC_USETEXTVIEWER);
    AdjustControlSize(IDC_IGNOREEOL);
    AdjustControlSize(IDC_COMPAREWHITESPACES);
    AdjustControlSize(IDC_IGNOREWHITESPACECHANGES);
    AdjustControlSize(IDC_IGNOREALLWHITESPACES);
    AdjustControlSize(IDC_INCLUDEMERGEINFO);

    CString sWindowTitle;
    GetWindowText(sWindowTitle);
    CAppUtils::SetWindowTitle(m_hWnd, m_path.GetUIPathString(), sWindowTitle);

    m_bTextView     = m_regTextView;
    m_bIncludeMerge = m_regIncludeMerge;
    m_ignoreSpaces  = static_cast<svn_diff_file_ignore_space_t>(static_cast<DWORD>(m_regIgnoreWhitespace));
    // set head revision as default revision
    if (m_endRev.IsHead())
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
    else
    {
        m_sEndRev = m_endRev.ToString();
        UpdateData(FALSE);
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
    }

    CheckRadioButton(IDC_COMPAREWHITESPACES, IDC_IGNOREALLWHITESPACES, IDC_IGNOREALLWHITESPACES);
    switch (m_ignoreSpaces)
    {
        case svn_diff_file_ignore_space_change:
            CheckRadioButton(IDC_COMPAREWHITESPACES, IDC_IGNOREALLWHITESPACES, IDC_IGNOREWHITESPACECHANGES);
            break;
        case svn_diff_file_ignore_space_all:
            CheckRadioButton(IDC_COMPAREWHITESPACES, IDC_IGNOREALLWHITESPACES, IDC_IGNOREALLWHITESPACES);
            break;
        case svn_diff_file_ignore_space_none:
        default:
            CheckRadioButton(IDC_COMPAREWHITESPACES, IDC_IGNOREALLWHITESPACES, IDC_COMPAREWHITESPACES);
            break;
    }

    if ((m_pParentWnd == nullptr) && (GetExplorerHWND()))
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    EnableSaveRestore(L"BlameDlg");

    return TRUE;
}

void CBlameDlg::OnOK()
{
    if (!UpdateData(TRUE))
        return; // don't dismiss dialog (error message already shown by MFC framework)

    m_regTextView     = m_bTextView;
    m_regIncludeMerge = m_bIncludeMerge;
    m_startRev        = SVNRev(m_sStartRev);
    m_endRev          = SVNRev(m_sEndRev);
    if (!m_startRev.IsValid())
    {
        ShowEditBalloon(IDC_REVISON_START, IDS_ERR_INVALIDREV, IDS_ERR_ERROR, TTI_ERROR);
        return;
    }
    m_endRev = SVNRev(m_sEndRev);
    if (GetCheckedRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N) == IDC_REVISION_HEAD)
    {
        m_endRev = SVNRev(L"HEAD");
    }
    if (!m_endRev.IsValid())
    {
        ShowEditBalloon(IDC_REVISION_END, IDS_ERR_INVALIDREV, IDS_ERR_ERROR, TTI_ERROR);
        return;
    }
    BOOL extBlame = CRegDWORD(L"Software\\TortoiseSVN\\TextBlame", FALSE);
    if (extBlame)
        m_bTextView = true;

    int rb = GetCheckedRadioButton(IDC_COMPAREWHITESPACES, IDC_IGNOREALLWHITESPACES);
    switch (rb)
    {
        case IDC_IGNOREWHITESPACECHANGES:
            m_ignoreSpaces = svn_diff_file_ignore_space_change;
            break;
        case IDC_IGNOREALLWHITESPACES:
            m_ignoreSpaces = svn_diff_file_ignore_space_all;
            break;
        case IDC_COMPAREWHITESPACES:
        default:
            m_ignoreSpaces = svn_diff_file_ignore_space_none;
            break;
    }

    m_regIgnoreWhitespace = m_ignoreSpaces;

    UpdateData(FALSE);

    CStateStandAloneDialog::OnOK();
}

void CBlameDlg::OnBnClickedHelp()
{
    OnHelp();
}

void CBlameDlg::OnEnChangeRevisionEnd()
{
    UpdateData();
    if (m_sEndRev.IsEmpty())
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
    else
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
}