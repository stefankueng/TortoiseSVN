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
#include "StdAfx.h"
#include "resource.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "UnicodeUtils.h"
#include <string>
#include "regexpr2.h"
#include "registry.h"
#include ".\sciedit.h"

using namespace std;
using namespace regex;


void CSciEditContextMenuInterface::InsertMenuItems(CMenu&, int&) {return;}
bool CSciEditContextMenuInterface::HandleMenuItemClick(int, CSciEdit *) {return false;}


#define STYLE_ISSUEBOLD			11
#define STYLE_ISSUEBOLDITALIC	12
#define STYLE_BOLD				14
#define STYLE_ITALIC			15
#define STYLE_UNDERLINED		16
#define STYLE_URL				17

#define STYLE_MASK 0x1f

#define SCI_ADDWORD			2000

struct loc_map {
	const char * cp;
	const char * def_enc;
};

struct loc_map enc2locale[] = {
	{"28591","ISO8859-1"},
	{"28592","ISO8859-2"},
	{"28593","ISO8859-3"},
	{"28594","ISO8859-4"},
	{"28595","ISO8859-5"},
	{"28596","ISO8859-6"},
	{"28597","ISO8859-7"},
	{"28598","ISO8859-8"},
	{"28599","ISO8859-9"},
	{"28605","ISO8859-15"},
	{"20866","KOI8-R"},
	{"21866","KOI8-U"},
	{"1251","microsoft-cp1251"},
	};


IMPLEMENT_DYNAMIC(CSciEdit, CWnd)

CSciEdit::CSciEdit(void) : m_DirectFunction(NULL)
	, m_DirectPointer(NULL)
	, pChecker(NULL)
	, pThesaur(NULL)
{
	m_hModule = ::LoadLibrary(_T("SciLexer.DLL"));
}

CSciEdit::~CSciEdit(void)
{
	m_personalDict.Save();
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
	Call(SCI_AUTOCSETFILLUPS, 0, (LPARAM)"\t([");
	Call(SCI_AUTOCSETMAXWIDTH, 0);
	//Set the default windows colors for edit controls
	Call(SCI_STYLESETFORE, STYLE_DEFAULT, ::GetSysColor(COLOR_WINDOWTEXT));
	Call(SCI_STYLESETBACK, STYLE_DEFAULT, ::GetSysColor(COLOR_WINDOW));
	Call(SCI_SETSELFORE, TRUE, ::GetSysColor(COLOR_HIGHLIGHTTEXT));
	Call(SCI_SETSELBACK, TRUE, ::GetSysColor(COLOR_HIGHLIGHT));
	Call(SCI_SETCARETFORE, ::GetSysColor(COLOR_WINDOWTEXT));
	Call(SCI_SETMODEVENTMASK, SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT | SC_PERFORMED_UNDO | SC_PERFORMED_REDO);
	Call(SCI_INDICSETFORE, 1, 0x0000FF);
	CStringA sWordChars;
	CStringA sWhiteSpace;
	for (int i=0; i<255; ++i)
	{
		if (i == '\r' || i == '\n')
			continue;
		else if (i < 0x20 || i == ' ')
			sWhiteSpace += (char)i;
		else if (isalnum(i) || i == '_' || i == '\'')
			sWordChars += (char)i;
	}
	Call(SCI_SETWORDCHARS, 0, (LPARAM)(LPCSTR)sWordChars);
	Call(SCI_SETWHITESPACECHARS, 0, (LPARAM)(LPCSTR)sWhiteSpace);
	// look for dictionary files and use them if found
	long langId = GetUserDefaultLCID();

	if ((lLanguage != 0)||(((DWORD)CRegStdWORD(_T("Software\\TortoiseSVN\\Spellchecker"), FALSE))==FALSE))
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
	Call(SCI_ASSIGNCMDKEY, SCK_END, SCI_LINEENDWRAP);
	Call(SCI_ASSIGNCMDKEY, SCK_END + (SCMOD_SHIFT << 16), SCI_LINEENDWRAPEXTEND);
	Call(SCI_ASSIGNCMDKEY, SCK_HOME, SCI_HOMEWRAP);
	Call(SCI_ASSIGNCMDKEY, SCK_HOME + (SCMOD_SHIFT << 16), SCI_HOMEWRAPEXTEND);
}

