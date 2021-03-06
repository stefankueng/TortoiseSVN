// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2013, 2015, 2020-2021 - TortoiseSVN

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
#include "CodeCollaboratorSettingsDlg.h"
#include "AppUtils.h"
#include "StringUtils.h"

// CodeCollaboratorSettingsDlg dialog

IMPLEMENT_DYNAMIC(CodeCollaboratorSettingsDlg, CStandAloneDialog)

CodeCollaboratorSettingsDlg::CodeCollaboratorSettingsDlg(CWnd* pParent /*=NULL*/)
    : CStandAloneDialog(CodeCollaboratorSettingsDlg::IDD, pParent)
{
    m_regCollabUser     = CRegString(L"Software\\TortoiseSVN\\CodeCollaborator\\CollabUser", L"");
    m_regCollabPassword = CRegString(L"Software\\TortoiseSVN\\CodeCollaborator\\CollabPassword", L"");
    m_regSvnUser        = CRegString(L"Software\\TortoiseSVN\\CodeCollaborator\\SvnUser", L"");
    m_regSvnPassword    = CRegString(L"Software\\TortoiseSVN\\CodeCollaborator\\SvnPassword", L"");
}

CodeCollaboratorSettingsDlg::~CodeCollaboratorSettingsDlg()
{
}

void CodeCollaboratorSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
    CStandAloneDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_COLLABORATOR_USEREDIT, m_collabUser);
    DDX_Text(pDX, IDC_COLLABORATOR_PASSEDIT, m_collabPassword);
    DDX_Text(pDX, IDC_SVN_USEREDIT, m_svnUser);
    DDX_Text(pDX, IDC_SVN_PASSEDIT, m_svnPassword);
}

BEGIN_MESSAGE_MAP(CodeCollaboratorSettingsDlg, CStandAloneDialog)
    ON_BN_CLICKED(IDOK, &CodeCollaboratorSettingsDlg::OnBnClickedOk)
END_MESSAGE_MAP()

BOOL CodeCollaboratorSettingsDlg::OnInitDialog()
{
    CStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    m_collabUser = static_cast<CString>(m_regCollabUser);
    m_svnUser    = static_cast<CString>(m_regSvnUser);

    UpdateData(FALSE);
    return TRUE;
}

void CodeCollaboratorSettingsDlg::OnBnClickedOk()
{
    UpdateData();
    if (!m_svnPassword.IsEmpty())
    {
        m_regSvnPassword = CStringUtils::Encrypt(static_cast<LPCWSTR>(m_svnPassword));
        m_regSvnPassword.write();
    }
    if (!m_collabPassword.IsEmpty())
    {
        m_regCollabPassword = CStringUtils::Encrypt(static_cast<LPCWSTR>(m_collabPassword));
        m_regCollabPassword.write();
    }
    m_regSvnUser = static_cast<CString>(m_svnUser);
    m_regSvnUser.write();
    m_regCollabUser = static_cast<CString>(m_collabUser);
    m_regCollabUser.write();

    CStandAloneDialog::OnOK();
}
