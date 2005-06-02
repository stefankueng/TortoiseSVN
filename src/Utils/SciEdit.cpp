// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2005 - Stefan Kueng

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
#include "StdAfx.h"
#include "resource.h"
#include "Utils.h"
#include <string>
#include "regexpr2.h"
#include "registry.h"
#include ".\sciedit.h"

using namespace std;
using namespace regex;


void CSciEditContextMenuInterface::InsertMenuItems(CMenu&, int&) {return;}
bool CSciEditContextMenuInterface::HandleMenuItemClick(int, CSciEdit *) {return false;}



#define STYLE_BOLD			11
#define STYLE_BOLDITALIC	12

IMPLEMENT_DYNAMIC(CSciEdit, CWnd)

CSciEdit::CSciEdit(void)
{
	m_DirectFunction = 0;
	m_DirectPointer = 0;
	pChecker = 0;
	pThesaur = 0;
	m_hModule = ::LoadLibrary(_T("SciLexer.DLL"));
}

CSciEdit::~CSciEdit(void)
{
	if (m_hModule)
		::FreeLibrary(m_hModule);
	if (pChecker)
		delete pChecker;
	if (pThesaur)
		delete pThesaur;
}

void CSciEdit::Init(LONG lLanguage)
{
	//Setup the direct access data
	m_DirectFunction = SendMessage(SCI_GETDIRECTFUNCTION, 0, 0);
	m_DirectPointer = SendMessage(SCI_GETDIRECTPOINTER, 0, 0);
	Call(SCI_SETMARGINWIDTHN, 1, 0);
	Call(SCI_SETUSETABS, 0);		//pressing TAB inserts spaces
	Call(SCI_SETWRAPVISUALFLAGS, SC_WRAPVISUALFLAG_END);
	Call(SCI_AUTOCSETIGNORECASE, 1);
	Call(SCI_SETLEXER, SCLEX_CONTAINER);
	Call(SCI_SETCODEPAGE, SC_CP_UTF8);
	Call(SCI_AUTOCSETFILLUPS, 0, (LPARAM)"([");
	Call(SCI_AUTOCSETMAXWIDTH, 0);
	//Set the default windows colors for edit controls
	Call(SCI_STYLESETFORE, STYLE_DEFAULT, ::GetSysColor(COLOR_WINDOWTEXT));
	Call(SCI_STYLESETBACK, STYLE_DEFAULT, ::GetSysColor(COLOR_WINDOW));
	Call(SCI_SETSELFORE, TRUE, ::GetSysColor(COLOR_HIGHLIGHTTEXT));
	Call(SCI_SETSELBACK, TRUE, ::GetSysColor(COLOR_HIGHLIGHT));
	Call(SCI_SETCARETFORE, ::GetSysColor(COLOR_WINDOWTEXT));
	
	// look for dictionary files and use them if found
	long langId = GetUserDefaultLCID();

	if ((lLanguage != 0)||(((DWORD)CRegStdWORD(_T("Software\\TortoiseSVN\\NoSpellchecker"), FALSE))==FALSE))
	{
		if (!((lLanguage)&&(!LoadDictionaries(lLanguage))))
		{
			do
			{
				LoadDictionaries(langId);
				DWORD lid = SUBLANGID(langId);
				lid--;
				if (lid > 0)
				{
					langId = MAKELANGID(PRIMARYLANGID(langId), lid);
				}
				else if (langId == 1033)
					langId = 0;
				else
					langId = 1033;
			} while ((langId)&&((pChecker==NULL)||(pThesaur==NULL)));
		}
	}
	Call(SCI_SETEDGEMODE, EDGE_NONE);
	Call(SCI_SETWRAPMODE, SC_WRAP_WORD);
}

void CSciEdit::Init(const ProjectProperties& props)
{
	Init(props.lProjectLanguage);
	m_sCommand = props.sCheckRe;
	m_sBugID = props.sBugIDRe;
	try
	{
		if (!m_sBugID.IsEmpty())
			m_patBugID.init((LPCTSTR)m_sBugID);
		if (!m_sCommand.IsEmpty())
			m_patCommand.init((LPCTSTR)m_sCommand);
	}
	catch (bad_alloc){m_sBugID.Empty();m_sCommand.Empty();}
	catch (bad_regexpr){m_sBugID.Empty();m_sCommand.Empty();}
	
	if (props.nLogWidthMarker)
	{
		Call(SCI_SETWRAPMODE, SC_WRAP_NONE);
		Call(SCI_SETEDGEMODE, EDGE_LINE);
		Call(SCI_SETEDGECOLUMN, props.nLogWidthMarker);
	}
	else
	{
		Call(SCI_SETEDGEMODE, EDGE_NONE);
		Call(SCI_SETWRAPMODE, SC_WRAP_WORD);
	}
	SetText(props.sLogTemplate);
}

