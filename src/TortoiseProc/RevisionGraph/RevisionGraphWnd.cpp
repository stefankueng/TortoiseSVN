// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2009 - TortoiseSVN

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
#include "TortoiseProc.h"
#include "Revisiongraphwnd.h"
#include "MessageBox.h"
#include "SVN.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "TempFile.h"
#include "UnicodeUtils.h"
#include "TSVNPath.h"
#include "SVNInfo.h"
#include "SVNDiff.h"
#include "RevisionGraphDlg.h"
#include "CachedLogInfo.h"
#include "RevisionIndex.h"
#include "RepositoryInfo.h"
#include "BrowseFolder.h"
#include "SVNProgressDlg.h"
#include "RevisionGraph/StandardLayout.h"
#include "RevisionGraph/UpsideDownLayout.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Gdiplus;

enum RevisionGraphContextMenuCommands
{
	// needs to start with 1, since 0 is the return value if *nothing* is clicked on in the context menu
	ID_SHOWLOG = 1,
	ID_COMPAREREVS,
	ID_COMPAREHEADS,
	ID_UNIDIFFREVS,
	ID_UNIDIFFHEADS,
	ID_MERGETO,
    ID_EXPAND_ALL,
    ID_JOIN_ALL,
    ID_GRAPH_EXPANDCOLLAPSE_ABOVE,
    ID_GRAPH_EXPANDCOLLAPSE_RIGHT,
    ID_GRAPH_EXPANDCOLLAPSE_BELOW,
    ID_GRAPH_SPLITJOIN_ABOVE,
    ID_GRAPH_SPLITJOIN_RIGHT,
    ID_GRAPH_SPLITJOIN_BELOW
};

CRevisionGraphWnd::CRevisionGraphWnd()
	: CWnd()
	, m_SelectedEntry1(NULL)
	, m_SelectedEntry2(NULL)
	, m_bThreadRunning(TRUE)
    , m_pProgress(NULL)
	, m_pDlgTip(NULL)
	, m_nFontSize(12)
    , m_bTweakTrunkColors(true)
    , m_bTweakTagsColors(true)
	, m_fZoomFactor(1.0)
	, m_ptRubberEnd(0,0)
	, m_ptRubberStart(0,0)
	, m_bShowOverview(false)
    , m_parent (NULL)
    , m_hoverIndex ((index_t)NO_INDEX)
    , m_hoverGlyphs (0)
    , m_tooltipIndex ((index_t)NO_INDEX)
    , m_showHoverGlyphs (false)
{
	memset(&m_lfBaseFont, 0, sizeof(LOGFONT));	
	for (int i=0; i<MAXFONTS; i++)
	{
		m_apFonts[i] = NULL;
	}

	WNDCLASS wndcls;
	HINSTANCE hInst = AfxGetInstanceHandle();
#define REVGRAPH_CLASSNAME _T("Revgraph_windowclass")
	if (!(::GetClassInfo(hInst, REVGRAPH_CLASSNAME, &wndcls)))
	{
		// otherwise we need to register a new class
		wndcls.style            = CS_DBLCLKS | CS_OWNDC;
		wndcls.lpfnWndProc      = ::DefWindowProc;
		wndcls.cbClsExtra       = wndcls.cbWndExtra = 0;
		wndcls.hInstance        = hInst;
		wndcls.hIcon            = NULL;
		wndcls.hCursor          = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
		wndcls.hbrBackground    = (HBRUSH) (COLOR_WINDOW + 1);
		wndcls.lpszMenuName     = NULL;
		wndcls.lpszClassName    = REVGRAPH_CLASSNAME;

		RegisterClass(&wndcls);
	}

	m_bShowOverview = CRegDWORD(_T("Software\\TortoiseSVN\\RevisionGraph\\ShowRevGraphOverview"), TRUE) != FALSE;
	m_bTweakTrunkColors = CRegDWORD(_T("Software\\TortoiseSVN\\RevisionGraph\\TweakTrunkColors"), TRUE) != FALSE;
	m_bTweakTagsColors = CRegDWORD(_T("Software\\TortoiseSVN\\RevisionGraph\\TweakTagsColors"), TRUE) != FALSE;
}

CRevisionGraphWnd::~CRevisionGraphWnd()
{
	for (int i=0; i<MAXFONTS; i++)
	{
		if (m_apFonts[i] != NULL)
		{
			m_apFonts[i]->DeleteObject();
			delete m_apFonts[i];
		}
		m_apFonts[i] = NULL;
	}
	if (m_pDlgTip)
		delete m_pDlgTip;
}

void CRevisionGraphWnd::DoDataExchange(CDataExchange* pDX)
{
	CWnd::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CRevisionGraphWnd, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipNotify)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipNotify)
	ON_WM_MOUSEWHEEL()
	ON_WM_CONTEXTMENU()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_SETCURSOR()
    ON_WM_TIMER()
    ON_MESSAGE(WM_WORKERTHREADDONE,OnWorkerThreadDone)
END_MESSAGE_MAP()

void CRevisionGraphWnd::Init(CWnd * pParent, LPRECT rect)
{
	WNDCLASS wndcls;
	HINSTANCE hInst = AfxGetInstanceHandle();
#define REVGRAPH_CLASSNAME _T("Revgraph_windowclass")
	if (!(::GetClassInfo(hInst, REVGRAPH_CLASSNAME, &wndcls)))
	{
		// otherwise we need to register a new class
		wndcls.style            = CS_DBLCLKS | CS_OWNDC;
		wndcls.lpfnWndProc      = ::DefWindowProc;
		wndcls.cbClsExtra       = wndcls.cbWndExtra = 0;
		wndcls.hInstance        = hInst;
		wndcls.hIcon            = NULL;
		wndcls.hCursor          = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
		wndcls.hbrBackground    = (HBRUSH) (COLOR_WINDOW + 1);
		wndcls.lpszMenuName     = NULL;
		wndcls.lpszClassName    = REVGRAPH_CLASSNAME;

		RegisterClass(&wndcls);
	}

	if (!IsWindow(m_hWnd))
		CreateEx(WS_EX_CLIENTEDGE, REVGRAPH_CLASSNAME, _T("RevGraph"), WS_CHILD|WS_VISIBLE|WS_TABSTOP, *rect, pParent, 0);
	m_pDlgTip = new CToolTipCtrl;
	if(!m_pDlgTip->Create(this))
	{
		TRACE("Unable to add tooltip!\n");
	}
	EnableToolTips();

	memset(&m_lfBaseFont, 0, sizeof(m_lfBaseFont));
	m_lfBaseFont.lfHeight = 0;
	m_lfBaseFont.lfWeight = FW_NORMAL;
	m_lfBaseFont.lfItalic = FALSE;
	m_lfBaseFont.lfCharSet = DEFAULT_CHARSET;
	m_lfBaseFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
	m_lfBaseFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	m_lfBaseFont.lfQuality = DEFAULT_QUALITY;
	m_lfBaseFont.lfPitchAndFamily = DEFAULT_PITCH;

	m_dwTicks = GetTickCount();

    m_parent = dynamic_cast<CRevisionGraphDlg*>(pParent);
}

