// TortoiseBlame - a Viewer for Subversion Blames

// Copyright (C) 2003-2006 - Stefan Kueng

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
#pragma once

#include "resource.h"
#include "Commdlg.h"
#include "Scintilla.h"
#include "SciLexer.h"

const COLORREF black = RGB(0,0,0);
const COLORREF white = RGB(0xff,0xff,0xff);
const COLORREF red = RGB(0xFF, 0, 0);
const COLORREF offWhite = RGB(0xFF, 0xFB, 0xF0);
const COLORREF darkGreen = RGB(0, 0x80, 0);
const COLORREF darkBlue = RGB(0, 0, 0x80);
const COLORREF lightBlue = RGB(0xA6, 0xCA, 0xF0);
const int blockSize = 128 * 1024;

#define BLAMESPACE 20
#define HEADER_HEIGHT 18

#define MAX_LOG_LENGTH 2000


#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))
#endif


class TortoiseBlame
{
public:
	TortoiseBlame();
	~TortoiseBlame();

	HINSTANCE hInstance;
	HINSTANCE hResource;
	HWND currentDialog;
	HWND wMain;
	HWND wEditor;
	HWND wBlame;
	HWND wHeader;
	HWND hwndTT;

	LRESULT SendEditor(UINT Msg, WPARAM wParam=0, LPARAM lParam=0);

	void GetRange(int start, int end, char *text);

	void SetTitle();
	BOOL OpenFile(const char *fileName);
	BOOL OpenLogFile(const char *fileName);

	void Command(int id);
	void Notify(SCNotification *notification);

	void SetAStyle(int style, COLORREF fore, COLORREF back=::GetSysColor(COLOR_WINDOW), int size=-1, const char *face=0);
	void InitialiseEditor();
	LONG GetBlameWidth();
	void DrawBlame(HDC hDC);
	void DrawHeader(HDC hDC);
	void StartSearch();
	void CopySelectedLogToClipboard();
	void BlamePreviousRevision();
	void DiffPreviousRevision();
	bool DoSearch(LPSTR what, DWORD flags);
	bool GotoLine(long line);
	void GotoLineDlg();
	static INT_PTR CALLBACK GotoDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void SetSelectedLine(LONG line) { m_SelectedLine=line;};

	LONG						m_mouserev;
	std::string					m_mouseauthor;
	LONG						m_selectedrev;
	std::string					m_selectedauthor;
	std::string					m_selecteddate;
	static long					m_gotoline;

	std::vector<LONG>			revs;
	std::vector<std::string>	dates;
	std::vector<std::string>	authors;
	std::map<LONG, std::string>	logmessages;
	char						m_szTip[MAX_LOG_LENGTH*2+5];
	wchar_t						m_wszTip[MAX_LOG_LENGTH*2+5];
	void StringExpand(LPSTR str);
	void StringExpand(LPWSTR str);
	BOOL						ttVisible;
protected:
	void CreateFont();
	void SetupLexer(LPCSTR filename);
	void SetupCppLexer();
	COLORREF InterColor(COLORREF c1, COLORREF c2, int Slider);
	std::vector<COLORREF>		colors;
	HFONT						m_font;
	LONG						m_blamewidth;
	LONG						m_revwidth;
	LONG						m_datewidth;
	LONG						m_authorwidth;
	LONG						m_linewidth;
	LONG						m_SelectedLine; ///< zero-based

	COLORREF					m_mouserevcolor;
	COLORREF					m_mouseauthorcolor;
	COLORREF					m_selectedrevcolor;
	COLORREF					m_selectedauthorcolor;
	COLORREF					m_windowcolor;
	COLORREF					m_textcolor;
	COLORREF					m_texthighlightcolor;

	LRESULT						m_directFunction;
	LRESULT						m_directPointer;
	FINDREPLACE					fr;
	TCHAR						szFindWhat[80];
};