BOOL CSciEdit::LoadDictionaries(LONG lLanguageID)
{
	//Setup the spell checker and thesaurus
	TCHAR buf[6];
	CString sFolder = CUtils::GetAppDirectory();
	CString sFolderUp = CUtils::GetAppParentDirectory();
	CString sFile;

	GetLocaleInfo(MAKELCID(lLanguageID, SORT_DEFAULT), LOCALE_SISO639LANGNAME, buf, sizeof(buf));
	sFile = buf;
	sFile += _T("_");
	GetLocaleInfo(MAKELCID(lLanguageID, SORT_DEFAULT), LOCALE_SISO3166CTRYNAME, buf, sizeof(buf));
	sFile += buf;
	if (pChecker==NULL)
	{
		if ((PathFileExists(sFolder + sFile + _T(".aff"))) &&
			(PathFileExists(sFolder + sFile + _T(".dic"))))
		{
			pChecker = new MySpell(CStringA(sFolder + sFile + _T(".aff")), CStringA(sFolder + sFile + _T(".dic")));
		}
		else if ((PathFileExists(sFolder + _T("dic\\") + sFile + _T(".aff"))) &&
			(PathFileExists(sFolder + _T("dic\\") + sFile + _T(".dic"))))
		{
			pChecker = new MySpell(CStringA(sFolder + _T("dic\\") + sFile + _T(".aff")), CStringA(sFolder + _T("dic\\") + sFile + _T(".dic")));
		}
		else if ((PathFileExists(sFolderUp + sFile + _T(".aff"))) &&
			(PathFileExists(sFolderUp + sFile + _T(".dic"))))
		{
			pChecker = new MySpell(CStringA(sFolderUp + sFile + _T(".aff")), CStringA(sFolderUp + sFile + _T(".dic")));
		}
		else if ((PathFileExists(sFolderUp + _T("dic\\") + sFile + _T(".aff"))) &&
			(PathFileExists(sFolderUp + _T("dic\\") + sFile + _T(".dic"))))
		{
			pChecker = new MySpell(CStringA(sFolderUp + _T("dic\\") + sFile + _T(".aff")), CStringA(sFolderUp + _T("dic\\") + sFile + _T(".dic")));
		}
		else if ((PathFileExists(sFolderUp + _T("Languages\\") + sFile + _T(".aff"))) &&
			(PathFileExists(sFolderUp + _T("Languages\\") + sFile + _T(".dic"))))
		{
			pChecker = new MySpell(CStringA(sFolderUp + _T("Languages\\") + sFile + _T(".aff")), CStringA(sFolderUp + _T("Languages\\") + sFile + _T(".dic")));
		}
	}
#if THESAURUS
	if (pThesaur==NULL)
	{
		if ((PathFileExists(sFolder + _T("th_") + sFile + _T("_v2.idx"))) &&
			(PathFileExists(sFolder + _T("th_") + sFile + _T("_v2.dat"))))
		{
			pThesaur = new MyThes(CStringA(sFolder + sFile + _T("_v2.idx")), CStringA(sFolder + sFile + _T("_v2.dat")));
		}
		else if ((PathFileExists(sFolder + _T("dic\\th_") + sFile + _T("_v2.idx"))) &&
			(PathFileExists(sFolder + _T("dic\\th_") + sFile + _T("_v2.dat"))))
		{
			pThesaur = new MyThes(CStringA(sFolder + _T("dic\\") + sFile + _T("_v2.idx")), CStringA(sFolder + _T("dic\\") + sFile + _T("_v2.dat")));
		}
		else if ((PathFileExists(sFolderUp + _T("th_") + sFile + _T("_v2.idx"))) &&
			(PathFileExists(sFolderUp + _T("th_") + sFile + _T("_v2.dat"))))
		{
			pThesaur = new MyThes(CStringA(sFolderUp + _T("th_") + sFile + _T("_v2.idx")), CStringA(sFolderUp + _T("th_") + sFile + _T("_v2.dat")));
		}
		else if ((PathFileExists(sFolderUp + _T("dic\\th_") + sFile + _T("_v2.idx"))) &&
			(PathFileExists(sFolderUp + _T("dic\\th_") + sFile + _T("_v2.dat"))))
		{
			pThesaur = new MyThes(CStringA(sFolderUp + _T("dic\\th_") + sFile + _T("_v2.idx")), CStringA(sFolderUp + _T("dic\\th_") + sFile + _T("_v2.dat")));
		}
		else if ((PathFileExists(sFolderUp + _T("Languages\\th_") + sFile + _T("_v2.idx"))) &&
			(PathFileExists(sFolderUp + _T("Languages\\th_") + sFile + _T("_v2.dat"))))
		{
			pThesaur = new MyThes(CStringA(sFolderUp + _T("Languages\\th_") + sFile + _T("_v2.idx")), CStringA(sFolderUp + _T("Languages\\th_") + sFile + _T("_v2.dat")));
		}
	}
#endif
	if ((pThesaur)||(pChecker))
		return TRUE;
	return FALSE;
}

