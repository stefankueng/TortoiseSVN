// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008, 2010 - TortoiseSVN

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
#include "TortoiseUDiff.h"
#include "MainWindow.h"
#include "CmdLineParser.h"

#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")


#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(nCmdShow);

	MSG msg;
	HACCEL hAccelTable;

	CCmdLineParser parser(lpCmdLine);

	if (parser.HasKey(_T("?")) || parser.HasKey(_T("help")) ||(!parser.HasKey(_T("patchfile")) && !parser.HasKey(_T("p"))))
	{
		TCHAR buf[1024];
		LoadString(hInstance, IDS_COMMANDLINEHELP, buf, sizeof(buf)/sizeof(TCHAR));
		MessageBox(NULL, buf, _T("TortoiseUDiff"), MB_ICONINFORMATION);
		return 0;
	}

	INITCOMMONCONTROLSEX used = {
		sizeof(INITCOMMONCONTROLSEX),
		ICC_STANDARD_CLASSES | ICC_BAR_CLASSES
	};
	InitCommonControlsEx(&used);


	if (::LoadLibrary(_T("SciLexer.DLL")) == NULL)
		return FALSE;

	CMainWindow mainWindow(hInstance);
	mainWindow.SetRegistryPath(_T("Software\\TortoiseSVN\\UDiffViewerWindowPos"));
	if (parser.HasVal(_T("title")))
		mainWindow.SetTitle(parser.GetVal(_T("title")));
	else if (parser.HasVal(_T("patchfile")))
		mainWindow.SetTitle(parser.GetVal(_T("patchfile")));
	else
		mainWindow.SetTitle(_T("diff from pipe"));

	if (mainWindow.RegisterAndCreateWindow())
	{
		bool bLoadedSuccessfully = false;
		if (parser.HasKey(_T("p")))
		{
			// input from console pipe
			// set console to raw mode
			DWORD oldMode;
			GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &oldMode);
			SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), oldMode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT));

			bLoadedSuccessfully = mainWindow.LoadFile(GetStdHandle(STD_INPUT_HANDLE));
		}
		else if (parser.HasVal(_T("patchfile")))
			bLoadedSuccessfully = mainWindow.LoadFile(parser.GetVal(_T("patchfile")));
		if (bLoadedSuccessfully)
		{
			::ShowWindow(mainWindow.GetHWNDEdit(), SW_SHOW);
			::SetFocus(mainWindow.GetHWNDEdit());

			hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TORTOISEUDIFF));

			// Main message loop:
			while (GetMessage(&msg, NULL, 0, 0))
			{
				if (!TranslateAccelerator(mainWindow, hAccelTable, &msg))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
			return (int) msg.wParam;
		}
	}
	return 0;
}


