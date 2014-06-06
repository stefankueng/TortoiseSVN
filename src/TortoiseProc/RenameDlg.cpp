// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2011, 2013-2014 - TortoiseSVN

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
#include "TSVNPath.h"
#include "RenameDlg.h"
#include "AppUtils.h"
#include "ControlsBridge.h"


IMPLEMENT_DYNAMIC(CRenameDlg, CResizableStandAloneDialog)
CRenameDlg::CRenameDlg(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CRenameDlg::IDD, pParent)
    , m_name(L"")
    , m_renameRequired(true)
    , m_pInputValidator(NULL)
    , m_bBalloonVisible(false)
    , m_bFSAutoComplete(false)
{
}

CRenameDlg::~CRenameDlg()
{
}

void CRenameDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_NAME, m_name);
}


BEGIN_MESSAGE_MAP(CRenameDlg, CResizableStandAloneDialog)
    ON_WM_SIZING()
    ON_EN_SETFOCUS(IDC_NAME, &CRenameDlg::OnEnSetfocusName)
END_MESSAGE_MAP()

BOOL CRenameDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    ExtendFrameIntoClientArea(IDC_DWM);
    m_aeroControls.SubclassOkCancel(this);

    if (m_bFSAutoComplete)
    {
        DWORD len = GetCurrentDirectory(0, NULL);
        if (len)
        {
            std::unique_ptr<TCHAR[]> originalCurrentDirectory(new TCHAR[len]);
            if (GetCurrentDirectory(len, originalCurrentDirectory.get()))
            {
                CAppUtils::EnableAutoComplete(GetDlgItem(IDC_NAME)->GetSafeHwnd(), originalCurrentDirectory.get(), AUTOCOMPLETELISTOPTIONS(ACLO_CURRENTDIR | ACLO_FILESYSONLY), AUTOCOMPLETEOPTIONS(ACO_AUTOSUGGEST | ACO_USETAB));
            }
        }
    }
    else
        SHAutoComplete(GetDlgItem(IDC_NAME)->m_hWnd, SHACF_DEFAULT);

    if (!m_windowtitle.IsEmpty())
        this->SetWindowText(m_windowtitle);
    if (!m_label.IsEmpty())
        SetDlgItemText(IDC_LABEL, m_label);

    if (!m_name.IsEmpty())
    {
        CString sTmp;
        sTmp.Format(IDS_RENAME_INFO, (LPCWSTR)m_name);
        SetDlgItemText(IDC_RENINFOLABEL, sTmp);
    }

    AddAnchor(IDC_RENINFOLABEL, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_LABEL, TOP_LEFT);
    AddAnchor(IDC_NAME, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_DWM, TOP_LEFT);
    AddAnchor(IDOK, BOTTOM_RIGHT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);

    CControlsBridge::AlignHorizontally(this, IDC_LABEL, IDC_NAME);
    if (GetExplorerHWND())
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    EnableSaveRestore(L"RenameDlg");
    m_originalName = m_name;
    return TRUE;
}

void CRenameDlg::OnOK()
{
    UpdateData();
    m_name.Trim();
    if (m_pInputValidator)
    {
        CString sError = m_pInputValidator->Validate(IDC_NAME, m_name);
        if (!sError.IsEmpty())
        {
            m_bBalloonVisible = true;
            ShowEditBalloon(IDC_NAME, sError, CString(MAKEINTRESOURCE(IDS_ERR_ERROR)), TTI_ERROR);
            return;
        }
    }
    bool nameAllowed = ((m_originalName != m_name) || !m_renameRequired)
                       && !m_name.IsEmpty();
    if (!nameAllowed)
    {
        m_bBalloonVisible = true;
        ShowEditBalloon(IDC_NAME, IDS_WARN_RENAMEREQUIRED, IDS_ERR_ERROR, TTI_ERROR);
        return;
    }

    CTSVNPath path(m_name);
    if (!path.IsValidOnWindows())
    {
        m_bBalloonVisible = true;
        ShowEditBalloon(IDC_NAME, IDS_WARN_NOVALIDPATH, IDS_ERR_ERROR, TTI_ERROR);
        return;
    }
    CResizableDialog::OnOK();
}

void CRenameDlg::OnSizing(UINT fwSide, LPRECT pRect)
{
    // don't allow the dialog to be changed in height
    CRect rcWindowRect;
    GetWindowRect(&rcWindowRect);
    switch (fwSide)
    {
    case WMSZ_BOTTOM:
    case WMSZ_BOTTOMLEFT:
    case WMSZ_BOTTOMRIGHT:
        pRect->bottom = pRect->top + rcWindowRect.Height();
        break;
    case WMSZ_TOP:
    case WMSZ_TOPLEFT:
    case WMSZ_TOPRIGHT:
        pRect->top = pRect->bottom - rcWindowRect.Height();
        break;
    }
    CResizableStandAloneDialog::OnSizing(fwSide, pRect);
}

void CRenameDlg::OnCancel()
{
    // find out if there's a balloon tip showing and if there is,
    // hide that tooltip but do NOT exit the dialog.
    if (m_bBalloonVisible)
    {
        Edit_HideBalloonTip(GetDlgItem(IDC_NAME)->GetSafeHwnd());
        return;
    }

    CResizableStandAloneDialog::OnCancel();
}

void CRenameDlg::OnEnSetfocusName()
{
    m_bBalloonVisible = false;
}