CPoint CRevisionGraphWnd::GetLogCoordinates (CPoint point) const
{
    // translate point into logical coordinates

    int nVScrollPos = GetScrollPos(SB_VERT);
    int nHScrollPos = GetScrollPos(SB_HORZ);

    return CPoint ( (int)((point.x + nHScrollPos) / m_fZoomFactor)
                  , (int)((point.y + nVScrollPos) / m_fZoomFactor));
}

index_t CRevisionGraphWnd::GetHitNode (CPoint point, CSize border) const
{
    // any nodes at all?

    CSyncPointer<const ILayoutNodeList> nodeList (m_state.GetNodes());
    if (!nodeList)
        return index_t(NO_INDEX);

    // search the nodes for one at that grid position

    return nodeList->GetAt (GetLogCoordinates (point), border);
}

DWORD CRevisionGraphWnd::GetHoverGlyphs (CPoint point) const
{
    // if there is no layout, there will be no nodes,
    // hence, no glyphs

    CSyncPointer<const ILayoutNodeList> nodeList (m_state.GetNodes());
    if (!nodeList)
        return 0;

    // get node at point or node that is close enough 
    // so that point may hit a glyph area

    index_t nodeIndex = GetHitNode(point);
    if (nodeIndex == NO_INDEX)
        nodeIndex = GetHitNode(point, CSize (GLYPH_SIZE, GLYPH_SIZE / 2));

    if (nodeIndex >= nodeList->GetCount())
        return 0;

    ILayoutNodeList::SNode node = nodeList->GetNode (nodeIndex);
    const CVisibleGraphNode* base = node.node;

    // what glyphs should be shown depending on position of point
    // relative to the node rect?

    CPoint logCoordinates = GetLogCoordinates (point);
    CRect r = node.rect;
    CPoint center = r.CenterPoint();

    CRect rightGlyphArea ( r.right - GLYPH_SIZE, center.y - GLYPH_SIZE / 2
                         , r.right + GLYPH_SIZE, center.y + GLYPH_SIZE / 2);
    CRect topGlyphArea ( center.x - GLYPH_SIZE, r.top - GLYPH_SIZE / 2
                       , center.x + GLYPH_SIZE, r.top + GLYPH_SIZE / 2);
    CRect bottomGlyphArea ( center.x - GLYPH_SIZE, r.bottom - GLYPH_SIZE / 2
                          , center.x + GLYPH_SIZE, r.bottom + GLYPH_SIZE / 2);

    bool upsideDown 
        = m_state.GetOptions()->GetOption<CUpsideDownLayout>()->IsActive();

    if (upsideDown)
    {
        std::swap (topGlyphArea.top, bottomGlyphArea.top);
        std::swap (topGlyphArea.bottom, bottomGlyphArea.bottom);
    }

    DWORD result = 0;
    if (rightGlyphArea.PtInRect (logCoordinates))
        result = base->GetFirstCopyTarget() != NULL
               ? CGraphNodeStates::COLLAPSED_RIGHT | CGraphNodeStates::SPLIT_RIGHT
               : 0;

    if (topGlyphArea.PtInRect (logCoordinates))
        result = base->GetSource() != NULL
               ? CGraphNodeStates::COLLAPSED_ABOVE | CGraphNodeStates::SPLIT_ABOVE
               : 0;

    if (bottomGlyphArea.PtInRect (logCoordinates))
        result = base->GetNext() != NULL
               ? CGraphNodeStates::COLLAPSED_BELOW | CGraphNodeStates::SPLIT_BELOW
               : 0;

    // if some nodes have already been split, don't allow collapsing etc.

    CSyncPointer<const CGraphNodeStates> nodeStates (m_state.GetNodeStates());
    if (result & nodeStates->GetFlags (base))
        result = 0;

    return result;
}
    
const CRevisionGraphState::SVisibleGlyph* CRevisionGraphWnd::GetHitGlyph (CPoint point) const
{
    float glyphSize = GLYPH_SIZE * m_fZoomFactor;

    CSyncPointer<const CRevisionGraphState::TVisibleGlyphs> 
        visibleGlyphs (m_state.GetVisibleGlyphs());

    for (size_t i = 0, count = visibleGlyphs->size(); i < count; ++i)
    {
        const CRevisionGraphState::SVisibleGlyph* entry = &(*visibleGlyphs)[i];

        float xRel = point.x - entry->leftTop.X;
        float yRel = point.y - entry->leftTop.Y;

        if (   (xRel >= 0) && (xRel < glyphSize)
            && (yRel >= 0) && (yRel < glyphSize))
        {
            return entry;
        }
    }

    return NULL;
}