LRESULT CSciEdit::Call(UINT message, WPARAM wParam, LPARAM lParam)
{
	ASSERT(::IsWindow(m_hWnd)); //Window must be valid
	ASSERT(m_DirectFunction); //Direct function must be valid
	return ((SciFnDirect) m_DirectFunction)(m_DirectPointer, message, wParam, lParam);
}

CString CSciEdit::StringFromControl(const CStringA& text)
{
	CString sText;
#ifdef UNICODE
	int codepage = Call(SCI_GETCODEPAGE);
	int reslen = MultiByteToWideChar(codepage, 0, text, text.GetLength(), 0, 0);	
	MultiByteToWideChar(codepage, 0, text, text.GetLength(), sText.GetBuffer(reslen+1), reslen+1);
	sText.ReleaseBuffer(reslen);
#else
	sText = text;	
#endif
	return sText;
}

CStringA CSciEdit::StringForControl(const CString& text)
{
	CStringA sTextA;
#ifdef UNICODE
	int codepage = Call(SCI_GETCODEPAGE);
	int reslen = WideCharToMultiByte(codepage, 0, text, text.GetLength(), 0, 0, 0, 0);
	WideCharToMultiByte(codepage, 0, text, text.GetLength(), sTextA.GetBuffer(reslen), reslen, 0, 0);
	sTextA.ReleaseBuffer(reslen);
#else
	sTextA = text;
#endif
	ATLTRACE("string length %d\n", sTextA.GetLength());
	return sTextA;
}

void CSciEdit::SetText(const CString& sText)
{
	CStringA sTextA = StringForControl(sText);
	Call(SCI_SETTEXT, 0, (LPARAM)(LPCSTR)sTextA);
	
	// Scintilla seems to have problems with strings that
	// aren't terminated by a newline char. Once that char
	// is there, it can be removed without problems.
	// So we add here a newline, then remove it again.
	Call(SCI_DOCUMENTEND);
	Call(SCI_NEWLINE);
	Call(SCI_DELETEBACK);
}

void CSciEdit::InsertText(const CString& sText, bool bNewLine)
{
	CStringA sTextA = StringForControl(sText);
	Call(SCI_REPLACESEL, 0, (LPARAM)(LPCSTR)sTextA);
	if (bNewLine)
		Call(SCI_REPLACESEL, 0, (LPARAM)(LPCSTR)"\n");
}

CString CSciEdit::GetText()
{
	LRESULT len = Call(SCI_GETTEXT, 0, 0);
	CStringA sTextA;
	Call(SCI_GETTEXT, len+1, (LPARAM)(LPCSTR)sTextA.GetBuffer(len+1));
	sTextA.ReleaseBuffer();
	return StringFromControl(sTextA);
}

CString CSciEdit::GetWordUnderCursor(bool bSelectWord)
{
	TEXTRANGEA textrange;
	int pos = Call(SCI_GETCURRENTPOS);
	textrange.chrg.cpMin = Call(SCI_WORDSTARTPOSITION, pos, TRUE);
	if ((pos == textrange.chrg.cpMin)||(textrange.chrg.cpMin < 0))
		return CString();
	textrange.chrg.cpMax = Call(SCI_WORDENDPOSITION, textrange.chrg.cpMin, TRUE);
	
	char * textbuffer = new char[textrange.chrg.cpMax - textrange.chrg.cpMin + 1];

	textrange.lpstrText = textbuffer;	
	Call(SCI_GETTEXTRANGE, 0, (LPARAM)&textrange);
	if (bSelectWord)
	{
		Call(SCI_SETSEL, textrange.chrg.cpMin, textrange.chrg.cpMax);
	}
	CString sRet = StringFromControl(textbuffer);
	delete textbuffer;
	return sRet;
}

