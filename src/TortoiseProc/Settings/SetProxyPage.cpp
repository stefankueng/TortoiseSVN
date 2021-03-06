// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007, 2009-2011, 2013-2015, 2021 - TortoiseSVN

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
#include "SetProxyPage.h"
#include "AppUtils.h"
#include "StringUtils.h"
#include "SVN.h"

IMPLEMENT_DYNAMIC(CSetProxyPage, ISettingsPropPage)
CSetProxyPage::CSetProxyPage()
    : ISettingsPropPage(CSetProxyPage::IDD)
    , m_serverport(0)
    , m_timeout(0)
    , m_isEnabled(FALSE)
{
    m_regServerAddress = CRegString(L"Software\\Tigris.org\\Subversion\\Servers\\global\\http-proxy-host", L"");
    m_regServerport    = CRegString(L"Software\\Tigris.org\\Subversion\\Servers\\global\\http-proxy-port", L"");
    m_regUsername      = CRegString(L"Software\\Tigris.org\\Subversion\\Servers\\global\\http-proxy-username", L"");
    m_regPassword      = CRegString(L"Software\\Tigris.org\\Subversion\\Servers\\global\\http-proxy-password", L"");
    m_regTimeout       = CRegString(L"Software\\Tigris.org\\Subversion\\Servers\\global\\http-proxy-timeout", L"");
    m_regSSHClient     = CRegString(L"Software\\TortoiseSVN\\SSH");
    m_sshClient        = m_regSSHClient;
    m_regExceptions    = CRegString(L"Software\\Tigris.org\\Subversion\\Servers\\global\\http-proxy-exceptions", L"");

    m_regServerAddressCopy = CRegString(L"Software\\TortoiseSVN\\Servers\\global\\http-proxy-host", L"");
    m_regServerportCopy    = CRegString(L"Software\\TortoiseSVN\\Servers\\global\\http-proxy-port", L"");
    m_regUsernameCopy      = CRegString(L"Software\\TortoiseSVN\\Servers\\global\\http-proxy-username", L"");
    m_regPasswordCopy      = CRegString(L"Software\\TortoiseSVN\\Servers\\global\\http-proxy-password", L"");
    m_regTimeoutCopy       = CRegString(L"Software\\TortoiseSVN\\Servers\\global\\http-proxy-timeout", L"");
    m_regExceptionsCopy    = CRegString(L"Software\\TortoiseSVN\\Servers\\global\\http-proxy-exceptions", L"");
}

CSetProxyPage::~CSetProxyPage()
{
}

void CSetProxyPage::DoDataExchange(CDataExchange* pDX)
{
    ISettingsPropPage::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_SERVERADDRESS, m_serverAddress);
    DDX_Text(pDX, IDC_SERVERPORT, m_serverport);
    DDX_Text(pDX, IDC_USERNAME, m_username);
    DDX_Text(pDX, IDC_PASSWORD, m_password);
    DDX_Text(pDX, IDC_TIMEOUT, m_timeout);
    DDX_Text(pDX, IDC_EXCEPTIONS, m_exceptions);
    DDX_Check(pDX, IDC_ENABLE, m_isEnabled);
    DDX_Text(pDX, IDC_SSHCLIENT, m_sshClient);
    DDX_Control(pDX, IDC_SSHCLIENT, m_cSSHClientEdit);
}

BEGIN_MESSAGE_MAP(CSetProxyPage, ISettingsPropPage)
    ON_EN_CHANGE(IDC_SERVERADDRESS, OnChange)
    ON_EN_CHANGE(IDC_SERVERPORT, OnChange)
    ON_EN_CHANGE(IDC_USERNAME, OnChange)
    ON_EN_CHANGE(IDC_PASSWORD, OnChange)
    ON_EN_CHANGE(IDC_TIMEOUT, OnChange)
    ON_EN_CHANGE(IDC_SSHCLIENT, OnChange)
    ON_EN_CHANGE(IDC_EXCEPTIONS, OnChange)
    ON_BN_CLICKED(IDC_ENABLE, OnBnClickedEnable)
    ON_BN_CLICKED(IDC_SSHBROWSE, OnBnClickedSshbrowse)
    ON_BN_CLICKED(IDC_EDITSERVERS, OnBnClickedEditservers)
END_MESSAGE_MAP()

