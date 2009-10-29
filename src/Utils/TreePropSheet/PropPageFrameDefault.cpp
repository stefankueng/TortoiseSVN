/********************************************************************
*
* Copyright (c) 2002 Sven Wiegand <mail@sven-wiegand.de>
*
* You can use this and modify this in any way you want,
* BUT LEAVE THIS HEADER INTACT.
*
* Redistribution is appreciated.
*
* $Workfile:$
* $Revision:$
* $Modtime:$
* $Author:$
*
* Revision History:
*	$History:$
*
*********************************************************************/

#include "stdafx.h"
#include "PropPageFrameDefault.h"
#include "ExtTextOutFL.h"
#include "XPTheme.h"

namespace TreePropSheet
{


static CXPTheme g_ThemeLib;
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//-------------------------------------------------------------------
// class CPropPageFrameDefault
//-------------------------------------------------------------------

BEGIN_MESSAGE_MAP(CPropPageFrameDefault, CWnd)
	//{{AFX_MSG_MAP(CPropPageFrameDefault)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


CPropPageFrameDefault::CPropPageFrameDefault()
{
}


CPropPageFrameDefault::~CPropPageFrameDefault()
{
	if (m_Images.GetSafeHandle())
		m_Images.DeleteImageList();
}


/////////////////////////////////////////////////////////////////////
// Overridings

BOOL CPropPageFrameDefault::Create(DWORD dwWindowStyle, const RECT &rect, CWnd *pwndParent, UINT nID)
{
	return CWnd::Create(
		AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW, AfxGetApp()->LoadStandardCursor(IDC_ARROW), GetSysColorBrush(COLOR_3DFACE)),
		_T("Page Frame"),
		dwWindowStyle, rect, pwndParent, nID);
}


CWnd* CPropPageFrameDefault::GetWnd()
{
	return static_cast<CWnd*>(this);
}


void CPropPageFrameDefault::SetCaption(LPCTSTR lpszCaption, HICON hIcon /*= NULL*/)
{
	CPropPageFrame::SetCaption(lpszCaption, hIcon);

	// build image list
	if (m_Images.GetSafeHandle())
		m_Images.DeleteImageList();
	if (hIcon)
	{
		ICONINFO	ii;
		if (!GetIconInfo(hIcon, &ii))
			return;

		CBitmap	bmMask;
		bmMask.Attach(ii.hbmMask);
		if (ii.hbmColor) DeleteObject(ii.hbmColor);

		BITMAP	bm;
		bmMask.GetBitmap(&bm);

		if (!m_Images.Create(bm.bmWidth, bm.bmHeight, ILC_COLOR32|ILC_MASK, 0, 1))
			return;

		if (m_Images.Add(hIcon) == -1)
			m_Images.DeleteImageList();
	}
}


CRect CPropPageFrameDefault::CalcMsgArea()
{
	CRect	rect;
	GetClientRect(rect);
	if (g_ThemeLib.IsAvailable() && g_ThemeLib.IsAppThemed())
	{
		if (g_ThemeLib.Open(m_hWnd, L"Tab"))
		{
			CRect	rectContent;
			CDC		*pDc = GetDC();
			g_ThemeLib.GetBackgroundContentRect(pDc->m_hDC, TABP_PANE, 0, rect, rectContent);
			ReleaseDC(pDc);
			g_ThemeLib.Close();
			
			if (GetShowCaption())
				rectContent.top = rect.top+GetCaptionHeight()+1;
			rect = rectContent;
		}
	}
	else if (GetShowCaption())
		rect.top+= GetCaptionHeight()+1;
	
	return rect;
}


CRect CPropPageFrameDefault::CalcCaptionArea()
{
	CRect	rect;
	GetClientRect(rect);
	if (g_ThemeLib.IsAvailable() && g_ThemeLib.IsAppThemed())
	{
		if (g_ThemeLib.Open(m_hWnd, L"Tab"))
		{
			CRect	rectContent;
			CDC		*pDc = GetDC();
			g_ThemeLib.GetBackgroundContentRect(pDc->m_hDC, TABP_PANE, 0, rect, rectContent);
			ReleaseDC(pDc);
			g_ThemeLib.Close();
			
			if (GetShowCaption())
				rectContent.bottom = rect.top+GetCaptionHeight();
			else
				rectContent.bottom = rectContent.top;

			rect = rectContent;
		}
	}
	else
	{
		if (GetShowCaption())
			rect.bottom = rect.top+GetCaptionHeight();
		else
			rect.bottom = rect.top;
	}

	return rect;
}