void CSciEdit::SetFont(CString sFontName, int iFontSizeInPoints)
{
	Call(SCI_STYLESETFONT, STYLE_DEFAULT, (LPARAM)(LPCSTR)StringForControl(sFontName));
	Call(SCI_STYLESETSIZE, STYLE_DEFAULT, iFontSizeInPoints);
	Call(SCI_STYLECLEARALL);
	// set the styles for the bug ID strings
	Call(SCI_STYLESETBOLD, STYLE_BOLD, TRUE);
	Call(SCI_STYLESETFORE, STYLE_BOLD, (LPARAM)GetSysColor(COLOR_HIGHLIGHT));
	Call(SCI_STYLESETBOLD, STYLE_BOLDITALIC, (LPARAM)TRUE);
	Call(SCI_STYLESETITALIC, STYLE_BOLDITALIC, (LPARAM)TRUE);
	Call(SCI_STYLESETFORE, STYLE_BOLDITALIC, (LPARAM)GetSysColor(COLOR_HIGHLIGHT));
}

void CSciEdit::SetAutoCompletionList(const CAutoCompletionList& list, const TCHAR separator)
{
	//copy the autocompletion list.
	
	//SK: instead of creating a copy of that list, we could accept a pointer
	//to the list and use that instead. But then the caller would have to make
	//sure that the list persists over the lifetime of the control!
	m_autolist.RemoveAll();
	for (INT_PTR i=0; i<list.GetCount(); ++i)
		m_autolist.Add(list[i]);
	m_separator = separator;
}

void CSciEdit::CheckSpelling()
{
	if (pChecker == NULL)
		return;
	
	TEXTRANGEA textrange;
	
	LRESULT firstline = Call(SCI_GETFIRSTVISIBLELINE);
	LRESULT lastline = firstline + Call(SCI_LINESONSCREEN);
	textrange.chrg.cpMin = Call(SCI_POSITIONFROMLINE, firstline);
	textrange.chrg.cpMax = textrange.chrg.cpMin;
	LRESULT lastpos = Call(SCI_POSITIONFROMLINE, lastline) + Call(SCI_LINELENGTH, lastline);
	if (lastpos < 0)
		lastpos = Call(SCI_GETLENGTH)-textrange.chrg.cpMin;
	while (textrange.chrg.cpMax < lastpos)
	{
		textrange.chrg.cpMin = Call(SCI_WORDSTARTPOSITION, textrange.chrg.cpMax+1, TRUE);
		if (textrange.chrg.cpMin < textrange.chrg.cpMax)
			break;
		textrange.chrg.cpMax = Call(SCI_WORDENDPOSITION, textrange.chrg.cpMin, TRUE);
		if (textrange.chrg.cpMin == textrange.chrg.cpMax)
		{
			textrange.chrg.cpMax++;
			continue;
		}
		char * textbuffer = new char[textrange.chrg.cpMax - textrange.chrg.cpMin + 1];
		textrange.lpstrText = textbuffer;
		Call(SCI_GETTEXTRANGE, 0, (LPARAM)&textrange);
		if (strlen(textrange.lpstrText) > 3)
		{
			if (!pChecker->spell(CStringA(StringFromControl(textrange.lpstrText))))
			{
				//mark word as misspelled
				Call(SCI_STARTSTYLING, textrange.chrg.cpMin, INDICS_MASK);
				Call(SCI_SETSTYLING, textrange.chrg.cpMax - textrange.chrg.cpMin, INDIC1_MASK);	
			}
			else
			{
				//mark word as correct (remove the squiggle line)
				Call(SCI_STARTSTYLING, textrange.chrg.cpMin, INDICS_MASK);
				Call(SCI_SETSTYLING, textrange.chrg.cpMax - textrange.chrg.cpMin, 0);			
			}
		}
		else
		{
			textrange.chrg.cpMin = textrange.chrg.cpMax;
		}
		delete textbuffer;
	}
}