void CRevisionGraphWnd::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	SCROLLINFO sinfo = {0};
	sinfo.cbSize = sizeof(SCROLLINFO);
	GetScrollInfo(SB_HORZ, &sinfo);

	// Determine the new position of scroll box.
	switch (nSBCode)
	{
	case SB_LEFT:      // Scroll to far left.
		sinfo.nPos = sinfo.nMin;
		break;
	case SB_RIGHT:      // Scroll to far right.
		sinfo.nPos = sinfo.nMax;
		break;
	case SB_ENDSCROLL:   // End scroll.
		break;
	case SB_LINELEFT:      // Scroll left.
		if (sinfo.nPos > sinfo.nMin)
			sinfo.nPos--;
		break;
	case SB_LINERIGHT:   // Scroll right.
		if (sinfo.nPos < sinfo.nMax)
			sinfo.nPos++;
		break;
	case SB_PAGELEFT:    // Scroll one page left.
		{
			if (sinfo.nPos > sinfo.nMin)
				sinfo.nPos = max(sinfo.nMin, sinfo.nPos - (int) sinfo.nPage);
		}
		break;
	case SB_PAGERIGHT:      // Scroll one page right.
		{
			if (sinfo.nPos < sinfo.nMax)
				sinfo.nPos = min(sinfo.nMax, sinfo.nPos + (int) sinfo.nPage);
		}
		break;
	case SB_THUMBPOSITION: // Scroll to absolute position. nPos is the position
		sinfo.nPos = sinfo.nTrackPos;      // of the scroll box at the end of the drag operation.
		break;
	case SB_THUMBTRACK:   // Drag scroll box to specified position. nPos is the
		sinfo.nPos = sinfo.nTrackPos;     // position that the scroll box has been dragged to.
		break;
	}
	SetScrollInfo(SB_HORZ, &sinfo);
	Invalidate (FALSE);
	__super::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CRevisionGraphWnd::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	SCROLLINFO sinfo = {0};
	sinfo.cbSize = sizeof(SCROLLINFO);
	GetScrollInfo(SB_VERT, &sinfo);

	// Determine the new position of scroll box.
	switch (nSBCode)
	{
	case SB_LEFT:      // Scroll to far left.
		sinfo.nPos = sinfo.nMin;
		break;
	case SB_RIGHT:      // Scroll to far right.
		sinfo.nPos = sinfo.nMax;
		break;
	case SB_ENDSCROLL:   // End scroll.
		break;
	case SB_LINELEFT:      // Scroll left.
		if (sinfo.nPos > sinfo.nMin)
			sinfo.nPos--;
		break;
	case SB_LINERIGHT:   // Scroll right.
		if (sinfo.nPos < sinfo.nMax)
			sinfo.nPos++;
		break;
	case SB_PAGELEFT:    // Scroll one page left.
		{
			if (sinfo.nPos > sinfo.nMin)
				sinfo.nPos = max(sinfo.nMin, sinfo.nPos - (int) sinfo.nPage);
		}
		break;
	case SB_PAGERIGHT:      // Scroll one page right.
		{
			if (sinfo.nPos < sinfo.nMax)
				sinfo.nPos = min(sinfo.nMax, sinfo.nPos + (int) sinfo.nPage);
		}
		break;
	case SB_THUMBPOSITION: // Scroll to absolute position. nPos is the position
		sinfo.nPos = sinfo.nTrackPos;      // of the scroll box at the end of the drag operation.
		break;
	case SB_THUMBTRACK:   // Drag scroll box to specified position. nPos is the
		sinfo.nPos = sinfo.nTrackPos;     // position that the scroll box has been dragged to.
		break;
	}
	SetScrollInfo(SB_VERT, &sinfo);
	Invalidate(FALSE);
	__super::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CRevisionGraphWnd::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);
	SetScrollbars(GetScrollPos(SB_VERT), GetScrollPos(SB_HORZ));
	Invalidate(FALSE);
}

void CRevisionGraphWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_bThreadRunning)
		return __super::OnLButtonDown(nFlags, point);

    CSyncPointer<const ILayoutNodeList> nodeList (m_state.GetNodes());

	ATLTRACE("right clicked on x=%d y=%d\n", point.x, point.y);
	SetFocus();
	bool bHit = false;
	bool bControl = !!(GetKeyState(VK_CONTROL)&0x8000);
	if (!m_bShowOverview || !m_OverviewRect.PtInRect(point))
	{
        const CRevisionGraphState::SVisibleGlyph* hitGlyph 
            = GetHitGlyph (point);

        if (hitGlyph != NULL)
        {
            ToggleNodeFlag (hitGlyph->node, hitGlyph->state);
        	return __super::OnLButtonDown(nFlags, point);
        }
        else
        {
            index_t nodeIndex = GetHitNode (point);
	        if (nodeIndex != NO_INDEX)
	        {
                const CVisibleGraphNode* reventry = nodeList->GetNode (nodeIndex).node;
		        if (bControl)
		        {
			        if (m_SelectedEntry1 == reventry)
			        {
				        if (m_SelectedEntry2)
				        {
					        m_SelectedEntry1 = m_SelectedEntry2;
					        m_SelectedEntry2 = NULL;
				        }
				        else
					        m_SelectedEntry1 = NULL;
			        }
			        else if (m_SelectedEntry2 == reventry)
				        m_SelectedEntry2 = NULL;
			        else if (m_SelectedEntry1)
				        m_SelectedEntry2 = reventry;
			        else
				        m_SelectedEntry1 = reventry;
		        }
		        else
		        {
			        if (m_SelectedEntry1 == reventry)
				        m_SelectedEntry1 = NULL;
			        else
				        m_SelectedEntry1 = reventry;
			        m_SelectedEntry2 = NULL;
		        }
		        bHit = true;
		        Invalidate(FALSE);
	        }
        }
    }

    if ((!bHit)&&(!bControl))
	{
		m_SelectedEntry1 = NULL;
		m_SelectedEntry2 = NULL;
		m_bIsRubberBand = true;
		ATLTRACE("LButtonDown: x = %ld, y = %ld\n", point.x, point.y);
		Invalidate(FALSE);
		if (m_bShowOverview && m_OverviewRect.PtInRect(point))
			m_bIsRubberBand = false;
	}
	m_ptRubberStart = point;
	
	UINT uEnable = MF_BYCOMMAND;
	if ((m_SelectedEntry1 != NULL)&&(m_SelectedEntry2 != NULL))
		uEnable |= MF_ENABLED;
	else
		uEnable |= MF_GRAYED;

	EnableMenuItem(GetParent()->GetMenu()->m_hMenu, ID_VIEW_COMPAREREVISIONS, uEnable);
	EnableMenuItem(GetParent()->GetMenu()->m_hMenu, ID_VIEW_COMPAREHEADREVISIONS, uEnable);
	EnableMenuItem(GetParent()->GetMenu()->m_hMenu, ID_VIEW_UNIFIEDDIFF, uEnable);
	EnableMenuItem(GetParent()->GetMenu()->m_hMenu, ID_VIEW_UNIFIEDDIFFOFHEADREVISIONS, uEnable);
	
	__super::OnLButtonDown(nFlags, point);
}

