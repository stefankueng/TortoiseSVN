// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - Stefan Kueng

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
#include "MainWindow.h"
#include "UnicodeUtils.h"

CMainWindow::CMainWindow(HINSTANCE hInst, const WNDCLASSEX* wcx /* = NULL*/) 
	: CWindow(hInst, wcx)
	, m_bShowFindBar(false)
{
	SetWindowTitle(_T("TortoiseUDiff"));
}

CMainWindow::~CMainWindow(void)
{
}

bool CMainWindow::RegisterAndCreateWindow()
{
	WNDCLASSEX wcx; 

	// Fill in the window class structure with default parameters 
	wcx.cbSize = sizeof(WNDCLASSEX);
	wcx.style = CS_HREDRAW | CS_VREDRAW;
	wcx.lpfnWndProc = CWindow::stWinMsgHandler;
	wcx.cbClsExtra = 0;
	wcx.cbWndExtra = 0;
	wcx.hInstance = hResource;
	wcx.hCursor = NULL;
	wcx.lpszClassName = ResString(hResource, IDS_APP_TITLE);
	wcx.hIcon = LoadIcon(hResource, MAKEINTRESOURCE(IDI_TORTOISEUDIFF));
	wcx.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
	wcx.lpszMenuName = MAKEINTRESOURCE(IDC_TORTOISEUDIFF);
	wcx.hIconSm	= LoadIcon(wcx.hInstance, MAKEINTRESOURCE(IDI_TORTOISEUDIFF));
	if (RegisterWindow(&wcx))
	{
		if (Create(WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SIZEBOX | WS_SYSMENU | WS_CLIPCHILDREN, NULL))
		{
			m_FindBar.SetParent(*this);
			m_FindBar.Create(hResource, IDD_FINDBAR, *this);
			ShowWindow(*this, SW_SHOW);
			UpdateWindow(*this);
			return true;
		}
	}
	return false;
}

