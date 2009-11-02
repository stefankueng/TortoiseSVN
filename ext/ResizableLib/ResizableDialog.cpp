// ResizableDialog.cpp : implementation file
//
/////////////////////////////////////////////////////////////////////////////
//
// This file is part of ResizableLib
// http://sourceforge.net/projects/resizablelib
//
// Copyright (C) 2000-2004 by Paolo Messina
// http://www.geocities.com/ppescher - mailto:ppescher@hotmail.com
//
// The contents of this file are subject to the Artistic License (the "License").
// You may not use this file except in compliance with the License. 
// You may obtain a copy of the License at:
// http://www.opensource.org/licenses/artistic-license.html
//
// If you find this code useful, credits would be nice!
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ResizableDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CResizableDialog

typedef HRESULT (__stdcall *DWM_EXTEND_FRAME_INTO_CLIENT_AREA)(HWND ,const MARGINS* );
typedef HRESULT (__stdcall *DWM_IS_COMPOSITION_ENABLED)(BOOL *pfEnabled);
typedef HRESULT (__stdcall *DWM_ENABLE_COMPOSITION)(UINT uCompositionAction);


BOOL CResizableDialog::IsDwmCompositionEnabled(void)
{
	if(m_hDwmApiLib == NULL)
	{
		return FALSE;
	}
	DWM_IS_COMPOSITION_ENABLED pfnDwmIsCompositionEnabled = (DWM_IS_COMPOSITION_ENABLED)GetProcAddress(m_hDwmApiLib, "DwmIsCompositionEnabled");
	if(!pfnDwmIsCompositionEnabled)
		return FALSE;
	BOOL bEnabled = FALSE;
	HRESULT hRes = pfnDwmIsCompositionEnabled(&bEnabled);
	return SUCCEEDED(hRes) && bEnabled;
}

inline void CResizableDialog::PrivateConstruct()
{
	m_bEnableSaveRestore = FALSE;
	m_dwGripTempState = 1;
	m_hDwmApiLib = LoadLibraryW(L"dwmapi.dll");
	m_bShowGrip = !IsDwmCompositionEnabled();
}

CResizableDialog::CResizableDialog()
{
	PrivateConstruct();
}

CResizableDialog::CResizableDialog(UINT nIDTemplate, CWnd* pParentWnd)
	: CDialog(nIDTemplate, pParentWnd)
{
	PrivateConstruct();
}

CResizableDialog::CResizableDialog(LPCTSTR lpszTemplateName, CWnd* pParentWnd)
	: CDialog(lpszTemplateName, pParentWnd)
{
	PrivateConstruct();
}

CResizableDialog::~CResizableDialog()
{
	if (m_hDwmApiLib)
		FreeLibrary(m_hDwmApiLib);
	m_hDwmApiLib = NULL;
}


BEGIN_MESSAGE_MAP(CResizableDialog, CDialog)
	//{{AFX_MSG_MAP(CResizableDialog)
	ON_WM_GETMINMAXINFO()
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_WM_ERASEBKGND()
	ON_WM_NCCREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CResizableDialog message handlers

BOOL CResizableDialog::OnNcCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (!CDialog::OnNcCreate(lpCreateStruct))
		return FALSE;

	// child dialogs don't want resizable border or size grip,
	// nor they can handle the min/max size constraints
	BOOL bChild = lpCreateStruct->style & WS_CHILD;

	// create and init the size-grip
	if (!CreateSizeGrip(!bChild))
		return FALSE;

	if (!bChild)
	{
		// set the initial size as the min track size
		SetMinTrackSize(CSize(lpCreateStruct->cx, lpCreateStruct->cy));
	}
	
	MakeResizable(lpCreateStruct);

	return TRUE;
}

void CResizableDialog::OnDestroy() 
{
	if (m_bEnableSaveRestore)
		SaveWindowRect(m_sSection, m_bRectOnly);

	// remove child windows
	RemoveAllAnchors();

	CDialog::OnDestroy();
}

void CResizableDialog::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);
	
	if (nType == SIZE_MAXHIDE || nType == SIZE_MAXSHOW)
		return;		// arrangement not needed

	if ((m_bShowGrip)||(!IsDwmCompositionEnabled()))
	{
		if (nType == SIZE_MAXIMIZED)
			HideSizeGrip(&m_dwGripTempState);
		else
			ShowSizeGrip(&m_dwGripTempState);
		// update grip and layout
		UpdateSizeGrip();
	}

	ArrangeLayout();
	// on Vista, the redrawing doesn't work right, so we have to work
	// around this by invalidating the whole dialog so the DWM recognizes
	// that it has to update the application window.
	OSVERSIONINFOEX inf;
	SecureZeroMemory(&inf, sizeof(OSVERSIONINFOEX));
	inf.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	GetVersionEx((OSVERSIONINFO *)&inf);
	WORD fullver = MAKEWORD(inf.dwMinorVersion, inf.dwMajorVersion);
	if (fullver >= 0x0600)
		Invalidate();
}

void CResizableDialog::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI) 
{
	MinMaxInfo(lpMMI);
}

// NOTE: this must be called after setting the layout
//       to have the dialog and its controls displayed properly
void CResizableDialog::EnableSaveRestore(LPCTSTR pszSection, BOOL bRectOnly)
{
	m_sSection = pszSection;

	m_bEnableSaveRestore = TRUE;
	m_bRectOnly = bRectOnly;

	// restore immediately
	LoadWindowRect(pszSection, bRectOnly);

	CMenu* pMenu = GetMenu();
	if ( pMenu )
		DrawMenuBar();
}

BOOL CResizableDialog::OnEraseBkgnd(CDC* pDC) 
{
	ClipChildren(pDC, FALSE);

	BOOL bRet = CDialog::OnEraseBkgnd(pDC);

	ClipChildren(pDC, TRUE);

	return bRet;
}

LRESULT CResizableDialog::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	if (message != WM_NCCALCSIZE || wParam == 0)
		return CDialog::WindowProc(message, wParam, lParam);

	LRESULT lResult = 0;
	HandleNcCalcSize(FALSE, (LPNCCALCSIZE_PARAMS)lParam, lResult);
	lResult = CDialog::WindowProc(message, wParam, lParam);
	HandleNcCalcSize(TRUE, (LPNCCALCSIZE_PARAMS)lParam, lResult);
	return lResult;
}
