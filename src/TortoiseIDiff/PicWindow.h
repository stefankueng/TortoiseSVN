// TortoiseIDiff - an image diff viewer in TortoiseSVN

// Copyright (C) 2006-2009 - TortoiseSVN

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
#pragma once
#include "commctrl.h"
#include "BaseWindow.h"
#include "TortoiseIDiff.h"
#include "Picture.h"
#include "NiceTrackbar.h"

#define HEADER_HEIGHT 30

#define ID_ANIMATIONTIMER 100
#define TIMER_ALPHASLIDER 101
#define ID_ALPHATOGGLETIMER 102

#define LEFTBUTTON_ID			101
#define RIGHTBUTTON_ID			102
#define PLAYBUTTON_ID			103
#define ALPHATOGGLEBUTTON_ID	104
#define BLENDALPHA_ID			105
#define BLENDXOR_ID				106

#define TRACKBAR_ID 101
#define SLIDER_HEIGHT 30
#define SLIDER_WIDTH 30


#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))
#endif

/**
 * \ingroup TortoiseIDiff
 * The image view window.
 * Shows an image and provides methods to scale the image or alpha blend it
 * over another image.
 */
class CPicWindow : public CWindow
{
public:
	CPicWindow(HINSTANCE hInst, const WNDCLASSEX* wcx = NULL) : CWindow(hInst, wcx)
		, bValid(false)
		, nHScrollPos(0)
		, nVScrollPos(0)
		, picscale(1.0)
		, transparentColor(::GetSysColor(COLOR_WINDOW))
		, pSecondPic(NULL)
		, blendAlpha(0.5f)
		, bShowInfo(false)
		, nDimensions(0)
		, nCurrentDimension(1)
		, nFrames(0)
		, nCurrentFrame(1)
		, bPlaying(false)
		, pTheOtherPic(NULL)
		, bLinkedPositions(true)
		, bFitSizes(false)
		, m_blend(BLEND_ALPHA)
		, bMainPic(false)
	{ 
		SetWindowTitle(_T("Picture Window"));
		m_lastTTPos.x = 0;
		m_lastTTPos.y = 0;
	};

	enum BlendType
	{
		BLEND_ALPHA,
		BLEND_XOR,
	};
	/// Registers the window class and creates the window
	bool RegisterAndCreateWindow(HWND hParent);

	/// Sets the image path and title to show
	void SetPic(tstring path, tstring title, bool bFirst);
	/// Returns the CPicture image object. Used to get an already loaded image
	/// object without having to load it again.
	CPicture * GetPic() {return &picture;}
	/// Sets the path and title of the second image which is alpha blended over the original
	void SetSecondPic(CPicture * pPicture = NULL, const tstring& sectit = _T(""), const tstring& secpath = _T(""), int hpos = 0, int vpos = 0)
	{
		pSecondPic = pPicture;
		pictitle2 = sectit;
		picpath2 = secpath;
		nVSecondScrollPos = vpos;
		nHSecondScrollPos = hpos;
	}

	void StopTimer() {KillTimer(*this, ID_ANIMATIONTIMER);}
	
	/// Returns the currently used alpha blending value (0.0-1.0)
	float GetBlendAlpha() const { return blendAlpha; }
	/// Sets the alpha blending value
	void SetBlendAlpha(BlendType type, float a) 
	{
		m_blend = type;
		blendAlpha = a;
		if (m_AlphaSlider.IsValid())
			SendMessage(m_AlphaSlider.GetWindow(), TBM_SETPOS, (WPARAM)1, (LPARAM)(a*16.0f));
		PositionTrackBar();
		InvalidateRect(*this, NULL, FALSE);
	}
	/// Toggle the alpha blending value
	void ToggleAlpha()
	{
		if( 0.0f != GetBlendAlpha() )
			SetBlendAlpha(m_blend, 0.0f);
		else
			SetBlendAlpha(m_blend, 1.0f);
	}

	/// Set the color that this PicWindow will display behind transparent images.
	void SetTransparentColor(COLORREF back) { transparentColor = back; InvalidateRect(*this, NULL, false); }

	/// Resizes the image to fit into the window. Small images are not enlarged.
	void FitImageInWindow();
	/// center the image in the view
	void CenterImage();
	/// Makes both images the same size, fitting into the window
	void FitSizes(bool bFit);
	/// Sets the zoom factor of the image
	void SetZoom(double dZoom, bool centermouse);
	/// Returns the currently used zoom factor in which the image is shown.
	double GetZoom() {return picscale;}
	/// Zooms in (true) or out (false) in nice steps
	void Zoom(bool in, bool centermouse);
	/// Sets the 'Other' pic window
	void SetOtherPicWindow(CPicWindow * pWnd) {pTheOtherPic = pWnd;}
	/// Links/Unlinks the two pic windows
	void LinkPositions(bool bLink) {bLinkedPositions = bLink;}