void CRevisionGraphWnd::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (!m_bIsRubberBand)
		return;		// we don't have a rubberband, so no zooming necessary

	m_bIsRubberBand = false;
	ReleaseCapture();
	if (m_bThreadRunning)
		return __super::OnLButtonUp(nFlags, point);
	// zooming is finished
	m_ptRubberEnd = CPoint(0,0);
	CRect rect = GetClientRect();
	int x = abs(m_ptRubberStart.x - point.x);
	int y = abs(m_ptRubberStart.y - point.y);

	if ((x < 20)&&(y < 20))
	{
		// too small zoom rectangle
		// assume zooming by accident
		Invalidate(FALSE);
		__super::OnLButtonUp(nFlags, point);
		return;
	}

	float xfact = float(rect.Width())/float(x);
	float yfact = float(rect.Height())/float(y);
	float fact = max(yfact, xfact);

	// find out where to scroll to
	x = min(m_ptRubberStart.x, point.x) + GetScrollPos(SB_HORZ);
	y = min(m_ptRubberStart.y, point.y) + GetScrollPos(SB_VERT);

	float fZoomfactor = m_fZoomFactor*fact;
	if (fZoomfactor > 20.0)
	{
		// with such a big zoomfactor, the user
		// most likely zoomed by accident
		Invalidate(FALSE);
		__super::OnLButtonUp(nFlags, point);
		return;
	}
	if (fZoomfactor > 2.0)
	{
		fZoomfactor = 2.0;
		fact = fZoomfactor/m_fZoomFactor;
	}

	CRevisionGraphDlg * pDlg = (CRevisionGraphDlg*)GetParent();
	if (pDlg)
	{
		m_fZoomFactor = fZoomfactor;
		pDlg->DoZoom (m_fZoomFactor);
		SetScrollbars(int(float(y)*fact), int(float(x)*fact));
	}
	__super::OnLButtonUp(nFlags, point);
}

bool CRevisionGraphWnd::CancelMouseZoom()
{
	bool bRet = m_bIsRubberBand;
	ReleaseCapture();
	if (m_bIsRubberBand)
		Invalidate(FALSE);
	m_bIsRubberBand = false;
	m_ptRubberEnd = CPoint(0,0);
	return bRet;
}

INT_PTR CRevisionGraphWnd::OnToolHitTest(CPoint point, TOOLINFO* pTI) const
{
	if (m_bThreadRunning)
		return -1;

    index_t nodeIndex = GetHitNode (point);
    if (m_tooltipIndex != nodeIndex)
    {
        // force tooltip to be updated

        m_tooltipIndex = nodeIndex;
        return -1;
    }

    if (nodeIndex == NO_INDEX)
        return -1;

    if ((GetHoverGlyphs (point) != 0) || (GetHitGlyph (point) != NULL))
        return -1;

	pTI->hwnd = this->m_hWnd;
    CWnd::GetClientRect(&pTI->rect);
	pTI->uFlags  |= TTF_ALWAYSTIP | TTF_IDISHWND;
	pTI->uId = (UINT)m_hWnd;
	pTI->lpszText = LPSTR_TEXTCALLBACK;

    return 1;
}

BOOL CRevisionGraphWnd::OnToolTipNotify(UINT /*id*/, NMHDR *pNMHDR, LRESULT *pResult)
{
    if (pNMHDR->idFrom != (UINT)m_hWnd)
		return FALSE;

    // need to handle both ANSI and UNICODE versions of the message
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;

	POINT point;
	DWORD ptW = GetMessagePos();
	point.x = GET_X_LPARAM(ptW);
	point.y = GET_Y_LPARAM(ptW);
	ScreenToClient(&point);

    CString strTipText = TooltipText (GetHitNode (point));

	*pResult = 0;
	if (strTipText.IsEmpty())
		return TRUE;
		
    CSize tooltipSize = UsableTooltipRect();
    strTipText = DisplayableText (strTipText, tooltipSize);

	if (pNMHDR->code == TTN_NEEDTEXTA)
	{
        ::SendMessage(pNMHDR->hwndFrom, TTM_SETMAXTIPWIDTH, 0, tooltipSize.cx);
		pTTTA->lpszText = m_szTip;
		WideCharToMultiByte(CP_ACP, 0, strTipText, -1, m_szTip, strTipText.GetLength()+1, 0, 0);
	}
	else
	{
		::SendMessage(pNMHDR->hwndFrom, TTM_SETMAXTIPWIDTH, 0, tooltipSize.cx);
		lstrcpyn(m_wszTip, strTipText, strTipText.GetLength()+1);
		pTTTW->lpszText = m_wszTip;
	}

	// show the tooltip for 32 seconds. A higher value than 32767 won't work
	// even though it's nowhere documented!
	::SendMessage(pNMHDR->hwndFrom, TTM_SETDELAYTIME, TTDT_AUTOPOP, 32767);
	return TRUE;    // message was handled
}

CSize CRevisionGraphWnd::UsableTooltipRect()
{
    // get screen size

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // get current mouse position

    CPoint cursorPos;
    if (GetCursorPos (&cursorPos) == FALSE)
    {
        // we could not determine the mouse position 
        // use screen / 2 minus some safety margin

        return CSize (screenWidth / 2 - 20, screenHeight / 2 - 20);
    }

    // tool tip will display in the biggest sector beside the cursor
    // deduct some safety margin (for the mouse cursor itself

    CSize biggestSector
        ( max (screenWidth - cursorPos.x - 40, cursorPos.x - 24)
        , max (screenHeight - cursorPos.y - 40, cursorPos.y - 24));

    return biggestSector;
}

CString CRevisionGraphWnd::DisplayableText ( const CString& wholeText
                                           , const CSize& tooltipSize)
{
    CDC* dc = GetDC();
    if (dc == NULL)
    {
        // no access to the device context -> truncate hard at 1000 chars

        return wholeText.GetLength() >= MAX_TT_LENGTH_DEFAULT
            ? wholeText.Left (MAX_TT_LENGTH_DEFAULT-4) + _T(" ...")
            : wholeText;
    }

    // select the tooltip font

    NONCLIENTMETRICS metrics;
    metrics.cbSize = sizeof (metrics);
    SystemParametersInfo (SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &metrics, 0);

    CFont font;
    font.CreateFontIndirect(&metrics.lfStatusFont);
    CFont* pOldFont = dc->SelectObject (&font); 

    // split into lines and fill the tooltip rect

    CString result;

    int remainingHeight = tooltipSize.cy;
    int pos = 0;
    while (pos < wholeText.GetLength())
    {
        // extract a whole line

        int nextPos = wholeText.Find ('\n', pos);
        if (nextPos < 0)
            nextPos = wholeText.GetLength();

        CString line = wholeText.Mid (pos, nextPos-pos+1);

        // find a way to make it fit

        CSize size = dc->GetTextExtent (line);
        while (size.cx > tooltipSize.cx)
        {
            line.Delete (line.GetLength()-1);
            int nextPos2 = line.ReverseFind (' ');
            if (nextPos2 < 0)
                break;

            line.Delete (pos+1, line.GetLength() - pos-1);
            size = dc->GetTextExtent (line);
        }

        // enough room for the new line?

        remainingHeight -= size.cy;
        if (remainingHeight <= size.cy)
        {
            result += _T("...");
            break;
        }

        // add the line

        result += line;
        pos += line.GetLength();
    }
        
    // relase temp. resources

    dc->SelectObject (pOldFont);
    ReleaseDC(dc);

    // ready

    return result;
}

