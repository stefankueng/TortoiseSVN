// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Tim Kemp and Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
#pragma once
#include "afxwin.h"
#include "IconStatic.h"

/**
 * \ingroup TortoiseProc
 * Settings page for the icon overlays.
 *
 * \par requirements
 * win95 or later\n
 * winNT4 or later\n
 * MFC\n
 *
 * \version 1.0
 * first version
 *
 * \date 04-14-2003
 *
 * \author Stefan Kueng
 *
 * \par license
 * This code is absolutely free to use and modify. The code is provided "as is" with
 * no expressed or implied warranty. The author accepts no liability if it causes
 * any damage to your computer, causes your pet to fall ill, increases baldness
 * or makes your car start emitting strange noises when you start it up.
 * This code has no bugs, just undocumented features!
 * 
 * \todo 
 *
 * \bug 
 *
 */
class CSetOverlayPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CSetOverlayPage)

public:
	CSetOverlayPage();   // standard constructor
	virtual ~CSetOverlayPage();
	/**
	 * Saves the changed settings to the registry.
	 * \remark If the dialog is closed/dismissed without calling
	 * this method first then all settings the user made must be
	 * discarded!
	 */
	void SaveData();

// Dialog Data
	enum { IDD = IDD_SETTINGSOVERLAY };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();

private:
	BOOL			m_bShowChangedDirs;
	BOOL			m_bRemovable;
	BOOL			m_bNetwork;
	BOOL			m_bFixed;
	BOOL			m_bCDROM;
	CRegDWORD		m_regShowChangedDirs;
	CRegDWORD		m_regDriveMaskRemovable;
	CRegDWORD		m_regDriveMaskRemote;
	CRegDWORD		m_regDriveMaskFixed;
	CRegDWORD		m_regDriveMaskCDROM;
	CBalloon		m_tooltips;
	CIconStatic		m_cDriveGroup;
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnBnClickedChangeddirs();
	afx_msg void OnBnClickedRemovable();
	afx_msg void OnBnClickedNetwork();
	afx_msg void OnBnClickedFixed();
	afx_msg void OnBnClickedCdrom();
	virtual BOOL OnApply();
};
