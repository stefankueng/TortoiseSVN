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

#include "stdafx.h"
#include "registry.h"
#include "TortoiseMerge.h"
#include "MainFrm.h"
#include <deque>
#include ".\BaseView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define MARGINWIDTH 20
#define HEADERHEIGHT 10

#define TAB_CHARACTER				_T('\xBB')
#define SPACE_CHARACTER				_T('\xB7')

#define WHITESPACE_CHARS			_T(" \t\xBB\xB7")

#define MAXFONTS 8

#define INLINEADDED_COLOR			RGB(255, 255, 150)
#define INLINEREMOVED_COLOR			RGB(200, 100, 100)
#define MODIFIED_COLOR				RGB(220, 220, 255)

CBaseView * CBaseView::m_pwndLeft = NULL;
CBaseView * CBaseView::m_pwndRight = NULL;
CBaseView * CBaseView::m_pwndBottom = NULL;
CLocatorBar * CBaseView::m_pwndLocator = NULL;
CLineDiffBar * CBaseView::m_pwndLineDiffBar = NULL;
CStatusBar * CBaseView::m_pwndStatusBar = NULL;
CMainFrame * CBaseView::m_pMainFrame = NULL;

CBaseView::CBaseView()
{
	m_pCacheBitmap = NULL;
	m_arDiffLines = NULL;
	m_arLineStates = NULL;
	m_nLineHeight = -1;
	m_nCharWidth = -1;
	m_nScreenChars = -1;
	m_nMaxLineLength = -1;
	m_nScreenLines = -1;
	m_nTopLine = 0;
	m_nOffsetChar = 0;
	m_nDigits = 0;
	m_nMouseLine = -1;
	m_bIsHidden = FALSE;
	lineendings = CFileTextLines::AUTOLINE;
	texttype = CFileTextLines::AUTOTYPE;
	m_bViewWhitespace = CRegDWORD(_T("Software\\TortoiseMerge\\ViewWhitespaces"), 1);
	m_bViewLinenumbers = CRegDWORD(_T("Software\\TortoiseMerge\\ViewLinenumbers"), 1);
	m_bShowInlineDiff = CRegDWORD(_T("Software\\TortoiseMerge\\DisplayBinDiff"), TRUE);
	m_InlineAddedBk = CRegDWORD(_T("Software\\TortoiseMerge\\InlineAdded"), INLINEADDED_COLOR);
	m_InlineRemovedBk = CRegDWORD(_T("Software\\TortoiseMerge\\InlineRemoved"), INLINEREMOVED_COLOR);
	m_ModifiedBk = CRegDWORD(_T("Software\\TortoiseMerge\\ColorModifiedB"), MODIFIED_COLOR);
	m_nSelBlockStart = -1;
	m_nSelBlockEnd = -1;
	m_bModified = FALSE;
	m_bOtherDiffChecked = false;
	m_nTabSize = (int)(DWORD)CRegDWORD(_T("Software\\TortoiseMerge\\TabSize"), 4);
	for (int i=0; i<MAXFONTS; i++)
	{
		m_apFonts[i] = NULL;
	}
	m_hConflictedIcon = (HICON)::LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_CONFLICTEDLINE), 
									IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	m_hRemovedIcon = (HICON)::LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_REMOVEDLINE), 
									IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	m_hAddedIcon = (HICON)::LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ADDEDLINE), 
									IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	m_hWhitespaceBlockIcon = (HICON)::LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_WHITESPACELINE),
									IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	m_hEqualIcon = (HICON)::LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_EQUALLINE),
									IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	EnableToolTips();
}

CBaseView::~CBaseView()
{
	if (m_pCacheBitmap)
	{
		m_pCacheBitmap->DeleteObject();
		delete m_pCacheBitmap;
	}
	for (int i=0; i<MAXFONTS; i++)
	{
		if (m_apFonts[i] != NULL)
		{
			m_apFonts[i]->DeleteObject();
			delete m_apFonts[i];
		}
		m_apFonts[i] = NULL;
	}
	DestroyIcon(m_hAddedIcon);
	DestroyIcon(m_hRemovedIcon);
	DestroyIcon(m_hConflictedIcon);
	DestroyIcon(m_hWhitespaceBlockIcon);
	DestroyIcon(m_hEqualIcon);
}

BEGIN_MESSAGE_MAP(CBaseView, CView)
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_WM_ERASEBKGND()
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_MOUSEWHEEL()
	ON_WM_SETCURSOR()
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_NAVIGATE_NEXTDIFFERENCE, OnMergeNextdifference)
	ON_COMMAND(ID_NAVIGATE_PREVIOUSDIFFERENCE, OnMergePreviousdifference)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipNotify)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipNotify)
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONUP()
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_WM_MOUSEMOVE()
	ON_COMMAND(ID_NAVIGATE_PREVIOUSCONFLICT, OnMergePreviousconflict)
	ON_COMMAND(ID_NAVIGATE_NEXTCONFLICT, OnMergeNextconflict)
	ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()


void CBaseView::DocumentUpdated()
{
	if (m_pCacheBitmap != NULL)
	{
		m_pCacheBitmap->DeleteObject();
		delete m_pCacheBitmap;
		m_pCacheBitmap = NULL;
	}
	m_nLineHeight = -1;
	m_nCharWidth = -1;
	m_nScreenChars = -1;
	m_nMaxLineLength = -1;
	m_nScreenLines = -1;
	m_nTopLine = 0;
	m_bModified = FALSE;
	m_bOtherDiffChecked = false;
	m_nDigits = 0;
	m_nMouseLine = -1;

	m_nTabSize = (int)(DWORD)CRegDWORD(_T("Software\\TortoiseMerge\\TabSize"), 4);
	m_bViewLinenumbers = CRegDWORD(_T("Software\\TortoiseMerge\\ViewLinenumbers"), 1);
	m_bShowInlineDiff = CRegDWORD(_T("Software\\TortoiseMerge\\DisplayBinDiff"), TRUE);
	m_InlineAddedBk = CRegDWORD(_T("Software\\TortoiseMerge\\InlineAdded"), INLINEADDED_COLOR);
	m_InlineRemovedBk = CRegDWORD(_T("Software\\TortoiseMerge\\InlineRemoved"), INLINEREMOVED_COLOR);
	m_ModifiedBk = CRegDWORD(_T("Software\\TortoiseMerge\\ColorModifiedB"), MODIFIED_COLOR);
	for (int i=0; i<MAXFONTS; i++)
	{
		if (m_apFonts[i] != NULL)
		{
			m_apFonts[i]->DeleteObject();
			delete m_apFonts[i];
		}
		m_apFonts[i] = NULL;
	}
	m_nSelBlockStart = -1;
	m_nSelBlockEnd = -1;
	RecalcVertScrollBar();
	RecalcHorzScrollBar();
	UpdateStatusBar();
	Invalidate();
}

void CBaseView::UpdateStatusBar()
{
	int nRemovedLines = 0;
	int nAddedLines = 0;
	int nConflictedLines = 0;

	if (m_arLineStates)
	{
		for (int i=0; i<m_arLineStates->GetCount(); i++)
		{
			CDiffData::DiffStates state = (CDiffData::DiffStates)m_arLineStates->GetAt(i);
			switch (state)
			{
			case CDiffData::DIFFSTATE_ADDED:
			case CDiffData::DIFFSTATE_IDENTICALADDED:
			case CDiffData::DIFFSTATE_THEIRSADDED:
			case CDiffData::DIFFSTATE_YOURSADDED:
			case CDiffData::DIFFSTATE_CONFLICTADDED:
				nAddedLines++;
				break;
			case CDiffData::DIFFSTATE_IDENTICALREMOVED:
			case CDiffData::DIFFSTATE_REMOVED:
			case CDiffData::DIFFSTATE_THEIRSREMOVED:
			case CDiffData::DIFFSTATE_YOURSREMOVED:
				nRemovedLines++;
				break;
			case CDiffData::DIFFSTATE_CONFLICTED:
				nConflictedLines++;
				break;
			}
		}
	}

	CString sBarText;
	CString sTemp;

	switch (texttype)
	{
	case CFileTextLines::ASCII:
		sBarText = _T("ASCII ");
		break;
	case CFileTextLines::BINARY:
		sBarText = _T("BINARY ");
		break;
	case CFileTextLines::UNICODE_LE:
		sBarText = _T("UTF-16LE ");
		break;
	case CFileTextLines::UTF8:
		sBarText = _T("UTF8 ");
		break;
	case CFileTextLines::UTF8BOM:
		sBarText = _T("UTF8 BOM ");
		break;
	}

	switch(lineendings)
	{
	case CFileTextLines::LF:
		sBarText += _T("LF ");
		break;
	case CFileTextLines::CRLF:
		sBarText += _T("CRLF ");
		break;
	case CFileTextLines::LFCR:
		sBarText += _T("LFCR ");
		break;
	case CFileTextLines::CR:
		sBarText += _T("CR ");
		break;
	}

	if (sBarText.IsEmpty())
		sBarText  += _T(" / ");

	if (nRemovedLines)
	{
		sTemp.Format(IDS_STATUSBAR_REMOVEDLINES, nRemovedLines);
		if (!sBarText.IsEmpty())
			sBarText += _T(" / ");
		sBarText += sTemp;
	}
	if (nAddedLines)
	{
		sTemp.Format(IDS_STATUSBAR_ADDEDLINES, nAddedLines);
		if (!sBarText.IsEmpty())
			sBarText += _T(" / ");
		sBarText += sTemp;
	}
	if (nConflictedLines)
	{
		sTemp.Format(IDS_STATUSBAR_CONFLICTEDLINES, nConflictedLines);
		if (!sBarText.IsEmpty())
			sBarText += _T(" / ");
		sBarText += sTemp;
	}
	if (m_pwndStatusBar)
	{
		UINT nID;
		UINT nStyle;
		int cxWidth;
		int nIndex = m_pwndStatusBar->CommandToIndex(m_nStatusBarID);
		if (m_nStatusBarID == ID_INDICATOR_BOTTOMVIEW)
		{
			sBarText.Format(IDS_STATUSBAR_CONFLICTS, nConflictedLines);
		}
		if (m_nStatusBarID == ID_INDICATOR_LEFTVIEW)
		{
			sTemp.LoadString(IDS_STATUSBAR_LEFTVIEW);
			sBarText = sTemp+sBarText;
		}
		if (m_nStatusBarID == ID_INDICATOR_RIGHTVIEW)
		{
			sTemp.LoadString(IDS_STATUSBAR_RIGHTVIEW);
			sBarText = sTemp+sBarText;
		}
		m_pwndStatusBar->GetPaneInfo(nIndex, nID, nStyle, cxWidth);
		//calculate the width of the text
		CSize size = m_pwndStatusBar->GetDC()->GetTextExtent(sBarText);
		m_pwndStatusBar->SetPaneInfo(nIndex, nID, nStyle, size.cx+2);
		m_pwndStatusBar->SetPaneText(nIndex, sBarText);
	}
}

BOOL CBaseView::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (!CView::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
		::LoadCursor(NULL, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW+1), NULL);

	CWnd *pParentWnd = CWnd::FromHandlePermanent(cs.hwndParent);
	if (pParentWnd == NULL || ! pParentWnd->IsKindOf(RUNTIME_CLASS(CSplitterWnd)))
	{
		//	View must always create its own scrollbars,
		//	if only it's not used within splitter
		cs.style |= (WS_HSCROLL | WS_VSCROLL);
	}
	cs.lpszClass = AfxRegisterWndClass(CS_DBLCLKS);
	return TRUE;
}

CFont* CBaseView::GetFont(BOOL bItalic /*= FALSE*/, BOOL bBold /*= FALSE*/, BOOL bStrikeOut /*= FALSE*/)
{
	int nIndex = 0;
	if (bBold)
		nIndex |= 1;
	if (bItalic)
		nIndex |= 2;
	if (bStrikeOut)
		nIndex |= 4;
	if (m_apFonts[nIndex] == NULL)
	{
		m_apFonts[nIndex] = new CFont;
		m_lfBaseFont.lfCharSet = DEFAULT_CHARSET;
		m_lfBaseFont.lfWeight = bBold ? FW_BOLD : FW_NORMAL;
		m_lfBaseFont.lfItalic = (BYTE) bItalic;
		m_lfBaseFont.lfStrikeOut = (BYTE) bStrikeOut;
		if (bStrikeOut)
			m_lfBaseFont.lfStrikeOut = (BYTE)(DWORD)CRegDWORD(_T("Software\\TortoiseMerge\\StrikeOut"), TRUE);
		m_lfBaseFont.lfHeight = -MulDiv((DWORD)CRegDWORD(_T("Software\\TortoiseMerge\\LogFontSize"), 10), GetDeviceCaps(this->GetDC()->m_hDC, LOGPIXELSY), 72);
		_tcsncpy_s(m_lfBaseFont.lfFaceName, 32, (LPCTSTR)(CString)CRegString(_T("Software\\TortoiseMerge\\LogFontName"), _T("Courier New")), 32);
		if (!m_apFonts[nIndex]->CreateFontIndirect(&m_lfBaseFont))
		{
			delete m_apFonts[nIndex];
			m_apFonts[nIndex] = NULL;
			return CView::GetFont();
		}
	}
	return m_apFonts[nIndex];
}