CString CRevisionGraphWnd::TooltipText (index_t index)
{
    if (index != NO_INDEX)
    {
        CSyncPointer<const ILayoutNodeList> nodeList (m_state.GetNodes());
        return nodeList->GetToolTip (index);
    }

    return CString();
}

void CRevisionGraphWnd::SaveGraphAs(CString sSavePath)
{
	CString extension = CPathUtils::GetFileExtFromPath(sSavePath);
	if (extension.CompareNoCase(_T(".wmf"))==0)
	{
		// save the graph as an enhanced metafile
		CMetaFileDC wmfDC;
		wmfDC.CreateEnhanced(NULL, sSavePath, NULL, _T("TortoiseSVN\0Revision Graph\0\0"));
		float fZoom = m_fZoomFactor;
		m_fZoomFactor = 1.0;
		DoZoom(m_fZoomFactor);
		CRect rect;
		rect = GetViewRect();
		DrawGraph(&wmfDC, rect, 0, 0, true);
		HENHMETAFILE hemf = wmfDC.CloseEnhanced();
		DeleteEnhMetaFile(hemf);
		m_fZoomFactor = fZoom;
		DoZoom(m_fZoomFactor);
	}
	else
	{
		// save the graph as a pixel picture instead of a vector picture
		// create dc to paint on
		try
		{
			CString sErrormessage;
			CWindowDC ddc(this);
			CDC dc;
			if (!dc.CreateCompatibleDC(&ddc))
			{
				LPVOID lpMsgBuf;
				if (!FormatMessage( 
					FORMAT_MESSAGE_ALLOCATE_BUFFER | 
					FORMAT_MESSAGE_FROM_SYSTEM | 
					FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					GetLastError(),
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
					(LPTSTR) &lpMsgBuf,
					0,
					NULL ))
				{
					return;
				}
				MessageBox( (LPCTSTR)lpMsgBuf, _T("Error"), MB_OK | MB_ICONINFORMATION );
				LocalFree( lpMsgBuf );
				return;
			}
			CRect rect;
			rect = GetGraphRect();
			rect.bottom = (LONG)(float(rect.Height()) * m_fZoomFactor);
			rect.right = (LONG)(float(rect.Width()) * m_fZoomFactor);
			BITMAPINFO bmi;
			HBITMAP hbm;
			LPBYTE pBits;
			// Initialize header to 0s.
			SecureZeroMemory(&bmi, sizeof(bmi));
			// Fill out the fields you care about.
			bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmi.bmiHeader.biWidth = rect.Width();
			bmi.bmiHeader.biHeight = rect.Height();
			bmi.bmiHeader.biPlanes = 1;
			bmi.bmiHeader.biBitCount = 24;
			bmi.bmiHeader.biCompression = BI_RGB;

			// Create the surface.
			hbm = CreateDIBSection(ddc.m_hDC, &bmi, DIB_RGB_COLORS,(void **)&pBits, NULL, 0);
			if (hbm==0)
			{
				CMessageBox::Show(m_hWnd, IDS_REVGRAPH_ERR_NOMEMORY, IDS_APPNAME, MB_ICONERROR);
				return;
			}
			HBITMAP oldbm = (HBITMAP)dc.SelectObject(hbm);
			// paint the whole graph
			DrawGraph(&dc, rect, 0, 0, true);
			// now use GDI+ to save the picture
			CLSID   encoderClsid;
			{
				Bitmap bitmap(hbm, NULL);
				if (bitmap.GetLastStatus()==Ok)
				{
					// Get the CLSID of the encoder.
					int ret = 0;
					if (CPathUtils::GetFileExtFromPath(sSavePath).CompareNoCase(_T(".png"))==0)
						ret = GetEncoderClsid(L"image/png", &encoderClsid);
					else if (CPathUtils::GetFileExtFromPath(sSavePath).CompareNoCase(_T(".jpg"))==0)
						ret = GetEncoderClsid(L"image/jpeg", &encoderClsid);
					else if (CPathUtils::GetFileExtFromPath(sSavePath).CompareNoCase(_T(".jpeg"))==0)
						ret = GetEncoderClsid(L"image/jpeg", &encoderClsid);
					else if (CPathUtils::GetFileExtFromPath(sSavePath).CompareNoCase(_T(".bmp"))==0)
						ret = GetEncoderClsid(L"image/bmp", &encoderClsid);
					else if (CPathUtils::GetFileExtFromPath(sSavePath).CompareNoCase(_T(".gif"))==0)
						ret = GetEncoderClsid(L"image/gif", &encoderClsid);
					else
					{
						sSavePath += _T(".jpg");
						ret = GetEncoderClsid(L"image/jpeg", &encoderClsid);
					}
					if (ret >= 0)
					{
						CStringW tfile = CStringW(sSavePath);
						bitmap.Save(tfile, &encoderClsid, NULL);
					}
					else
					{
						sErrormessage.Format(IDS_REVGRAPH_ERR_NOENCODER, (LPCTSTR)CPathUtils::GetFileExtFromPath(sSavePath));
					}
				}
				else
				{
					sErrormessage.LoadString(IDS_REVGRAPH_ERR_NOBITMAP);
				}
			}
			dc.SelectObject(oldbm);
			DeleteObject(hbm);
			dc.DeleteDC();
			if (!sErrormessage.IsEmpty())
			{
				CMessageBox::Show(m_hWnd, sErrormessage, _T("TortoiseSVN"), MB_ICONERROR);
			}
		}
		catch (CException * pE)
		{
			TCHAR szErrorMsg[2048];
			pE->GetErrorMessage(szErrorMsg, 2048);
			CMessageBox::Show(m_hWnd, szErrorMsg, _T("TortoiseSVN"), MB_ICONERROR);
		}
	}
}

