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

class IListCtrlTooltipProvider
{
public:
	virtual CString GetToolTipText(int nItem, int nSubItem) = 0;
};

/**
 * \ingroup Utils
 * Allows to show a hint text on a list control, basically hiding the list control
 * content. Can be used for example during lengthy operations (showing "please wait")
 * or to indicate why the list control is empty (showing "no data available").
 */
class CHintListCtrl : public CListCtrl
{
public: 
	CHintListCtrl();
	~CHintListCtrl();

	void ShowText(const CString& sText, bool forceupdate = false);
	void ClearText();
	bool HasText() const {return !m_sText.IsEmpty();}
	void SetTooltipProvider(IListCtrlTooltipProvider * provider) {pProvider = provider;}


protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
	virtual afx_msg BOOL OnToolTipText(UINT id, NMHDR * pNMHDR, LRESULT * pResult); 
	virtual INT_PTR OnToolHitTest(CPoint point, TOOLINFO * pTI) const;

private:
	CString			m_sText;
	IListCtrlTooltipProvider * pProvider;
};