void CBaseView::CalcLineCharDim()
{
	CDC *pDC = GetDC();
	CFont *pOldFont = pDC->SelectObject(GetFont());
	CSize szCharExt = pDC->GetTextExtent(_T("X"));
	m_nLineHeight = szCharExt.cy;
	if (m_nLineHeight <= 0)
		m_nLineHeight = -1;
	m_nCharWidth = szCharExt.cx;
	if (m_nCharWidth <= 0)
		m_nCharWidth = -1;
	pDC->SelectObject(pOldFont);
	ReleaseDC(pDC);
}

int CBaseView::GetScreenChars()
{
	if (m_nScreenChars == -1)
	{
		CRect rect;
		GetClientRect(&rect);
		m_nScreenChars = (rect.Width() - GetMarginWidth()) / GetCharWidth();
	}
	return m_nScreenChars;
}

int CBaseView::GetAllMinScreenChars() const 
{
	int nChars = 0;
	if ((m_pwndLeft)&&(m_pwndLeft->IsWindowVisible()))
		nChars = m_pwndLeft->GetScreenChars();
	if ((m_pwndRight)&&(m_pwndRight->IsWindowVisible()))
		nChars = (nChars < m_pwndRight->GetScreenChars() ? nChars : m_pwndRight->GetScreenChars());
	if ((m_pwndBottom)&&(m_pwndBottom->IsWindowVisible()))
		nChars = (nChars < m_pwndBottom->GetScreenChars() ? nChars : m_pwndBottom->GetScreenChars());
	return nChars;
}

int CBaseView::GetAllMaxLineLength() const 
{
	int nLength = 0;
	if ((m_pwndLeft)&&(m_pwndLeft->IsWindowVisible()))
		nLength = m_pwndLeft->GetMaxLineLength();
	if ((m_pwndRight)&&(m_pwndRight->IsWindowVisible()))
		nLength = (nLength > m_pwndRight->GetMaxLineLength() ? nLength : m_pwndRight->GetMaxLineLength());
	if ((m_pwndBottom)&&(m_pwndBottom->IsWindowVisible()))
		nLength = (nLength > m_pwndBottom->GetMaxLineLength() ? nLength : m_pwndBottom->GetMaxLineLength());
	return nLength;
}

int CBaseView::GetLineHeight()
{
	if (m_nLineHeight == -1)
		CalcLineCharDim();
	if (m_nLineHeight <= 0)
		return 1;
	return m_nLineHeight;
}

int CBaseView::GetCharWidth()
{
	if (m_nCharWidth == -1)
		CalcLineCharDim();
	if (m_nCharWidth <= 0)
		return 1;
	return m_nCharWidth;
}

int CBaseView::GetMaxLineLength()
{
	if (m_nMaxLineLength == -1)
	{
		m_nMaxLineLength = 0;
		int nLineCount = GetLineCount();
		for (int i=0; i<nLineCount; i++)
		{
			int nActualLength = GetLineActualLength(i);
			if (m_nMaxLineLength < nActualLength)
				m_nMaxLineLength = nActualLength;
		}
	}
	return m_nMaxLineLength;
}

int CBaseView::GetLineActualLength(int index) const
{
	if (m_arDiffLines == NULL)
		return 0;
	const CString& sLine = m_arDiffLines->GetAt(index);
	const TCHAR*pLine = (LPCTSTR)sLine;
	
	int nTabSize = GetTabSize();
	int nLineLength = 0;

	int len = sLine.GetLength();
	for (int i=0; i<len; i++)
	{
		if (pLine[i] == _T('\t'))
			nLineLength += (nTabSize - nLineLength % nTabSize);
		else
			nLineLength ++;
	}

	ASSERT(nLineLength >= 0);
	return nLineLength;
}

int CBaseView::GetLineLength(int index) const
{
	if (m_arDiffLines == NULL)
		return 0;
	int nLineLength = m_arDiffLines->GetAt(index).GetLength();
	ASSERT(nLineLength >= 0);
	return nLineLength;
}

int CBaseView::GetDiffLineLength(int index) const
{
	if (m_arDiffDiffLines == NULL)
		return 0;
	int nLineLength = m_arDiffDiffLines->GetAt(index).GetLength();
	ASSERT(nLineLength >= 0);
	return nLineLength;
}

int CBaseView::GetLineCount() const
{
	if (m_arDiffLines == NULL)
		return 1;
	int nLineCount = (int)m_arDiffLines->GetCount();
	ASSERT(nLineCount >= 0);
	return nLineCount;
}

LPCTSTR CBaseView::GetLineChars(int index) const
{
	if (m_arDiffLines == NULL)
		return 0;
	return m_arDiffLines->GetAt(index);
}

LPCTSTR CBaseView::GetDiffLineChars(int index)
{
	if (!m_bOtherDiffChecked)
	{
		// find out what the 'other' file is
		m_arDiffDiffLines = NULL;
		m_arDiffDiffStates = NULL;
		if (this == m_pwndLeft)
		{
			if ((m_pwndRight)&&(m_pwndRight->IsWindowVisible()))
			{
				m_arDiffDiffLines = m_pwndRight->m_arDiffLines;
				m_arDiffDiffStates = m_pwndRight->m_arLineStates;
			}
		}
		if (this == m_pwndRight)
		{
			if ((m_pwndLeft)&&(m_pwndLeft->IsWindowVisible()))
			{
				m_arDiffDiffLines = m_pwndLeft->m_arDiffLines;
				m_arDiffDiffStates = m_pwndLeft->m_arLineStates;
			}
		}
		m_bOtherDiffChecked = true;
	}
	if (m_arDiffDiffLines)
	{
		return m_arDiffDiffLines->GetAt(index);
	}
	return 0;
}

bool CBaseView::IsBlockWhitespaceOnly(int nLineIndex, bool& bIdentical)
{
#define MAX_WHITESPACEBLOCK_SIZE	8
	CDiffData::DiffStates origstateThis = (CDiffData::DiffStates)m_arLineStates->GetAt(nLineIndex);
	if (!m_bOtherDiffChecked)
	{
		// find out what the 'other' file is
		m_arDiffDiffLines = NULL;
		m_arDiffDiffStates = NULL;
		if (this == m_pwndLeft)
		{
			if ((m_pwndRight)&&(m_pwndRight->IsWindowVisible()))
			{
				m_arDiffDiffLines = m_pwndRight->m_arDiffLines;
				m_arDiffDiffStates = m_pwndRight->m_arLineStates;
			}
		}
		if (this == m_pwndRight)
		{
			if ((m_pwndLeft)&&(m_pwndLeft->IsWindowVisible()))
			{
				m_arDiffDiffLines = m_pwndLeft->m_arDiffLines;
				m_arDiffDiffStates = m_pwndLeft->m_arLineStates;
			}
		}
		m_bOtherDiffChecked = true;
	}
	if (m_arDiffDiffLines)
	{
		if (origstateThis == CDiffData::DIFFSTATE_NORMAL)
		{
			if (m_arDiffDiffLines->GetAt(nLineIndex).Compare(m_arDiffLines->GetAt(nLineIndex))==0)
				return false;
		}

		// Go back at most MAX_WHITESPACEBLOCK_SIZE lines to see where this block ends
		int nStartBlockThis = nLineIndex;
		int nEndBlockThis = nLineIndex;
		while ((nStartBlockThis > 0)&&(nStartBlockThis > (nLineIndex-MAX_WHITESPACEBLOCK_SIZE)))
		{
			CDiffData::DiffStates state = (CDiffData::DiffStates)m_arLineStates->GetAt(nStartBlockThis-1);
			if ((origstateThis == CDiffData::DIFFSTATE_EMPTY)&&(state != CDiffData::DIFFSTATE_NORMAL))
				origstateThis = state;
			if ((origstateThis == state)||(state == CDiffData::DIFFSTATE_EMPTY))
				nStartBlockThis--;
			else
				break;
		}
		while ((nEndBlockThis < (GetLineCount()-1))&&(nEndBlockThis < (nLineIndex+MAX_WHITESPACEBLOCK_SIZE)))
		{
			CDiffData::DiffStates state = (CDiffData::DiffStates)m_arLineStates->GetAt(nEndBlockThis+1);
			if ((origstateThis == CDiffData::DIFFSTATE_EMPTY)&&(state != CDiffData::DIFFSTATE_NORMAL))
				origstateThis = state;
			if ((origstateThis == state)||(state == CDiffData::DIFFSTATE_EMPTY))
				nEndBlockThis++;
			else
				break;
		}
		int nStartBlockOther = nLineIndex;
		int nEndBlockOther = nLineIndex;
		CDiffData::DiffStates origstateOther = (CDiffData::DiffStates)m_arDiffDiffStates->GetAt(nLineIndex);
		while ((nStartBlockOther > 0)&&(nStartBlockOther > (nLineIndex-MAX_WHITESPACEBLOCK_SIZE)))
		{
			CDiffData::DiffStates state = (CDiffData::DiffStates)m_arDiffDiffStates->GetAt(nStartBlockOther-1);
			if ((origstateOther == CDiffData::DIFFSTATE_EMPTY)&&(state != CDiffData::DIFFSTATE_NORMAL))
				origstateOther = state;
			if ((origstateOther == state)||(state == CDiffData::DIFFSTATE_EMPTY))
				nStartBlockOther--;
			else
				break;
		}
		while ((nEndBlockOther < (GetLineCount()-1))&&(nEndBlockOther < (nLineIndex+MAX_WHITESPACEBLOCK_SIZE)))
		{
			CDiffData::DiffStates state = (CDiffData::DiffStates)m_arDiffDiffStates->GetAt(nEndBlockOther+1);
			if ((origstateOther == CDiffData::DIFFSTATE_EMPTY)&&(state != CDiffData::DIFFSTATE_NORMAL))
				origstateOther = state;
			if ((origstateOther == state)||(state == CDiffData::DIFFSTATE_EMPTY))
				nEndBlockOther++;
			else
				break;
		}
		CString mine;
		for (int i=nStartBlockThis; i<=nEndBlockThis; ++i)
			mine += m_arDiffLines->GetAt(i);
		CString other;
		for (int i=nStartBlockOther; i<=nEndBlockOther; ++i)
			other += m_arDiffDiffLines->GetAt(i);
		bIdentical = mine.Compare(other)==0;
		mine.Replace(_T(" "), _T(""));
		mine.Replace(_T("\t"), _T(""));
		mine.Replace(_T("\r"), _T(""));
		mine.Replace(_T("\n"), _T(""));
		other.Replace(_T(" "), _T(""));
		other.Replace(_T("\t"), _T(""));
		other.Replace(_T("\r"), _T(""));
		other.Replace(_T("\n"), _T(""));
		if ((mine.Compare(other)==0)&&(!mine.IsEmpty()))
			return true;
	}
	return false;
}

int CBaseView::GetLineNumber(int index) const
{
	if (m_arLineLines == NULL)
		return -1;
	if (m_arLineLines->GetAt(index)==DIFF_EMPTYLINENUMBER)
		return -1;
	return m_arLineLines->GetAt(index);
}

int CBaseView::GetScreenLines()
{
	if (m_nScreenLines == -1)
	{
		CRect rect;
		GetClientRect(&rect);
		m_nScreenLines = (rect.Height() - HEADERHEIGHT) / GetLineHeight();
	}
	return m_nScreenLines;
}

int CBaseView::GetAllMinScreenLines() const
{
	int nLines = 0;
	if ((m_pwndLeft)&&(m_pwndLeft->IsWindowVisible()))
		nLines = m_pwndLeft->GetScreenLines();
	if ((m_pwndRight)&&(m_pwndRight->IsWindowVisible()))
		nLines = (nLines < m_pwndRight->GetScreenLines() ? nLines : m_pwndRight->GetScreenLines());
	if ((m_pwndBottom)&&(m_pwndBottom->IsWindowVisible()))
		nLines = (nLines < m_pwndBottom->GetScreenLines() ? nLines : m_pwndBottom->GetScreenLines());
	return nLines;
}

