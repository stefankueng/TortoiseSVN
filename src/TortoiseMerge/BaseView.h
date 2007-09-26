// TortoiseMerge - a Diff/Patch program

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
#include "DiffData.h"
#include "SVNLineDiff.h"
#include "ScrollTool.h"
#include "Undo.h"
#include "LocatorBar.h"

/**
 * \ingroup TortoiseMerge
 *
 * View class providing the basic functionality for
 * showing diffs. Has three parent classes which inherit
 * from this base class: CLeftView, CRightView and CBottomView.
 */
class CBaseView : public CView
{
    DECLARE_DYNCREATE(CBaseView)
friend class CLineDiffBar;
public:
	CBaseView();
	virtual ~CBaseView();

public:
	/**
	 * Indicates that the underlying document has been updated. Reloads all
	 * data and redraws the view.
	 */
	virtual void	DocumentUpdated();
	/**
	 * Returns the number of lines visible on the view.
	 */
	int				GetScreenLines();
	/**
	 * Scrolls the view to the given line.
	 * \param nNewTopLine The new top line to scroll the view to
	 * \param bTrackScrollBar If TRUE, then the scrollbars are affected too.
	 */
	void			ScrollToLine(int nNewTopLine, BOOL bTrackScrollBar = TRUE);
	void			ScrollAllToLine(int nNewTopLine, BOOL bTrackScrollBar = TRUE);
	void			ScrollSide(int delta);
	void			GoToLine(int nNewLine, BOOL bAll = TRUE);

	void			SelectLines(int nLine1, int nLine2 = -1);
	void			HiglightLines(int start, int end = -1);
	inline BOOL		IsHidden() const  {return m_bIsHidden;}
	inline void		SetHidden(BOOL bHidden) {m_bIsHidden = bHidden;}
	inline BOOL		IsModified() const  {return m_bModified;}
	void			SetModified(BOOL bModified = TRUE) {m_bModified = bModified;}
	BOOL			HasSelection() {return (!((m_nSelBlockEnd < 0)||(m_nSelBlockStart < 0)||(m_nSelBlockStart > m_nSelBlockEnd)));}
	BOOL			GetSelection(int& start, int& end) {start=m_nSelBlockStart; end=m_nSelBlockEnd; return HasSelection();}
	void			SetInlineWordDiff(bool bWord) {m_bInlineWordDiff = bWord;}

	BOOL			IsLineRemoved(int nLineIndex);
	bool			IsBlockWhitespaceOnly(int nLineIndex, bool& bIdentical);
	bool			IsLineConflicted(int nLineIndex);

	CViewData *		m_pViewData;
	CViewData *		m_pOtherViewData;

	CString			m_sWindowName;		///< The name of the view which is shown as a window title to the user
	CString			m_sFullFilePath;	///< The full path of the file shown
	CFileTextLines::UnicodeType texttype;	///< the text encoding this view uses
	EOL lineendings; ///< the line endings the view uses

	BOOL			m_bViewWhitespace;	///< If TRUE, then SPACE and TAB are shown as special characters
	BOOL			m_bShowInlineDiff;	///< If TRUE, diffs in lines are marked colored
	int				m_nTopLine;			///< The topmost text line in the view

	static CLocatorBar * m_pwndLocator;	///< Pointer to the locator bar on the left
	static CLineDiffBar * m_pwndLineDiffBar;	///< Pointer to the line diff bar at the bottom
	static CStatusBar * m_pwndStatusBar;///< Pointer to the status bar
	static CMainFrame * m_pMainFrame;	///< Pointer to the mainframe