LRESULT CALLBACK CMainWindow::WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		{
			m_hwnd = hwnd;
			Initialize();
		}
		break;
	case WM_COMMAND:
		{
			return DoCommand(LOWORD(wParam));
		}
		break;
	case WM_MOUSEWHEEL:
		{
			if (GET_KEYSTATE_WPARAM(wParam) == MK_SHIFT)
			{
				// scroll sideways
				SendEditor(SCI_LINESCROLL, -GET_WHEEL_DELTA_WPARAM(wParam)/40, 0);
			}
			else
				return DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
		break;
	case WM_SIZE:
		{
			RECT rect;
			GetClientRect(*this, &rect);
			if (m_bShowFindBar)
			{
				::SetWindowPos(m_hWndEdit, HWND_TOP, 
					rect.left, rect.top,
					rect.right-rect.left, rect.bottom-rect.top-30,
					SWP_SHOWWINDOW);
				::SetWindowPos(m_FindBar, HWND_TOP,
					rect.left, rect.bottom-30,
					rect.right-rect.left, 30,
					SWP_SHOWWINDOW);
			}
			else
			{
				::SetWindowPos(m_hWndEdit, HWND_TOP, 
					rect.left, rect.top,
					rect.right-rect.left, rect.bottom-rect.top,
					SWP_SHOWWINDOW);
				::ShowWindow(m_FindBar, SW_HIDE);
			}
		}
		break;
	case WM_GETMINMAXINFO:
		{
			MINMAXINFO * mmi = (MINMAXINFO*)lParam;
			mmi->ptMinTrackSize.x = 100;
			mmi->ptMinTrackSize.y = 100;
			return 0;
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_CLOSE:
		{
			CRegStdWORD w = CRegStdWORD(_T("Software\\TortoiseSVN\\UDiffViewerWidth"), (DWORD)CW_USEDEFAULT);
			CRegStdWORD h = CRegStdWORD(_T("Software\\TortoiseSVN\\UDiffViewerHeight"), (DWORD)CW_USEDEFAULT);
			RECT rect;
			::GetWindowRect(*this, &rect);
			w = rect.right-rect.left;
			h = rect.bottom-rect.top;
		}
		::DestroyWindow(m_hwnd);
		break;
	case COMMITMONITOR_FINDMSGNEXT:
		{
			SendEditor(SCI_CHARRIGHT);
			SendEditor(SCI_SEARCHANCHOR);
			m_bMatchCase = !!wParam;
			m_findtext = (LPCTSTR)lParam;
			SendEditor(SCI_SEARCHNEXT, m_bMatchCase ? SCFIND_MATCHCASE : 0, (LPARAM)CUnicodeUtils::StdGetUTF8(m_findtext).c_str());
			SendEditor(SCI_SCROLLCARET);
		}
		break;
	case COMMITMONITOR_FINDMSGPREV:
		{
			SendEditor(SCI_SEARCHANCHOR);
			m_bMatchCase = !!wParam;
			m_findtext = (LPCTSTR)lParam;
			SendEditor(SCI_SEARCHPREV, m_bMatchCase ? SCFIND_MATCHCASE : 0, (LPARAM)CUnicodeUtils::StdGetUTF8(m_findtext).c_str());
			SendEditor(SCI_SCROLLCARET);
		}
		break;
	case COMMITMONITOR_FINDEXIT:
		{
			RECT rect;
			GetClientRect(*this, &rect);
			m_bShowFindBar = false;
			::ShowWindow(m_FindBar, SW_HIDE);
			::SetWindowPos(m_hWndEdit, HWND_TOP, 
				rect.left, rect.top,
				rect.right-rect.left, rect.bottom-rect.top,
				SWP_SHOWWINDOW);
		}
		break;
	case COMMITMONITOR_FINDRESET:
		SendEditor(SCI_SETSELECTIONSTART, 0);
		SendEditor(SCI_SETSELECTIONEND, 0);
		SendEditor(SCI_SEARCHANCHOR);
		break;
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	return 0;
};

LRESULT CMainWindow::DoCommand(int id)
{
	switch (id) 
	{
	case ID_FILE_EXIT:
		::PostQuitMessage(0);
		return 0;
	case IDM_SHOWFINDBAR:
		{
			m_bShowFindBar = true;
			::ShowWindow(m_FindBar, SW_SHOW);
			RECT rect;
			GetClientRect(*this, &rect);
			::SetWindowPos(m_hWndEdit, HWND_TOP, 
				rect.left, rect.top,
				rect.right-rect.left, rect.bottom-rect.top-30,
				SWP_SHOWWINDOW);
			::SetWindowPos(m_FindBar, HWND_TOP,
				rect.left, rect.bottom-30,
				rect.right-rect.left, 30,
				SWP_SHOWWINDOW);
			::SetFocus(m_FindBar);
			SendEditor(SCI_SETSELECTIONSTART, 0);
			SendEditor(SCI_SETSELECTIONEND, 0);
			SendEditor(SCI_SEARCHANCHOR);
		}
		break;
	case IDM_FINDNEXT:
		SendEditor(SCI_CHARRIGHT);
		SendEditor(SCI_SEARCHANCHOR);
		SendEditor(SCI_SEARCHNEXT, m_bMatchCase ? SCFIND_MATCHCASE : 0, (LPARAM)CUnicodeUtils::StdGetUTF8(m_findtext).c_str());
		SendEditor(SCI_SCROLLCARET);
		break;
	case IDM_FINDPREV:
		SendEditor(SCI_SEARCHANCHOR);
		SendEditor(SCI_SEARCHPREV, m_bMatchCase ? SCFIND_MATCHCASE : 0, (LPARAM)CUnicodeUtils::StdGetUTF8(m_findtext).c_str());
		SendEditor(SCI_SCROLLCARET);
		break;
	case IDM_FINDEXIT:
		{
			if (IsWindowVisible(m_FindBar))
			{
				RECT rect;
				GetClientRect(*this, &rect);
				m_bShowFindBar = false;
				::ShowWindow(m_FindBar, SW_HIDE);
				::SetWindowPos(m_hWndEdit, HWND_TOP, 
					rect.left, rect.top,
					rect.right-rect.left, rect.bottom-rect.top,
					SWP_SHOWWINDOW);
			}
			else
				PostQuitMessage(0);
		}
		break;
	default:
		break;
	};
	return 1;
}


LRESULT CMainWindow::SendEditor(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (m_directFunction)
	{
		return ((SciFnDirect) m_directFunction)(m_directPointer, Msg, wParam, lParam);
	}
	return ::SendMessage(m_hWndEdit, Msg, wParam, lParam);	
}

bool CMainWindow::Initialize()
{
	::SetWindowPos(*this, HWND_TOP, 0, 0, 
		(int)(DWORD)CRegStdWORD(_T("Software\\TortoiseSVN\\UDiffViewerWidth"), (DWORD)640), 
		(int)(DWORD)CRegStdWORD(_T("Software\\TortoiseSVN\\UDiffViewerHeight"), (DWORD)480),
		SWP_NOMOVE);

	m_hWndEdit = ::CreateWindow(
		_T("Scintilla"),
		_T("Source"),
		WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_CLIPCHILDREN,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		*this,
		0,
		hResource,
		0);
	if (m_hWndEdit == NULL)
		return false;

	RECT rect;
	GetClientRect(*this, &rect);
	::SetWindowPos(m_hWndEdit, HWND_TOP, 
		rect.left, rect.top,
		rect.right-rect.left, rect.bottom-rect.top,
		SWP_SHOWWINDOW);

	m_directFunction = SendMessage(m_hWndEdit, SCI_GETDIRECTFUNCTION, 0, 0);
	m_directPointer = SendMessage(m_hWndEdit, SCI_GETDIRECTPOINTER, 0, 0);

	// Set up the global default style. These attributes are used wherever no explicit choices are made.
	SetAStyle(STYLE_DEFAULT, ::GetSysColor(COLOR_WINDOWTEXT), ::GetSysColor(COLOR_WINDOW), 10, "Courier New");
	SendEditor(SCI_SETTABWIDTH, 4);
	SendEditor(SCI_SETREADONLY, TRUE);
	LRESULT pix = SendEditor(SCI_TEXTWIDTH, STYLE_LINENUMBER, (LPARAM)"_99999");
	SendEditor(SCI_SETMARGINWIDTHN, 0, pix);
	SendEditor(SCI_SETMARGINWIDTHN, 1);
	SendEditor(SCI_SETMARGINWIDTHN, 2);
	//Set the default windows colors for edit controls
	SendEditor(SCI_STYLESETFORE, STYLE_DEFAULT, ::GetSysColor(COLOR_WINDOWTEXT));
	SendEditor(SCI_STYLESETBACK, STYLE_DEFAULT, ::GetSysColor(COLOR_WINDOW));
	SendEditor(SCI_SETSELFORE, TRUE, ::GetSysColor(COLOR_HIGHLIGHTTEXT));
	SendEditor(SCI_SETSELBACK, TRUE, ::GetSysColor(COLOR_HIGHLIGHT));
	SendEditor(SCI_SETCARETFORE, ::GetSysColor(COLOR_WINDOWTEXT));

	return true;
}

bool CMainWindow::LoadFile(LPCTSTR filename)
{
	SendEditor(SCI_SETREADONLY, FALSE);
	SendEditor(SCI_CLEARALL);
	SendEditor(EM_EMPTYUNDOBUFFER);
	SendEditor(SCI_SETSAVEPOINT);
	SendEditor(SCI_CANCEL);
	SendEditor(SCI_SETUNDOCOLLECTION, 0);

	FILE *fp = NULL;
	_tfopen_s(&fp, filename, _T("rb"));
	if (fp) 
	{
		//SetTitle();
		char data[4096];
		int lenFile = fread(data, 1, sizeof(data), fp);
		bool bUTF8 = IsUTF8(data, lenFile);
		while (lenFile > 0) 
		{
			SendEditor(SCI_ADDTEXT, lenFile,
				reinterpret_cast<LPARAM>(static_cast<char *>(data)));
			lenFile = fread(data, 1, sizeof(data), fp);
		}
		fclose(fp);
		SendEditor(SCI_SETCODEPAGE, bUTF8 ? SC_CP_UTF8 : GetACP());
	}
	else 
	{
		return false;
	}

	SendEditor(SCI_SETUNDOCOLLECTION, 1);
	::SetFocus(m_hWndEdit);
	SendEditor(EM_EMPTYUNDOBUFFER);
	SendEditor(SCI_SETSAVEPOINT);
	SendEditor(SCI_GOTOPOS, 0);
	SendEditor(SCI_SETREADONLY, TRUE);

	SendEditor(SCI_CLEARDOCUMENTSTYLE, 0, 0);
	SendEditor(SCI_SETSTYLEBITS, 5, 0);

	//SetAStyle(SCE_DIFF_DEFAULT, RGB(0, 0, 0));
	SetAStyle(SCE_DIFF_COMMAND, RGB(0x0A, 0x24, 0x36));
	SetAStyle(SCE_DIFF_POSITION, RGB(0xFF, 0, 0));
	SetAStyle(SCE_DIFF_HEADER, RGB(0x80, 0, 0), RGB(0xFF, 0xFF, 0x80));
	SetAStyle(SCE_DIFF_COMMENT, RGB(0, 0x80, 0));
	SendEditor(SCI_STYLESETBOLD, SCE_DIFF_COMMENT, TRUE);
	SetAStyle(SCE_DIFF_DELETED, ::GetSysColor(COLOR_WINDOWTEXT), RGB(0xFF, 0x80, 0x80));
	SetAStyle(SCE_DIFF_ADDED, ::GetSysColor(COLOR_WINDOWTEXT), RGB(0x80, 0xFF, 0x80));

	SendEditor(SCI_SETLEXER, SCLEX_DIFF);
	SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)"revision");
	SendEditor(SCI_COLOURISE, 0, -1);
	::ShowWindow(m_hWndEdit, SW_SHOW);
	return true;
}