int CBaseView::GetAllLineCount() const
{
	int nLines = 0;
	if ((m_pwndLeft)&&(m_pwndLeft->IsWindowVisible()))
		nLines = m_pwndLeft->GetLineCount();
	if ((m_pwndRight)&&(m_pwndRight->IsWindowVisible()))
		nLines = (nLines > m_pwndRight->GetLineCount() ? nLines : m_pwndRight->GetLineCount());
	if ((m_pwndBottom)&&(m_pwndBottom->IsWindowVisible()))
		nLines = (nLines > m_pwndBottom->GetLineCount() ? nLines : m_pwndBottom->GetLineCount());
	return nLines;
}

void CBaseView::RecalcAllVertScrollBars(BOOL bPositionOnly /*= FALSE*/)
{
	if ((m_pwndLeft)&&(m_pwndLeft->IsWindowVisible()))
		m_pwndLeft->RecalcVertScrollBar(bPositionOnly);
	if ((m_pwndRight)&&(m_pwndRight->IsWindowVisible()))
		m_pwndRight->RecalcVertScrollBar(bPositionOnly);
	if ((m_pwndBottom)&&(m_pwndBottom->IsWindowVisible()))
		m_pwndBottom->RecalcVertScrollBar(bPositionOnly);
}

void CBaseView::RecalcVertScrollBar(BOOL bPositionOnly /*= FALSE*/)
{
	SCROLLINFO si;
	si.cbSize = sizeof(si);
	if (bPositionOnly)
	{
		si.fMask = SIF_POS;
		si.nPos = m_nTopLine;
	}
	else
	{
		EnableScrollBarCtrl(SB_VERT, TRUE);
		if (GetAllMinScreenLines() >= GetAllLineCount() && m_nTopLine > 0)
		{
			m_nTopLine = 0;
			Invalidate();
		}
		si.fMask = SIF_DISABLENOSCROLL | SIF_PAGE | SIF_POS | SIF_RANGE;
		si.nMin = 0;
		si.nMax = GetAllLineCount();
		si.nPage = GetAllMinScreenLines();
		si.nPos = m_nTopLine;
	}
	VERIFY(SetScrollInfo(SB_VERT, &si));
}

void CBaseView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	CView::OnVScroll(nSBCode, nPos, pScrollBar);
	if (m_pwndLeft)
		m_pwndLeft->OnDoVScroll(nSBCode,  nPos, pScrollBar, this);
	if (m_pwndRight)
		m_pwndRight->OnDoVScroll(nSBCode,  nPos, pScrollBar, this);
	if (m_pwndBottom)
		m_pwndBottom->OnDoVScroll(nSBCode,  nPos, pScrollBar, this);
	if (m_pwndLocator)
		m_pwndLocator->Invalidate();
}

void CBaseView::OnDoVScroll(UINT nSBCode, UINT /*nPos*/, CScrollBar* /*pScrollBar*/, CBaseView * master)
{
	//	Note we cannot use nPos because of its 16-bit nature
	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	VERIFY(master->GetScrollInfo(SB_VERT, &si));

	int nPageLines = GetScreenLines();
	int nLineCount = GetLineCount();

	RECT thumbrect;
	POINT thumbpoint;
	int nNewTopLine;

	static LONG textwidth = 0;
	static CString sFormat(MAKEINTRESOURCE(IDS_VIEWSCROLLTIPTEXT));
	switch (nSBCode)
	{
	case SB_TOP:
		nNewTopLine = 0;
		break;
	case SB_BOTTOM:
		nNewTopLine = nLineCount - nPageLines + 1;
		break;
	case SB_LINEUP:
		nNewTopLine = m_nTopLine - 1;
		break;
	case SB_LINEDOWN:
		nNewTopLine = m_nTopLine + 1;
		break;
	case SB_PAGEUP:
		nNewTopLine = m_nTopLine - si.nPage + 1;
		break;
	case SB_PAGEDOWN:
		nNewTopLine = m_nTopLine + si.nPage - 1;
		break;
	case SB_THUMBPOSITION:
		m_ScrollTool.Clear();
		nNewTopLine = si.nTrackPos;
		textwidth = 0;
		break;
	case SB_THUMBTRACK:
		nNewTopLine = si.nTrackPos;
		if (GetFocus() == this)
		{
			GetClientRect(&thumbrect);
			ClientToScreen(&thumbrect);
			thumbpoint.x = thumbrect.right;
			thumbpoint.y = thumbrect.top + ((thumbrect.bottom-thumbrect.top)*si.nTrackPos)/(si.nMax-si.nMin);
			m_ScrollTool.Init(&thumbpoint);
			if (textwidth == 0)
			{
				CString sTemp = sFormat;
				sTemp.Format(sFormat, m_nDigits, 10*m_nDigits-1);
				textwidth = m_ScrollTool.GetTextWidth(sTemp);
			}
			thumbpoint.x -= textwidth;
			int line = GetLineNumber(nNewTopLine);
			if (line >= 0)
				m_ScrollTool.SetText(&thumbpoint, sFormat, m_nDigits, GetLineNumber(nNewTopLine)+1);
			else
				m_ScrollTool.SetText(&thumbpoint, _T(" "));
		}
		break;
	default:
		return;
	}

	if (nNewTopLine < 0)
		nNewTopLine = 0;
	if (nNewTopLine >= nLineCount)
		nNewTopLine = nLineCount - 1;
	ScrollToLine(nNewTopLine);
}

void CBaseView::RecalcAllHorzScrollBars(BOOL bPositionOnly /*= FALSE*/)
{
	if ((m_pwndLeft)&&(m_pwndLeft->IsWindowVisible()))
		m_pwndLeft->RecalcHorzScrollBar(bPositionOnly);
	if ((m_pwndRight)&&(m_pwndRight->IsWindowVisible()))
		m_pwndRight->RecalcHorzScrollBar(bPositionOnly);
	if ((m_pwndBottom)&&(m_pwndBottom->IsWindowVisible()))
		m_pwndBottom->RecalcHorzScrollBar(bPositionOnly);
}

void CBaseView::RecalcHorzScrollBar(BOOL bPositionOnly /*= FALSE*/)
{
	SCROLLINFO si;
	si.cbSize = sizeof(si);
	if (bPositionOnly)
	{
		si.fMask = SIF_POS;
		si.nPos = m_nOffsetChar;
	}
	else
	{
		EnableScrollBarCtrl(SB_HORZ, TRUE);
		if (GetAllMinScreenChars() >= GetAllMaxLineLength() && m_nOffsetChar > 0)
		{
			m_nOffsetChar = 0;
			Invalidate();
		}
		si.fMask = SIF_DISABLENOSCROLL | SIF_PAGE | SIF_POS | SIF_RANGE;
		si.nMin = 0;
		si.nMax = GetAllMaxLineLength() + GetMarginWidth()/GetCharWidth();
		si.nPage = GetAllMinScreenChars();
		si.nPos = m_nOffsetChar;
	}
	VERIFY(SetScrollInfo(SB_HORZ, &si));
}

void CBaseView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	CView::OnHScroll(nSBCode, nPos, pScrollBar);
	if (m_pwndLeft)
		m_pwndLeft->OnDoHScroll(nSBCode,  nPos, pScrollBar, this);
	if (m_pwndRight)
		m_pwndRight->OnDoHScroll(nSBCode,  nPos, pScrollBar, this);
	if (m_pwndBottom)
		m_pwndBottom->OnDoHScroll(nSBCode,  nPos, pScrollBar, this);
	if (m_pwndLocator)
		m_pwndLocator->Invalidate();
}
void CBaseView::OnDoHScroll(UINT nSBCode, UINT /*nPos*/, CScrollBar* /*pScrollBar*/, CBaseView * master) 
{
	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	VERIFY(master->GetScrollInfo(SB_HORZ, &si));

	int nPageChars = GetScreenChars();
	int nMaxLineLength = GetMaxLineLength();

	int nNewOffset;
	switch (nSBCode)
	{
	case SB_LEFT:
		nNewOffset = 0;
		break;
	case SB_BOTTOM:
		nNewOffset = nMaxLineLength - nPageChars + 1;
		break;
	case SB_LINEUP:
		nNewOffset = m_nOffsetChar - 1;
		break;
	case SB_LINEDOWN:
		nNewOffset = m_nOffsetChar + 1;
		break;
	case SB_PAGEUP:
		nNewOffset = m_nOffsetChar - si.nPage + 1;
		break;
	case SB_PAGEDOWN:
		nNewOffset = m_nOffsetChar + si.nPage - 1;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		nNewOffset = si.nTrackPos;
		break;
	default:
		return;
	}

	if (nNewOffset >= nMaxLineLength)
		nNewOffset = nMaxLineLength - 1;
	if (nNewOffset < 0)
		nNewOffset = 0;
	ScrollToChar(nNewOffset, TRUE);
}

void CBaseView::ScrollToChar(int nNewOffsetChar, BOOL bTrackScrollBar /*= TRUE*/)
{
	if (m_nOffsetChar != nNewOffsetChar)
	{
		int nScrollChars = m_nOffsetChar - nNewOffsetChar;
		m_nOffsetChar = nNewOffsetChar;
		CRect rcScroll;
		GetClientRect(&rcScroll);
		rcScroll.left += GetMarginWidth();
		rcScroll.top += GetLineHeight()+HEADERHEIGHT;
		ScrollWindow(nScrollChars * GetCharWidth(), 0, &rcScroll, &rcScroll);
		// update the view header
		rcScroll.left = 0;
		rcScroll.top = 0;
		rcScroll.bottom = GetLineHeight()+HEADERHEIGHT;
		InvalidateRect(&rcScroll, FALSE);
		UpdateWindow();
		if (bTrackScrollBar)
			RecalcHorzScrollBar(TRUE);
	}
}

void CBaseView::ScrollSide(int delta)
{
		int nNewOffset = m_nOffsetChar;
		nNewOffset += delta;
		int nMaxLineLength = GetMaxLineLength();
		if (nNewOffset >= nMaxLineLength)
			nNewOffset = nMaxLineLength - 1;
		if (nNewOffset < 0)
			nNewOffset = 0;
		ScrollToChar(nNewOffset, TRUE);
		if (m_pwndLineDiffBar)
			m_pwndLineDiffBar->Invalidate();
}

void CBaseView::ScrollToLine(int nNewTopLine, BOOL bTrackScrollBar /*= TRUE*/)
{
	if (m_nTopLine != nNewTopLine)
	{
		if (nNewTopLine < 0)
			nNewTopLine = 0;
		int nScrollLines = m_nTopLine - nNewTopLine;
		m_nTopLine = nNewTopLine;
		CRect rcScroll;
		GetClientRect(&rcScroll);
		rcScroll.top += GetLineHeight()+HEADERHEIGHT;
		ScrollWindow(0, nScrollLines * GetLineHeight(), &rcScroll, &rcScroll);
		UpdateWindow();
		if (bTrackScrollBar)
			RecalcVertScrollBar(TRUE);
	}
}


void CBaseView::DrawMargin(CDC *pdc, const CRect &rect, int nLineIndex)
{
	pdc->FillSolidRect(rect, ::GetSysColor(COLOR_SCROLLBAR));

	if ((nLineIndex >= 0)&&(this->m_arLineStates)&&(this->m_arLineStates->GetCount()))
	{
		CDiffData::DiffStates state = (CDiffData::DiffStates)this->m_arLineStates->GetAt(nLineIndex);
		HICON icon = NULL;
		switch (state)
		{
		case CDiffData::DIFFSTATE_ADDED:
		case CDiffData::DIFFSTATE_THEIRSADDED:
		case CDiffData::DIFFSTATE_YOURSADDED:
		case CDiffData::DIFFSTATE_IDENTICALADDED:
		case CDiffData::DIFFSTATE_CONFLICTADDED:
			icon = m_hAddedIcon;
			break;
		case CDiffData::DIFFSTATE_REMOVED:
		case CDiffData::DIFFSTATE_THEIRSREMOVED:
		case CDiffData::DIFFSTATE_YOURSREMOVED:
		case CDiffData::DIFFSTATE_IDENTICALREMOVED:
			icon = m_hRemovedIcon;
			break;
		case CDiffData::DIFFSTATE_CONFLICTED:
			icon = m_hConflictedIcon;
			break;
		default:
			break;
		}
		bool bIdentical = false;
		if (IsBlockWhitespaceOnly(nLineIndex, bIdentical))
		{
			if (bIdentical)
				icon = m_hEqualIcon;
			else
				icon = m_hWhitespaceBlockIcon;
		}

		if (icon)
		{
			::DrawIconEx(pdc->m_hDC, rect.left + 2, rect.top + (rect.Height()-16)/2, icon, 16, 16, NULL, NULL, DI_NORMAL);
		}
		if ((m_bViewLinenumbers)&&(m_nDigits))
		{
			int nLineNumber = GetLineNumber(nLineIndex);
			if (nLineNumber >= 0)
			{
				CString sLinenumberFormat;
				CString sLinenumber;
				sLinenumberFormat.Format(_T("%%%dd"), m_nDigits);
				sLinenumber.Format(sLinenumberFormat, nLineNumber+1);
				pdc->SetBkColor(::GetSysColor(COLOR_SCROLLBAR));
				pdc->SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));

				pdc->SelectObject(GetFont());
				pdc->ExtTextOut(rect.left + 18, rect.top, ETO_CLIPPED, &rect, sLinenumber, NULL);
			}
		}
	}
}

