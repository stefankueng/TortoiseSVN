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
#include "StdAfx.h"
#include "commctrl.h"
#include "Commdlg.h"
#include "MainWindow.h"

#pragma comment(lib, "comctl32.lib")

stdstring	CMainWindow::leftpicpath;
stdstring	CMainWindow::leftpictitle;

stdstring	CMainWindow::rightpicpath;
stdstring	CMainWindow::rightpictitle;


bool CMainWindow::RegisterAndCreateWindow()
{
	WNDCLASSEX wcx; 

	// Fill in the window class structure with default parameters 
	wcx.cbSize = sizeof(WNDCLASSEX);
	wcx.style = CS_HREDRAW | CS_VREDRAW;
	wcx.lpfnWndProc = CWindow::stWinMsgHandler;
	wcx.cbClsExtra = 0;
	wcx.cbWndExtra = 0;
	wcx.hInstance = hInstance;
	wcx.hCursor = LoadCursor(NULL, IDC_SIZEWE);
	wcx.lpszClassName = ResString(hInstance, IDS_APP_TITLE);
	wcx.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TORTOISEIDIFF));
	wcx.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
	wcx.lpszMenuName = MAKEINTRESOURCE(IDC_TORTOISEIDIFF);
	wcx.hIconSm	= LoadIcon(wcx.hInstance, MAKEINTRESOURCE(IDI_TORTOISEIDIFF));
	if (RegisterWindow(&wcx))
	{
		if (Create(WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_VISIBLE, NULL))
		{
			ShowWindow(m_hwnd, SW_SHOW);
			UpdateWindow(m_hwnd);
			return true;
		}
	}
	return false;
}

void CMainWindow::PositionChildren(RECT * clientrect /* = NULL */)
{
	if (clientrect == NULL)
		return;
	if (bOverlap)
	{
		SetWindowPos(picWindow1, NULL, clientrect->left, clientrect->top+SLIDER_HEIGHT, clientrect->right-clientrect->left, clientrect->bottom-clientrect->top-SLIDER_HEIGHT, SWP_SHOWWINDOW);
		SetWindowPos(hTrackbar, NULL, clientrect->left, clientrect->top, clientrect->right-clientrect->left, SLIDER_HEIGHT, SWP_SHOWWINDOW);
	}
	else
	{
		RECT child;
		child.left = clientrect->left;
		child.top = clientrect->top;
		child.right = nSplitterPos-nSplitterBorder;
		child.bottom = clientrect->bottom;
		SetWindowPos(picWindow1, NULL, child.left, child.top, child.right-child.left, child.bottom-child.top, SWP_FRAMECHANGED|SWP_SHOWWINDOW);
		child.left = nSplitterPos+nSplitterBorder;
		child.right = clientrect->right;
		SetWindowPos(picWindow2, NULL, child.left, child.top, child.right-child.left, child.bottom-child.top, SWP_FRAMECHANGED|SWP_SHOWWINDOW);
	}
	InvalidateRect(*this, NULL, FALSE);
}

