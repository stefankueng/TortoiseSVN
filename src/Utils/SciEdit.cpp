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
#include ".\sciedit.h"

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
	
	TCHAR buffer[11];
	GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IDEFAULTANSICODEPAGE,(TCHAR *)buffer,10);
	buffer[10]=0;
	int codepage=0;
	codepage=_tstoi((TCHAR *)buffer);
	Call(SCI_SETCODEPAGE, codepage);
	
	// look for dictionary files and use them if found
	long langId = GetUserDefaultLCID();
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

BOOL CSciEdit::LoadDictionaries(LONG lLanguageID)
{
	//Setup the spell checker and thesaurus
	TCHAR buf[MAX_PATH];
	CString sFolder;
	CString sFolderUp;
	CString sFile;
	if (GetModuleFileName(NULL, buf, MAX_PATH))
	{
		sFolder = CString(buf);
		sFolder = sFolder.Left(sFolder.ReverseFind('\\'));
		sFolderUp = sFolder.Left(sFolder.ReverseFind('\\'));
		sFolder += _T("\\");
		sFolderUp += _T("\\");

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
		}
#if THESAURUS
		if (pThesaur==NULL)
		{
			if ((PathFileExists(sFolder + _T("th_") + sFile + _T(".idx"))) &&
				(PathFileExists(sFolder + _T("th_") + sFile + _T(".dat"))))
			{
				pThesaur = new MyThes(CStringA(sFolder + sFile + _T(".idx")), CStringA(sFolder + sFile + _T(".dat")));
			}
			else if ((PathFileExists(sFolder + _T("dic\\th_") + sFile + _T(".idx"))) &&
				(PathFileExists(sFolder + _T("dic\\th_") + sFile + _T(".dat"))))
			{
				pThesaur = new MyThes(CStringA(sFolder + _T("dic\\") + sFile + _T(".idx")), CStringA(sFolder + _T("dic\\") + sFile + _T(".dat")));
			}
			else if ((PathFileExists(sFolderUp + _T("th_") + sFile + _T(".idx"))) &&
				(PathFileExists(sFolderUp + _T("th_") + sFile + _T(".dat"))))
			{
				pThesaur = new MyThes(CStringA(sFolderUp + _T("th_") + sFile + _T(".idx")), CStringA(sFolderUp + _T("th_") + sFile + _T(".dat")));
			}
			else if ((PathFileExists(sFolderUp + _T("dic\\th_") + sFile + _T(".idx"))) &&
				(PathFileExists(sFolderUp + _T("dic\\th_") + sFile + _T(".dat"))))
			{
				pThesaur = new MyThes(CStringA(sFolderUp + _T("dic\\th_") + sFile + _T(".idx")), CStringA(sFolderUp + _T("dic\\th_") + sFile + _T(".dat")));
			}
		}
#endif
	}
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
	WideCharToMultiByte(codepage, 0, text, text.GetLength(), sTextA.GetBuffer(reslen+1), reslen+1, 0, 0);
	sTextA.ReleaseBuffer(reslen);
#else
	sTextA = text;
#endif
	return sTextA;
}

void CSciEdit::SetText(const CString& sText)
{
	Call(SCI_SETTEXT, 0, (LPARAM)(LPCSTR)StringForControl(sText));
}

void CSciEdit::InsertText(const CString& sText, bool bNewLine)
{
	Call(SCI_REPLACESEL, 0, (LPARAM)(LPCSTR)StringForControl(sText));
	if (bNewLine)
		Call(SCI_REPLACESEL, 0, (LPARAM)(LPCSTR)"\n");
}

CString CSciEdit::GetText()
{
	LRESULT len = Call(SCI_GETLENGTH);
	CStringA sTextA;
	Call(SCI_GETTEXT, len+1, (LPARAM)(LPCSTR)sTextA.GetBuffer(len+1));
	sTextA.ReleaseBuffer();
	return StringFromControl(sTextA);
}