int CBaseView::GetMarginWidth()
{
	if ((m_bViewLinenumbers)&&(m_arDiffLines)&&(m_arDiffLines->GetCount()))
	{
		int nWidth = GetCharWidth();
		if (m_nDigits <= 0)
		{
			int nLength = (int)m_arDiffLines->GetCount();
			// find out how many digits are needed to show the highest line number
			int nDigits = 1;
			while (nLength / 10)
			{
				nDigits++;
				nLength /= 10;
			}
			m_nDigits = nDigits;
		}
		return (MARGINWIDTH + (m_nDigits * nWidth) + 2);
	}
	return MARGINWIDTH;
}

void CBaseView::DrawHeader(CDC *pdc, const CRect &rect)
{
	CRect textrect(rect.left, rect.top, rect.Width(), GetLineHeight()+HEADERHEIGHT);
	CDiffData diffdata;
	COLORREF crBk, crFg;
	diffdata.GetColors(CDiffData::DIFFSTATE_NORMAL, crBk, crFg);
	crBk = ::GetSysColor(COLOR_SCROLLBAR);
	if ((m_pwndBottom)&&(m_pwndBottom->IsWindowVisible()))
	{
		pdc->SetBkColor(crBk);
	}
	else
	{

		if (this == m_pwndRight)
		{
			diffdata.GetColors(CDiffData::DIFFSTATE_ADDED, crBk, crFg);
			pdc->SetBkColor(crBk);
		}
		else
		{
			diffdata.GetColors(CDiffData::DIFFSTATE_REMOVED, crBk, crFg);
			pdc->SetBkColor(crBk);
		}
	}
	pdc->FillSolidRect(textrect, crBk);

	pdc->SetTextColor(crFg);

	pdc->SelectObject(GetFont(FALSE, TRUE, FALSE));
	if (IsModified())
	{
		if (m_sWindowName.Left(2).Compare(_T("* "))!=0)
			m_sWindowName = _T("* ") + m_sWindowName;
	}
	else
	{
		if (m_sWindowName.Left(2).Compare(_T("* "))==0)
			m_sWindowName = m_sWindowName.Mid(2);
	}
	CString sViewTitle = m_sWindowName;
	int nStringLength = (GetCharWidth()*m_sWindowName.GetLength());
	if (nStringLength > rect.Width())
	{
		int offset = min(m_nOffsetChar, (nStringLength-rect.Width())/GetCharWidth());

		sViewTitle = m_sWindowName.Mid(offset);
	}
	pdc->ExtTextOut(max(rect.left + (rect.Width()-nStringLength)/2, 1), 
		rect.top+(HEADERHEIGHT/2), ETO_CLIPPED, textrect, sViewTitle, NULL);
	if (this->GetFocus() == this)
		pdc->DrawEdge(textrect, EDGE_BUMP, BF_RECT);
	else
		pdc->DrawEdge(textrect, EDGE_ETCHED, BF_RECT);
}

void CBaseView::OnDraw(CDC * pDC)
{
	CRect rcClient;
	GetClientRect(rcClient);
	
	int nLineCount = GetLineCount();
	int nLineHeight = GetLineHeight();

	CDC cacheDC;
	VERIFY(cacheDC.CreateCompatibleDC(pDC));
	if (m_pCacheBitmap == NULL)
	{
		m_pCacheBitmap = new CBitmap;
		VERIFY(m_pCacheBitmap->CreateCompatibleBitmap(pDC, rcClient.Width(), nLineHeight));
	}
	CBitmap *pOldBitmap = cacheDC.SelectObject(m_pCacheBitmap);

	DrawHeader(pDC, rcClient);
	
	CRect rcLine;
	rcLine = rcClient;
	rcLine.top += nLineHeight+HEADERHEIGHT;
	rcLine.bottom = rcLine.top + nLineHeight;
	CRect rcCacheMargin(0, 0, GetMarginWidth(), nLineHeight);
	CRect rcCacheLine(GetMarginWidth(), 0, rcLine.Width(), nLineHeight);

	int nCurrentLine = m_nTopLine;
	while (rcLine.top < rcClient.bottom)
	{
		if (nCurrentLine < nLineCount)
		{
			DrawMargin(&cacheDC, rcCacheMargin, nCurrentLine);
			DrawSingleLine(&cacheDC, rcCacheLine, nCurrentLine);
		}
		else
		{
			DrawMargin(&cacheDC, rcCacheMargin, -1);
			DrawSingleLine(&cacheDC, rcCacheLine, -1);
		}

		VERIFY(pDC->BitBlt(rcLine.left, rcLine.top, rcLine.Width(), rcLine.Height(), &cacheDC, 0, 0, SRCCOPY));

		nCurrentLine ++;
		rcLine.OffsetRect(0, nLineHeight);
	}

	cacheDC.SelectObject(pOldBitmap);
	cacheDC.DeleteDC();
}

BOOL CBaseView::IsLineRemoved(int nLineIndex)
{
	DWORD state = 0;
	if (m_arLineStates)
		state = m_arLineStates->GetAt(nLineIndex);
	BOOL ret = FALSE;
	switch (state)
	{
	case CDiffData::DIFFSTATE_REMOVED:
	case CDiffData::DIFFSTATE_THEIRSREMOVED:
	case CDiffData::DIFFSTATE_YOURSREMOVED:
	case CDiffData::DIFFSTATE_IDENTICALREMOVED:
		ret = TRUE;
		break;
	default:
		ret = FALSE;
		break;
	}
	return ret;
}

COLORREF CBaseView::IntenseColor(long scale, COLORREF col)
{
	// if the color is already dark (grayscale below 127),
	// then lighten the color by 'scale', otherwise darken it
	int Gray  = (((int)GetRValue(col)) + GetGValue(col) + GetBValue(col))/3;
	if (Gray > 127)
	{
		long red   = MulDiv(GetRValue(col),(255-scale),255);
		long green = MulDiv(GetGValue(col),(255-scale),255);
		long blue  = MulDiv(GetBValue(col),(255-scale),255);

		return RGB(red, green, blue);
	}
	long R = MulDiv(255-GetRValue(col),scale,255)+GetRValue(col);
	long G = MulDiv(255-GetGValue(col),scale,255)+GetGValue(col);
	long B = MulDiv(255-GetBValue(col),scale,255)+GetBValue(col);

	return RGB(R, G, B);
}

void CBaseView::DrawSingleLine(CDC *pDC, const CRect &rc, int nLineIndex)
{
	if (nLineIndex >= GetLineCount())
		nLineIndex = -1;
	ASSERT(nLineIndex >= -1);

	if (nLineIndex == -1)
	{
		// Draw line beyond the text
		COLORREF bkGnd, crText;
		m_pMainFrame->m_Data.GetColors(CDiffData::DIFFSTATE_UNKNOWN, bkGnd, crText);
		pDC->FillSolidRect(rc, bkGnd);
		return;
	} 

	// Acquire the background color for the current line
	COLORREF crBkgnd, crText;
	if ((m_arLineStates)&&(m_arLineStates->GetCount()>nLineIndex))
	{
		m_pMainFrame->m_Data.GetColors((CDiffData::DiffStates)m_arLineStates->GetAt(nLineIndex), crBkgnd, crText);
		if ((nLineIndex >= m_nSelBlockStart)&&(nLineIndex <= m_nSelBlockEnd))
		{
			if (m_bFocused)
			{
				crBkgnd = IntenseColor(70, crBkgnd);
				crText = IntenseColor(70, crText);
			}
			else
			{
				crBkgnd = IntenseColor(30, crBkgnd);
				crText = IntenseColor(30, crText);
			}
		}
	}
	else
		m_pMainFrame->m_Data.GetColors(CDiffData::DIFFSTATE_UNKNOWN, crBkgnd, crText);

	int nLength = GetLineLength(nLineIndex);
	if (nLength == 0)
	{
		// Draw the empty line
		CRect rect = rc;
		pDC->FillSolidRect(rect, crBkgnd);
		if (m_bFocused)
		{
			if (nLineIndex == m_nDiffBlockStart)
			{
				pDC->FillSolidRect(rc.left, rc.top, rc.Width(), 2, RGB(0,0,0));
			}		
			if (nLineIndex == m_nDiffBlockEnd)
			{
				pDC->FillSolidRect(rc.left, rc.bottom-2, rc.Width(), 2, RGB(0,0,0));
			}
		}
		return;
	}

	LPCTSTR pszChars = GetLineChars(nLineIndex);
	LPCTSTR pszDiffChars = GetDiffLineChars(nLineIndex);
	int nDiffLength = GetDiffLineLength(nLineIndex);

	if (pszChars == NULL)
		return;

	// Draw the line
	CPoint origin(rc.left - m_nOffsetChar * GetCharWidth(), rc.top);
	pDC->SetBkColor(crBkgnd);
	pDC->SetTextColor(crText);

	pDC->SelectObject(GetFont(FALSE, FALSE, IsLineRemoved(nLineIndex)));
	if (nLength > 0)
	{
		CString line;
		ExpandChars(pszChars, 0, nLength, line);
		int nWidth = rc.right - origin.x;
		int savedx = origin.x;
		if (nWidth > 0)
		{
			int nCharWidth = GetCharWidth();
			int nCount = line.GetLength();
			int nCountFit = nWidth / nCharWidth + 1;
			if (nCount > nCountFit)
				nCount = nCountFit;

			if ((pszDiffChars)&&
				(CDiffData::DIFFSTATE_NORMAL != (CDiffData::DiffStates)m_arLineStates->GetAt(nLineIndex))&&
				!line.IsEmpty()&&
				(pszDiffChars[0]!=0) &&
				((m_pwndBottom == NULL) || (m_pwndBottom->IsHidden())))
			{
				CString diffline;
				ExpandChars(pszDiffChars, 0, nDiffLength, diffline);
				svn_diff_t * diff = NULL;
				m_svnlinediff.Diff(&diff, line, line.GetLength(), diffline, diffline.GetLength(), true);
				if ((diff)&&(m_bShowInlineDiff))
				{
					svn_diff_t * tempdiff = diff;
					int diffcounts = 0;
					int origcounts = 0;
					apr_off_t origsize = 0;
					apr_off_t diffsize = 0;
					// First check if we really should show inline diffs
					// Inline diffs are only useful if the two lines are not
					// completely different but at least a little bit similar
					while (tempdiff)
					{
						if (tempdiff->type == svn_diff__type_common)
						{
							origcounts++;
							origsize += tempdiff->original_length;
						}
						else
						{
							diffcounts++;
							diffsize += tempdiff->original_length;
							diffsize += tempdiff->modified_length;
						}
						tempdiff = tempdiff->next;
					}
					if ((origcounts > diffcounts)&&(origsize > diffsize))
					{
						tempdiff = diff;
						int lineoffset = 0;
						CString sDispTemp;
						typedef struct  
						{
							int x, y, cx, cy;
							COLORREF clr;
						} graphrects;
						std::deque<graphrects> rects;
						// If we're here means we consider the lines modified, not
						// removed/added. We use a different background color for
						// modified lines:
						crBkgnd = m_ModifiedBk;
						while (tempdiff)
						{
							if (tempdiff->type == svn_diff__type_common)
							{
								pDC->SetBkColor(crBkgnd);
								pDC->SetTextColor(crText);
								for (int i=0; i<tempdiff->original_length; ++i)
								{
									sDispTemp += m_svnlinediff.m_line1tokens[lineoffset].c_str();
									lineoffset++;
								}
								VERIFY(pDC->ExtTextOut(origin.x, origin.y, ETO_CLIPPED, &rc, (LPCTSTR)sDispTemp, sDispTemp.GetLength(), NULL));
								origin.x += (sDispTemp.GetLength() * nCharWidth);
								sDispTemp.Empty();
							}
							if (tempdiff->type == svn_diff__type_diff_modified)
							{
								if (tempdiff->original_length == tempdiff->modified_length)
								{
									if (IsLineRemoved(nLineIndex))
										pDC->SetBkColor(m_InlineRemovedBk);
									else
										pDC->SetBkColor(m_InlineAddedBk);
									pDC->SetTextColor(crText);
									for (int i=0; i<tempdiff->original_length; ++i)
									{
										sDispTemp += m_svnlinediff.m_line1tokens[lineoffset].c_str();
										lineoffset++;
									}
									VERIFY(pDC->ExtTextOut(origin.x, origin.y, ETO_CLIPPED, &rc, (LPCTSTR)sDispTemp, sDispTemp.GetLength(), NULL));
									origin.x += (sDispTemp.GetLength() * nCharWidth);
									sDispTemp.Empty();
								}
								else
								{
									if (tempdiff->original_length < tempdiff->modified_length)
									{
										if (IsLineRemoved(nLineIndex))
											pDC->SetBkColor(m_InlineRemovedBk);
										else
											pDC->SetBkColor(m_InlineAddedBk);
										pDC->SetTextColor(crText);
										for (int i=0; i<tempdiff->original_length; ++i)
										{
											sDispTemp += m_svnlinediff.m_line1tokens[lineoffset].c_str();
											lineoffset++;
										}
										VERIFY(pDC->ExtTextOut(origin.x, origin.y, ETO_CLIPPED, &rc, (LPCTSTR)sDispTemp, sDispTemp.GetLength(), NULL));
										origin.x += (sDispTemp.GetLength() * nCharWidth);
										sDispTemp.Empty();
										// now draw a removed line
										graphrects r = {origin.x-1, origin.y, 1, m_nLineHeight, m_InlineRemovedBk};
										rects.push_back(r);
									}
									if (tempdiff->original_length > tempdiff->modified_length)
									{
										if (IsLineRemoved(nLineIndex))
											pDC->SetBkColor(m_InlineRemovedBk);
										else
											pDC->SetBkColor(m_InlineAddedBk);
										pDC->SetTextColor(crText);
										for (int i=0; i<tempdiff->modified_length; ++i)
										{
											sDispTemp += m_svnlinediff.m_line1tokens[lineoffset].c_str();
											lineoffset++;
										}
										UINT len = (UINT)(tempdiff->original_length-tempdiff->modified_length);
										for (UINT i=0; i<len; ++i)
										{
											sDispTemp += m_svnlinediff.m_line1tokens[lineoffset].c_str();
											lineoffset++;
										}
										VERIFY(pDC->ExtTextOut(origin.x, origin.y, ETO_CLIPPED, &rc, (LPCTSTR)sDispTemp, sDispTemp.GetLength(), NULL));
										origin.x += (sDispTemp.GetLength() * nCharWidth);
										sDispTemp.Empty();
									}
								}
							}
							tempdiff = tempdiff->next;
						}
						for (std::deque<graphrects>::iterator it = rects.begin(); it != rects.end(); ++it)
						{
							pDC->FillSolidRect(it->x, it->y, it->cx, it->cy, it->clr);
						}
					}
					else
						VERIFY(pDC->ExtTextOut(origin.x, origin.y, ETO_CLIPPED, &rc, line, nCount, NULL));
				}
				else
					VERIFY(pDC->ExtTextOut(origin.x, origin.y, ETO_CLIPPED, &rc, line, nCount, NULL));
			}
			else
				VERIFY(pDC->ExtTextOut(origin.x, origin.y, ETO_CLIPPED, &rc, line, nCount, NULL));
		} // if (nWidth > 0) 
		
		origin.x = savedx + pDC->GetTextExtent(line).cx;
	} // if (nLength > 0) 

	//	Draw whitespaces to the left of the text
	CRect frect = rc;
	if (origin.x > frect.left)
		frect.left = origin.x;
	if (frect.right > frect.left)
	{
		if (frect.right > frect.left)
			pDC->FillSolidRect(frect, crBkgnd);
	}
	COLORREF rectcol = m_bFocused ? GetSysColor(COLOR_WINDOWTEXT) : GetSysColor(COLOR_GRAYTEXT);
	if ((nLineIndex == m_nDiffBlockStart)||(nLineIndex == m_nSelBlockStart))
	{
		pDC->FillSolidRect(rc.left, rc.top, rc.Width(), 2, rectcol);
	}		
	if ((nLineIndex == m_nDiffBlockEnd)||(nLineIndex == m_nSelBlockEnd))
	{
		pDC->FillSolidRect(rc.left, rc.bottom-2, rc.Width(), 2, rectcol);
	}
}