LRESULT CALLBACK CMainWindow::WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		{
			m_hwnd = hwnd;
			picWindow1.RegisterAndCreateWindow(hwnd);
			picWindow1.SetPic(leftpicpath, leftpictitle);
			picWindow2.RegisterAndCreateWindow(hwnd);
			picWindow2.SetPic(rightpicpath, rightpictitle);
			// center the splitter
			RECT rect;
			GetClientRect(hwnd, &rect);
			nSplitterPos = (rect.right-rect.left)/2;
			// create a slider control
			hTrackbar = CreateTrackbar(*this, 0, 255);
			ShowWindow(hTrackbar, SW_HIDE);
		}
		break;
	case WM_COMMAND:
		{
			return DoCommand(LOWORD(wParam));
		}
		break;
	case WM_HSCROLL:
		{
			if (bOverlap)
			{
				if (LOWORD(wParam) == TB_THUMBTRACK)
				{
					// while tracking, only redraw every ten ticks
					if ((HIWORD(wParam)%10) == 0)
						picWindow1.SetSecondPicAlpha((BYTE)SendMessage(hTrackbar, TBM_GETPOS, 0, 0));
				}
				else
					picWindow1.SetSecondPicAlpha((BYTE)SendMessage(hTrackbar, TBM_GETPOS, 0, 0));
			}
		}
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc;
			RECT rect;

			::GetClientRect(*this, &rect);
			hdc = BeginPaint(hwnd, &ps);
			SetBkColor(hdc, GetSysColor(COLOR_3DFACE));
			::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);
			EndPaint(hwnd, &ps);
		}
		break;
	case WM_SIZE:
		{
			RECT rect;
			GetClientRect(hwnd, &rect);
			nSplitterPos = (rect.right-rect.left)/2;
			PositionChildren(&rect);
		}
		break;
	case WM_MOVE:
		{
			RECT rect;
			GetClientRect(hwnd, &rect);
			PositionChildren(&rect);
		}
		break;
	case WM_LBUTTONDOWN:
		Splitter_OnLButtonDown(hwnd, uMsg, wParam, lParam);
		break;
	case WM_LBUTTONUP:
		Splitter_OnLButtonUp(hwnd, uMsg, wParam, lParam);
		break;
	case WM_MOUSEMOVE:
		Splitter_OnMouseMove(hwnd, uMsg, wParam, lParam);
		break;
	case WM_DESTROY:
		bWindowClosed = TRUE;
		PostQuitMessage(0);
		break;
	case WM_CLOSE:
		::DestroyWindow(m_hwnd);
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
	case ID_FILE_OPEN:
		{
			if (OpenDialog())
			{
				picWindow1.SetPic(leftpicpath, _T(""));
				picWindow1.FitImageInWindow();
				picWindow2.SetPic(rightpicpath, _T(""));
				picWindow2.FitImageInWindow();
				RECT rect;
				GetClientRect(*this, &rect);
				PositionChildren(&rect);
			}
		}
		break;
	case ID_VIEW_OVERLAPIMAGES:
		{
			bOverlap = !bOverlap;
			HMENU hMenu = GetMenu(*this);
			UINT uCheck = MF_BYCOMMAND;
			uCheck |= bOverlap ? MF_CHECKED : MF_UNCHECKED;
			CheckMenuItem(hMenu, ID_VIEW_OVERLAPIMAGES, uCheck);

			ShowWindow(picWindow2, bOverlap ? SW_HIDE : SW_SHOW);
			ShowWindow(hTrackbar, bOverlap ? SW_SHOW : SW_HIDE);
			if (bOverlap)
			{
				picWindow1.SetSecondPic(picWindow2.GetPic(), rightpictitle, rightpicpath);
				picWindow1.SetSecondPicAlpha(127);
				SendMessage(hTrackbar, TBM_SETPOS, (WPARAM)1, (LPARAM)127);
			}
			else
			{
				picWindow1.SetSecondPic();
			}

			RECT rect;
			GetClientRect(*this, &rect);
			PositionChildren(&rect);
			return 0;
		}
		break;
	case ID_VIEW_FITIMAGESINWINDOW:
		{
			picWindow1.FitImageInWindow();
			picWindow2.FitImageInWindow();
			RECT rect;
			GetClientRect(*this, &rect);
			PositionChildren(&rect);
		}
		break;
	case ID_VIEW_ORININALSIZE:
		{
			picWindow1.SetZoom(1.0);
			picWindow2.SetZoom(1.0);
			RECT rect;
			GetClientRect(*this, &rect);
			PositionChildren(&rect);
		}
		break;
	case IDM_EXIT:
		::PostQuitMessage(0);
		return 0;
		break;
	default:
		break;
	};
	return 1;
}

// splitter stuff
void CMainWindow::DrawXorBar(HDC hdc, int x1, int y1, int width, int height)
{
	static WORD _dotPatternBmp[8] = 
	{ 
		0x0055, 0x00aa, 0x0055, 0x00aa, 
		0x0055, 0x00aa, 0x0055, 0x00aa
	};

	HBITMAP hbm;
	HBRUSH  hbr, hbrushOld;

	hbm = CreateBitmap(8, 8, 1, 1, _dotPatternBmp);
	hbr = CreatePatternBrush(hbm);

	SetBrushOrgEx(hdc, x1, y1, 0);
	hbrushOld = (HBRUSH)SelectObject(hdc, hbr);

	PatBlt(hdc, x1, y1, width, height, PATINVERT);

	SelectObject(hdc, hbrushOld);

	DeleteObject(hbr);
	DeleteObject(hbm);
}