BOOL CSetProxyPage::OnInitDialog()
{
    ISettingsPropPage::OnInitDialog();

    m_tooltips.AddTool(IDC_SERVERADDRESS, IDS_SETTINGS_PROXYSERVER_TT);
    m_tooltips.AddTool(IDC_EXCEPTIONS, IDS_SETTINGS_PROXYEXCEPTIONS_TT);

    m_sshClient     = m_regSSHClient;
    m_serverAddress = m_regServerAddress;
    m_serverport    = _wtoi(static_cast<CString>(m_regServerport));
    m_username      = m_regUsername;
    m_password      = m_regPassword;
    m_exceptions    = m_regExceptions;
    m_timeout       = _wtoi(static_cast<CString>(m_regTimeout));

    if (m_serverAddress.IsEmpty())
    {
        m_isEnabled = FALSE;
        EnableGroup(FALSE);
        // now since we already created our registry entries
        // we delete them here again...
        m_regServerAddress.removeValue();
        m_regServerport.removeValue();
        m_regUsername.removeValue();
        m_regPassword.removeValue();
        m_regTimeout.removeValue();
        m_regExceptions.removeValue();
    }
    else
    {
        m_isEnabled = TRUE;
        EnableGroup(TRUE);
    }
    if (m_serverAddress.IsEmpty())
        m_serverAddress = m_regServerAddressCopy;
    if (m_serverport == 0)
        m_serverport = _wtoi(static_cast<CString>(m_regServerportCopy));
    if (m_username.IsEmpty())
        m_username = m_regUsernameCopy;
    if (m_password.IsEmpty())
        m_password = m_regPasswordCopy;
    if (m_exceptions.IsEmpty())
        m_exceptions = m_regExceptionsCopy;
    if (m_timeout == 0)
        m_timeout = _wtoi(static_cast<CString>(m_regTimeoutCopy));

    SHAutoComplete(::GetDlgItem(m_hWnd, IDC_SSHCLIENT), SHACF_FILESYSTEM | SHACF_FILESYS_ONLY);

    UpdateData(FALSE);

    return TRUE;
}

void CSetProxyPage::OnBnClickedEnable()
{
    UpdateData();
    if (m_isEnabled)
    {
        EnableGroup(TRUE);
    }
    else
    {
        EnableGroup(FALSE);
    }
    SetModified();
}

void CSetProxyPage::EnableGroup(BOOL b)
{
    DialogEnableWindow(IDC_SERVERADDRESS, b);
    DialogEnableWindow(IDC_SERVERPORT, b);
    DialogEnableWindow(IDC_USERNAME, b);
    DialogEnableWindow(IDC_PASSWORD, b);
    DialogEnableWindow(IDC_TIMEOUT, b);
    DialogEnableWindow(IDC_EXCEPTIONS, b);
    DialogEnableWindow(IDC_PROXYLABEL1, b);
    DialogEnableWindow(IDC_PROXYLABEL2, b);
    DialogEnableWindow(IDC_PROXYLABEL3, b);
    DialogEnableWindow(IDC_PROXYLABEL4, b);
    DialogEnableWindow(IDC_PROXYLABEL5, b);
    DialogEnableWindow(IDC_PROXYLABEL6, b);
}

void CSetProxyPage::OnChange()
{
    SetModified();
}

BOOL CSetProxyPage::OnApply()
{
    UpdateData();
    if (m_isEnabled)
    {
        CString temp;
        Store(m_serverAddress, m_regServerAddress);
        m_regServerAddressCopy = m_serverAddress;
        temp.Format(L"%u", m_serverport);
        Store(temp, m_regServerport);
        m_regServerportCopy = temp;
        Store(m_username, m_regUsername);
        m_regUsernameCopy = m_username;
        Store(m_password, m_regPassword);
        m_regPasswordCopy = m_password;
        temp.Format(L"%u", m_timeout);
        Store(temp, m_regTimeout);
        m_regTimeoutCopy = temp;
        Store(m_exceptions, m_regExceptions);
        m_regExceptionsCopy = m_exceptions;
    }
    else
    {
        m_regServerAddress.removeValue();
        m_regServerport.removeValue();
        m_regUsername.removeValue();
        m_regPassword.removeValue();
        m_regTimeout.removeValue();
        m_regExceptions.removeValue();

        CString temp;
        m_regServerAddressCopy = m_serverAddress;
        temp.Format(L"%u", m_serverport);
        m_regServerportCopy = temp;
        m_regUsernameCopy   = m_username;
        m_regPasswordCopy   = m_password;
        temp.Format(L"%u", m_timeout);
        m_regTimeoutCopy    = temp;
        m_regExceptionsCopy = m_exceptions;
    }
    m_regSSHClient = m_sshClient;
    SetModified(FALSE);
    return ISettingsPropPage::OnApply();
}

void CSetProxyPage::OnBnClickedSshbrowse()
{
    CString openPath;
    if (CAppUtils::FileOpenSave(openPath, nullptr, IDS_SETTINGS_SELECTSSH, IDS_PROGRAMSFILEFILTER, true, CString(), m_hWnd))
    {
        UpdateData();
        PathQuoteSpaces(openPath.GetBuffer(MAX_PATH));
        openPath.ReleaseBuffer(-1);
        m_sshClient = openPath;
        UpdateData(FALSE);
        SetModified();
    }
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void CSetProxyPage::OnBnClickedEditservers()
{
    SVN::EnsureConfigFile();
    PWSTR pszPath = nullptr;
    if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, nullptr, &pszPath) == S_OK)
    {
        CString path = pszPath;
        CoTaskMemFree(pszPath);

        path += L"\\Subversion\\servers";
        CAppUtils::StartTextViewer(path);
    }
}