void CBaseView::ExpandChars(LPCTSTR pszChars, int nOffset, int nCount, CString &line)
{
	if (nCount <= 0)
	{
		line = _T("");
		return;
	}

	int nTabSize = GetTabSize();

	int nActualOffset = 0;
	for (int i=0; i<nOffset; i++)
	{
		if (pszChars[i] == _T('\t'))
			nActualOffset += (nTabSize - nActualOffset % nTabSize);
		else
			nActualOffset ++;
	}

	pszChars += nOffset;
	int nLength = nCount;

	int nTabCount = 0;
	for (int i=0; i<nLength; i++)
	{
		if (pszChars[i] == _T('\t'))
			nTabCount ++;
	}

	LPTSTR pszBuf = line.GetBuffer(nLength + nTabCount * (nTabSize - 1) + 1);
	int nCurPos = 0;
	if (nTabCount > 0 || m_bViewWhitespace)
	{
		for (int i=0; i<nLength; i++)
		{
			if (pszChars[i] == _T('\t'))
			{
				int nSpaces = nTabSize - (nActualOffset + nCurPos) % nTabSize;
				if (m_bViewWhitespace)
				{
					pszBuf[nCurPos ++] = TAB_CHARACTER;
					nSpaces --;
				}
				while (nSpaces > 0)
				{
					pszBuf[nCurPos ++] = _T(' ');
					nSpaces --;
				}
			}
			else
			{
				if (pszChars[i] == _T(' ') && m_bViewWhitespace)
					pszBuf[nCurPos] = SPACE_CHARACTER;
				else
					pszBuf[nCurPos] = pszChars[i];
				nCurPos ++;
			}
		}
	}
	else
	{
		memcpy(pszBuf, pszChars, sizeof(TCHAR) * nLength);
		nCurPos = nLength;
	}
	pszBuf[nCurPos] = 0;
	line.ReleaseBuffer();
}

void CBaseView::ScrollAllToLine(int nNewTopLine, BOOL bTrackScrollBar)
{
	if ((m_pwndLeft)&&(m_pwndRight))
	{
		m_pwndLeft->ScrollToLine(nNewTopLine, bTrackScrollBar);
		m_pwndRight->ScrollToLine(nNewTopLine, bTrackScrollBar);
	}
	else
	{
		if (m_pwndLeft)
			m_pwndLeft->ScrollToLine(nNewTopLine, bTrackScrollBar);
		if (m_pwndRight)
			m_pwndRight->ScrollToLine(nNewTopLine, bTrackScrollBar);
	}
	if (m_pwndBottom)
		m_pwndBottom->ScrollToLine(nNewTopLine, bTrackScrollBar);
	if (m_pwndLocator)
		m_pwndLocator->Invalidate();
}

void CBaseView::GoToLine(int nNewLine, BOOL bAll)
{
	//almost the same as ScrollAllToLine, but try to put the line in the
	//middle of the view, not on top
	int nNewTopLine = nNewLine - GetScreenLines()/2;
	if (nNewTopLine < 0)
		nNewTopLine = 0;
	if (m_arDiffLines)
	{
		if (nNewTopLine >= this->m_arDiffLines->GetCount())
			nNewTopLine = m_arDiffLines->GetCount()-1;
		if (bAll)
			ScrollAllToLine(nNewTopLine);
		else
			ScrollToLine(nNewTopLine);
	}
}

BOOL CBaseView::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
}

int CBaseView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	memset(&m_lfBaseFont, 0, sizeof(m_lfBaseFont));
	//lstrcpy(m_lfBaseFont.lfFaceName, _T("Courier New"));
	//lstrcpy(m_lfBaseFont.lfFaceName, _T("FixedSys"));
	m_lfBaseFont.lfHeight = 0;
	m_lfBaseFont.lfWeight = FW_NORMAL;
	m_lfBaseFont.lfItalic = FALSE;
	m_lfBaseFont.lfCharSet = DEFAULT_CHARSET;
	m_lfBaseFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
	m_lfBaseFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	m_lfBaseFont.lfQuality = DEFAULT_QUALITY;
	m_lfBaseFont.lfPitchAndFamily = DEFAULT_PITCH;

	return 0;
}

void CBaseView::OnDestroy()
{
	CView::OnDestroy();
	for (int i=0; i<MAXFONTS; i++)
	{
		if (m_apFonts[i] != NULL)
		{
			m_apFonts[i]->DeleteObject();
			delete m_apFonts[i];
			m_apFonts[i] = NULL;
		}
	}
	if (m_pCacheBitmap != NULL)
	{
		delete m_pCacheBitmap;
		m_pCacheBitmap = NULL;
	}
}

void CBaseView::OnSize(UINT nType, int cx, int cy)
{
	if (m_pCacheBitmap != NULL)
	{
		m_pCacheBitmap->DeleteObject();
		delete m_pCacheBitmap;
		m_pCacheBitmap = NULL;
	}
	// make sure the view header is redrawn
	CRect rcScroll;
	GetClientRect(&rcScroll);
	rcScroll.bottom = GetLineHeight()+HEADERHEIGHT;
	InvalidateRect(&rcScroll, FALSE);

	m_nScreenLines = -1;
	m_nScreenChars = -1;
	RecalcVertScrollBar();
	RecalcHorzScrollBar();
	CView::OnSize(nType, cx, cy);
}

BOOL CBaseView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (m_pwndLeft)
		m_pwndLeft->OnDoMouseWheel(nFlags, zDelta, pt);
	if (m_pwndRight)
		m_pwndRight->OnDoMouseWheel(nFlags, zDelta, pt);
	if (m_pwndBottom)
		m_pwndBottom->OnDoMouseWheel(nFlags, zDelta, pt);
	if (m_pwndLocator)
		m_pwndLocator->Invalidate();
	return CView::OnMouseWheel(nFlags, zDelta, pt);
}

void CBaseView::OnDoMouseWheel(UINT /*nFlags*/, short zDelta, CPoint /*pt*/)
{
	if (GetKeyState(VK_CONTROL)&0x8000)
	{
		// Ctrl-Wheel scrolls sideways
		ScrollSide(-zDelta/30);
	}
	else
	{
		int nLineCount = GetLineCount();
		int nTopLine = m_nTopLine;
		nTopLine -= (zDelta/30);
		if (nTopLine < 0)
			nTopLine = 0;
		if (nTopLine >= nLineCount)
			nTopLine = nLineCount - 1;
		ScrollToLine(nTopLine, TRUE);
	}
}

BOOL CBaseView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (nHitTest == HTCLIENT)
	{
		::SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW)));	// Set To Arrow Cursor
		return TRUE;
	}
	return CView::OnSetCursor(pWnd, nHitTest, message);
}

void CBaseView::OnKillFocus(CWnd* pNewWnd)
{
	CView::OnKillFocus(pNewWnd);
	m_bFocused = FALSE;
	Invalidate();
}

void CBaseView::OnSetFocus(CWnd* pOldWnd)
{
	CView::OnSetFocus(pOldWnd);
	m_bFocused = TRUE;
	Invalidate();
}

int CBaseView::GetLineFromPoint(CPoint point)
{
	ScreenToClient(&point);
	return (((point.y - HEADERHEIGHT) / GetLineHeight()) + m_nTopLine);
}

void CBaseView::OnContextMenu(CPoint /*point*/, int /*nLine*/)
{
}

BOOL CBaseView::ShallShowContextMenu(CDiffData::DiffStates /*state*/, int /*nLine*/)
{
	return FALSE;
}

