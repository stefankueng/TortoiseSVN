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
#pragma once
#include "RevisionGraph/RevisionGraphState.h"
#include "ProgressDlg.h"
#include "Colors.h"
#include "SVNDiff.h"
#include "AppUtils.h"

using namespace Gdiplus;

#define REVGRAPH_PREVIEW_WIDTH 100
#define REVGRAPH_PREVIEW_HEIGHT 200

// we need at least 5x2 pixels per node 
// to draw a meaningful pre-view

#define REVGRAPH_PREVIEW_MAX_NODES (REVGRAPH_PREVIEW_HEIGHT * REVGRAPH_PREVIEW_WIDTH / 10)

// don't try to draw nodes smaller than that:

#define REVGRAPH_MIN_NODE_HIGHT (0.5f)

// size of the expand / collapse / split / join square gylphs

#define GLYPH_SIZE 16

// glyph display delay definitions

enum
{
    GLYPH_HOVER_EVENT = 10,     // timer ID for the glyph display delay
    GLYPH_HOVER_DELAY = 250     // delay until the glyphs are shown [ms]
};

/**
 * \ingroup TortoiseProc
 * node shapes for the revision graph
 */
enum NodeShape
{
	TSVNRectangle,
	TSVNRoundRect,
	TSVNOctangle,
	TSVNEllipse
};

#define MAXFONTS				4
#define	MAX_TT_LENGTH			60000
#define	MAX_TT_LENGTH_DEFAULT	1000

// forward declarations

class CRevisionGraphDlg;

/**
 * \ingroup TortoiseProc
 * Window class showing a revision graph.
 *
 * The analyzation of the log data is done in the child class CRevisionGraph.
 * Here, we handle the window notifications.
 */
class CRevisionGraphWnd : public CWnd //, public CRevisionGraph
{
public:
	CRevisionGraphWnd();   // standard constructor
	virtual ~CRevisionGraphWnd();
	enum 
    { 
        IDD = IDD_REVISIONGRAPH,
        WM_WORKERTHREADDONE = WM_APP +1
    };


	CString			m_sPath;
    SVNRev          m_pegRev;
	volatile LONG	m_bThreadRunning;
	CProgressDlg* 	m_pProgress;

    CRevisionGraphState m_state;

	void			InitView();
	void			Init(CWnd * pParent, LPRECT rect);
	void			SaveGraphAs(CString sSavePath);

    bool            FetchRevisionData ( const CString& path
                                      , SVNRev pegRevision);
    bool            AnalyzeRevisionData();

    bool            GetShowOverview() const;
    void            SetShowOverview (bool value);

	void			CompareRevs(bool bHead);
	void			UnifiedDiffRevs(bool bHead);

	CRect       	GetGraphRect();
	CRect           GetClientRect();
	CRect           GetWindowRect();
	CRect           GetViewRect();
	void			DoZoom(float nZoomFactor);
	bool			CancelMouseZoom();

    void            SetDlgTitle (bool offline);

  	void			BuildPreview();

protected:
	DWORD			m_dwTicks;
	CRect			m_OverviewPosRect;
	CRect			m_OverviewRect;

	bool			m_bShowOverview;

    CRevisionGraphDlg *m_parent;

	const CVisibleGraphNode * m_SelectedEntry1;
	const CVisibleGraphNode * m_SelectedEntry2;
	LOGFONT			m_lfBaseFont;
	CFont *			m_apFonts[MAXFONTS];
	int				m_nFontSize;
	CToolTipCtrl *	m_pDlgTip;
	char			m_szTip[MAX_TT_LENGTH+1];
	wchar_t			m_wszTip[MAX_TT_LENGTH+1];
    CString			m_sTitle;

	float			m_fZoomFactor;
	CColors			m_Colors;
    bool            m_bTweakTrunkColors;
    bool            m_bTweakTagsColors;
	bool			m_bIsRubberBand;
	CPoint			m_ptRubberStart;
	CPoint			m_ptRubberEnd;

	CBitmap			m_Preview;
	int				m_previewWidth;
	int				m_previewHeight;
    float           m_previewZoom;