LRESULT CMainWindow::Splitter_OnLButtonDown(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pt;
	HDC hdc;
	RECT rect;
	RECT clientrect;

	pt.x = (short)LOWORD(lParam);  // horizontal position of cursor 
	pt.y = (short)HIWORD(lParam);

	GetClientRect(hwnd, &clientrect);
	GetWindowRect(hwnd, &rect);
	POINT zero = {0,0};
	ClientToScreen(hwnd, &zero);
	OffsetRect(&clientrect, zero.x-rect.left, zero.y-rect.top);

	//convert the mouse coordinates relative to the top-left of
	//the window
	ClientToScreen(hwnd, &pt);
	pt.x -= rect.left;
	pt.y -= rect.top;

	//same for the window coordinates - make them relative to 0,0
	OffsetRect(&rect, -rect.left, -rect.top);

	if (pt.x < 0) 
	{
		pt.x = 0;
	}
	if (pt.x > rect.right-4) 
	{
		pt.x = rect.right-4;
	}

	bDragMode = true;

	SetCapture(hwnd);

	hdc = GetWindowDC(hwnd);
	DrawXorBar(hdc, pt.x+2, clientrect.top, 4, clientrect.bottom-clientrect.top-2);
	ReleaseDC(hwnd, hdc);

	oldx = pt.x;

	return 0;
}


LRESULT CMainWindow::Splitter_OnLButtonUp(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	RECT rect;
	RECT clientrect;

	POINT pt;
	pt.x = (short)LOWORD(lParam);  // horizontal position of cursor 
	pt.y = (short)HIWORD(lParam);

	if (bDragMode == FALSE)
		return 0;

	GetClientRect(hwnd, &clientrect);
	GetWindowRect(hwnd, &rect);
	POINT zero = {0,0};
	ClientToScreen(hwnd, &zero);
	OffsetRect(&clientrect, zero.x-rect.left, zero.y-rect.top);

	ClientToScreen(hwnd, &pt);
	pt.x -= rect.left;
	pt.y -= rect.top;

	OffsetRect(&rect, -rect.left, -rect.top);

	if (pt.x < 0)
	{
		pt.x = 0;
	}
	if (pt.x > rect.right-4) 
	{
		pt.x = rect.right-4;
	}

	hdc = GetWindowDC(hwnd);
	DrawXorBar(hdc, oldx+2, clientrect.top, 4, clientrect.bottom-clientrect.top-2);			
	ReleaseDC(hwnd, hdc);

	oldx = pt.x;

	bDragMode = false;

	//convert the splitter position back to screen coords.
	GetWindowRect(hwnd, &rect);
	pt.x += rect.left;
	pt.y += rect.top;

	//now convert into CLIENT coordinates
	ScreenToClient(hwnd, &pt);
	GetClientRect(hwnd, &rect);
	nSplitterPos = pt.x;

	//position the child controls
	PositionChildren(&rect);

	ReleaseCapture();

	return 0;
}

LRESULT CMainWindow::Splitter_OnMouseMove(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	RECT rect;
	RECT clientrect;

	POINT pt;

	if (bDragMode == FALSE)
		return 0;

	pt.x = (short)LOWORD(lParam);  // horizontal position of cursor 
	pt.y = (short)HIWORD(lParam);

	GetClientRect(hwnd, &clientrect);
	GetWindowRect(hwnd, &rect);
	POINT zero = {0,0};
	ClientToScreen(hwnd, &zero);
	OffsetRect(&clientrect, zero.x-rect.left, zero.y-rect.top);

	if (pt.x < 0)
	{
		pt.x = 0;
	}
	if (pt.x > rect.right-4) 
	{
		pt.x = rect.right-4;
	}

	if (pt.x != oldx && wParam & MK_LBUTTON)
	{
		hdc = GetWindowDC(hwnd);

		DrawXorBar(hdc, oldx+2, clientrect.top, 4, clientrect.bottom-clientrect.top-2);
		DrawXorBar(hdc, pt.x+2, clientrect.top, 4, clientrect.bottom-clientrect.top-2);

		ReleaseDC(hwnd, hdc);

		oldx = pt.x;
	}

	return 0;
}