BOOL CRevisionGraphWnd::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (m_bThreadRunning)
		return __super::OnMouseWheel(nFlags, zDelta, pt);
	int orientation = GetKeyState(VK_CONTROL)&0x8000 ? SB_HORZ : SB_VERT;
	int pos = GetScrollPos(orientation);
	pos -= (zDelta);
	SetScrollPos(orientation, pos);
	Invalidate(FALSE);
	return __super::OnMouseWheel(nFlags, zDelta, pt);
}

bool CRevisionGraphWnd::UpdateSelectedEntry (const CVisibleGraphNode * clickedentry)
{
	if ((m_SelectedEntry1 == NULL)&&(clickedentry == NULL))
		return false;

	if (m_SelectedEntry1 == NULL)
	{
		m_SelectedEntry1 = clickedentry;
		Invalidate(FALSE);
	}
	if ((m_SelectedEntry2 == NULL)&&(clickedentry != m_SelectedEntry1))
	{
		m_SelectedEntry1 = clickedentry;
		Invalidate(FALSE);
	}
	if (m_SelectedEntry1 && m_SelectedEntry2)
	{
		if ((m_SelectedEntry2 != clickedentry)&&(m_SelectedEntry1 != clickedentry))
			return false;
	}
	if (m_SelectedEntry1 == NULL)
		return false;

    return true;
}

void CRevisionGraphWnd::AddSVNOps (CMenu& popup)
{
    bool bothPresent =  (m_SelectedEntry1 != NULL)
                     && !m_SelectedEntry1->GetClassification().Is (CNodeClassification::IS_DELETED)
                     && (m_SelectedEntry2 != NULL)
                     && !m_SelectedEntry2->GetClassification().Is (CNodeClassification::IS_DELETED);

    bool bSameURL = (m_SelectedEntry2 && m_SelectedEntry1 && (m_SelectedEntry1->GetPath() == m_SelectedEntry2->GetPath()));
	CString temp;
	if (m_SelectedEntry1 && (m_SelectedEntry2 == NULL))
	{
		temp.LoadString(IDS_REPOBROWSE_SHOWLOG);
		popup.AppendMenu(MF_STRING | MF_ENABLED, ID_SHOWLOG, temp);
		popup.AppendMenu(MF_SEPARATOR, NULL);
		if (PathIsDirectory(m_sPath))
		{
			temp.LoadString(IDS_LOG_POPUP_MERGEREV);
			popup.AppendMenu(MF_STRING | MF_ENABLED, ID_MERGETO, temp);
		}
	}
	if (bothPresent)
	{
		temp.LoadString(IDS_REVGRAPH_POPUP_COMPAREREVS);
		popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COMPAREREVS, temp);
	    if (!bSameURL)
	    {
		    temp.LoadString(IDS_REVGRAPH_POPUP_COMPAREHEADS);
		    popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COMPAREHEADS, temp);
	    }

		temp.LoadString(IDS_REVGRAPH_POPUP_UNIDIFFREVS);
		popup.AppendMenu(MF_STRING | MF_ENABLED, ID_UNIDIFFREVS, temp);
		if (!bSameURL)
		{
			temp.LoadString(IDS_REVGRAPH_POPUP_UNIDIFFHEADS);
			popup.AppendMenu(MF_STRING | MF_ENABLED, ID_UNIDIFFHEADS, temp);
		}
	}
}

void CRevisionGraphWnd::AddGraphOps (CMenu& popup, const CVisibleGraphNode * node)
{
    CSyncPointer<CGraphNodeStates> nodeStates (m_state.GetNodeStates());

    CString temp;
    if (node == NULL)
    {
        DWORD state = nodeStates->GetCombinedFlags();
        if (state != 0)
        {
            if (state & CGraphNodeStates::COLLAPSED_ALL)
            {
                temp.LoadString (IDS_REVGRAPH_POPUP_EXPAND_ALL);
                popup.AppendMenu(MF_STRING | MF_ENABLED, ID_EXPAND_ALL, temp);
            }

            if (state & CGraphNodeStates::SPLIT_ALL)
            {
    	        temp.LoadString (IDS_REVGRAPH_POPUP_JOIN_ALL);
	            popup.AppendMenu(MF_STRING | MF_ENABLED, ID_JOIN_ALL, temp);
            }
        }
    }
    else
    {
        popup.AppendMenu(MF_SEPARATOR, NULL);
        DWORD state = nodeStates->GetFlags (node);

        if (node->GetSource() || (state & CGraphNodeStates::COLLAPSED_ABOVE))
        {
            temp.LoadString ((state & CGraphNodeStates::COLLAPSED_ABOVE) 
                             ? IDS_REVGRAPH_POPUP_EXPAND_ABOVE 
                             : IDS_REVGRAPH_POPUP_COLLAPSE_ABOVE);
            popup.AppendMenu(MF_STRING | MF_ENABLED, ID_GRAPH_EXPANDCOLLAPSE_ABOVE, temp);
        }

        if (node->GetFirstCopyTarget() || (state & CGraphNodeStates::COLLAPSED_RIGHT))
        {
            temp.LoadString ((state & CGraphNodeStates::COLLAPSED_RIGHT) 
                             ? IDS_REVGRAPH_POPUP_EXPAND_RIGHT 
                             : IDS_REVGRAPH_POPUP_COLLAPSE_RIGHT);
            popup.AppendMenu(MF_STRING | MF_ENABLED, ID_GRAPH_EXPANDCOLLAPSE_RIGHT, temp);
        }

        if (node->GetNext() || (state & CGraphNodeStates::COLLAPSED_BELOW))
        {
            temp.LoadString ((state & CGraphNodeStates::COLLAPSED_BELOW) 
                             ? IDS_REVGRAPH_POPUP_EXPAND_BELOW 
                             : IDS_REVGRAPH_POPUP_COLLAPSE_BELOW);
            popup.AppendMenu(MF_STRING | MF_ENABLED, ID_GRAPH_EXPANDCOLLAPSE_BELOW, temp);
        }

        if (node->GetSource() || (state & CGraphNodeStates::SPLIT_ABOVE))
        {
            temp.LoadString ((state & CGraphNodeStates::SPLIT_ABOVE) 
                             ? IDS_REVGRAPH_POPUP_JOIN_ABOVE 
                             : IDS_REVGRAPH_POPUP_SPLIT_ABOVE);
            popup.AppendMenu(MF_STRING | MF_ENABLED, ID_GRAPH_SPLITJOIN_ABOVE, temp);
        }

        if (node->GetFirstCopyTarget() || (state & CGraphNodeStates::SPLIT_RIGHT))
        {
            temp.LoadString ((state & CGraphNodeStates::SPLIT_RIGHT) 
                             ? IDS_REVGRAPH_POPUP_JOIN_RIGHT 
                             : IDS_REVGRAPH_POPUP_SPLIT_RIGHT);
            popup.AppendMenu(MF_STRING | MF_ENABLED, ID_GRAPH_SPLITJOIN_RIGHT, temp);
        }

        if (node->GetNext() || (state & CGraphNodeStates::SPLIT_BELOW))
        {
            temp.LoadString ((state & CGraphNodeStates::SPLIT_BELOW) 
                             ? IDS_REVGRAPH_POPUP_JOIN_BELOW 
                             : IDS_REVGRAPH_POPUP_SPLIT_BELOW);
            popup.AppendMenu(MF_STRING | MF_ENABLED, ID_GRAPH_SPLITJOIN_BELOW, temp);
        }
    }
}