    index_t         m_hoverIndex;   // node the cursor currently hovers over
    DWORD           m_hoverGlyphs;  // the glyphs shown for \ref m_hoverIndex
    mutable index_t m_tooltipIndex; // the node index we fetched the tooltip for
    bool            m_showHoverGlyphs;  // if true, show the glyphs we currently hover over
                                    // (will be activated only after some delay)
	
	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd(CDC* pDC);
	afx_msg void	OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void	OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void	OnSize(UINT nType, int cx, int cy);
	afx_msg INT_PTR	OnToolHitTest(CPoint point, TOOLINFO* pTI) const;
	afx_msg void	OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg BOOL	OnToolTipNotify(UINT id, NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg BOOL	OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void	OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void	OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void	OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL	OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void	OnTimer(UINT_PTR nIDEvent);
	afx_msg LRESULT	OnWorkerThreadDone(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
private:

    enum GlyphType
    {
        NoGlyph = -1,
        ExpandGlyph = 0,    // "+"
        CollapseGlyph = 1,  // "-"
        SplitGlyph = 2,     // "x"
        JoinGlyph = 3,      // "o"
    };

    enum GlyphPosition
    {
        Above = 0,    
        Right = 4, 
        Below = 8,     
    };

    typedef bool (SVNDiff::*TDiffFunc)(const CTSVNPath& url1, const SVNRev& rev1, 
	            					   const CTSVNPath& url2, const SVNRev& rev2, 
						               SVNRev peg,
						               bool ignoreancestry,
						               bool blame);
    typedef bool (*TStartDiffFunc)(HWND hWnd, const CTSVNPath& url1, const SVNRev& rev1, 
						           const CTSVNPath& url2, const SVNRev& rev2, 
						           const SVNRev& peg, const SVNRev& headpeg,
						           bool bAlternateDiff,
						           bool bIgnoreAncestry,
                                   bool blame);

    void            Compare (TDiffFunc diffFunc, TStartDiffFunc startDiffFunc, bool bHead);

    bool            UpdateSelectedEntry (const CVisibleGraphNode * clickedentry);
    void            AddSVNOps (CMenu& popup);
    void            AddGraphOps (CMenu& popup, const CVisibleGraphNode * node);
    void            DoShowLog();
    void            DoMergeTo();
    void            ResetNodeFlags (DWORD flags);
    void            ToggleNodeFlag (const CVisibleGraphNode *node, DWORD flag);

	void			SetScrollbars(int nVert = 0, int nHorz = 0, int oldwidth = 0, int oldheight = 0);
	CFont*			GetFont(BOOL bItalic = FALSE, BOOL bBold = FALSE);

    CSize           UsableTooltipRect();
    CString         DisplayableText (const CString& wholeText, const CSize& tooltipSize);
    CString         TooltipText (index_t index);

    CPoint          GetLogCoordinates (CPoint point) const;
    index_t         GetHitNode (CPoint point, CSize border = CSize (0, 0)) const;
    DWORD           GetHoverGlyphs (CPoint point) const;
    const CRevisionGraphState::SVisibleGlyph* GetHitGlyph (CPoint point) const;

    void            ClearVisibleGlyphs (const CRect& rect);

    typedef PointF TCutRectangle[8];
    void            CutawayPoints (const RectF& rect, float cutLen, TCutRectangle& result);
    void            DrawRoundedRect (Graphics& graphics, const Pen* pen, const Brush* brush, const RectF& rect);
	void			DrawOctangle (Graphics& graphics, const Pen* pen, const Brush* brush, const RectF& rect);
    void            DrawShape (Graphics& graphics, const Pen* pen, const Brush* brush, const RectF& rect, NodeShape shape);
	void			DrawShadow(Graphics& graphics, const RectF& rect,
							   Color shadowColor, NodeShape shape);
	void			DrawNode(Graphics& graphics, const RectF& rect,
							 Color contour, Color overlayColor,
                             const CVisibleGraphNode *node, NodeShape shape);
    RectF           TransformRectToScreen (const CRect& rect, const CSize& offset) const;
    RectF           GetNodeRect (const ILayoutNodeList::SNode& node, const CSize& offset) const;
    RectF           GetBranchCover (const ILayoutNodeList* nodeList, index_t nodeIndex, bool upward, const CSize& offset);

    void            DrawSquare (Graphics& graphics, const PointF& leftTop, 
                                const Color& lightColor, const Color& darkColor, const Color& penColor);
    void            DrawGlyph (Graphics& graphics, Image* glyphs, const PointF& leftTop,
                               GlyphType glyph, GlyphPosition position);
    void            DrawGlyphs (Graphics& graphics, Image* glyphs, const CVisibleGraphNode* node, const PointF& center, 
                                GlyphType glyph1, GlyphType glyph2, GlyphPosition position, DWORD state1, DWORD state2, bool showAll);
    void            DrawGlyphs (Graphics& graphics, Image* glyphs, const CVisibleGraphNode* node, const RectF& nodeRect,
                                DWORD state, DWORD allowed, bool upsideDown);
    void            DrawMarker (Graphics& graphics, const PointF& leftTop, 
                                const Color& lightColor, const Color& darkColor);
    void            IndicateGlyphDirection ( Graphics& graphics, const ILayoutNodeList* nodeList    
                                           , const ILayoutNodeList::SNode& node, const RectF& nodeRect
                                           , DWORD glyphs, bool upsideDown, const CSize& offset);

    void            DrawStripes (Graphics& graphics, const CSize& offset);

    void            DrawShadows (Graphics& graphics, const CRect& logRect, const CSize& offset);
    void            DrawNodes (Graphics& graphics, Image* glyphs, const CRect& logRect, const CSize& offset);
    void            DrawConnections (CDC* pDC, const CRect& logRect, const CSize& offset);
    void            DrawTexts (CDC* pDC, const CRect& logRect, const CSize& offset);
    void            DrawCurrentNodeGlyphs (Graphics& graphics, Image* glyphs, const CSize& offset);
    void			DrawGraph(CDC* pDC, const CRect& rect, int nVScrollPos, int nHScrollPos, bool bDirectDraw);

	int				GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
	void			DrawRubberBand();
};