HWND CMainWindow::CreateTrackbar(HWND hwndParent, UINT iMin, UINT iMax)
{ 
	InitCommonControls();

	HWND hwndTrack = CreateWindowEx( 
		0,									// no extended styles 
		TRACKBAR_CLASS,						// class name 
		_T("Trackbar Control"),				// title (caption) 
		WS_CHILD | WS_VISIBLE | TBS_HORZ,	// style 
		10, 10,								// position 
		200, 30,							// size 
		hwndParent,							// parent window 
		(HMENU)TRACKBAR_ID,					// control identifier 
		hInstance,							// instance 
		NULL								// no WM_CREATE parameter 
		); 

	SendMessage(hwndTrack, TBM_SETRANGE, 
		(WPARAM) TRUE,						// redraw flag 
		(LPARAM) MAKELONG(iMin, iMax));		// min. & max. positions 

	return hwndTrack; 
}

bool CMainWindow::OpenDialog()
{
	return (DialogBox(hInstance, MAKEINTRESOURCE(IDD_OPEN), *this, (DLGPROC)OpenDlgProc)==IDOK);
}

BOOL CALLBACK CMainWindow::OpenDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
	case WM_INITDIALOG:
		{
			// center on the parent window
			HWND hParentWnd = ::GetParent(hwndDlg);
			RECT parentrect, childrect, centeredrect;
			GetWindowRect(hParentWnd, &parentrect);
			GetWindowRect(hwndDlg, &childrect);
			centeredrect.left = parentrect.left + ((parentrect.right-parentrect.left-childrect.right+childrect.left)/2);
			centeredrect.right = centeredrect.left + (childrect.right-childrect.left);
			centeredrect.top = parentrect.top + ((parentrect.bottom-parentrect.top-childrect.bottom+childrect.top)/2);
			centeredrect.bottom = centeredrect.top + (childrect.bottom-childrect.top);
			SetWindowPos(hwndDlg, NULL, centeredrect.left, centeredrect.top, centeredrect.right-centeredrect.left, centeredrect.bottom-centeredrect.top, SWP_SHOWWINDOW);
		}
		break;
	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{
		case IDC_LEFTBROWSE:
			{
				TCHAR path[MAX_PATH] = {0};
				if (AskForFile(hwndDlg, path))
				{
					SetDlgItemText(hwndDlg, IDC_LEFTIMAGE, path);
				}
			}
			break;
		case IDC_RIGHTBROWSE:
			{
				TCHAR path[MAX_PATH] = {0};
				if (AskForFile(hwndDlg, path))
				{
					SetDlgItemText(hwndDlg, IDC_RIGHTIMAGE, path);
				}
			}
			break;
		case IDOK: 
			{
				TCHAR path[MAX_PATH];
				if (!GetDlgItemText(hwndDlg, IDC_LEFTIMAGE, path, MAX_PATH)) 
					*path = 0;
				leftpicpath = path;
				if (!GetDlgItemText(hwndDlg, IDC_RIGHTIMAGE, path, MAX_PATH))
					*path = 0;
				rightpicpath = path;
			}
			// Fall through. 
		case IDCANCEL: 
			EndDialog(hwndDlg, wParam); 
			return TRUE; 
		} 
	} 
	return FALSE; 
}

bool CMainWindow::AskForFile(HWND owner, TCHAR * path)
{
	OPENFILENAME ofn;		// common dialog box structure
	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = owner;
	ofn.lpstrFile = path;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = ResString(hInst, IDS_OPENIMAGEFILE);
	ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_EXPLORER;
	ofn.hInstance = hInst;
//Pictures (*.wmf, *.jpg, *.png, *.bmp, *.gif)|*.wmf;*.jpg;*.jpeg;*.png;*.bmp;*.gif|All (*.*)|*.*||
	TCHAR filters[] = _T("Images\0*.wmf;*.jpg;*jpeg;*.bmp;*.gif;*.png;*.ico\0All (*.*)\0*.*\0\0");
	ofn.lpstrFilter = filters;
	ofn.nFilterIndex = 1;
	// Display the Open dialog box. 
	if (GetOpenFileName(&ofn)==FALSE)
	{
		return false;
	}
	return true;
}