void CRevisionGraphWnd::DoShowLog()
{
	CString sCmd;
	CString URL = m_state.GetRepositoryRoot() 
                + CUnicodeUtils::GetUnicode (m_SelectedEntry1->GetPath().GetPath().c_str());
	URL = CUnicodeUtils::GetUnicode(CPathUtils::PathEscape(CUnicodeUtils::GetUTF8(URL)));
	sCmd.Format(_T("\"%s\" /command:log /path:\"%s\" /startrev:%ld"), 
		(LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")), 
		(LPCTSTR)URL,
        m_SelectedEntry1->GetRevision());

	if (!SVN::PathIsURL(CTSVNPath(m_sPath)))
	{
		sCmd += _T(" /propspath:\"");
		sCmd += m_sPath;
		sCmd += _T("\"");
	}	

	CAppUtils::LaunchApplication(sCmd, NULL, false);
}

void CRevisionGraphWnd::DoMergeTo()
{
	CString URL = m_state.GetRepositoryRoot() 
                + CUnicodeUtils::GetUnicode (m_SelectedEntry1->GetPath().GetPath().c_str());
	URL = CUnicodeUtils::GetUnicode(CPathUtils::PathEscape(CUnicodeUtils::GetUTF8(URL)));

	CString path = m_sPath;
	CBrowseFolder folderBrowser;
	folderBrowser.SetInfo(CString(MAKEINTRESOURCE(IDS_LOG_MERGETO)));
	if (folderBrowser.Show(GetSafeHwnd(), path, path) == CBrowseFolder::OK)
	{
		CSVNProgressDlg dlg;
		dlg.SetCommand(CSVNProgressDlg::SVNProgress_Merge);
		dlg.SetPathList(CTSVNPathList(CTSVNPath(path)));
		dlg.SetUrl(URL);
		dlg.SetSecondUrl(URL);
		SVNRevRangeArray revarray;
		revarray.AddRevRange(m_SelectedEntry1->GetRevision(), svn_revnum_t(m_SelectedEntry1->GetRevision())-1);
		dlg.SetRevisionRanges(revarray);
		dlg.DoModal();
	}
}

void CRevisionGraphWnd::ResetNodeFlags (DWORD flags)
{
    m_state.GetNodeStates()->ResetFlags (flags);
    m_parent->StartWorkerThread();
}

void CRevisionGraphWnd::ToggleNodeFlag (const CVisibleGraphNode *node, DWORD flag)
{
    CSyncPointer<CGraphNodeStates> nodeStates (m_state.GetNodeStates());

    if (nodeStates->GetFlags (node) & flag)
        nodeStates->ResetFlags (node, flag);
    else
        nodeStates->SetFlags (node, flag);

    m_parent->StartWorkerThread();
}

void CRevisionGraphWnd::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	if (m_bThreadRunning)
		return;

    CSyncPointer<const ILayoutNodeList> nodeList (m_state.GetNodes());

	CPoint clientpoint = point;
	this->ScreenToClient(&clientpoint);
	ATLTRACE("right clicked on x=%d y=%d\n", clientpoint.x, clientpoint.y);

    index_t nodeIndex = GetHitNode (clientpoint);
	const CVisibleGraphNode * clickedentry = NULL;
    if (nodeIndex != NO_INDEX)
    {
        clickedentry = nodeList->GetNode (nodeIndex).node;
    }

    if (   !UpdateSelectedEntry (clickedentry) 
        && !m_state.GetNodeStates()->GetCombinedFlags())
		return;

    CMenu popup;
	if (popup.CreatePopupMenu())
	{
        AddSVNOps (popup);
        AddGraphOps (popup, clickedentry);

		// if the context menu is invoked through the keyboard, we have to use
		// a calculated position on where to anchor the menu on
		if ((point.x == -1) && (point.y == -1))
		{
			CRect rect = GetWindowRect();
			point = rect.CenterPoint();
		}

		int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
		switch (cmd)
		{
		case ID_COMPAREREVS:
    		if (m_SelectedEntry1 != NULL)
	    		CompareRevs(false);
			break;
		case ID_COMPAREHEADS:
    		if (m_SelectedEntry1 != NULL)
    			CompareRevs(true);
			break;
		case ID_UNIDIFFREVS:
    		if (m_SelectedEntry1 != NULL)
    			UnifiedDiffRevs(false);
			break;
		case ID_UNIDIFFHEADS:
    		if (m_SelectedEntry1 != NULL)
    			UnifiedDiffRevs(true);
			break;
		case ID_SHOWLOG:
			DoShowLog();
			break;
		case ID_MERGETO:
            DoMergeTo();
			break;
        case ID_EXPAND_ALL:
            ResetNodeFlags (CGraphNodeStates::COLLAPSED_ALL);
            break;
        case ID_JOIN_ALL:
            ResetNodeFlags (CGraphNodeStates::SPLIT_ALL);
            break;
        case ID_GRAPH_EXPANDCOLLAPSE_ABOVE:
            ToggleNodeFlag (clickedentry, CGraphNodeStates::COLLAPSED_ABOVE);
            break;
        case ID_GRAPH_EXPANDCOLLAPSE_RIGHT:
            ToggleNodeFlag (clickedentry, CGraphNodeStates::COLLAPSED_RIGHT);
            break;
        case ID_GRAPH_EXPANDCOLLAPSE_BELOW:
            ToggleNodeFlag (clickedentry, CGraphNodeStates::COLLAPSED_BELOW);
            break;
        case ID_GRAPH_SPLITJOIN_ABOVE:
            ToggleNodeFlag (clickedentry, CGraphNodeStates::SPLIT_ABOVE);
            break;
        case ID_GRAPH_SPLITJOIN_RIGHT:
            ToggleNodeFlag (clickedentry, CGraphNodeStates::SPLIT_RIGHT);
            break;
        case ID_GRAPH_SPLITJOIN_BELOW:
            ToggleNodeFlag (clickedentry, CGraphNodeStates::SPLIT_BELOW);
            break;
		}
	}
}