void CBaseView::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	int nLine = GetLineFromPoint(point);

	if (!m_arLineStates)
		return;
	if (m_nSelBlockEnd >= GetLineCount())
		m_nSelBlockEnd = GetLineCount()-1;
	if ((nLine <= m_arLineStates->GetCount())&&(nLine > m_nTopLine))
	{
		int nIndex = nLine - 1;
		CDiffData::DiffStates state = (CDiffData::DiffStates)m_arLineStates->GetAt(nIndex);
		if ((state != CDiffData::DIFFSTATE_NORMAL) && (state != CDiffData::DIFFSTATE_UNKNOWN))
		{
			// if there's nothing selected, or if the selection is outside the window then
			// select the diff block under the cursor.
			if (((m_nSelBlockStart<0)&&(m_nSelBlockEnd<0)&&(ShallShowContextMenu(state, nLine)))||
				((m_nSelBlockEnd < m_nTopLine)||(m_nSelBlockStart > m_nTopLine+m_nScreenLines)))
			{
				while (nIndex >= 0)
				{
					if (nIndex == 0)
					{
						nIndex--;
						break;
					}
					if (state != (CDiffData::DiffStates)m_arLineStates->GetAt(--nIndex))
						break;
				}
				m_nSelBlockStart = nIndex+1;
				while (nIndex < (m_arLineStates->GetCount()-1))
				{
					if (state != (CDiffData::DiffStates)m_arLineStates->GetAt(++nIndex))
						break;
				}
				if ((nIndex == (m_arLineStates->GetCount()-1))&&(state == (CDiffData::DiffStates)m_arLineStates->GetAt(nIndex)))
					m_nSelBlockEnd = nIndex;
				else
					m_nSelBlockEnd = nIndex-1;
				Invalidate();
			}
		}
		if ((m_nSelBlockStart <= (nLine-1))&&(m_nSelBlockEnd >= (nLine-1))&&(ShallShowContextMenu(state, nLine)))
		{
			OnContextMenu(point, nLine);
			m_nSelBlockStart = -1;
			m_nSelBlockEnd = -1;
			RefreshViews();
		}
	}
}

void CBaseView::RefreshViews()
{
	if (m_pwndLeft)
	{
		m_pwndLeft->UpdateStatusBar();
		m_pwndLeft->Invalidate();
	}
	if (m_pwndRight)
	{
		m_pwndRight->UpdateStatusBar();
		m_pwndRight->Invalidate();
	}
	if (m_pwndBottom)
	{
		m_pwndBottom->UpdateStatusBar();
		m_pwndBottom->Invalidate();
	}
	if (m_pwndLocator)
		m_pwndLocator->Invalidate();
}

void CBaseView::GoToFirstDifference()
{
	int nCenterPos = 0;
	if ((m_arLineStates)&&(0 < m_arLineStates->GetCount()))
	{
		while (nCenterPos < m_arLineStates->GetCount())
		{
			CDiffData::DiffStates linestate = (CDiffData::DiffStates)m_arLineStates->GetAt(nCenterPos);
			if ((linestate != CDiffData::DIFFSTATE_NORMAL) &&
				(linestate != CDiffData::DIFFSTATE_UNKNOWN))
				break;
			nCenterPos++;
		}
		int nTopPos = nCenterPos - (GetScreenLines()/2);
		if (nTopPos < 0)
			nTopPos = 0;
		ScrollAllToLine(nTopPos);
		RecalcAllVertScrollBars(TRUE);
	}
}

void CBaseView::SetupDiffBars(int start, int end)
{
	if ((m_pwndBottom)&&(m_pwndBottom->IsWindowVisible()))
	{
		m_pwndBottom->m_nDiffBlockStart = start;
		m_pwndBottom->m_nDiffBlockEnd = end;
		m_pwndBottom->Invalidate();
	}
	if ((m_pwndLeft)&&(m_pwndLeft->IsWindowVisible()))
	{
		m_pwndLeft->m_nDiffBlockStart = start;
		m_pwndLeft->m_nDiffBlockEnd = end;
		m_pwndLeft->Invalidate();
	}
	if ((m_pwndRight)&&(m_pwndRight->IsWindowVisible()))
	{
		m_pwndRight->m_nDiffBlockStart = start;
		m_pwndRight->m_nDiffBlockEnd = end;
		m_pwndRight->Invalidate();
	}
}

void CBaseView::HiglightLines(int start, int end /* = -1 */)
{
	m_nDiffBlockStart = start;
	if (end < 0)
		end = start;
	m_nDiffBlockEnd = end;
	Invalidate();
}

void CBaseView::SetupSelection(int start, int end)
{
	if ((m_pwndBottom)&&(m_pwndBottom->IsWindowVisible()))
	{
		m_pwndBottom->m_nSelBlockStart = start;
		m_pwndBottom->m_nSelBlockEnd = end;
		m_pwndBottom->Invalidate();
	}
	if ((m_pwndLeft)&&(m_pwndLeft->IsWindowVisible()))
	{
		m_pwndLeft->m_nSelBlockStart = start;
		m_pwndLeft->m_nSelBlockEnd = end;
		m_pwndLeft->Invalidate();
	}
	if ((m_pwndRight)&&(m_pwndRight->IsWindowVisible()))
	{
		m_pwndRight->m_nSelBlockStart = start;
		m_pwndRight->m_nSelBlockEnd = end;
		m_pwndRight->Invalidate();
	}
}

void CBaseView::OnMergePreviousconflict()
{
	int nCenterPos = m_nTopLine + (GetScreenLines()/2);
	if ((m_nSelBlockStart >= 0)&&(m_nSelBlockEnd >= 0))
		nCenterPos = m_nSelBlockStart;
	if ((m_arLineStates)&&(m_nTopLine < m_arLineStates->GetCount()))
	{
		if (nCenterPos >= m_arLineStates->GetCount())
			nCenterPos = (int)m_arLineStates->GetCount()-1;
		CDiffData::DiffStates state = (CDiffData::DiffStates)m_arLineStates->GetAt(nCenterPos);
		while ((nCenterPos >= 0) &&
			((CDiffData::DiffStates)m_arLineStates->GetAt(nCenterPos--)==state))
			;
		if (nCenterPos < (m_arLineStates->GetCount()-1))
			nCenterPos++;
		while (nCenterPos >= 0)
		{
			CDiffData::DiffStates linestate = (CDiffData::DiffStates)m_arLineStates->GetAt(nCenterPos);
			if ((linestate == CDiffData::DIFFSTATE_CONFLICTADDED) ||
				(linestate == CDiffData::DIFFSTATE_CONFLICTED) ||
				(linestate == CDiffData::DIFFSTATE_CONFLICTEMPTY))
				break;
			nCenterPos--;
		}
		if (nCenterPos < 0)
			nCenterPos = 0;
		m_nSelBlockStart = nCenterPos;
		m_nSelBlockEnd = nCenterPos;
		CDiffData::DiffStates linestate = (CDiffData::DiffStates)m_arLineStates->GetAt(nCenterPos);
		while ((m_nSelBlockStart < (m_arLineStates->GetCount()-1))&&(m_nSelBlockStart > 0))
		{
			if (linestate != (CDiffData::DiffStates)m_arLineStates->GetAt(--m_nSelBlockStart))
				break;
		}
		if (((m_nSelBlockStart == (m_arLineStates->GetCount()-1))&&(linestate == (CDiffData::DiffStates)m_arLineStates->GetAt(m_nSelBlockStart)))||m_nSelBlockStart==0)
			m_nSelBlockStart = m_nSelBlockStart;
		else
			m_nSelBlockStart = m_nSelBlockStart+1;
		int nTopPos = nCenterPos - (GetScreenLines()/2);
		if (nTopPos < 0)
			nTopPos = 0;
		ScrollAllToLine(nTopPos, FALSE);
		RecalcAllVertScrollBars(TRUE);
		SetupSelection(m_nSelBlockStart, m_nSelBlockEnd);
		SetupDiffBars(m_nDiffBlockStart, m_nDiffBlockEnd);
		ShowDiffLines(m_nDiffBlockStart);
	}
}

void CBaseView::OnMergeNextconflict()
{
	int nCenterPos = m_nTopLine + (GetScreenLines()/2);
	if ((m_nSelBlockStart >= 0)&&(m_nSelBlockEnd >= 0))
		nCenterPos = m_nSelBlockEnd;
	if ((m_arLineStates)&&(m_nTopLine < m_arLineStates->GetCount()))
	{
		if (nCenterPos >= m_arLineStates->GetCount())
			nCenterPos = (int)m_arLineStates->GetCount()-1;
		CDiffData::DiffStates state = (CDiffData::DiffStates)m_arLineStates->GetAt(nCenterPos);
		while ((nCenterPos < m_arLineStates->GetCount()) &&
			((CDiffData::DiffStates)m_arLineStates->GetAt(nCenterPos++)==state))
			;
		if (nCenterPos > 0)
			nCenterPos--;
		while (nCenterPos < m_arLineStates->GetCount())
		{
			CDiffData::DiffStates linestate = (CDiffData::DiffStates)m_arLineStates->GetAt(nCenterPos);
			if ((linestate == CDiffData::DIFFSTATE_CONFLICTADDED) ||
				(linestate == CDiffData::DIFFSTATE_CONFLICTED) ||
				(linestate == CDiffData::DIFFSTATE_CONFLICTEMPTY))
				break;
			nCenterPos++;
		}
		if (nCenterPos > (m_arLineStates->GetCount()-1))
			nCenterPos = m_arLineStates->GetCount()-1;
		m_nSelBlockStart = nCenterPos;
		m_nSelBlockEnd = nCenterPos;
		CDiffData::DiffStates linestate = (CDiffData::DiffStates)m_arLineStates->GetAt(nCenterPos);
		while (m_nSelBlockEnd < (m_arLineStates->GetCount()-1))
		{
			if (linestate != (CDiffData::DiffStates)m_arLineStates->GetAt(++m_nSelBlockEnd))
				break;
		}
		if ((m_nSelBlockEnd == (m_arLineStates->GetCount()-1))&&(linestate == (CDiffData::DiffStates)m_arLineStates->GetAt(m_nSelBlockEnd)))
			m_nSelBlockEnd = m_nSelBlockEnd;
		else
			m_nSelBlockEnd = m_nSelBlockEnd-1;
		int nTopPos = nCenterPos - (GetScreenLines()/2);
		if (nTopPos < 0)
			nTopPos = 0;
		ScrollAllToLine(nTopPos, FALSE);
		RecalcAllVertScrollBars(TRUE);
		SetupSelection(m_nSelBlockStart, m_nSelBlockEnd);
		SetupDiffBars(m_nDiffBlockStart, m_nDiffBlockEnd);
		ShowDiffLines(m_nDiffBlockStart);
	}
}

void CBaseView::OnMergeNextdifference()
{
	int nCenterPos = m_nTopLine + (GetScreenLines()/2);
	if ((m_nDiffBlockStart >= 0)&&(m_nDiffBlockEnd >= 0))
		nCenterPos = m_nDiffBlockEnd;
	if ((m_arLineStates)&&(m_nTopLine < m_arLineStates->GetCount()))
	{
		if (nCenterPos >= m_arLineStates->GetCount())
			nCenterPos = (int)m_arLineStates->GetCount()-1;
		CDiffData::DiffStates state = (CDiffData::DiffStates)m_arLineStates->GetAt(nCenterPos);
		while ((nCenterPos < m_arLineStates->GetCount()) &&
			((CDiffData::DiffStates)m_arLineStates->GetAt(nCenterPos++)==state))
			;
		if (nCenterPos > 0)
			nCenterPos--;
		while (nCenterPos < m_arLineStates->GetCount())
		{
			CDiffData::DiffStates linestate = (CDiffData::DiffStates)m_arLineStates->GetAt(nCenterPos);
			if ((linestate != CDiffData::DIFFSTATE_NORMAL) &&
				(linestate != CDiffData::DIFFSTATE_UNKNOWN))
				break;
			nCenterPos++;
		}
		if (nCenterPos > (m_arLineStates->GetCount()-1))
			nCenterPos = m_arLineStates->GetCount()-1;
		m_nDiffBlockStart = nCenterPos;
		m_nDiffBlockEnd = nCenterPos;
		CDiffData::DiffStates linestate = (CDiffData::DiffStates)m_arLineStates->GetAt(nCenterPos);
		while (m_nDiffBlockEnd < (m_arLineStates->GetCount()-1))
		{
			if (linestate != (CDiffData::DiffStates)m_arLineStates->GetAt(++m_nDiffBlockEnd))
				break;
		}
		if ((m_nDiffBlockEnd == (m_arLineStates->GetCount()-1))&&(linestate == (CDiffData::DiffStates)m_arLineStates->GetAt(m_nDiffBlockEnd)))
			m_nDiffBlockEnd = m_nDiffBlockEnd;
		else
			m_nDiffBlockEnd = m_nDiffBlockEnd-1;
		int nTopPos = nCenterPos - (GetScreenLines()/2);
		if (nTopPos < 0)
			nTopPos = 0;
			
		SetupSelection(m_nDiffBlockStart, m_nDiffBlockEnd);
		ScrollAllToLine(nTopPos, FALSE);
		RecalcAllVertScrollBars(TRUE);
		SetupDiffBars(m_nDiffBlockStart, m_nDiffBlockEnd);
		ShowDiffLines(m_nDiffBlockStart);
	}
}