void CSciEdit::SuggestSpellingAlternatives()
{
	if (pChecker == NULL)
		return;
	CString word = GetWordUnderCursor(true);
	Call(SCI_SETCURRENTPOS, Call(SCI_WORDSTARTPOSITION, Call(SCI_GETCURRENTPOS), TRUE));
	if (word.IsEmpty())
		return;
	char ** wlst;
	int ns = pChecker->suggest(&wlst, CStringA(word));
	if (ns > 0)
	{
		CString suggestions;
		for (int i=0; i < ns; i++) 
		{
			suggestions += CString(wlst[i]) + m_separator;
			free(wlst[i]);
		} 
		free(wlst);
		suggestions.TrimRight(m_separator);
		if (suggestions.IsEmpty())
			return;
		Call(SCI_AUTOCSETSEPARATOR, (WPARAM)CStringA(m_separator).GetAt(0));
		Call(SCI_AUTOCSETDROPRESTOFWORD, 1);
		Call(SCI_AUTOCSHOW, 0, (LPARAM)(LPCSTR)StringForControl(suggestions));
	}

}

void CSciEdit::DoAutoCompletion()
{
	if (m_autolist.GetCount()==0)
		return;
	if (Call(SCI_AUTOCACTIVE))
		return;
	CString word = GetWordUnderCursor();
	if (word.GetLength() < 3)
		return;		//don't autocomplete yet, word is too short
	int pos = Call(SCI_GETCURRENTPOS);
	if (pos != Call(SCI_WORDENDPOSITION, pos, TRUE))
		return;	//don't autocomplete if we're not at the end of a word
	CString sAutoCompleteList;
	
	for (INT_PTR index = 0; index < m_autolist.GetCount(); ++index)
	{
		int compare = word.CompareNoCase(m_autolist[index].Left(word.GetLength()));
		if (compare>0)
			continue;
		else if (compare == 0)
		{
			sAutoCompleteList += m_autolist[index] + m_separator;
		}
		else
		{
			break;
		}
	}
	sAutoCompleteList.TrimRight(m_separator);
	if (sAutoCompleteList.IsEmpty())
		return;

	Call(SCI_AUTOCSETSEPARATOR, (WPARAM)CStringA(m_separator).GetAt(0));
	Call(SCI_AUTOCSHOW, word.GetLength(), (LPARAM)(LPCSTR)StringForControl(sAutoCompleteList));
}

BOOL CSciEdit::OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult)
{
	if (message != WM_NOTIFY)
		return CWnd::OnChildNotify(message, wParam, lParam, pLResult);
	
	LPNMHDR lpnmhdr = (LPNMHDR) lParam;
	SCNotification * lpSCN = (SCNotification *)lParam;
	
	if(lpnmhdr->hwndFrom==m_hWnd)
	{
		switch(lpnmhdr->code)
		{
		case SCN_CHARADDED:
			if ((lpSCN->ch < 32)&&(lpSCN->ch != 13)&&(lpSCN->ch != 10))
				Call(SCI_DELETEBACK);
			else
			{
				CheckSpelling();
				DoAutoCompletion();
			}
			return TRUE;
			break;
		case SCN_STYLENEEDED:
			MarkEnteredBugID(lpnmhdr);
			return TRUE;
			break;
		}
	}
	return CWnd::OnChildNotify(message, wParam, lParam, pLResult);
}

BEGIN_MESSAGE_MAP(CSciEdit, CWnd)
	ON_WM_KEYDOWN()
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

void CSciEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	switch (nChar)
	{
	case (VK_TAB):
		{
			if (GetKeyState(VK_CONTROL)&0x8000)
			{
				//Ctrl-Tab was pressed, this means we should provide the user with
				//a list of possible spell checking alternatives to the word under
				//the cursor
				SuggestSpellingAlternatives();
				return;
			}
			else
			{
				::PostMessage(GetParent()->GetSafeHwnd(), WM_NEXTDLGCTL, GetKeyState(VK_SHIFT)&0x8000, 0);
				return;
			}
		}
		break;
	case (VK_ESCAPE):
		{
			if ((Call(SCI_AUTOCACTIVE)==0)&&(Call(SCI_CALLTIPACTIVE)==0))
				::SendMessage(GetParent()->GetSafeHwnd(), WM_CLOSE, 0, 0);
		}
		break;
	case (VK_SPACE):
		{
			if ((GetKeyState(VK_CONTROL)&0x8000)&&(GetKeyState(VK_SHIFT)&0x8000))
			{
				DoAutoCompletion();
				return;
			}
		}
		break;
	}
	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CSciEdit::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	int anchor = Call(SCI_GETANCHOR);
	int currentpos = Call(SCI_GETCURRENTPOS);
	if ((point.x == -1) && (point.y == -1))
	{
		CRect rect;
		GetClientRect(&rect);
		ClientToScreen(&rect);
		point = rect.CenterPoint();
	}
	else
	{
		// change the cursor position to the point where the user
		// right-clicked.
		CPoint clientpoint = point;
		ScreenToClient(&clientpoint);
		int pointpos = Call(SCI_POSITIONFROMPOINT, clientpoint.x, clientpoint.y);
		Call(SCI_SETANCHOR, pointpos);
		Call(SCI_SETCURRENTPOS, pointpos);
	}
	CString sMenuItemText;
	CMenu popup;
	if (popup.CreatePopupMenu())
	{
		bool bCanUndo = !!Call(SCI_CANUNDO);
		bool bCanRedo = !!Call(SCI_CANREDO);
		bool bHasSelection = (Call(SCI_GETANCHOR) != Call(SCI_GETCURRENTPOS));
		bool bCanPaste = !!Call(SCI_CANPASTE);
		UINT uEnabledMenu = MF_STRING | MF_ENABLED;
		UINT uDisabledMenu = MF_STRING | MF_GRAYED;
		
		sMenuItemText.LoadString(IDS_SCIEDIT_UNDO);
		popup.AppendMenu(bCanUndo ? uEnabledMenu : uDisabledMenu, SCI_UNDO, sMenuItemText);
		sMenuItemText.LoadString(IDS_SCIEDIT_REDO);
		popup.AppendMenu(bCanRedo ? uEnabledMenu : uDisabledMenu, SCI_REDO, sMenuItemText);
		
		popup.AppendMenu(MF_SEPARATOR);
		
		sMenuItemText.LoadString(IDS_SCIEDIT_CUT);
		popup.AppendMenu(bHasSelection ? uEnabledMenu : uDisabledMenu, SCI_CUT, sMenuItemText);
		sMenuItemText.LoadString(IDS_SCIEDIT_COPY);
		popup.AppendMenu(bHasSelection ? uEnabledMenu : uDisabledMenu, SCI_COPY, sMenuItemText);
		sMenuItemText.LoadString(IDS_SCIEDIT_PASTE);
		popup.AppendMenu(bCanPaste ? uEnabledMenu : uDisabledMenu, SCI_PASTE, sMenuItemText);

		popup.AppendMenu(MF_SEPARATOR);
		
		sMenuItemText.LoadString(IDS_SCIEDIT_SELECTALL);
		popup.AppendMenu(uEnabledMenu, SCI_SELECTALL, sMenuItemText);

		popup.AppendMenu(MF_SEPARATOR);

		// now add any custom context menus
		int nCustoms = 1;
		for (INT_PTR handlerindex = 0; handlerindex < m_arContextHandlers.GetCount(); ++handlerindex)
		{
			CSciEditContextMenuInterface * pHandler = m_arContextHandlers.GetAt(handlerindex);
			pHandler->InsertMenuItems(popup, nCustoms);
		}
		if (nCustoms)
		{
			// custom menu entries present, so add another separator
			popup.AppendMenu(MF_SEPARATOR);
		}
		int menuid = nCustoms;

		CMenu corrections;
		corrections.CreatePopupMenu();
		CStringA worda = CStringA(GetWordUnderCursor());
		int nCorrections = nCustoms;
		if ((pChecker)&&(!worda.IsEmpty()))
		{
			char ** wlst;
			int ns = pChecker->suggest(&wlst,worda);
			if (ns > 0)
			{
				for (int i=0; i < ns; i++) 
				{
					CString sug = CString(wlst[i]);
					corrections.InsertMenu((UINT)-1, 0, menuid++, sug);
					free(wlst[i]);
				} 
				free(wlst);
			}

			if ((ns > 0)&&(point.x >= 0))
			{
				sMenuItemText.LoadString(IDS_SPELLEDIT_CORRECTIONS);
				popup.InsertMenu((UINT)-1, MF_POPUP, (UINT_PTR)corrections.m_hMenu, sMenuItemText);
				nCorrections = ns;
			}
		}
#if THESAURUS
		// add found thesauri to submenu's
		CMenu thesaurs;
		thesaurs.CreatePopupMenu();
		int nThesaurs = 0;
		CPtrArray menuArray;
		if ((pThesaur)&&(!worda.IsEmpty()))
		{
			mentry * pmean;
			worda.MakeLower();
			int count = pThesaur->Lookup(worda, worda.GetLength(),&pmean);
			if (count)
			{
				mentry * pm = pmean;
				for (int  i=0; i < count; i++) 
				{
					CMenu * submenu = new CMenu();
					menuArray.Add(submenu);
					submenu->CreateMenu();
					for (int j=0; j < pm->count; j++) 
					{
						CString sug = CString(pm->psyns[j]);
						submenu->InsertMenu((UINT)-1, 0, menuid++, sug);
					}
					thesaurs.InsertMenu((UINT)-1, MF_POPUP, (UINT_PTR)(submenu->m_hMenu), CString(pm->defn));
					pm++;
				}
			}  
			if ((count > 0)&&(point.x >= 0))
			{
#ifdef IDS_SPELLEDIT_THESAURUS
				sMenuItemText.LoadString(IDS_SPELLEDIT_THESAURUS);
				popup.InsertMenu((UINT)-1, MF_POPUP, (UINT_PTR)thesaurs.m_hMenu, sMenuItemText);
#else
				popup.InsertMenu((UINT)-1, MF_POPUP, (UINT_PTR)thesaurs.m_hMenu, _T("Thesaurus"));
#endif
				nThesaurs = menuid;
			}

			pThesaur->CleanUpAfterLookup(&pmean, count);
		}
#endif
		int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
		switch (cmd)
		{
		case 0:
			break;	// no command selected
		case SCI_UNDO:
		case SCI_REDO:
		case SCI_CUT:
		case SCI_COPY:
		case SCI_PASTE:
		case SCI_SELECTALL:
			Call(cmd);
			break;
		default:
			if (cmd < nCustoms)
			{
				for (INT_PTR handlerindex = 0; handlerindex < m_arContextHandlers.GetCount(); ++handlerindex)
				{
					CSciEditContextMenuInterface * pHandler = m_arContextHandlers.GetAt(handlerindex);
					if (pHandler->HandleMenuItemClick(cmd, this))
						break;
				}		
			}
			else if (cmd <= (nCorrections+nCustoms))
			{
				GetWordUnderCursor(true);
				CString temp;
				corrections.GetMenuString(cmd, temp, 0);
				Call(SCI_REPLACESEL, 0, (LPARAM)(LPCSTR)StringForControl(temp));
			}
#if THESAURUS
			else if (cmd <= (nThesaurs+nCorrections+nCustoms))
			{
				GetWordUnderCursor(true);
				CString temp;
				thesaurs.GetMenuString(cmd, temp, 0);
				Call(SCI_REPLACESEL, 0, (LPARAM)(LPCSTR)StringForControl(temp));
			}
#endif
		}
#ifdef THESAURUS
		for (INT_PTR index = 0; index < menuArray.GetCount(); ++index)
		{
			CMenu * pMenu = (CMenu*)menuArray[index];
			delete pMenu;
		}
#endif
	}
	// restore the anchor and cursor position
	Call(SCI_SETCURRENTPOS, currentpos);
	Call(SCI_SETANCHOR, anchor);
}

