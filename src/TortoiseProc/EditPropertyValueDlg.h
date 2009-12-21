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
#include "StandAloneDlg.h"
#include "ProjectProperties.h"
#include "Tooltip.h"
#include "AeroControls.h"

#define	MAX_TT_LENGTH			10000

/**
 * \ingroup TortoiseProc
 * Helper dialog to edit the Subversion properties.
 */

class CEditPropertyValueDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CEditPropertyValueDlg)

public:
	CEditPropertyValueDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CEditPropertyValueDlg();

	enum { IDD = IDD_EDITPROPERTYVALUE };

	void			SetPropertyName(const std::string& sName);
	void			SetPropertyValue(const std::string& sValue);
	std::string		GetPropertyValue() {return m_PropValue;}
	std::string		GetPropertyName() {return m_PropName;}
	bool			GetRecursive() {return !!m_bRecursive;}
	bool			IsBinary() {return m_bIsBinary;}
	bool			IsChanged() { return m_bChanged;}

	void			SetFolder() {m_bFolder = true;}
	void			SetMultiple() {m_bMultiple = true;}
	void			SetDialogTitle(const CString& sTitle) {m_sTitle = sTitle;}
	void			SetPathList(const CTSVNPathList& pathlist) {m_pathList = pathlist;}

	void			RevProps(bool bRevProps = false) {m_bRevProps = bRevProps;}
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnBnClickedHelp();
	afx_msg void OnBnClickedLoadprop();
	afx_msg void OnEnChangePropvalue();

	DECLARE_MESSAGE_MAP()

	void CheckRecursive();
protected:
	CToolTips	m_tooltips;
	CComboBox	m_PropNames;
	CFont		m_valueFont;
	std::string m_PropValue;
	std::string m_PropName;
	CString		m_sPropValue;
	CString		m_sPropName;
	CString		m_sTitle;
	BOOL		m_bRecursive;
	bool		m_bFolder;
	bool		m_bMultiple;
	bool		m_bIsBinary;
	bool		m_bRevProps;
	bool		m_bChanged;
	CTSVNPathList		m_pathList;
	ProjectProperties	m_ProjectProperties;
	AeroControlBase		m_aeroControls;
};
