// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - TortoiseSVN

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
#include "MyGraph.h"
#include "XPImageButton.h"
#include "TSVNPath.h"
#include "UnicodeUtils.h"

/**
 * \ingroup TortoiseProc
 * Helper class for drawing and then saving the drawing to a metafile (wmf)
 */
class CMyMetaFileDC : public CMetaFileDC
{
public:
	HGDIOBJ CMyMetaFileDC::SelectObject(HGDIOBJ hObject) 
	{
		return (hObject != NULL) ? ::SelectObject(m_hDC, hObject) : NULL; 
	}
};

/**
 * \ingroup TortoiseProc
 * Helper dialog showing statistics gathered from the log messages shown in the
 * log dialog.
 */
class CStatGraphDlg : public CResizableStandAloneDialog//CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CStatGraphDlg)

public:
	CStatGraphDlg(CWnd* pParent = NULL);
	virtual ~CStatGraphDlg();

	enum { IDD = IDD_STATGRAPH };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnCbnSelchangeGraphcombo();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnBnClickedStacked();
	afx_msg void OnNeedText(NMHDR *pnmh, LRESULT *pResult);
	afx_msg void OnBnClickedGraphbarbutton();
	afx_msg void OnBnClickedGraphbarstackedbutton();
	afx_msg void OnBnClickedGraphlinebutton();
	afx_msg void OnBnClickedGraphlinestackedbutton();
	afx_msg void OnBnClickedGraphpiebutton();
	afx_msg void OnFileSavestatgraphas();

	DECLARE_MESSAGE_MAP()

	CPtrArray		m_graphDataArray;
	MyGraph			m_graph;
	CComboBox		m_cGraphType;
	CSliderCtrl		m_Skipper;
	BOOL			m_bIgnoreAuthorCase;

	CXPImageButton	m_btnGraphBar;
	CXPImageButton	m_btnGraphBarStacked;
	CXPImageButton	m_btnGraphLine;
	CXPImageButton	m_btnGraphLineStacked;
	CXPImageButton	m_btnGraphPie;

	HICON			m_hGraphBarIcon;
	HICON			m_hGraphBarStackedIcon;
	HICON			m_hGraphLineIcon;
	HICON			m_hGraphLineStackedIcon;
	HICON			m_hGraphPieIcon;

	MyGraph::GraphType	m_GraphType;
	bool			m_bStacked;

	void		ShowCommitsByDate();
	void		ShowCommitsByAuthor();
	void		ShowStats();

	int			GetWeek(const CTime& time);
	int			GetWeeksCount();
	int			m_weekcount;

	void		ShowLabels(BOOL bShow);
	void		RedrawGraph();
	void		CountCommits(std::map<stdstring, LONG> &authors, 
								std::map<stdstring, LONG> &AuthorCommits, 
								std::map<stdstring, LONG> &AuthorCommitsMin, 
								std::map<stdstring, LONG> &AuthorCommitsMax, 
								std::map<stdstring, LONG> &authorcommits);

	enum UnitType
	{
		Weeks,
		Months,
		Quarters,
		Years
	};
	void		InitUnits();
	int			GetUnitCount();
	int			GetUnit(const CTime& time);
	CStatGraphDlg::UnitType	GetUnitType();
	CString		GetUnitString();
	CString		GetUnitLabel(int unit, CTime &lasttime);

	void		EnableDisableMenu();

	void		SaveGraph(CString sFilename);
	int			GetEncoderClsid(const WCHAR* format, CLSID* pClsid);

	CToolTipCtrl* m_pToolTip;
public:
	CDWordArray	*	m_parDates;
	CDWordArray	*	m_parFileChanges;
	CStringArray *	m_parAuthors;
	CTSVNPath		m_path;
};