BOOL CSciEdit::MarkEnteredBugID(NMHDR* nmhdr)
{
	if (m_sCommand.IsEmpty())
		return FALSE;
	SCNotification* notify = (SCNotification*)nmhdr;
	// get the text between the start and end position we have to style
	const int line_number = Call(SCI_LINEFROMPOSITION, Call(SCI_GETENDSTYLED));
	const int start_pos = Call(SCI_POSITIONFROMLINE, (WPARAM)line_number);
	const int end_pos = notify->position;

	if (start_pos == end_pos)
		return FALSE;
	
	char * textbuffer = new char[end_pos - start_pos + 2];
	TEXTRANGEA textrange;
	textrange.lpstrText = textbuffer;
	textrange.chrg.cpMin = start_pos;
	textrange.chrg.cpMax = end_pos;
	Call(SCI_GETTEXTRANGE, 0, (LPARAM)&textrange);
	CString msg = StringFromControl(textbuffer);
	delete textbuffer;
	int offset1 = 0;
	int offset2 = 0;
	
	Call(SCI_STARTSTYLING, start_pos, 0x1f);

	if (!m_sBugID.IsEmpty())
	{
		try
		{
			match_results results;
			match_results::backref_type br;
			do 
			{
				br = m_patCommand.match( (LPCTSTR)msg.Mid(offset1), results );
				if( br.matched ) 
				{
					// clear the styles up to the match position
					Call(SCI_SETSTYLING, results.rstart(0), STYLE_DEFAULT);
					ATLTRACE("STYLE_DEFAULT %d chars\n", results.rstart(0)-offset1);
					offset1 += results.rstart(0);
					offset2 = offset1 + results.rlength(0);
					ATLTRACE("matched string : %ws\n", msg.Mid(offset1, offset2-offset1));
					{
						int idoffset1=offset1;
						int idoffset2=offset2;
						match_results idresults;
						match_results::backref_type idbr;
						do 
						{
							idbr = m_patBugID.match( (LPCTSTR)msg.Mid(idoffset1, offset2-idoffset1), idresults);
							if (idbr.matched)
							{
								// bold style up to the id match
								if (idresults.rstart(0) > 0)
									Call(SCI_SETSTYLING, idresults.rstart(0), STYLE_BOLD);
								idoffset1 += idresults.rstart(0);
								idoffset2 = idoffset1 + idresults.rlength(0);
								ATLTRACE("matched id : %ws\n", msg.Mid(idoffset1, idoffset2-idoffset1));
								// bold and recursive style for the bug ID itself
								if (idoffset2-idoffset1 > 0)
									Call(SCI_SETSTYLING, idoffset2-idoffset1, STYLE_BOLDITALIC);
								idoffset1 = idoffset2;
							}
							else
							{
								// bold style for the rest of the string which isn't matched
								if (offset2-idoffset1 > 0)
									Call(SCI_SETSTYLING, offset2-idoffset1, STYLE_BOLD);
							}
						} while(idbr.matched);
					}
					offset1 = offset2;
				}
				else
				{
					// bold style for the rest of the string which isn't matched
					if (end_pos-offset2 > 0)
						Call(SCI_SETSTYLING, end_pos-offset2, STYLE_DEFAULT);
				}
			} while(br.matched);
			return TRUE;
		}
		catch (bad_alloc) {}
		catch (bad_regexpr){}
	}
	else
	{
		try
		{
			match_results results;
			match_results::backref_type br;
			TCHAR * szMsg = msg.GetBuffer(msg.GetLength()+1);
			do 
			{
				br = m_patCommand.match( &szMsg[offset1], results );
				if( br.matched ) 
				{
					// clear the styles up to the match position
					ATLTRACE("matched string : %ws\n", results.backref(0).str().c_str());
					Call(SCI_SETSTYLING, results.rstart(0), STYLE_DEFAULT);
					ATLTRACE("STYLE_DEFAULT %d chars\n", results.rstart(0));
					offset1 += results.rstart(0);
					{
						for (size_t i=1; i<results.cbackrefs(); ++i)
						{
							if (results.backref(i).begin()-szMsg-offset1 > 0)
							{
								Call(SCI_SETSTYLING, results.backref(i).begin()-szMsg-offset1, STYLE_BOLD);
								ATLTRACE("STYLE_BOLD %d chars\n", results.backref(i).begin()-szMsg-offset1);
							}
							offset1 = results.backref(i).end()-szMsg;
							if (results.backref(i).end()-results.backref(i).begin() > 0)
							{
								Call(SCI_SETSTYLING, results.backref(i).end()-results.backref(i).begin(), STYLE_BOLDITALIC);
								ATLTRACE("STYLE_BOLDITALIC %d chars\n", results.backref(i).end()-results.backref(i).begin());
							}
						}
					}
				}
				else
				{
					// bold style for the rest of the string which isn't matched
					if (end_pos > offset1)
						Call(SCI_SETSTYLING, end_pos-offset1, STYLE_DEFAULT);
				}
			} while(br.matched);
			msg.ReleaseBuffer();
			return TRUE;
		}
		catch (bad_alloc) {}
		catch (bad_regexpr){}
	}
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////

void CAutoCompletionList::AddSorted(const CString& elem, bool bNoDuplicates /*= true*/)
{
	if (elem.IsEmpty())
		return;
	if (GetCount()==0)
		return InsertAt(0, elem);
	
	int nMin = 0;
	int nMax = GetUpperBound();
	while (nMin <= nMax)
	{
		UINT nHit = (UINT)(nMin + nMax) >> 1; // fast divide by 2
		int cmp = elem.CompareNoCase(GetAt(nHit));

		if (cmp > 0)
			nMin = nHit + 1;
		else if (cmp < 0)
			nMax = nHit - 1;
		else if (bNoDuplicates)
			return; // already in the array
	}
	return InsertAt(nMin, elem);
}

