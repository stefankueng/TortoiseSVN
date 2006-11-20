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
#include "commctrl.h"
#include "BaseWindow.h"
#include "TortoiseIDiff.h"
#include "Picture.h"

#define HEADER_HEIGHT 30

#define ID_ANIMATIONTIMER 100
#define TIMER_ALPHASLIDER 101

#define LEFTBUTTON_ID	101
#define RIGHTBUTTON_ID	102
#define PLAYBUTTON_ID	103

#define TRACKBAR_ID 101
#define SLIDER_HEIGHT 30
#define SLIDER_WIDTH 30


/**
 * The image view window.
 * Shows an image and provides methods to scale the image or alpha blend it
 * over another image.
 */
class CPicWindow : public CWindow
{
public:
	CPicWindow(HINSTANCE hInst, const WNDCLASSEX* wcx = NULL) : CWindow(hInst, wcx)
		, bValid(false)
		, bFirstpaint(true)
		, nHScrollPos(0)
		, nVScrollPos(0)
		, picscale(1.0)
		, pSecondPic(NULL)
		, alpha(255)
		, bShowInfo(true)
		, nDimensions(0)
		, nCurrentDimension(1)
		, nFrames(0)
		, nCurrentFrame(1)
		, bPlaying(false)
		, pTheOtherPic(NULL)
		, bLinked(true)
		, hwndAlphaSlider(NULL)
	{ 
		SetWindowTitle(_T("Picture Window"));
	};

	/// Registers the window class and creates the window
	bool RegisterAndCreateWindow(HWND hParent);

	/// Sets the image path and title to show
	void SetPic(stdstring path, stdstring title);
	/// Returns the CPicture image object. Used to get an already loaded image
	/// object without having to load it again.
	CPicture * GetPic() {return &picture;}
	/// Sets the path and title of the second image which is alpha blended over the original
	void SetSecondPic(CPicture * pPicture = NULL, const stdstring& sectit = _T(""), const stdstring& secpath = _T(""))
	{
		pSecondPic = pPicture;
		pictitle2 = sectit;
		picpath2 = secpath;
	}

	void StopTimer() {KillTimer(*this, ID_ANIMATIONTIMER);}
	/// Returns the currently used alpha blending value (0-255)
	BYTE GetSecondPicAlpha() {return alpha;}
	/// Sets the alpha blending value
	void SetSecondPicAlpha(BYTE a) 
	{
		SendMessage(hwndAlphaSlider, TBM_SETPOS, (WPARAM)1, (LPARAM)a);
		alpha = a; 
		InvalidateRect(*this, NULL, FALSE);
	}
	/// Resizes the image to fit into the window. Small images are not enlarged.
	void FitImageInWindow();
	/// Sets the zoom factor of the image
	void SetZoom(double dZoom);
	/// Returns the currently used zoom factor in which the image is shown.
	double GetZoom() {return picscale;}
	/// Zooms in (true) or out (false) in nice steps
	void Zoom(bool in);
	/// Sets the 'Other' pic window
	void SetOtherPicWindow(CPicWindow * pWnd) {pTheOtherPic = pWnd;}
	/// Links/Unlinks the two pic windows
	void LinkWindows(bool bLink) {bLinked = bLink;}

	void ShowInfo(bool bShow = true) {bShowInfo = bShow; InvalidateRect(*this, NULL, false);}

	bool HasMultipleImages();
protected:
	/// the message handler for this window
	LRESULT CALLBACK	WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	/// Draws the view title bar
	void				DrawViewTitle(HDC hDC, RECT * rect);
	/// Creates the image buttons
	bool				CreateButtons();
	/// Sets up the scrollbars as needed
	void				SetupScrollBars();
	/// Handles vertical scrolling
	void				OnVScroll(UINT nSBCode, UINT nPos);
	/// Handles horizontal scrolling
	void				OnHScroll(UINT nSBCode, UINT nPos);
	/// Handles the mouse wheel
	void				OnMouseWheel(short fwKeys, short zDelta);
	/// Returns the client rectangle, without the scrollbars and the view title.
	/// Basically the rectangle the image can use.
	void				GetClientRect(RECT * pRect);
	/// the WM_PAINT function
	void				Paint(HWND hwnd);
	/// Positions the buttons
	void				PositionChildren();
	/// Rounds a double to a given precision
	double				RoundDouble(double doValue, int nPrecision);
	/// advance to the next image in the file
	void				NextImage();
	/// go back to the previous image in the file
	void				PrevImage();
	/// starts/stops the animation
	void				Animate(bool bStart);
	/// Creates the trackbar (the alpha blending slider control)
	HWND				CreateTrackbar(HWND hwndParent, UINT iMin, UINT iMax);
	/// Moves the alpha slider trackbar to the correct position
	void				PositionTrackBar();

	stdstring			picpath;			///< the path to the image we show
	stdstring			pictitle;			///< the string to show in the image view as a title
	CPicture			picture;			///< the picture object of the image
	bool				bValid;				///< true if the picture object is valid, i.e. if the image could be loaded and can be shown
	double				picscale;			///< the scale factor of the image
	bool				bFirstpaint;		///< true if the image is painted the first time. Used to initialize some stuff when the window is valid for sure.
	CPicture *			pSecondPic;			///< if set, this is the picture to draw transparently above the original
	CPicWindow *		pTheOtherPic;		///< pointer to the other picture window. Used for "linking" the two windows when scrolling/zooming/...
	bool				bLinked;			///< if true, the two pic windows are linked together for scrolling/zooming/...
	stdstring 			pictitle2;			///< the title of the second picture
	stdstring 			picpath2;			///< the path of the second picture
	BYTE				alpha;				///< the alpha value for the transparency of the second picture
	bool				bShowInfo;			///< true if the info rectangle of the image should be shown
	// scrollbar info
	int					nVScrollPos;		///< vertical scroll position
	int					nHScrollPos;		///< horizontal scroll position
	// image frames/dimensions
	UINT				nDimensions;
	UINT				nCurrentDimension;
	UINT				nFrames;
	UINT				nCurrentFrame;
	HWND				hwndLeftBtn;
	HWND				hwndRightBtn;
	HWND				hwndPlayBtn;
	HWND				hwndAlphaSlider;
	HICON				hLeft;
	HICON				hRight;
	HICON				hPlay;
	HICON				hStop;
	bool				bPlaying;
	RECT				m_inforect;
};