CString CSciEdit::GetWordUnderCursor(bool bSelectWord)
{
	char textbuffer[100];
	TEXTRANGEA textrange;
	textrange.lpstrText = textbuffer;	
	int pos = Call(SCI_GETCURRENTPOS);
	textrange.chrg.cpMin = Call(SCI_WORDSTARTPOSITION, pos, TRUE);
	textrange.chrg.cpMax = Call(SCI_WORDENDPOSITION, textrange.chrg.cpMin, TRUE);
	Call(SCI_GETTEXTRANGE, 0, (LPARAM)&textrange);
	if (bSelectWord)
	{
		Call(SCI_SETSEL, textrange.chrg.cpMin, textrange.chrg.cpMax);
	}
	return StringFromControl(textbuffer);
}

void CSciEdit::SetFont(CString sFontName, int iFontSizeInPoints)
{
	Call(SCI_STYLESETFONT, STYLE_DEFAULT, (LPARAM)(LPCSTR)StringForControl(sFontName));
	Call(SCI_STYLESETSIZE, STYLE_DEFAULT, iFontSizeInPoints);
	Call(SCI_STYLECLEARALL);
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
	
	char textbuffer[100];
	TEXTRANGEA textrange;
	textrange.lpstrText = textbuffer;	
	
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
		if (textrange.chrg.cpMax - textrange.chrg.cpMin < 100)
		{
			Call(SCI_GETTEXTRANGE, 0, (LPARAM)&textrange);
			if (strlen(textrange.lpstrText) > 3)
			{
				if (!pChecker->spell(textrange.lpstrText))
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
		}
		else
		{
			textrange.chrg.cpMin = textrange.chrg.cpMax;
		}
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
		Call(SCI_AUTOCSHOW, 0, (LPARAM)(LPCSTR)StringForControl(suggestions));
		Call(SCI_AUTOCSETDROPRESTOFWORD, 1);
	}

}

void CSciEdit::DoAutoCompletion()
{
	if (m_autolist.GetCount()==0)
		return;
	CString word = GetWordUnderCursor();
	if (word.GetLength() < 3)
		return;		//don't autocomplete yet, word is too short
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
			break;
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

	if(lpnmhdr->hwndFrom==m_hWnd)
	{
		switch(lpnmhdr->code)
		{
		case SCN_CHARADDED:
			CheckSpelling();
			DoAutoCompletion();
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
		}
		break;
	case (VK_ESCAPE):
		{
			if ((Call(SCI_AUTOCACTIVE)==0)&&(Call(SCI_CALLTIPACTIVE)==0))
				::SendMessage(GetParent()->GetSafeHwnd(), WM_CLOSE, 0, 0);
		}
		break;
	}
	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CSciEdit::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	if ((point.x == -1) && (point.y == -1))
	{
		CRect rect;
		GetClientRect(&rect);
		ClientToScreen(&rect);
		point = rect.CenterPoint();
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

		CMenu corrections;
		corrections.CreatePopupMenu();
		CStringA worda = StringForControl(GetWordUnderCursor());
		int nCorrections = 0;
		if ((pChecker)&&(!worda.IsEmpty()))
		{
			char ** wlst;
			int ns = pChecker->suggest(&wlst,worda);
			if (ns > 0)
			{
				for (int i=0; i < ns; i++) 
				{
					CString sug = CString(wlst[i]);
					corrections.InsertMenu((UINT)-1, 0, i+1, sug);
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
			int menuid = 50;		//offset (spell check menu items start with 1, thesaurus entries with 50)
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
			if (cmd <= nCorrections)
			{
				GetWordUnderCursor(true);
				CString temp;
				corrections.GetMenuString(cmd, temp, 0);
				Call(SCI_REPLACESEL, 0, (LPARAM)(LPCSTR)StringForControl(temp));
			}
#if THESAURUS
			else if (cmd <= nThesaurs)
			{
				GetWordUnderCursor(true);
				CString temp;
				thesaurs.GetMenuString(cmd, temp, 0);
				Call(SCI_REPLACESEL, 0, (LPARAM)(LPCSTR)StringForControl(temp));
			}
#endif
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void CAutoCompletionList::AddSorted(const CString& elem, bool bNoDuplicates /*= true*/)
{
	if (GetCount()==0)
		return InsertAt(0, elem);
	
	int nMin = 0;
	int nMax = GetUpperBound();
	while (nMin <= nMax)
	{
		UINT nHit = (UINT)(nMin + nMax) >> 1; // fast devide by 2
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