void CPropPageFrameDefault::DrawCaption(CDC *pDc, CRect rect, LPCTSTR lpszCaption, HICON hIcon)
{
	COLORREF	clrLeft = GetSysColor(COLOR_INACTIVECAPTION);
	COLORREF	clrRight = pDc->GetPixel(rect.right-1, rect.top);
	FillGradientRectH(pDc, rect, clrLeft, clrRight);

	// draw icon
	if (hIcon && m_Images.GetSafeHandle() && m_Images.GetImageCount() == 1)
	{
		IMAGEINFO	ii;
		m_Images.GetImageInfo(0, &ii);
		CPoint		pt(6, rect.CenterPoint().y - (ii.rcImage.bottom-ii.rcImage.top)/2);
		m_Images.Draw(pDc, 0, pt, ILD_TRANSPARENT);
		rect.left+= (ii.rcImage.right-ii.rcImage.left) + 6;
	}

	// draw text
	rect.left+= 2;

	COLORREF	clrPrev = pDc->SetTextColor(GetSysColor(COLOR_CAPTIONTEXT));
	int				nBkStyle = pDc->SetBkMode(TRANSPARENT);
	CFont			*pFont = (CFont*)pDc->SelectStockObject(SYSTEM_FONT);

	//pDc->DrawText(lpszCaption, rect, DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);

	TextOutTryFL(pDc->GetSafeHdc(), rect.left, rect.top, lpszCaption, (int)_tcslen(lpszCaption));

	pDc->SetTextColor(clrPrev);
	pDc->SetBkMode(nBkStyle);
	pDc->SelectObject(pFont);
}


/////////////////////////////////////////////////////////////////////
// Implementation helpers

void CPropPageFrameDefault::FillGradientRectH(CDC *pDc, const RECT &rect, COLORREF clrLeft, COLORREF clrRight)
{
	// pre calculation
	int	nSteps = rect.right-rect.left;
	int	nRRange = GetRValue(clrRight)-GetRValue(clrLeft);
	int	nGRange = GetGValue(clrRight)-GetGValue(clrLeft);
	int	nBRange = GetBValue(clrRight)-GetBValue(clrLeft);

	double	dRStep = (double)nRRange/(double)nSteps;
	double	dGStep = (double)nGRange/(double)nSteps;
	double	dBStep = (double)nBRange/(double)nSteps;

	double	dR = (double)GetRValue(clrLeft);
	double	dG = (double)GetGValue(clrLeft);
	double	dB = (double)GetBValue(clrLeft);

	CPen	*pPrevPen = NULL;
	for (int x = rect.left; x <= rect.right; ++x)
	{
		CPen	Pen(PS_SOLID, 1, RGB((BYTE)dR, (BYTE)dG, (BYTE)dB));
		pPrevPen = pDc->SelectObject(&Pen);
		pDc->MoveTo(x, rect.top);
		pDc->LineTo(x, rect.bottom);
		pDc->SelectObject(pPrevPen);
		
		dR+= dRStep;
		dG+= dGStep;
		dB+= dBStep;
	}
}


/////////////////////////////////////////////////////////////////////
// message handlers

void CPropPageFrameDefault::OnPaint() 
{
	CPaintDC dc(this);
	Draw(&dc);	
}


BOOL CPropPageFrameDefault::OnEraseBkgnd(CDC* pDC) 
{
	if (g_ThemeLib.IsAvailable() && g_ThemeLib.IsAppThemed())
	{
		if (g_ThemeLib.Open(m_hWnd, L"TREEVIEW"))
		{
			CRect	rect;
			GetClientRect(rect);
			g_ThemeLib.DrawBackground(pDC->m_hDC, 0, 0, rect, NULL);

			g_ThemeLib.Close();
		}
		return TRUE;
	}
	else
	{
		return CWnd::OnEraseBkgnd(pDC);
	}
}



} //namespace TreePropSheet