void CBaseView::OnMergePreviousdifference()
{
	int nCenterPos = m_nTopLine + (GetScreenLines()/2);
	if ((m_nDiffBlockStart >= 0)&&(m_nDiffBlockEnd >= 0))
		nCenterPos = m_nDiffBlockStart;
	if ((m_arLineStates)&&(m_nTopLine < m_arLineStates->GetCount()))
	{
		if (nCenterPos >= m_arLineStates->GetCount())
			nCenterPos = (int)m_arLineStates->GetCount()-1;
		CDiffData::DiffStates state = (CDiffData::DiffStates)m_arLineStates->GetAt(nCenterPos);
		while ((nCenterPos >= 0) &&
			((CDiffData::DiffStates)m_arLineStates->GetAt(nCenterPos--)==state))
			;
		if (nCenterPos < (m_arLineStates->GetCount()-1))
			nCenterPos++;
		while (nCenterPos >= 0)
		{
			CDiffData::DiffStates linestate = (CDiffData::DiffStates)m_arLineStates->GetAt(nCenterPos);
			if ((linestate != CDiffData::DIFFSTATE_NORMAL) &&
				(linestate != CDiffData::DIFFSTATE_UNKNOWN))
				break;
			nCenterPos--;
		}
		if (nCenterPos < 0)
			nCenterPos = 0;
		m_nDiffBlockStart = nCenterPos;
		m_nDiffBlockEnd = nCenterPos;
		CDiffData::DiffStates linestate = (CDiffData::DiffStates)m_arLineStates->GetAt(nCenterPos);
		while ((m_nDiffBlockStart < (m_arLineStates->GetCount()-1))&&(m_nDiffBlockStart > 0))
		{
			if (linestate != (CDiffData::DiffStates)m_arLineStates->GetAt(--m_nDiffBlockStart))
				break;
		}
		if (((m_nDiffBlockStart == (m_arLineStates->GetCount()-1))&&(linestate == (CDiffData::DiffStates)m_arLineStates->GetAt(m_nDiffBlockStart)))||m_nDiffBlockStart==0)
			m_nDiffBlockStart = m_nDiffBlockStart;
		else
			m_nDiffBlockStart = m_nDiffBlockStart+1;
		int nTopPos = nCenterPos - (GetScreenLines()/2);
		if (nTopPos < 0)
			nTopPos = 0;
		SetupSelection(m_nDiffBlockStart, m_nDiffBlockEnd);
		ScrollAllToLine(nTopPos, FALSE);
		RecalcAllVertScrollBars(TRUE);
		SetupDiffBars(m_nDiffBlockStart, m_nDiffBlockEnd);
		ShowDiffLines(m_nDiffBlockStart);
	}
}

BOOL CBaseView::OnToolTipNotify(UINT /*id*/, NMHDR *pNMHDR, LRESULT *pResult)
{
	// need to handle both ANSI and UNICODE versions of the message
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
	CString strTipText;
	UINT nID = (UINT)pNMHDR->idFrom;
	if (pNMHDR->code == TTN_NEEDTEXTA && (pTTTA->uFlags & TTF_IDISHWND) ||
		pNMHDR->code == TTN_NEEDTEXTW && (pTTTW->uFlags & TTF_IDISHWND))
	{
		// idFrom is actually the HWND of the tool
		nID = ::GetDlgCtrlID((HWND)nID);
	}

	if (pNMHDR->idFrom == (UINT)m_hWnd)
	{
		if (m_sWindowName.Left(2).Compare(_T("* "))==0)
		{
			strTipText = m_sWindowName.Mid(2) + _T("\r\n") + m_sFullFilePath;
		}
		else
		{
			strTipText = m_sWindowName + _T("\r\n") + m_sFullFilePath;
		}
	}
	else
		return FALSE;
	
	*pResult = 0;
	if (strTipText.IsEmpty())
		return TRUE;

	if (pNMHDR->code == TTN_NEEDTEXTA)
	{
		pTTTA->lpszText = m_szTip;
		WideCharToMultiByte(CP_ACP, 0, strTipText, -1, m_szTip, strTipText.GetLength()+1, 0, 0);
	}
	else
	{
		lstrcpyn(m_wszTip, strTipText, strTipText.GetLength()+1);
		pTTTW->lpszText = m_wszTip;
	}

	return TRUE;    // message was handled
}


INT_PTR CBaseView::OnToolHitTest(CPoint point, TOOLINFO* pTI) const
{
	CRect rcClient;
	GetClientRect(rcClient);
	CRect textrect(rcClient.left, rcClient.top, rcClient.Width(), m_nLineHeight+HEADERHEIGHT);
	if (textrect.PtInRect(point))
	{
		// inside the header part of the view (showing the filename)
		pTI->hwnd = this->m_hWnd;
		this->GetClientRect(&pTI->rect);
		pTI->uFlags  |= TTF_ALWAYSTIP | TTF_IDISHWND;
		pTI->uId = (UINT)m_hWnd;
		pTI->lpszText = LPSTR_TEXTCALLBACK;

		// we want multiline tooltips
		CToolTipCtrl* pToolTip = AfxGetModuleThreadState()->m_pToolTip;
		if (pToolTip->GetSafeHwnd() != NULL)
		{
			pToolTip->SetMaxTipWidth(INT_MAX);
		}

		return 1;
	}
	return -1;
}

BOOL CBaseView::PreTranslateMessage(MSG* pMsg)
{
	return CView::PreTranslateMessage(pMsg);
}

void CBaseView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar==VK_PRIOR)
	{
		if (GetKeyState(VK_CONTROL)&0x8000)
		{
			int nPageWidth = GetScreenChars();
			if (m_pwndLeft)
				m_pwndLeft->ScrollSide(-nPageWidth);
			if (m_pwndRight)
				m_pwndRight->ScrollSide(-nPageWidth);
			if (m_pwndBottom)
				m_pwndBottom->ScrollSide(-nPageWidth);
		}
		else
		{
			int nPageChars = GetScreenLines();
			int nLineCount = GetLineCount();
			int nNewTopLine = 0;

			nNewTopLine = m_nTopLine - nPageChars + 1;
			if (nNewTopLine < 0)
				nNewTopLine = 0;
			if (nNewTopLine >= nLineCount)
				nNewTopLine = nLineCount - 1;
			ScrollAllToLine(nNewTopLine);
		}
	}
	if (nChar==VK_NEXT)
	{
		if (GetKeyState(VK_CONTROL)&0x8000)
		{
			int nPageWidth = GetScreenChars();
			if (m_pwndLeft)
				m_pwndLeft->ScrollSide(nPageWidth);
			if (m_pwndRight)
				m_pwndRight->ScrollSide(nPageWidth);
			if (m_pwndBottom)
				m_pwndBottom->ScrollSide(nPageWidth);
		}
		else
		{
			int nPageChars = GetScreenLines();
			int nLineCount = GetLineCount();
			int nNewTopLine = 0;

			nNewTopLine = m_nTopLine + nPageChars - 1;
			if (nNewTopLine < 0)
				nNewTopLine = 0;
			if (nNewTopLine >= nLineCount)
				nNewTopLine = nLineCount - 1;
			ScrollAllToLine(nNewTopLine);
		}
	}
	if (nChar==VK_HOME)
	{
		if (GetKeyState(VK_CONTROL)&0x8000)
			ScrollAllToLine(0);
		else
		{
			ScrollToChar(0);
		}
	}
	if (nChar==VK_END)
	{
		if (GetKeyState(VK_CONTROL)&0x8000)
			ScrollAllToLine(GetLineCount()-GetAllMinScreenLines());
		else
		{
			int charpos = GetMaxLineLength() - GetScreenChars();
			if (charpos > 0)
				ScrollToChar(charpos);
		}
	}
	if (nChar==VK_DOWN)
	{
		if (m_nSelBlockEnd < GetLineCount()-1)
		{
			m_nSelBlockEnd++;
			Invalidate();
		}
	}
	if (nChar==VK_UP)
	{
		if (m_nSelBlockEnd > m_nSelBlockStart)
		{
			m_nSelBlockEnd--;
			Invalidate();
		}
	}
	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CBaseView::OnLButtonUp(UINT nFlags, CPoint point)
{
	int nClickedLine = (((point.y - HEADERHEIGHT) / GetLineHeight()) + m_nTopLine);
	nClickedLine--;		//we need the index
	if ((nClickedLine >= m_nTopLine)&&(nClickedLine < GetLineCount()))
	{
		m_nDiffBlockEnd = -1;
		m_nDiffBlockStart = -1;
		if (nFlags & MK_SHIFT)
		{
			if (m_nSelBlockStart > nClickedLine)
				m_nSelBlockStart = nClickedLine;
			else if ((m_nSelBlockEnd < nClickedLine)&&(m_nSelBlockStart >= 0))
				m_nSelBlockEnd = nClickedLine;
			else
			{
				m_nSelBlockStart = m_nSelBlockEnd = nClickedLine;
			}
		}
		else
		{
			if ((m_nSelBlockStart == nClickedLine) && (m_nSelBlockEnd == nClickedLine))
			{
				// deselect!
				m_nSelBlockStart = -1;
				m_nSelBlockEnd = -1;
			}
			else
			{
				// select the line
				m_nSelBlockStart = m_nSelBlockEnd = nClickedLine;
			}
		}
		SetupSelection(m_nSelBlockStart, m_nSelBlockEnd);
		Invalidate();
	}

	CView::OnLButtonUp(nFlags, point);
}

void CBaseView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	int nClickedLine = (((point.y - HEADERHEIGHT) / GetLineHeight()) + m_nTopLine);
	nClickedLine--;		//we need the index
	if ((nClickedLine >= m_nTopLine)&&(nClickedLine < GetLineCount()))
	{
		// doubleclick on the margin?
		if (point.x <= GetMarginWidth())
		{
			//toggle between linestates
			CDiffData::DiffStates state = (CDiffData::DiffStates)m_arLineStates->GetAt(nClickedLine);
			switch (state)
			{
			case CDiffData::DIFFSTATE_ADDED:
			case CDiffData::DIFFSTATE_IDENTICALADDED:
			case CDiffData::DIFFSTATE_THEIRSADDED:
			case CDiffData::DIFFSTATE_YOURSADDED:
				state = CDiffData::DIFFSTATE_REMOVED;
				if ((this == m_pwndRight)||(this == m_pwndBottom))
					SetModified();
				break;
			case CDiffData::DIFFSTATE_IDENTICALREMOVED:
			case CDiffData::DIFFSTATE_REMOVED:
			case CDiffData::DIFFSTATE_THEIRSREMOVED:
			case CDiffData::DIFFSTATE_YOURSREMOVED:
				state = CDiffData::DIFFSTATE_ADDED;
				if ((this == m_pwndRight)||(this == m_pwndBottom))
					SetModified();
				break;
			}
			m_arLineStates->SetAt(nClickedLine, state);
			Invalidate();
		}
	}

	CView::OnLButtonDblClk(nFlags, point);
}

void CBaseView::OnEditCopy()
{
	if ((m_nSelBlockEnd < 0)||(m_nSelBlockStart < 0)||(m_nSelBlockStart > m_nSelBlockEnd))
		return;
	// first store the selected lines in one CString
	CString sCopyData;
	for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
	{
		sCopyData += this->m_arDiffLines->GetAt(i);
		sCopyData += _T("\r\n");
	}
	CStringA sCopyDataASCII = CStringA(sCopyData);
	if (!sCopyDataASCII.IsEmpty())
	{
		if (OpenClipboard())
		{
			EmptyClipboard();
			HGLOBAL hClipboardData;
			hClipboardData = GlobalAlloc(GMEM_DDESHARE, sCopyDataASCII.GetLength()+1);
			char * pchData;
			pchData = (char*)GlobalLock(hClipboardData);
			strcpy_s(pchData, sCopyDataASCII.GetLength()+1, (LPCSTR)sCopyDataASCII);
			GlobalUnlock(hClipboardData);
			SetClipboardData(CF_TEXT,hClipboardData);
			CloseClipboard();
		}
	}
}