	void			GoToFirstDifference();
protected:
	virtual BOOL	PreCreateWindow(CREATESTRUCT& cs);
	virtual void	OnDraw(CDC * pDC);
	virtual INT_PTR OnToolHitTest(CPoint point, TOOLINFO* pTI) const;
	virtual BOOL	PreTranslateMessage(MSG* pMsg);
	BOOL			OnToolTipNotify(UINT id, NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void	OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void	OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg BOOL	OnEraseBkgnd(CDC* pDC);
	afx_msg int		OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void	OnDestroy();
	afx_msg void	OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL	OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg BOOL	OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void	OnKillFocus(CWnd* pNewWnd);
	afx_msg void	OnSetFocus(CWnd* pOldWnd);
	afx_msg void	OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void	OnMergeNextdifference();
	afx_msg void	OnMergePreviousdifference();
	afx_msg void	OnMergePreviousconflict();
	afx_msg void	OnMergeNextconflict();
	afx_msg void	OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void	OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void	OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void	OnEditCopy();
	afx_msg void	OnMouseMove(UINT nFlags, CPoint point);

	DECLARE_MESSAGE_MAP()

protected:
	void			DrawHeader(CDC *pdc, const CRect &rect);
	void			DrawMargin(CDC *pdc, const CRect &rect, int nLineIndex);
	void			DrawSingleLine(CDC *pDC, const CRect &rc, int nLineIndex);
	/**
	 * Draws the line ending 'char'.
	 */
	void			DrawLineEnding(CDC *pDC, const CRect &rc, int nLineIndex, const CPoint& origin);
	void			ExpandChars(LPCTSTR pszChars, int nOffset, int nCount, CString &line);

	void			RecalcVertScrollBar(BOOL bPositionOnly = FALSE);
	void			RecalcAllVertScrollBars(BOOL bPositionOnly = FALSE);
	void			RecalcHorzScrollBar(BOOL bPositionOnly = FALSE);
	void			RecalcAllHorzScrollBars(BOOL bPositionOnly = FALSE);

	void			ScrollToChar(int nNewOffsetChar, BOOL bTrackScrollBar = TRUE);

	void			OnDoMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	void			OnDoHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar, CBaseView * master);
	void			OnDoVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar, CBaseView * master);

	void			SetupDiffBars(int start, int end);
	void			SetupSelection(int start, int end);
	void			ShowDiffLines(int nLine);
	
	int				GetTabSize() const {return m_nTabSize;}

	int				GetLineActualLength(int index) const;
	int				GetLineCount() const;
	void			CalcLineCharDim();
	int				GetLineHeight();
	int				GetCharWidth();
	int				GetMaxLineLength();
	int				GetLineLength(int index) const;
	int				GetDiffLineLength(int index) const;
	int				GetScreenChars();
	int				GetAllMinScreenChars() const;
	int				GetAllMaxLineLength() const;
	int				GetAllLineCount() const;
	int				GetAllMinScreenLines() const;
	LPCTSTR			GetLineChars(int index) const;
	LPCTSTR			GetDiffLineChars(int index);
	int				GetLineNumber(int index) const;
	CFont *			GetFont(BOOL bItalic = FALSE, BOOL bBold = FALSE, BOOL bStrikeOut = FALSE);
	int				GetLineFromPoint(CPoint point);
	int				GetMarginWidth();
	void			RefreshViews();
	COLORREF		IntenseColor(long scale, COLORREF col);

	virtual	void	OnContextMenu(CPoint point, int nLine, DiffStates state);
	/**
	 * Updates the status bar pane. Call this if the document changed.
	 */
	void			UpdateStatusBar();

	void			UseTheirAndYourBlock(viewstate &rightstate, viewstate &bottomstate, viewstate &leftstate);
	void			UseYourAndTheirBlock(viewstate &rightstate, viewstate &bottomstate, viewstate &leftstate);
	void			UseBothLeftFirst(viewstate &rightstate, viewstate &leftstate);
	void			UseBothRightFirst(viewstate &rightstate, viewstate &leftstate);

	bool			IsLeftViewGood() const {return ((m_pwndLeft)&&(m_pwndLeft->IsWindowVisible()));}
	bool			IsRightViewGood() const {return ((m_pwndRight)&&(m_pwndRight->IsWindowVisible()));}
	bool			IsBottomViewGood() const {return ((m_pwndBottom)&&(m_pwndBottom->IsWindowVisible()));}
protected:
	COLORREF		m_InlineRemovedBk;
	COLORREF		m_InlineAddedBk;
	COLORREF		m_ModifiedBk;
	UINT			m_nStatusBarID;		///< The ID of the status bar pane used by this view. Must be set by the parent class.

	SVNLineDiff		m_svnlinediff;
	BOOL			m_bOtherDiffChecked;
	BOOL			m_bModified;
	BOOL			m_bFocused;
	BOOL			m_bViewLinenumbers;
	BOOL			m_bIsHidden;
	int				m_nLineHeight;
	int				m_nCharWidth;
	int				m_nMaxLineLength;
	int				m_nScreenLines;
	int				m_nScreenChars;
	int				m_nOffsetChar;
	int				m_nTabSize;
	int				m_nDigits;
	bool			m_bInlineWordDiff;

	int				m_nSelBlockStart;
	int				m_nSelBlockEnd;
	int				m_nDiffBlockStart;
	int				m_nDiffBlockEnd;

	int				m_nMouseLine;

	HICON			m_hAddedIcon;
	HICON			m_hRemovedIcon;
	HICON			m_hConflictedIcon;
	HICON			m_hConflictedIgnoredIcon;
	HICON			m_hWhitespaceBlockIcon;
	HICON			m_hEqualIcon;

	HICON			m_hLineEndingCR;
	HICON			m_hLineEndingCRLF;
	HICON			m_hLineEndingLF;

	LOGFONT			m_lfBaseFont;
	CFont *			m_apFonts[8];
	CString			m_sConflictedText;

	CBitmap *		m_pCacheBitmap;
	CDC *			m_pDC;
	CScrollTool		m_ScrollTool;
	
	char			m_szTip[MAX_PATH*2+1];
	wchar_t			m_wszTip[MAX_PATH*2+1];
	// These three pointers lead to the three parent
	// classes CLeftView, CRightView and CBottomView
	// and are used for the communication between
	// the views (e.g. synchronized scrolling, ...)
	// To find out which parent class this object
	// is made of just compare e.g. (m_pwndLeft==this).
	static CBaseView * m_pwndLeft;		///< Pointer to the left view. Must be set by the CLeftView parent class.
	static CBaseView * m_pwndRight;		///< Pointer to the right view. Must be set by the CRightView parent class.
	static CBaseView * m_pwndBottom;	///< Pointer to the bottom view. Must be set by the CBottomView parent class.
};


