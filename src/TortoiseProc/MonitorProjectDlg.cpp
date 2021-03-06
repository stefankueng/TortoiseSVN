// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2014, 2016, 2020-2021 - TortoiseSVN

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
#include "MonitorProjectDlg.h"

// CMonitorProjectDlg dialog

IMPLEMENT_DYNAMIC(CMonitorProjectDlg, CStandAloneDialog)

CMonitorProjectDlg::CMonitorProjectDlg(CWnd* pParent /*=NULL*/)
    : CStandAloneDialog(CMonitorProjectDlg::IDD, pParent)
    , m_sName(_T(""))
    , m_sPathOrURL(_T(""))
    , m_sUsername(_T(""))
    , m_sPassword(_T(""))
    , m_sIgnoreUsers(_T(""))
    , m_sIgnoreRegex(_T(""))
    , m_isParentPath(false)
    , m_monitorInterval(30)
{
}

CMonitorProjectDlg::~CMonitorProjectDlg()
{
}

void CMonitorProjectDlg::DoDataExchange(CDataExchange* pDX)
{
    CStandAloneDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_NAME, m_sName);
    DDX_Text(pDX, IDC_PATHORURL, m_sPathOrURL);
    DDX_Text(pDX, IDC_USERNAME, m_sUsername);
    DDX_Text(pDX, IDC_PASSWORD, m_sPassword);
    DDX_Text(pDX, IDC_INTERVAL, m_monitorInterval);
    DDV_MinMaxInt(pDX, m_monitorInterval, 1, INT_MAX);
    DDX_Text(pDX, IDC_IGNOREUSERS, m_sIgnoreUsers);
    DDX_Text(pDX, IDC_IGNOREREGEX, m_sIgnoreRegex);
    DDX_Check(pDX, IDC_PARENTPATH, m_isParentPath);
}

BEGIN_MESSAGE_MAP(CMonitorProjectDlg, CStandAloneDialog)
END_MESSAGE_MAP()

// CMonitorProjectDlg message handlers

void CMonitorProjectDlg::OnOK()
{
    UpdateData();

    try
    {
        std::wregex r1 = std::wregex(m_sIgnoreRegex);
        UNREFERENCED_PARAMETER(r1);
    }
    catch (std::exception&)
    {
        CString        text  = CString(MAKEINTRESOURCE(IDS_ERR_INVALIDREGEX));
        CString        title = CString(MAKEINTRESOURCE(IDS_ERR_ERROR));
        EDITBALLOONTIP bt;
        bt.cbStruct = sizeof(bt);
        bt.pszText  = text;
        bt.pszTitle = title;
        bt.ttiIcon  = TTI_WARNING;
        SendDlgItemMessage(IDC_IGNOREREGEX, EM_SHOWBALLOONTIP, 0, reinterpret_cast<LPARAM>(&bt));
        return;
    }
    // remove newlines in case the url was pasted with such
    m_sPathOrURL.Remove('\n');
    m_sPathOrURL.Remove('\r');
    CStandAloneDialog::OnOK();
}