void CSciEdit::Init(const ProjectProperties& props)
{
	Init(props.lProjectLanguage);
	m_sCommand = CStringA(CUnicodeUtils::GetUTF8(props.sCheckRe));
	m_sBugID = CStringA(CUnicodeUtils::GetUTF8(props.sBugIDRe));
	m_sUrl = CStringA(CUnicodeUtils::GetUTF8(props.sUrl));
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
	CString sFolder = CPathUtils::GetAppDirectory();
	CString sFolderUp = CPathUtils::GetAppParentDirectory();
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
			pChecker = new Hunspell(CStringA(sFolder + sFile + _T(".aff")), CStringA(sFolder + sFile + _T(".dic")));
		}
		else if ((PathFileExists(sFolder + _T("dic\\") + sFile + _T(".aff"))) &&
			(PathFileExists(sFolder + _T("dic\\") + sFile + _T(".dic"))))
		{
			pChecker = new Hunspell(CStringA(sFolder + _T("dic\\") + sFile + _T(".aff")), CStringA(sFolder + _T("dic\\") + sFile + _T(".dic")));
		}
		else if ((PathFileExists(sFolderUp + sFile + _T(".aff"))) &&
			(PathFileExists(sFolderUp + sFile + _T(".dic"))))
		{
			pChecker = new Hunspell(CStringA(sFolderUp + sFile + _T(".aff")), CStringA(sFolderUp + sFile + _T(".dic")));
		}
		else if ((PathFileExists(sFolderUp + _T("dic\\") + sFile + _T(".aff"))) &&
			(PathFileExists(sFolderUp + _T("dic\\") + sFile + _T(".dic"))))
		{
			pChecker = new Hunspell(CStringA(sFolderUp + _T("dic\\") + sFile + _T(".aff")), CStringA(sFolderUp + _T("dic\\") + sFile + _T(".dic")));
		}
		else if ((PathFileExists(sFolderUp + _T("Languages\\") + sFile + _T(".aff"))) &&
			(PathFileExists(sFolderUp + _T("Languages\\") + sFile + _T(".dic"))))
		{
			pChecker = new Hunspell(CStringA(sFolderUp + _T("Languages\\") + sFile + _T(".aff")), CStringA(sFolderUp + _T("Languages\\") + sFile + _T(".dic")));
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
	if (pChecker)
	{
		const char * encoding = pChecker->get_dic_encoding();
		ATLTRACE(encoding);
		int n = sizeof(enc2locale) / sizeof(enc2locale[0]);
		m_spellcodepage = 0;
		for (int i = 0; i < n; i++) 
		{
			if (strcmp(encoding,enc2locale[i].def_enc) == 0)
			{
				m_spellcodepage = atoi(enc2locale[i].cp);
			}
		}
		m_personalDict.Init(lLanguageID);
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
	Call(SCI_STYLESETFONT, STYLE_DEFAULT, (LPARAM)(LPCSTR)CStringA(sFontName));
	Call(SCI_STYLESETSIZE, STYLE_DEFAULT, iFontSizeInPoints);
	Call(SCI_STYLECLEARALL);

	LPARAM color = (LPARAM)GetSysColor(COLOR_HIGHLIGHT);
	// set the styles for the bug ID strings
	Call(SCI_STYLESETBOLD, STYLE_ISSUEBOLD, (LPARAM)TRUE);
	Call(SCI_STYLESETFORE, STYLE_ISSUEBOLD, color);
	Call(SCI_STYLESETBOLD, STYLE_ISSUEBOLDITALIC, (LPARAM)TRUE);
	Call(SCI_STYLESETITALIC, STYLE_ISSUEBOLDITALIC, (LPARAM)TRUE);
	Call(SCI_STYLESETFORE, STYLE_ISSUEBOLDITALIC, color);
	Call(SCI_STYLESETHOTSPOT, STYLE_ISSUEBOLDITALIC, (LPARAM)TRUE);

	// set the formatted text styles
	Call(SCI_STYLESETBOLD, STYLE_BOLD, (LPARAM)TRUE);
	Call(SCI_STYLESETITALIC, STYLE_ITALIC, (LPARAM)TRUE);
	Call(SCI_STYLESETUNDERLINE, STYLE_UNDERLINED, (LPARAM)TRUE);

	// set the style for URLs
	Call(SCI_STYLESETFORE, STYLE_URL, color);
	Call(SCI_STYLESETHOTSPOT, STYLE_URL, (LPARAM)TRUE);

	Call(SCI_SETHOTSPOTACTIVEUNDERLINE, (LPARAM)TRUE);
}

void CSciEdit::SetAutoCompletionList(const std::set<CString>& list, const TCHAR separator)
{
	//copy the autocompletion list.
	
	//SK: instead of creating a copy of that list, we could accept a pointer
	//to the list and use that instead. But then the caller would have to make
	//sure that the list persists over the lifetime of the control!
	m_autolist.clear();
	m_autolist = list;
	m_separator = separator;
}

BOOL CSciEdit::IsMisspelled(const CString& sWord)
{
	// convert the string from the control to the encoding of the spell checker module.
	CStringA sWordA;
	if (m_spellcodepage)
	{
		char * buf;
		buf = sWordA.GetBuffer(sWord.GetLength()*4 + 1);
		int lengthIncTerminator =
			WideCharToMultiByte(m_spellcodepage, 0, sWord, -1, buf, sWord.GetLength()*4, NULL, NULL);
		sWordA.ReleaseBuffer(lengthIncTerminator-1);
	}
	else
		sWordA = CStringA(sWord);
	
	return
		!_istdigit(sWord.GetAt(0)) &&
		(m_autolist.find(sWord) == m_autolist.end()) && // check if the word is in our autocompletion list
		!pChecker->spell(sWordA) &&
		!m_personalDict.FindWord(sWord);
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
		ATLASSERT(textrange.chrg.cpMax >= textrange.chrg.cpMin);
		char * textbuffer = new char[textrange.chrg.cpMax - textrange.chrg.cpMin + 2];
		textrange.lpstrText = textbuffer;
		textrange.chrg.cpMax++;
		Call(SCI_GETTEXTRANGE, 0, (LPARAM)&textrange);
		int len = strlen(textrange.lpstrText);
		if (len && textrange.lpstrText[len - 1] == '.')
		{
			// Try to ignore file names from the autolist.
			// Do do this, for each word ending with '.' we extract next word and check
			// whether the combined string is present in autolist. 
			TEXTRANGEA twoWords;
			twoWords.chrg.cpMin = textrange.chrg.cpMin;
			twoWords.chrg.cpMax = Call(SCI_WORDENDPOSITION, textrange.chrg.cpMax + 1, TRUE);
			twoWords.lpstrText = new char[twoWords.chrg.cpMax - twoWords.chrg.cpMin + 1];
			Call(SCI_GETTEXTRANGE, 0, (LPARAM)&twoWords);
			CString sWord = StringFromControl(twoWords.lpstrText);
			delete twoWords.lpstrText;
			if (m_autolist.find(sWord) != m_autolist.end())
			{
				//mark word as correct (remove the squiggle line)
				Call(SCI_STARTSTYLING, twoWords.chrg.cpMin, INDICS_MASK);
				Call(SCI_SETSTYLING, twoWords.chrg.cpMax - twoWords.chrg.cpMin, 0);
				textrange.chrg.cpMax = twoWords.chrg.cpMax;
				delete textbuffer;
				continue;
			}
		}
		textrange.lpstrText[len - 1] = 0;
		textrange.chrg.cpMax--;
		if (strlen(textrange.lpstrText) > 0)
		{
			CString sWord = StringFromControl(textrange.lpstrText);
			if ((GetStyleAt(textrange.chrg.cpMin) != STYLE_URL) && IsMisspelled(sWord))
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

void CSciEdit::DoAutoCompletion(int nMinPrefixLength)
{
	if (m_autolist.size()==0)
		return;
	if (Call(SCI_AUTOCACTIVE))
		return;
	CString word = GetWordUnderCursor();
	if (word.GetLength() < nMinPrefixLength)
		return;		//don't autocomplete yet, word is too short
	int pos = Call(SCI_GETCURRENTPOS);
	if (pos != Call(SCI_WORDENDPOSITION, pos, TRUE))
		return;	//don't autocomplete if we're not at the end of a word
	CString sAutoCompleteList;
	
	for (std::set<CString>::const_iterator lowerit = m_autolist.lower_bound(word);
		lowerit != m_autolist.end(); ++lowerit)
	{
		int compare = word.CompareNoCase(lowerit->Left(word.GetLength()));
		if (compare>0)
			continue;
		else if (compare == 0)
		{
			sAutoCompleteList += *lowerit + m_separator;
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
			{
				if ((lpSCN->ch < 32)&&(lpSCN->ch != 13)&&(lpSCN->ch != 10))
					Call(SCI_DELETEBACK);
				else
				{
					DoAutoCompletion(3);
				}
				return TRUE;
			}
			break;
		case SCN_STYLENEEDED:
			{
				int startstylepos = Call(SCI_GETENDSTYLED);
				int endstylepos = ((SCNotification *)lpnmhdr)->position;
				MarkEnteredBugID(startstylepos, endstylepos);
				StyleEnteredText(startstylepos, endstylepos);
				StyleURLs(startstylepos, endstylepos);
				CheckSpelling();
				WrapLines(startstylepos, endstylepos);
				return TRUE;
			}
			break;
		case SCN_HOTSPOTCLICK:
			{
				TEXTRANGEA textrange;
				textrange.chrg.cpMin = lpSCN->position;
				textrange.chrg.cpMax = lpSCN->position;
				DWORD style = GetStyleAt(lpSCN->position);
				while (GetStyleAt(textrange.chrg.cpMin - 1) == style)
					--textrange.chrg.cpMin;
				while (GetStyleAt(textrange.chrg.cpMax + 1) == style)
					++textrange.chrg.cpMax;
				++textrange.chrg.cpMax;
				char * textbuffer = new char[textrange.chrg.cpMax - textrange.chrg.cpMin + 1];
				textrange.lpstrText = textbuffer;	
				Call(SCI_GETTEXTRANGE, 0, (LPARAM)&textrange);
				CString url;
				if (style == STYLE_URL)
					url = StringFromControl(textbuffer);
				else
				{
					url = m_sUrl;
					url.Replace(_T("%BUGID%"), StringFromControl(textbuffer));
				}
				delete textbuffer;
				if (!url.IsEmpty())
					ShellExecute(GetParent()->GetSafeHwnd(), _T("open"), url, NULL, NULL, SW_SHOWDEFAULT);
			}
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
	case (VK_ESCAPE):
		{
			if ((Call(SCI_AUTOCACTIVE)==0)&&(Call(SCI_CALLTIPACTIVE)==0))
				::SendMessage(GetParent()->GetSafeHwnd(), WM_CLOSE, 0, 0);
		}
		break;
	}
	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

BOOL CSciEdit::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_SPACE:
			{
				if (GetKeyState(VK_CONTROL) & 0x8000)
				{
					DoAutoCompletion(1);
					return TRUE;
				}
			}
			break;
		case VK_TAB:
			// The TAB cannot be handled in OnKeyDown because it is too late by then.
			{
				if (GetKeyState(VK_CONTROL)&0x8000)
				{
					//Ctrl-Tab was pressed, this means we should provide the user with
					//a list of possible spell checking alternatives to the word under
					//the cursor
					SuggestSpellingAlternatives();
					return TRUE;
				}
				else if (!Call(SCI_AUTOCACTIVE))
				{
					::PostMessage(GetParent()->GetSafeHwnd(), WM_NEXTDLGCTL, GetKeyState(VK_SHIFT)&0x8000, 0);
					return TRUE;
				}
			}
			break;
		}
	}
	return CWnd::PreTranslateMessage(pMsg);
}

void CSciEdit::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	int anchor = Call(SCI_GETANCHOR);
	int currentpos = Call(SCI_GETCURRENTPOS);
	int selstart = Call(SCI_GETSELECTIONSTART);
	int selend = Call(SCI_GETSELECTIONEND);
	int pointpos = 0;
	if ((point.x == -1) && (point.y == -1))
	{
		CRect rect;
		GetClientRect(&rect);
		ClientToScreen(&rect);
		point = rect.CenterPoint();
		pointpos = Call(SCI_GETCURRENTPOS);
	}
	else
	{
		// change the cursor position to the point where the user
		// right-clicked.
		CPoint clientpoint = point;
		ScreenToClient(&clientpoint);
		pointpos = Call(SCI_POSITIONFROMPOINT, clientpoint.x, clientpoint.y);
	}
	CString sMenuItemText;
	CMenu popup;
	bool bRestoreCursor = true;
	if (popup.CreatePopupMenu())
	{
		bool bCanUndo = !!Call(SCI_CANUNDO);
		bool bCanRedo = !!Call(SCI_CANREDO);
		bool bHasSelection = (selend-selstart > 0);
		bool bCanPaste = !!Call(SCI_CANPASTE);
		UINT uEnabledMenu = MF_STRING | MF_ENABLED;
		UINT uDisabledMenu = MF_STRING | MF_GRAYED;

		// find the word under the cursor
		CString sWord;		
		if (pointpos)
		{
			// setting the cursor clears the selection
			Call(SCI_SETANCHOR, pointpos);
			Call(SCI_SETCURRENTPOS, pointpos);
			sWord = GetWordUnderCursor();
			// restore the selection
			Call(SCI_SETSELECTIONSTART, selstart);
			Call(SCI_SETSELECTIONEND, selend);
		}
		else
			sWord = GetWordUnderCursor();
		CStringA worda = CStringA(sWord);

		int nCorrections = 1;
		bool bSpellAdded = false;
		// check if the word under the cursor is spelled wrong
		if ((pChecker)&&(!worda.IsEmpty()))
		{
			char ** wlst;
			// get the spell suggestions
			int ns = pChecker->suggest(&wlst,worda);
			if (ns > 0)
			{
				// add the suggestions to the context menu
				for (int i=0; i < ns; i++) 
				{
					bSpellAdded = true;
					CString sug = CString(wlst[i]);
					popup.InsertMenu((UINT)-1, 0, nCorrections++, sug);
					free(wlst[i]);
				} 
				free(wlst);
			}
		}
		// only add a separator if spelling correction suggestions were added
		if (bSpellAdded)
			popup.AppendMenu(MF_SEPARATOR);

		// also allow the user to add the word to the custom dictionary so
		// it won't show up as misspelled anymore
		if ((sWord.GetLength()<PDICT_MAX_WORD_LENGTH)&&((pChecker)&&(m_autolist.find(sWord) == m_autolist.end())&&(!pChecker->spell(worda)))&&
			(!_istdigit(sWord.GetAt(0)))&&(!m_personalDict.FindWord(sWord)))
		{
			sMenuItemText.Format(IDS_SCIEDIT_ADDWORD, sWord);
			popup.AppendMenu(uEnabledMenu, SCI_ADDWORD, sMenuItemText);
			// another separator
			popup.AppendMenu(MF_SEPARATOR);
		}

		// add the 'default' entries
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

		sMenuItemText.LoadString(IDS_SCIEDIT_SPLITLINES);
		popup.AppendMenu(bHasSelection ? uEnabledMenu : uDisabledMenu, SCI_LINESSPLIT, sMenuItemText);

		popup.AppendMenu(MF_SEPARATOR);

		int nCustoms = nCorrections;
		// now add any custom context menus
		for (INT_PTR handlerindex = 0; handlerindex < m_arContextHandlers.GetCount(); ++handlerindex)
		{
			CSciEditContextMenuInterface * pHandler = m_arContextHandlers.GetAt(handlerindex);
			pHandler->InsertMenuItems(popup, nCustoms);
		}
		if (nCustoms > nCorrections)
		{
			// custom menu entries present, so add another separator
			popup.AppendMenu(MF_SEPARATOR);
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
						submenu->InsertMenu((UINT)-1, 0, nThesaurs++, sug);
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
				nThesaurs = nCustoms;
			}
			else
			{
				sMenuItemText.LoadString(IDS_SPELLEDIT_NOTHESAURUS);
				popup.AppendMenu(MF_DISABLED | MF_GRAYED | MF_STRING, 0, sMenuItemText);
			}

			pThesaur->CleanUpAfterLookup(&pmean, count);
		}
		else
		{
			sMenuItemText.LoadString(IDS_SPELLEDIT_NOTHESAURUS);
			popup.AppendMenu(MF_DISABLED | MF_GRAYED | MF_STRING, 0, sMenuItemText);
		}
#endif
		int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
		switch (cmd)
		{
		case 0:
			break;	// no command selected
		case SCI_SELECTALL:
			bRestoreCursor = false;
			// fall through
		case SCI_UNDO:
		case SCI_REDO:
		case SCI_CUT:
		case SCI_COPY:
		case SCI_PASTE:
			Call(cmd);
			break;
		case SCI_ADDWORD:
			m_personalDict.AddWord(sWord);
			CheckSpelling();
			break;
		case SCI_LINESSPLIT:
			{
				int marker = Call(SCI_GETEDGECOLUMN) * Call(SCI_TEXTWIDTH, 0, (LPARAM)" ");
				if (marker)
				{
					Call(SCI_TARGETFROMSELECTION);
					Call(SCI_LINESJOIN);
					Call(SCI_LINESSPLIT, marker);
				}
			}
			break;
		default:
			if (cmd < nCorrections)
			{
				Call(SCI_SETANCHOR, pointpos);
				Call(SCI_SETCURRENTPOS, pointpos);
				GetWordUnderCursor(true);
				CString temp;
				popup.GetMenuString(cmd, temp, 0);
				// setting the cursor clears the selection
				Call(SCI_REPLACESEL, 0, (LPARAM)(LPCSTR)StringForControl(temp));
			}
			else if (cmd < (nCorrections+nCustoms))
			{
				for (INT_PTR handlerindex = 0; handlerindex < m_arContextHandlers.GetCount(); ++handlerindex)
				{
					CSciEditContextMenuInterface * pHandler = m_arContextHandlers.GetAt(handlerindex);
					if (pHandler->HandleMenuItemClick(cmd, this))
						break;
				}		
			}
#if THESAURUS
			else if (cmd <= (nThesaurs+nCorrections+nCustoms))
			{
				Call(SCI_SETANCHOR, pointpos);
				Call(SCI_SETCURRENTPOS, pointpos);
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
	if (bRestoreCursor)
	{
		// restore the anchor and cursor position
		Call(SCI_SETCURRENTPOS, currentpos);
		Call(SCI_SETANCHOR, anchor);
	}
}

bool CSciEdit::StyleEnteredText(int startstylepos, int endstylepos)
{
	bool bStyled = false;
	const int line = Call(SCI_LINEFROMPOSITION, startstylepos);
	const int line_number_end = Call(SCI_LINEFROMPOSITION, endstylepos);
	for (int line_number = line; line_number <= line_number_end; ++line_number)
	{
		int offset = Call(SCI_POSITIONFROMLINE, line_number);
		int line_len = Call(SCI_LINELENGTH, line_number);
		char * linebuffer = new char[line_len+1];
		Call(SCI_GETLINE, line_number, (LPARAM)linebuffer);
		linebuffer[line_len] = 0;
		int start = 0;
		int end = 0;
		while (FindStyleChars(linebuffer, '*', start, end))
		{
			Call(SCI_STARTSTYLING, start+offset, STYLE_MASK);
			Call(SCI_SETSTYLING, end-start, STYLE_BOLD);
			bStyled = true;
			start = end;
		}
		start = 0;
		end = 0;
		while (FindStyleChars(linebuffer, '^', start, end))
		{
			Call(SCI_STARTSTYLING, start+offset, STYLE_MASK);
			Call(SCI_SETSTYLING, end-start, STYLE_ITALIC);
			bStyled = true;
			start = end;
		}
		start = 0;
		end = 0;
		while (FindStyleChars(linebuffer, '_', start, end))
		{
			Call(SCI_STARTSTYLING, start+offset, STYLE_MASK);
			Call(SCI_SETSTYLING, end-start, STYLE_UNDERLINED);
			bStyled = true;
			start = end;
		}
		delete linebuffer;
	}
	return bStyled;
}

bool CSciEdit::WrapLines(int startpos, int endpos)
{
	int markerX = Call(SCI_GETEDGECOLUMN) * Call(SCI_TEXTWIDTH, 0, (LPARAM)" ");
	if (markerX)
	{
		Call(SCI_SETTARGETSTART, startpos);
		Call(SCI_SETTARGETEND, endpos);
		Call(SCI_LINESSPLIT, markerX);
		return true;
	}
	return false;
}

void CSciEdit::AdvanceUTF8(const char * str, int& pos)
{
	if ((str[pos] & 0xE0)==0xC0)
	{
		// utf8 2-byte sequence
		pos += 2;
	}
	else if ((str[pos] & 0xF0)==0xE0)
	{
		// utf8 3-byte sequence
		pos += 3;
	}
	else if ((str[pos] & 0xF8)==0xF0)
	{
		// utf8 4-byte sequence
		pos += 4;
	}
	else
		pos++;
}

bool CSciEdit::FindStyleChars(const char * line, char styler, int& start, int& end)
{
	int i=0;
	int u=0;
	while (i < start)
	{
		AdvanceUTF8(line, i);
		u++;
	}

	bool bFoundMarker = false;
	CString sULine = CUnicodeUtils::GetUnicode(line);
	// find a starting marker
	while (line[i] != 0)
	{
		if (line[i] == styler)
		{
			if ((line[i+1]!=0)&&(IsCharAlphaNumeric(sULine[u+1]))&&
				(((u>0)&&(!IsCharAlphaNumeric(sULine[u-1]))) || (u==0)))
			{
				start = i+1;
				AdvanceUTF8(line, i);
				u++;
				bFoundMarker = true;
				break;
			}
		}
		AdvanceUTF8(line, i);
		u++;
	}
	if (!bFoundMarker)
		return false;
	// find ending marker
	bFoundMarker = false;
	while (line[i] != 0)
	{
		if (line[i] == styler)
		{
			if ((IsCharAlphaNumeric(sULine[u-1]))&&
				((((u+1)<sULine.GetLength())&&(!IsCharAlphaNumeric(sULine[u+1]))) || ((u+1) == sULine.GetLength()))
				)
			{
				end = i;
				i++;
				bFoundMarker = true;
				break;
			}
		}
		AdvanceUTF8(line, i);
		u++;
	}
	return bFoundMarker;
}

BOOL CSciEdit::MarkEnteredBugID(int startstylepos, int endstylepos)
{
	if (m_sCommand.IsEmpty())
		return FALSE;
	// get the text between the start and end position we have to style
	const int line_number = Call(SCI_LINEFROMPOSITION, startstylepos);
	int start_pos = Call(SCI_POSITIONFROMLINE, (WPARAM)line_number);
	int end_pos = endstylepos;

	if (start_pos == end_pos)
		return FALSE;
	if (start_pos > end_pos)
	{
		int switchtemp = start_pos;
		start_pos = end_pos;
		end_pos = switchtemp;
	}

	char * textbuffer = new char[end_pos - start_pos + 2];
	TEXTRANGEA textrange;
	textrange.lpstrText = textbuffer;
	textrange.chrg.cpMin = start_pos;
	textrange.chrg.cpMax = end_pos;
	Call(SCI_GETTEXTRANGE, 0, (LPARAM)&textrange);
	CString msg = CString(textbuffer);
	delete textbuffer;
	size_t offset1 = 0;
	size_t offset2 = 0;

	// used for pattern matching by GRETA
	restring reMsg = (LPCTSTR)msg;

	Call(SCI_STARTSTYLING, start_pos, STYLE_MASK);

	if (!m_sBugID.IsEmpty())
	{
		try
		{
			match_results results;
			match_results::backref_type br;
			do 
			{
				br = m_patCommand.match( reMsg, results, offset1 );
				if( br.matched ) 
				{
					// clear the styles up to the match position
					Call(SCI_SETSTYLING, results.rstart(0), STYLE_DEFAULT);
					ATLTRACE("STYLE_DEFAULT %d chars\n", results.rstart(0)-offset1);
					offset1 += results.rstart(0);
					offset2 = offset1 + results.rlength(0);
					ATLTRACE(_T("matched string : %s\n"), results.backref(0).str().c_str());
					{
						size_t idoffset1=offset1;
						size_t idoffset2=offset2;
						match_results idresults;
						match_results::backref_type idbr;
						do 
						{
							idbr = m_patBugID.match( reMsg, idresults, idoffset1, offset2-idoffset1);
							if (idbr.matched)
							{
								// bold style up to the id match
								if (idresults.rstart(0) > 0)
									Call(SCI_SETSTYLING, idresults.rstart(0), STYLE_ISSUEBOLD);
								idoffset1 += idresults.rstart(0);
								idoffset2 = idoffset1 + idresults.rlength(0);
								ATLTRACE(_T("matched id : %s\n"), idresults.backref(0).str().c_str());
								// bold and recursive style for the bug ID itself
								if (idoffset2-idoffset1 > 0)
									Call(SCI_SETSTYLING, idoffset2-idoffset1, STYLE_ISSUEBOLDITALIC);
								idoffset1 = idoffset2;
							}
							else
							{
								// bold style for the rest of the string which isn't matched
								if (offset2-idoffset1 > 0)
									Call(SCI_SETSTYLING, offset2-idoffset1, STYLE_ISSUEBOLD);
							}
						} while((idbr.matched)&&(offset2-idoffset1));
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
		catch ( ... )
		{
			ATLTRACE("exception occurred! most likely a bad regex!\n");
		}
	}
	else
	{
		try
		{
			match_results results;
			match_results::backref_type br;
			do 
			{
				br = m_patCommand.match( reMsg, results, offset1 );

				if( br.matched ) 
				{
					// clear the styles up to the match position
					ATLTRACE(_T("matched string : %s\n"), results.backref(0).str().c_str());
					Call(SCI_SETSTYLING, results.rstart(0), STYLE_DEFAULT);
					ATLTRACE("STYLE_DEFAULT %d chars\n", results.rstart(0));
					offset1 += results.rstart(0);
					{
						ATLTRACE("matched results : %ld\n", results.cbackrefs());
						for (size_t test=0; test<results.cbackrefs(); ++test)
						{
							ATLTRACE(_T("matched (%d): %s\n"), test, results.backref(test).str().c_str());
						}
						// we define group 1 as the whole issue text
						// and group 2 as the issue number
						if ((results.cbackrefs() > 2)&&(results.backref(2).matched))
						{
							if (results.backref(2).str().empty())
								Call(SCI_SETSTYLING, results.rlength(1), STYLE_ISSUEBOLD);
							else
								Call(SCI_SETSTYLING, results.rlength(1)-results.rlength(2), STYLE_ISSUEBOLD);
							Call(SCI_SETSTYLING, results.rlength(2), STYLE_ISSUEBOLDITALIC);
						}
						else if ((results.cbackrefs() > 1)&&(results.backref(1).str().size()>0))
						{
							Call(SCI_SETSTYLING, results.rstart(1)-results.rstart(0), STYLE_ISSUEBOLD);
							Call(SCI_SETSTYLING, results.rlength(1), STYLE_ISSUEBOLDITALIC);
							Call(SCI_SETSTYLING, results.rlength(0)-results.rlength(1)-(results.rstart(1)-results.rstart(0)), STYLE_ISSUEBOLD);
						}
						offset1 += results.rlength(0);
					}
				}
				else
				{
					// bold style for the rest of the string which isn't matched
					if (end_pos > (int)offset1)
						Call(SCI_SETSTYLING, end_pos-offset1, STYLE_DEFAULT);
				}
			} while ((br.matched)&&((int)offset1<end_pos));
			msg.ReleaseBuffer();
			return TRUE;
		}
		catch (bad_alloc) {}
		catch (bad_regexpr){}
		catch ( ... )
		{
			ATLTRACE("exception occurred! most likely a bad regex!\n");
		}
	}
	return FALSE;
}

bool CSciEdit::IsValidURLChar(wchar_t ch)
{
	return isalnum(ch) ||
		ch == '_' || ch == '/' || ch == ';' || ch == '?' || ch == '&' || ch == '=' ||
		ch == '%' || ch == ':' || ch == '.' || ch == '#' || ch == '-' || ch == '+';
}

void CSciEdit::StyleURLs(int startstylepos, int endstylepos) {
	const int line_number = Call(SCI_LINEFROMPOSITION, startstylepos);
	startstylepos = Call(SCI_POSITIONFROMLINE, (WPARAM)line_number);

	int len = endstylepos - startstylepos + 1;
	char* textbuffer = new char[len + 1];
	TEXTRANGEA textrange;
	textrange.lpstrText = textbuffer;
	textrange.chrg.cpMin = startstylepos;
	textrange.chrg.cpMax = endstylepos;
	Call(SCI_GETTEXTRANGE, 0, (LPARAM)&textrange);
	CString msg(textbuffer);
	delete textbuffer;

	int starturl = -1;
	for(int i = 0; i <= len; ++i)
	{
		if ((i < len) && IsValidURLChar(msg[i]))
		{
			if (starturl < 0)
				starturl = i;
		}
		else
		{
			if ((starturl >= 0) && ::PathIsURL(msg.Mid(starturl, i - starturl)))
			{
				ASSERT(startstylepos + i <= endstylepos);
				Call(SCI_STARTSTYLING, startstylepos + starturl, STYLE_MASK);
				Call(SCI_SETSTYLING, i - starturl, STYLE_URL);
			}
			starturl = -1;
		}
	}
}
