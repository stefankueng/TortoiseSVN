// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2009 - TortoiseSVN

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

#include "auto_buffer.h"

/**
 * \ingroup Utils
 * Validator interface for the Filter edit control CFilterEdit
 */
class IFilterEditValidator
{
public:
	virtual bool	Validate(LPCTSTR string) = 0;
};

/**
 * \ingroup Utils
 * Filter edit control.
 * An edit control with a 'close' button on the right which clears the text
 * in the control, and an info button on the left (optional) where a context 
 * menu or other selection window can be shown.
 * \image html "filterEdit.jpg"
 *
 * Example on how to show a context menu for the info button:
 * \code
 * LRESULT CFilterEditTestDlg::OnFilterContext(WPARAM wParam, LPARAM lParam)
 * {
 *   RECT * rect = (LPRECT)lParam;
 *   POINT point;
 *   point.x = rect->left;
 *   point.y = rect->bottom;
 *   CMenu popup;
 *   if (popup.CreatePopupMenu())
 *   {
 *     popup.AppendMenu(MF_STRING | MF_ENABLED, 1, _T("string 1"));
 *     popup.AppendMenu(MF_SEPARATOR, NULL);
 *     popup.AppendMenu(MF_STRING | MF_ENABLED, 2, _T("string 2"));
 *     popup.AppendMenu(MF_STRING | MF_ENABLED, 3, _T("string 3"));
 *     popup.AppendMenu(MF_STRING | MF_ENABLED, 4, _T("string 4"));
 *     popup.AppendMenu(MF_STRING | MF_ENABLED, 5, _T("string 5"));
 *     popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
 *   }
 *   return 0;
 * }
 * \endcode
 */
class CFilterEdit : public CEdit
{
	DECLARE_DYNAMIC(CFilterEdit)
public:
	CFilterEdit();
	virtual ~CFilterEdit();

	static const UINT WM_FILTEREDIT_INFOCLICKED;
	static const UINT WM_FILTEREDIT_CANCELCLICKED;

	/**
	 * Sets the icons to show for the cancel button. The first icon represents
	 * the normal state, the second one when the button is pressed.
	 * if \c bShowAlways is true, then the cancel button is shown even if there
	 * is no text in the control.
	 * \note To catch the WM_FILTEREDIT_CANCELCLICKED notification, handle the message directly (or use the 
	 * WM_MESSAGE() macro). The LPARAM parameter of the message contains the
	 * rectangle (pointer to RECT) of the info icon in screen coordinates.
	 */
	BOOL SetCancelBitmaps(UINT uCancelNormal, UINT uCancelPressed, BOOL bShowAlways = FALSE);

	/**
	 * Sets the info icon shown on the left.
	 * A notification is sent when the user clicks on that icon.
	 * The notification is either WM_FILTEREDIT_INFOCLICKED or the one
	 * set with SetButtonClickedMessageId().
	 *
	 * To catch the notification, handle the message directly (or use the 
	 * WM_MESSAGE() macro). The LPARAM parameter of the message contains the
	 * rectangle (pointer to RECT) of the info icon in screen coordinates.
	 */
	BOOL SetInfoIcon(UINT uInfo);

	/**
	 * Sets the message Id which is sent when the user clicks on the info
	 * button.
	 */
	void SetButtonClickedMessageId(UINT iButtonClickedMessageId, UINT iCancelClickedMessageId);

	/**
	 * To provide a cue banner even though we require the edit control to be multi line
	 */
	BOOL SetCueBanner(LPCWSTR lpcwText);

	void SetValidator(IFilterEditValidator * pValidator) {m_pValidator = pValidator;}
protected:
	virtual void	PreSubclassWindow( );
	virtual BOOL	PreTranslateMessage( MSG* pMsg );

	afx_msg BOOL	OnEraseBkgnd(CDC* pDC);
	afx_msg void	OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg int		OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void	OnSize(UINT nType, int cx, int cy);
	afx_msg LRESULT OnSetFont(WPARAM wParam, LPARAM lParam);
	afx_msg BOOL	OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void	OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg BOOL	OnEnChange();
	afx_msg HBRUSH	CtlColor(CDC* /*pDC*/, UINT /*nCtlColor*/);
	afx_msg void	OnPaint();
	afx_msg void	OnEnKillfocus();
	afx_msg void	OnEnSetfocus();
	DECLARE_MESSAGE_MAP()


	void			ResizeWindow();
	CSize			GetIconSize(HICON hIcon);
	void			Validate();
	void			DrawDimText();

protected:
	HICON					m_hIconCancelNormal;
	HICON					m_hIconCancelPressed;
	HICON					m_hIconInfo;
	CSize					m_sizeCancelIcon;
	CSize					m_sizeInfoIcon;
	CRect					m_rcEditArea;
	CRect					m_rcButtonArea;
	CRect					m_rcInfoArea;
	BOOL					m_bShowCancelButtonAlways;
	BOOL					m_bPressed;
	UINT					m_iButtonClickedMessageId;
	UINT					m_iCancelClickedMessageId;
	COLORREF				m_backColor;
	HBRUSH					m_brBack;
	IFilterEditValidator *	m_pValidator;
	auto_buffer<TCHAR>		m_pCueBanner;
};