void CRevisionGraphWnd::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_bThreadRunning)
	{
		return __super::OnMouseMove(nFlags, point);
	}
	if (!m_bIsRubberBand)
	{
		if (m_bShowOverview && (m_OverviewRect.PtInRect(point))&&(nFlags & MK_LBUTTON))
		{
			// scrolling
            CRect viewRect = GetViewRect();
			int x = (int)((point.x-m_OverviewRect.left - (m_OverviewPosRect.Width()/2)) / m_previewZoom  * m_fZoomFactor);
			int y = (int)((point.y - (m_OverviewPosRect.Height()/2)) / m_previewZoom  * m_fZoomFactor);
			SetScrollbars(y, x);
			Invalidate(FALSE);
			return __super::OnMouseMove(nFlags, point);
		}
		else
        {
            // update screen if we hover over a different
            // node than during the last redraw

            CPoint clientPoint = point;
            GetCursorPos (&clientPoint);
            ScreenToClient (&clientPoint);

            const CRevisionGraphState::SVisibleGlyph* hitGlyph 
                = GetHitGlyph (clientPoint);
            const CFullGraphNode* glyphNode 
                = hitGlyph ? hitGlyph->node->GetBase() : NULL;

            const CFullGraphNode* hoverNode = NULL;
            if (m_hoverIndex != NO_INDEX)
            {
                CSyncPointer<const ILayoutNodeList> nodeList (m_state.GetNodes());
                if (m_hoverIndex < nodeList->GetCount())
                    hoverNode = nodeList->GetNode (m_hoverIndex).node->GetBase();
            }

            bool onHoverNodeGlyph = (hoverNode != NULL) && (glyphNode == hoverNode);
            if (   !onHoverNodeGlyph 
                && (   (m_hoverIndex != GetHitNode (clientPoint))
                    || (m_hoverGlyphs != GetHoverGlyphs (clientPoint))))
            {
                m_showHoverGlyphs = false;

                KillTimer (GLYPH_HOVER_EVENT);
                SetTimer (GLYPH_HOVER_EVENT, GLYPH_HOVER_DELAY, NULL);

                Invalidate(FALSE);
            }

			return __super::OnMouseMove(nFlags, point);
        }
	}

	if ((abs(m_ptRubberStart.x - point.x) < 2)&&(abs(m_ptRubberStart.y - point.y) < 2))
	{
		return __super::OnMouseMove(nFlags, point);
	}

	SetCapture();

	if ((m_ptRubberEnd.x != 0)||(m_ptRubberEnd.y != 0))
		DrawRubberBand();
	m_ptRubberEnd = point;
	CRect rect = GetClientRect();
	m_ptRubberEnd.x = max(m_ptRubberEnd.x, rect.left);
	m_ptRubberEnd.x = min(m_ptRubberEnd.x, rect.right);
	m_ptRubberEnd.y = max(m_ptRubberEnd.y, rect.top);
	m_ptRubberEnd.y = min(m_ptRubberEnd.y, rect.bottom);
	DrawRubberBand();

	__super::OnMouseMove(nFlags, point);
}

BOOL CRevisionGraphWnd::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    CRect viewRect = GetViewRect();

    LPTSTR cursorID = IDC_ARROW;
    HINSTANCE resourceHandle = NULL;

	if ((nHitTest == HTCLIENT)&&(pWnd == this)&&(viewRect.Width())&&(viewRect.Height())&&(message))
	{
		POINT pt;
		if (GetCursorPos(&pt))
		{
			ScreenToClient(&pt);
			if (m_OverviewPosRect.PtInRect(pt))
			{
                resourceHandle = AfxGetResourceHandle();
                cursorID = GetKeyState(VK_LBUTTON) & 0x8000
                         ? MAKEINTRESOURCE(IDC_PANCURDOWN)
                         : MAKEINTRESOURCE(IDC_PANCUR);
			}
		}
	}

	HCURSOR hCur = LoadCursor(resourceHandle, MAKEINTRESOURCE(cursorID));
    if (GetCursor() != hCur)
	    SetCursor (hCur);

	return TRUE;
}

void CRevisionGraphWnd::OnTimer (UINT_PTR nIDEvent)
{
    if (nIDEvent == GLYPH_HOVER_EVENT)
    {
        KillTimer (GLYPH_HOVER_EVENT);

        m_showHoverGlyphs = true;
        Invalidate (FALSE);
    }
    else
    {
        __super::OnTimer (nIDEvent);
    }
}

LRESULT CRevisionGraphWnd::OnWorkerThreadDone(WPARAM, LPARAM)
{
	InitView();
	BuildPreview();
    Invalidate(FALSE);

    SVN svn;
	LogCache::CRepositoryInfo& cachedProperties 
        = svn.GetLogCachePool()->GetRepositoryInfo();

    CSyncPointer<const CFullHistory> fullHistoy (m_state.GetFullHistory());
    if (fullHistoy.get() != NULL)
    {
	    SetDlgTitle (cachedProperties.IsOffline 
            ( fullHistoy->GetRepositoryUUID()
            , fullHistoy->GetRepositoryRoot()
            , false));
    }

    return 0;
}

void CRevisionGraphWnd::SetDlgTitle (bool offline)
{
	if (m_sTitle.IsEmpty())
		GetParent()->GetWindowText(m_sTitle);

	CString newTitle;
	if (offline)
    	newTitle.Format (IDS_REVGRAPH_DLGTITLEOFFLINE, (LPCTSTR)m_sTitle);
    else
        newTitle = m_sTitle;

	GetParent()->SetWindowText (newTitle);
}

