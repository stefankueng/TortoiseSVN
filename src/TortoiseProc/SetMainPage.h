// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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
#include "Balloon.h"
#include "FontPreviewCombo.h"
#include "Registry.h"
#include "afxwin.h"


/**
 * \ingroup TortoiseProc
 * This is the mainpage of the settings. It contains all the most important
 * settings.
 */
class CSetMainPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CSetMainPage)

public:
	CSetMainPage();
	virtual ~CSetMainPage();
	/**
	 * Saves the changed settings to the registry.
	 * returns 0 if no restart is needed for the changes to take effect
	 * \remark If the dialog is closed/dismissed without calling
	 * this method first then all settings the user made must be
	 * discarded!
	 */
	int SaveData();
	
	UINT GetIconID() {return IDI_GENERAL;}

	enum { IDD = IDD_SETTINGSMAIN };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	
	CString GetVersionFromFile(const CString & p_strDateiname);

private:
	CRegString		m_regExtensions;
	CString			m_sTempExtensions;
	CBalloon		m_tooltips;
	CComboBox		m_LanguageCombo;
	CRegDWORD		m_regLanguage;
	DWORD			m_dwLanguage;
	CRegDWORD		m_regCheckNewer;
	BOOL			m_bCheckNewer;
	CRegString		m_regLastCommitTime;
	BOOL			m_bLastCommitTime;

	BOOL			m_bUseDotNetHack;

public:
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnApply();
	afx_msg void OnModified();
	afx_msg void OnASPHACK();
	afx_msg void OnBnClickedEditconfig();
	afx_msg void OnBnClickedChecknewerbutton();
	afx_msg void OnBnClickedSounds();
};
