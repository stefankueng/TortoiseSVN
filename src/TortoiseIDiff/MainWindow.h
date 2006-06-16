// TortoiseIDiff - an image diff viewer in TortoiseSVN

// Copyright (C) 2006 - Stefan Kueng

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
#include "BaseWindow.h"
#include "PicWindow.h"
#include "commctrl.h"
#include "TortoiseIDiff.h"

#define SLIDER_HEIGHT 30
#define SPLITTER_BORDER 2
#define TRACKBAR_ID 101
#define TIMER_ALPHASLIDER 100

#define WINDOW_MINHEIGTH 200
#define WINDOW_MINWIDTH 200

/**
 * The main window of TortoiseIDiff.
 * Hosts the two image views, the menu, toolbar, slider, ...
 */
class CMainWindow : public CWindow
{
public:
	CMainWindow(HINSTANCE hInst, const WNDCLASSEX* wcx = NULL) : CWindow(hInst, wcx)
		, picWindow1(hInst)
		, picWindow2(hInst)
		, oldx(-4)
		, bMoved(false)
		, bDragMode(false)
		, nSplitterPos(100)
		, bOverlap(false)
		, bShowInfo(true)
	{ 
		SetWindowTitle((LPCTSTR)ResString(hInstance, IDS_APP_TITLE));
	};

	/**
	 * Registers the window class and creates the window.
	 */
	bool RegisterAndCreateWindow();

	/**
	 * Sets the image path and title for the left image view.
	 */
	void SetLeft(stdstring leftpath, stdstring lefttitle) {leftpicpath=leftpath; leftpictitle=lefttitle;}
	/**
	 * Sets the image path and the title for the right image view.
	 */
	void SetRight(stdstring rightpath, stdstring righttitle) {rightpicpath=rightpath; rightpictitle=righttitle;}

protected:
	/// the message handler for this window
	LRESULT CALLBACK	WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	/// Handles all the WM_COMMAND window messages (e.g. menu commands)
	LRESULT				DoCommand(int id);

	/// Positions the child windows. Call this after the window sizes/positions have changed.
	void				PositionChildren(RECT * clientrect = NULL);
	/// Creates the trackbar (the alpha blending slider control)
	HWND				CreateTrackbar(HWND hwndParent, UINT iMin, UINT iMax);
	/// Shows the "Open images" dialog where the user can select the images to diff
	bool				OpenDialog();
	static BOOL CALLBACK OpenDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
	static bool			AskForFile(HWND owner, TCHAR * path);

	// splitter methods
	void				DrawXorBar(HDC hdc, int x1, int y1, int width, int height);
	LRESULT				Splitter_OnLButtonDown(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
	LRESULT				Splitter_OnLButtonUp(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
	LRESULT				Splitter_OnMouseMove(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

	// toolbar
	bool				CreateToolbar();
	HWND				hwndTB;
	HIMAGELIST			hToolbarImgList;

	// command line params
	static stdstring	leftpicpath;
	static stdstring	leftpictitle;

	static stdstring	rightpicpath;
	static stdstring	rightpictitle;

	// image data
	CPicWindow		picWindow1;
	CPicWindow		picWindow2;
	bool			bShowInfo;

	// splitter data
	int				oldx;
	bool			bMoved;
	bool			bDragMode;
	int				nSplitterPos;

	// one/two pane view
	bool			bOverlap;
	HWND			hTrackbar;

};