	void ShowInfo(bool bShow = true) {bShowInfo = bShow; InvalidateRect(*this, NULL, false);}
	/// Sets up the scrollbars as needed
	void SetupScrollBars();

	bool HasMultipleImages();

	int GetHPos() {return nHScrollPos;}
	int GetVPos() {return nVScrollPos;}
	void SetZoomValue(double z) {picscale = z; InvalidateRect(*this, NULL, FALSE);}

	/// Handles the mouse wheel
	void				OnMouseWheel(short fwKeys, short zDelta);
protected:
	/// the message handler for this window
	LRESULT CALLBACK	WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	/// Draws the view title bar
	void				DrawViewTitle(HDC hDC, RECT * rect);
	/// Creates the image buttons
	bool				CreateButtons();
	/// Handles vertical scrolling
	void				OnVScroll(UINT nSBCode, UINT nPos);
	/// Handles horizontal scrolling
	void				OnHScroll(UINT nSBCode, UINT nPos);
	/// Returns the client rectangle, without the scrollbars and the view title.
	/// Basically the rectangle the image can use.
	void				GetClientRect(RECT * pRect);
	/// Returns the client rectangle, without the view title but with the scrollbars
	void				GetClientRectWithScrollbars(RECT * pRect);
	/// the WM_PAINT function
	void				Paint(HWND hwnd);
	/// Draw pic to hdc, with a border, scaled by scale.
	void				ShowPicWithBorder(HDC hdc, const RECT &bounds, CPicture &pic, double scale);
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
	void				CreateTrackbar(HWND hwndParent);
	/// Moves the alpha slider trackbar to the correct position
	void				PositionTrackBar();
	/// creates the info string used in the info box and the tooltips
	void				BuildInfoString(TCHAR * buf, int size, bool bTooltip);

	tstring			picpath;			///< the path to the image we show
	tstring			pictitle;			///< the string to show in the image view as a title
	CPicture			picture;			///< the picture object of the image
	bool				bValid;				///< true if the picture object is valid, i.e. if the image could be loaded and can be shown
	double				picscale;			///< the scale factor of the image
	COLORREF			transparentColor;	///< the color to draw under the images
	bool				bFirstpaint;		///< true if the image is painted the first time. Used to initialize some stuff when the window is valid for sure.
	CPicture *			pSecondPic;			///< if set, this is the picture to draw transparently above the original
	CPicWindow *		pTheOtherPic;		///< pointer to the other picture window. Used for "linking" the two windows when scrolling/zooming/...
	bool				bMainPic;			///< if true, this is the first image
	bool				bLinkedPositions;	///< if true, the two image windows are linked together for scrolling/zooming/...
	bool				bFitSizes;		///< if true, the two image windows are always zoomed so they match their size
	BlendType			m_blend;			///< type of blending to use
	tstring 			pictitle2;			///< the title of the second picture
	tstring 			picpath2;			///< the path of the second picture
	float				blendAlpha;			///<the alpha value for transparency blending
	bool				bShowInfo;			///< true if the info rectangle of the image should be shown
	TCHAR				m_wszTip[8192];
	char				m_szTip[8192];
	POINT				m_lastTTPos;
	HWND				hwndTT;
	// scrollbar info
	int					nVScrollPos;		///< vertical scroll position
	int					nHScrollPos;		///< horizontal scroll position
	int					nVSecondScrollPos;	///< vertical scroll position of second pic at the moment of enabling overlap mode
	int					nHSecondScrollPos;	///< horizontal scroll position of second pic at the moment of enabling overlap mode
	POINT				ptPanStart;			///< the point of the last mouse click
	int					startVScrollPos;	///< the vertical scroll position when panning starts
	int					startHScrollPos;	///< the horizontal scroll position when panning starts
	int					startVSecondScrollPos;	///< the vertical scroll position of the second pic when panning starts
	int					startHSecondScrollPos;	///< the horizontal scroll position of the second pic when panning starts
	// image frames/dimensions
	UINT				nDimensions;
	UINT				nCurrentDimension;
	UINT				nFrames;
	UINT				nCurrentFrame;

	// controls
	HWND				hwndLeftBtn;
	HWND				hwndRightBtn;
	HWND				hwndPlayBtn;
	CNiceTrackbar		m_AlphaSlider;
	HWND				hwndAlphaToggleBtn;
	HICON				hLeft;
	HICON				hRight;
	HICON				hPlay;
	HICON				hStop;
	HICON				hAlphaToggle;
	bool				bPlaying;
	RECT				m_inforect;
};