void CBaseView::OnMouseMove(UINT nFlags, CPoint point)
{
	int nMouseLine = (((point.y - HEADERHEIGHT) / GetLineHeight()) + m_nTopLine);
	nMouseLine--;		//we need the index
	ShowDiffLines(nMouseLine);
	CView::OnMouseMove(nFlags, point);
}

void CBaseView::SelectLines(int nLine1, int nLine2)
{
	if (nLine2 == -1)
		nLine2 = nLine1;
	m_nSelBlockStart = nLine1;
	m_nSelBlockEnd = nLine2;
	Invalidate();
}

void CBaseView::ShowDiffLines(int nLine)
{
	if ((nLine >= m_nTopLine)&&(nLine < GetLineCount()))
	{
		if ((m_pwndRight)&&(m_pwndRight->m_arLineStates)&&(m_pwndLeft)&&(m_pwndLeft->m_arLineStates)&&(!m_pMainFrame->m_bOneWay))
		{
			nLine = (nLine > m_pwndRight->m_arLineStates->GetCount() ? -1 : nLine);
			nLine = (nLine > m_pwndLeft->m_arLineStates->GetCount() ? -1 : nLine);

			if (nLine >= 0)
			{
				if (nLine != m_nMouseLine)
				{
					m_nMouseLine = nLine;
					if (nLine >= GetLineCount())
						nLine = -1;
					m_pwndLineDiffBar->ShowLines(nLine);
				}
			}
		}
	}
}

void CBaseView::UseTheirAndYourBlock(viewstate &rightstate, viewstate &bottomstate, viewstate &leftstate)
{
	for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
	{
		bottomstate.difflines[i] = m_pwndBottom->m_arDiffLines->GetAt(i);
		m_pwndBottom->m_arDiffLines->SetAt(i, m_pwndLeft->m_arDiffLines->GetAt(i));
		bottomstate.linestates[i] = m_pwndBottom->m_arLineStates->GetAt(i);
		m_pwndBottom->m_arLineStates->SetAt(i, m_pwndLeft->m_arLineStates->GetAt(i));
	}

	// your block is done, now insert their block
	int index = m_nSelBlockEnd+1;
	for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
	{
		bottomstate.addedlines.push_back(m_nSelBlockEnd+1);
		m_pwndBottom->m_arDiffLines->InsertAt(index, m_pwndRight->m_arDiffLines->GetAt(i));
		m_pwndBottom->m_arLineLines->InsertAt(index, m_pwndLeft->m_arLineLines->GetAt(i));
		m_pwndBottom->m_arLineStates->InsertAt(index++, m_pwndRight->m_arLineStates->GetAt(i));
	}
	// adjust line numbers
	for (int i=m_nSelBlockEnd+1; i<GetLineCount(); ++i)
	{
		long oldline = (long)m_pwndBottom->m_arLineLines->GetAt(i);
		if (oldline >= 0)
			m_pwndBottom->m_arLineLines->SetAt(i, oldline+(index-m_nSelBlockEnd));
	}

	// now insert an empty block in both yours and theirs
	for (int emptyblocks=0; emptyblocks < m_nSelBlockEnd-m_nSelBlockStart+1; ++emptyblocks)
		leftstate.addedlines.push_back(m_nSelBlockStart);
	m_pwndLeft->m_arDiffLines->InsertAt(m_nSelBlockStart, _T(""), m_nSelBlockEnd-m_nSelBlockStart+1);
	m_pwndLeft->m_arLineStates->InsertAt(m_nSelBlockStart, CDiffData::DIFFSTATE_EMPTY, m_nSelBlockEnd-m_nSelBlockStart+1);
	m_pwndLeft->m_arLineLines->InsertAt(m_nSelBlockStart, (DWORD)-1, m_nSelBlockEnd-m_nSelBlockStart+1);
	for (int emptyblocks=0; emptyblocks < m_nSelBlockEnd-m_nSelBlockStart+1; ++emptyblocks)
		rightstate.addedlines.push_back(m_nSelBlockEnd+1);
	m_pwndRight->m_arDiffLines->InsertAt(m_nSelBlockEnd+1, _T(""), m_nSelBlockEnd-m_nSelBlockStart+1);
	m_pwndRight->m_arLineStates->InsertAt(m_nSelBlockEnd+1, CDiffData::DIFFSTATE_EMPTY, m_nSelBlockEnd-m_nSelBlockStart+1);
	m_pwndRight->m_arLineLines->InsertAt(m_nSelBlockEnd+1, (DWORD)-1, m_nSelBlockEnd-m_nSelBlockStart+1);
	RecalcAllVertScrollBars();
	m_pwndBottom->SetModified();
	m_pwndLeft->SetModified();
	m_pwndRight->SetModified();
}

void CBaseView::UseYourAndTheirBlock(viewstate &rightstate, viewstate &bottomstate, viewstate &leftstate)
{
	for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
	{
		bottomstate.difflines[i] = m_pwndBottom->m_arDiffLines->GetAt(i);
		m_pwndBottom->m_arDiffLines->SetAt(i, m_pwndRight->m_arDiffLines->GetAt(i));
		bottomstate.linestates[i] = m_pwndBottom->m_arLineStates->GetAt(i);
		m_pwndBottom->m_arLineStates->SetAt(i, m_pwndRight->m_arLineStates->GetAt(i));
		rightstate.linestates[i] = m_pwndRight->m_arLineStates->GetAt(i);
		m_pwndRight->m_arLineStates->SetAt(i, CDiffData::DIFFSTATE_YOURSADDED);
	}

	// your block is done, now insert their block
	int index = m_nSelBlockEnd+1;
	for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
	{
		bottomstate.addedlines.push_back(m_nSelBlockEnd+1);
		m_pwndBottom->m_arDiffLines->InsertAt(index, m_pwndLeft->m_arDiffLines->GetAt(i));
		m_pwndBottom->m_arLineLines->InsertAt(index, m_pwndLeft->m_arLineLines->GetAt(i));
		m_pwndBottom->m_arLineStates->InsertAt(index++, m_pwndLeft->m_arLineStates->GetAt(i));
		leftstate.linestates[i] = m_pwndLeft->m_arLineStates->GetAt(i);
		m_pwndLeft->m_arLineStates->SetAt(i, CDiffData::DIFFSTATE_THEIRSADDED);
	}
	// adjust line numbers
	for (int i=m_nSelBlockEnd+1; i<GetLineCount(); ++i)
	{
		long oldline = (long)m_pwndBottom->m_arLineLines->GetAt(i);
		if (oldline >= 0)
			m_pwndBottom->m_arLineLines->SetAt(i, oldline+(index-m_nSelBlockEnd));
	}

	// now insert an empty block in both yours and theirs
	for (int emptyblocks=0; emptyblocks < m_nSelBlockEnd-m_nSelBlockStart+1; ++emptyblocks)
		leftstate.addedlines.push_back(m_nSelBlockStart);
	m_pwndLeft->m_arDiffLines->InsertAt(m_nSelBlockStart, _T(""), m_nSelBlockEnd-m_nSelBlockStart+1);
	m_pwndLeft->m_arLineStates->InsertAt(m_nSelBlockStart, CDiffData::DIFFSTATE_EMPTY, m_nSelBlockEnd-m_nSelBlockStart+1);
	m_pwndLeft->m_arLineLines->InsertAt(m_nSelBlockStart, (DWORD)-1, m_nSelBlockEnd-m_nSelBlockStart+1);
	for (int emptyblocks=0; emptyblocks < m_nSelBlockEnd-m_nSelBlockStart+1; ++emptyblocks)
		rightstate.addedlines.push_back(m_nSelBlockEnd+1);
	m_pwndRight->m_arDiffLines->InsertAt(m_nSelBlockEnd+1, _T(""), m_nSelBlockEnd-m_nSelBlockStart+1);
	m_pwndRight->m_arLineStates->InsertAt(m_nSelBlockEnd+1, CDiffData::DIFFSTATE_EMPTY, m_nSelBlockEnd-m_nSelBlockStart+1);
	m_pwndRight->m_arLineLines->InsertAt(m_nSelBlockEnd+1, (DWORD)-1, m_nSelBlockEnd-m_nSelBlockStart+1);
	RecalcAllVertScrollBars();
	m_pwndBottom->SetModified();
	m_pwndLeft->SetModified();
	m_pwndRight->SetModified();
}

void CBaseView::UseBothRightFirst(viewstate &rightstate, viewstate &leftstate)
{
	for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
	{
		rightstate.linestates[i] = m_pwndRight->m_arLineStates->GetAt(i);
		m_pwndRight->m_arLineStates->SetAt(i, CDiffData::DIFFSTATE_YOURSADDED);
	}

	// your block is done, now insert their block
	int index = m_nSelBlockEnd+1;
	for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
	{
		rightstate.addedlines.push_back(m_nSelBlockEnd+1);
		m_pwndRight->m_arDiffLines->InsertAt(index, m_pwndLeft->m_arDiffLines->GetAt(i));
		m_pwndRight->m_arLineLines->InsertAt(index, m_pwndRight->m_arLineLines->GetAt(i));
		m_pwndRight->m_arLineStates->InsertAt(index++, CDiffData::DIFFSTATE_THEIRSADDED);
	}
	// adjust line numbers
	index--;
	for (int i=m_nSelBlockEnd+1; i<GetLineCount(); ++i)
	{
		long oldline = (long)m_pwndRight->m_arLineLines->GetAt(i);
		if (oldline >= 0)
			m_pwndRight->m_arLineLines->SetAt(i, oldline+(index-m_nSelBlockEnd));
	}

	// now insert an empty block in the left view
	for (int emptyblocks=0; emptyblocks < m_nSelBlockEnd-m_nSelBlockStart+1; ++emptyblocks)
		leftstate.addedlines.push_back(m_nSelBlockEnd+1);
	m_pwndLeft->m_arDiffLines->InsertAt(m_nSelBlockEnd+1, _T(""), m_nSelBlockEnd-m_nSelBlockStart+1);
	m_pwndLeft->m_arLineStates->InsertAt(m_nSelBlockEnd+1, CDiffData::DIFFSTATE_EMPTY, m_nSelBlockEnd-m_nSelBlockStart+1);
	m_pwndLeft->m_arLineLines->InsertAt(m_nSelBlockEnd+1, (DWORD)-1, m_nSelBlockEnd-m_nSelBlockStart+1);
	RecalcAllVertScrollBars();
	m_pwndLeft->SetModified();
	m_pwndRight->SetModified();
}

void CBaseView::UseBothLeftFirst(viewstate &rightstate, viewstate &leftstate)
{
	// get line number from just before the block
	long linenumber = 0;
	if (m_nSelBlockStart > 0)
		linenumber = m_pwndRight->m_arLineLines->GetAt(m_nSelBlockStart-1);
	linenumber++;
	for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
	{
		rightstate.addedlines.push_back(m_nSelBlockStart);
		m_pwndRight->m_arDiffLines->InsertAt(i, m_pwndLeft->m_arDiffLines->GetAt(i));
		m_pwndRight->m_arLineStates->InsertAt(i, CDiffData::DIFFSTATE_THEIRSADDED);
		m_pwndRight->m_arLineLines->InsertAt(i, linenumber++);
	}
	// adjust line numbers
	for (int i=m_nSelBlockEnd+1; i<GetLineCount(); ++i)
	{
		long oldline = (long)m_pwndRight->m_arLineLines->GetAt(i);
		if (oldline >= 0)
			m_pwndRight->m_arLineLines->SetAt(i, oldline+(m_nSelBlockEnd-m_nSelBlockStart)+1);
	}

	// now insert an empty block in both yours and theirs
	for (int emptyblocks=0; emptyblocks < m_nSelBlockEnd-m_nSelBlockStart+1; ++emptyblocks)
		leftstate.addedlines.push_back(m_nSelBlockStart);
	m_pwndLeft->m_arDiffLines->InsertAt(m_nSelBlockStart, _T(""), m_nSelBlockEnd-m_nSelBlockStart+1);
	m_pwndLeft->m_arLineStates->InsertAt(m_nSelBlockStart, CDiffData::DIFFSTATE_EMPTY, m_nSelBlockEnd-m_nSelBlockStart+1);
	m_pwndLeft->m_arLineLines->InsertAt(m_nSelBlockStart, (DWORD)-1, m_nSelBlockEnd-m_nSelBlockStart+1);
	RecalcAllVertScrollBars();
	m_pwndLeft->SetModified();
	m_pwndRight->SetModified();
}




