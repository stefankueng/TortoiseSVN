// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Stefan Kueng

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
#include "scintilla.h"
#include "myspell\\myspell.hxx"
#include "myspell\\mythes.hxx"

/**
 * \ingroup Utils
 * Helper class which extends the MFC CStringArray class. The only method added
 * to that class is AddSorted() which adds a new element in a sorted order.
 * That way the array is kept sorted.
 *
 * \par requirements
 * win95 or later
 * winNT4 or later
 * MFC
 *
 * \version 1.0
 * first version
 *
 * \date JAN-2005
 *
 * \author Stefan Kueng
 *
 */
class CAutoCompletionList : public CStringArray
{
public:
	void		AddSorted(const CString& elem, bool bNoDuplicates = true);
};

/**
 * \ingroup Utils
 * Encapsulates the Scintilla edit control. Useable as a replacement for the
 * MFC CEdit control, but not a drop-in replacement!
 * Also provides additional features like spell checking, autocompletion, ...
 *
 * \par requirements
 * win95 or later
 * winNT4 or later
 * MFC
 * Scintilla edit control (http://www.scintilla.org)
 *
 * \version 1.0
 * first version
 *
 * \date JAN-2005
 *
 * \author Stefan Kueng
 *
 */
class CSciEdit : public CWnd
{
	DECLARE_DYNAMIC(CSciEdit)
public:
	CSciEdit(void);
	~CSciEdit(void);
	/**
	 * Initialize the scintilla control. Must be called prior to any other
	 * method!
	 */
	void		Init();
	/**
	 * Execute a scintilla command, e.g. SCI_GETLINE.
	 */
	LRESULT		Call(UINT message, WPARAM wParam = 0, LPARAM lParam = 0);
	/**
	 * The specified text is written to the scintilla control.
	 */
	void		SetText(const CString& sText);
	/**
	 * Retreives the text in the scintilla control.
	 */
	CString		GetText(void);
	/**
	 * Sets the font for the control.
	 */
	void		SetFont(CString sFontName, int iFontSizeInPoints);
	/**
	 * Adds a list of words for use in autocompletion.
	 */
	void		SetAutoCompletionList(const CAutoCompletionList& list, const TCHAR separator = ';');
	/**
	 * Returns the word located under the cursor.
	 */
	CString		GetWordUnderCursor(bool bSelectWord = false);
	
	CStringA	StringForControl(const CString& text);
	CString		StringFromControl(const CStringA& text);
private:
	HMODULE		m_hModule;
	LRESULT		m_DirectFunction;
	LRESULT		m_DirectPointer;
	MySpell *	pChecker;
	MyThes *	pThesaur;
	CAutoCompletionList m_autolist;
	TCHAR		m_separator;
protected:
	virtual BOOL OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult);
	void		CheckSpelling(void);
	void		SuggestSpellingAlternatives(void);
	void		DoAutoCompletion(void);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	DECLARE_MESSAGE_MAP()
};
