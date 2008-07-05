// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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
#pragma once

#include "SettingsPropPage.h"
#include "Tooltip.h"
#include "Registry.h"
#include "FileDropEdit.h"


/**
 * \ingroup TortoiseProc
 * This is the Proxy page of the settings dialog. It gives the user the
 * possibility to set the proxy settings for subversion clients.
 */
class CSetProxyPage : public ISettingsPropPage
{
	DECLARE_DYNAMIC(CSetProxyPage)

public:
	CSetProxyPage();
	virtual ~CSetProxyPage();

	UINT GetIconID() {return IDI_PROXY;}

	enum { IDD = IDD_SETTINGSPROXY };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnApply();
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnChange();
	afx_msg void OnBnClickedEnable();
	afx_msg void OnBnClickedSshbrowse();
	afx_msg void OnBnClickedEditservers();

	void EnableGroup(BOOL b);

	DECLARE_MESSAGE_MAP()
private:
	CToolTips	m_tooltips;
	CString		m_serveraddress;
	CRegString	m_regServeraddress;
	CRegString	m_regServeraddress_copy;
	UINT		m_serverport;
	CRegString	m_regServerport;
	CRegString	m_regServerport_copy;
	CString		m_username;
	CRegString	m_regUsername;
	CRegString	m_regUsername_copy;
	CString		m_password;
	CRegString	m_regPassword;
	CRegString	m_regPassword_copy;
	UINT		m_timeout;
	CRegString	m_regTimeout;
	CRegString	m_regTimeout_copy;
	BOOL		m_isEnabled;
	CRegString	m_regSSHClient;
	CString		m_SSHClient;
	CRegString	m_regExceptions;
	CRegString	m_regExceptions_copy;
	CString		m_Exceptions;
	CFileDropEdit m_cSSHClientEdit;
};