void CMainWindow::SetTitle(LPCTSTR title)
{
	int len = _tcslen(title);
	TCHAR * pBuf = new TCHAR[len+40];
	_stprintf_s(pBuf, len+40, _T("%s - TortoiseUDiff"), title);
	SetWindowTitle(std::wstring(pBuf));
	delete [] pBuf;
}

void CMainWindow::SetAStyle(int style, COLORREF fore, COLORREF back, int size, const char *face) 
{
	SendEditor(SCI_STYLESETFORE, style, fore);
	SendEditor(SCI_STYLESETBACK, style, back);
	if (size >= 1)
		SendEditor(SCI_STYLESETSIZE, style, size);
	if (face) 
		SendEditor(SCI_STYLESETFONT, style, reinterpret_cast<LPARAM>(face));
}

bool CMainWindow::IsUTF8(LPVOID pBuffer, int cb)
{
	if (cb < 2)
		return true;
	UINT16 * pVal = (UINT16 *)pBuffer;
	UINT8 * pVal2 = (UINT8 *)(pVal+1);
	// scan the whole buffer for a 0x0000 sequence
	// if found, we assume a binary file
	for (int i=0; i<(cb-2); i=i+2)
	{
		if (0x0000 == *pVal++)
			return false;
	}
	pVal = (UINT16 *)pBuffer;
	if (*pVal == 0xFEFF)
		return false;
	if (cb < 3)
		return false;
	if (*pVal == 0xBBEF)
	{
		if (*pVal2 == 0xBF)
			return true;
	}
	// check for illegal UTF8 chars
	pVal2 = (UINT8 *)pBuffer;
	for (int i=0; i<cb; ++i)
	{
		if ((*pVal2 == 0xC0)||(*pVal2 == 0xC1)||(*pVal2 >= 0xF5))
			return false;
		pVal2++;
	}
	pVal2 = (UINT8 *)pBuffer;
	bool bUTF8 = false;
	for (int i=0; i<(cb-3); ++i)
	{
		if ((*pVal2 & 0xE0)==0xC0)
		{
			pVal2++;i++;
			if ((*pVal2 & 0xC0)!=0x80)
				return false;
			bUTF8 = true;
		}
		if ((*pVal2 & 0xF0)==0xE0)
		{
			pVal2++;i++;
			if ((*pVal2 & 0xC0)!=0x80)
				return false;
			pVal2++;i++;
			if ((*pVal2 & 0xC0)!=0x80)
				return false;
			bUTF8 = true;
		}
		if ((*pVal2 & 0xF8)==0xF0)
		{
			pVal2++;i++;
			if ((*pVal2 & 0xC0)!=0x80)
				return false;
			pVal2++;i++;
			if ((*pVal2 & 0xC0)!=0x80)
				return false;
			pVal2++;i++;
			if ((*pVal2 & 0xC0)!=0x80)
				return false;
			bUTF8 = true;
		}
		pVal2++;
	}
	if (bUTF8)
		return true;
	return false;
}
