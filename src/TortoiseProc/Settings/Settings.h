// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2009 - TortoiseSVN

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
#include "SetMainPage.h"
#include "SetProxyPage.h"
#include "SetOverlayPage.h"
#include "SettingsProgsDiff.h"
#include "SettingsProgsMerge.h"
#include "SettingsProgsUniDiff.h"
#include "SetOverlayIcons.h"
#include "SetLookAndFeelPage.h"
#include "SetDialogs.h"
#include "SettingsColors.h"
#include "SetMisc.h"
#include "SetLogCache.h"
#include "SettingsLogCaches.h"
#include "SetSavedDataPage.h"
#include "SetHooks.h"
#include "SetBugTraq.h"
#include "SettingsTBlame.h"
#include "SettingsRevisionGraph.h"
#include "SettingsRevGraphColors.h"
#include "TreePropSheet/TreePropSheet.h"
#include "SettingsAdvanced.h"

#include "AeroGlass.h"
#include "AeroControls.h"

using namespace TreePropSheet;

/**
 * \ingroup TortoiseProc
 * This is the container for all settings pages. A setting page is
 * a class derived from CPropertyPage with an additional method called
 * SaveData(). The SaveData() method is used by the dialog to save
 * the settings the user has made - if that method is not called then
 * it means that the changes are discarded! Each settings page has
 * to make sure that no changes are saved outside that method.
 */
class CSettings : public CTreePropSheet
{
	DECLARE_DYNAMIC(CSettings)
private:
	/**
	 * Adds all pages to this Settings-Dialog.
	 */
	void AddPropPages();
	/**
	 * Removes the pages and frees up memory.
	 */
	void RemovePropPages();

private:
	CSetMainPage *			m_pMainPage;
	CSetProxyPage *			m_pProxyPage;
	CSetOverlayPage *		m_pOverlayPage;
	CSetOverlayIcons *		m_pOverlaysPage;
	CSettingsProgsDiff*		m_pProgsDiffPage;
	CSettingsProgsMerge *	m_pProgsMergePage;
	CSettingsProgsUniDiff * m_pProgsUniDiffPage;
	CSetLookAndFeelPage *	m_pLookAndFeelPage;
	CSetDialogs *			m_pDialogsPage;
    CSettingsRevisionGraph* m_pRevisionGraphPage;
    CSettingsRevisionGraphColors* m_pRevisionGraphColorsPage;
	CSettingsColors *		m_pColorsPage;
	CSetMisc *				m_pMiscPage;
	CSetLogCache *			m_pLogCachePage;
    CSettingsLogCaches*     m_pLogCacheListPage;
	CSetSavedDataPage *		m_pSavedPage;
	CSetHooks *				m_pHooksPage;
	CSetBugTraq *			m_pBugTraqPage;
	CSettingsTBlame *		m_pTBlamePage;
	CSettingsAdvanced *		m_pAdvanced;

	HICON					m_hIcon;
	CDwmApiImpl				m_Dwm;
	AeroControlBase			m_aeroControls;
public:
	CSettings(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	virtual ~CSettings();

	/**
	 * Calls the SaveData()-methods of each of the settings pages.
	 */
	void HandleRestart();
protected:
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg HCURSOR OnQueryDragIcon();
};


