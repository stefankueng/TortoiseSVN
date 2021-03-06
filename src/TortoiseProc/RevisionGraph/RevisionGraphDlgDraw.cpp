// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2011, 2013-2016, 2018-2021 - TortoiseSVN

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
#include "MyMemDC.h"
#include "RevisionGraphDlg.h"
#include "SVN.h"
#include "UnicodeUtils.h"
#include "RevisionGraphWnd.h"
#include "IRevisionGraphLayout.h"
#include "UpsideDownLayout.h"
#include "ShowTreeStripes.h"
#include "DPIAware.h"
#include "Theme.h"

#ifdef _DEBUG
// ReSharper disable once CppInconsistentNaming
#    define new DEBUG_NEW
#    undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Gdiplus;

/************************************************************************/
/* Graphing functions                                                   */
/************************************************************************/
CFont* CRevisionGraphWnd::GetFont(BOOL bItalic /*= FALSE*/, BOOL bBold /*= FALSE*/)
{
    int nIndex = 0;
    if (bBold)
        nIndex |= 1;
    if (bItalic)
        nIndex |= 2;
    if (m_apFonts[nIndex] == nullptr)
    {
        m_apFonts[nIndex]        = new CFont;
        m_lfBaseFont.lfWeight    = bBold ? FW_BOLD : FW_NORMAL;
        m_lfBaseFont.lfItalic    = static_cast<BYTE>(bItalic);
        m_lfBaseFont.lfStrikeOut = static_cast<BYTE>(FALSE);
        m_lfBaseFont.lfHeight    = -CDPIAware::Instance().PointsToPixels(GetSafeHwnd(), m_nFontSize);
        // use the empty font name, so GDI takes the first font which matches
        // the specs. Maybe this will help render chinese/japanese chars correctly.
        wcsncpy_s(m_lfBaseFont.lfFaceName, L"Segoe UI", _countof(m_lfBaseFont.lfFaceName) - 1);
        if (!m_apFonts[nIndex]->CreateFontIndirect(&m_lfBaseFont))
        {
            delete m_apFonts[nIndex];
            m_apFonts[nIndex] = nullptr;
            return CWnd::GetFont();
        }
    }
    return m_apFonts[nIndex];
}

// ReSharper disable once CppMemberFunctionMayBeStatic
BOOL CRevisionGraphWnd::OnEraseBkgnd(CDC* /*pDC*/)
{
    return TRUE;
}

void CRevisionGraphWnd::OnPaint()
{
    CPaintDC dc(this); // device context for painting
    CRect    rect = GetClientRect();
    if (IsUpdateJobRunning())
    {
        dc.FillSolidRect(rect, CTheme::Instance().GetThemeColor(::GetSysColor(COLOR_APPWORKSPACE)));
        CWnd::OnPaint();
        return;
    }
    else if (!m_state.GetNodes())
    {
        CString sNoGraphText;
        sNoGraphText.LoadString(IDS_REVGRAPH_ERR_NOGRAPH);
        dc.FillSolidRect(rect, CTheme::Instance().GetThemeColor(RGB(255, 255, 255), true));
        dc.ExtTextOut(20, 20, ETO_CLIPPED, nullptr, sNoGraphText, nullptr);
        return;
    }

    GraphicsDevice dev;
    dev.pDC = &dc;
    DrawGraph(dev, rect, GetScrollPos(SB_VERT), GetScrollPos(SB_HORZ), false);
}

void CRevisionGraphWnd::ClearVisibleGlyphs(const CRect& rect)
{
    float glyphSize = CDPIAware::Instance().Scale(GetSafeHwnd(), GLYPH_SIZE) * m_fZoomFactor;

    CSyncPointer<CRevisionGraphState::TVisibleGlyphs>
        visibleGlyphs(m_state.GetVisibleGlyphs());

    for (size_t i = visibleGlyphs->size(), count = i; i > 0; --i)
    {
        const PointF& leftTop = (*visibleGlyphs)[i - 1].leftTop;
        CRect         glyphRect(static_cast<int>(leftTop.X), static_cast<int>(leftTop.Y), static_cast<int>(leftTop.X + glyphSize), static_cast<int>(leftTop.Y + glyphSize));

        if (CRect().IntersectRect(glyphRect, rect))
        {
            (*visibleGlyphs)[i - 1] = (*visibleGlyphs)[--count];
            visibleGlyphs->pop_back();
        }
    }
}

void CRevisionGraphWnd::CutawayPoints(const RectF& rect, float cutLen, TCutRectangle& result)
{
    result[0] = PointF(rect.X, rect.Y + cutLen);
    result[1] = PointF(rect.X + cutLen, rect.Y);
    result[2] = PointF(rect.GetRight() - cutLen, rect.Y);
    result[3] = PointF(rect.GetRight(), rect.Y + cutLen);
    result[4] = PointF(rect.GetRight(), rect.GetBottom() - cutLen);
    result[5] = PointF(rect.GetRight() - cutLen, rect.GetBottom());
    result[6] = PointF(rect.X + cutLen, rect.GetBottom());
    result[7] = PointF(rect.X, rect.GetBottom() - cutLen);
}

void CRevisionGraphWnd::DrawRoundedRect(GraphicsDevice& graphics, const Color& penColor, int penWidth, const Pen* pen, const Color& fillColor, const Brush* brush, const RectF& rect) const
{
    enum
    {
        POINT_COUNT = 8
    };

    float  radius = CDPIAware::Instance().Scale(GetSafeHwnd(), CORNER_SIZE) * m_fZoomFactor;
    PointF points[POINT_COUNT];
    CutawayPoints(rect, radius, points);

    if (graphics.graphics)
    {
        GraphicsPath path;
        path.AddArc(points[0].X, points[1].Y, radius, radius, 180, 90);
        path.AddArc(points[2].X, points[2].Y, radius, radius, 270, 90);
        path.AddArc(points[5].X, points[4].Y, radius, radius, 0, 90);
        path.AddArc(points[7].X, points[7].Y, radius, radius, 90, 90);

        points[0].Y -= radius / 2;
        path.AddLine(points[7], points[0]);

        if (brush != nullptr)
        {
            graphics.graphics->FillPath(brush, &path);
        }
        if (pen != nullptr)
            graphics.graphics->DrawPath(pen, &path);
    }
    else if (graphics.pSvg)
    {
        graphics.pSvg->RoundedRectangle(static_cast<int>(rect.X), static_cast<int>(rect.Y), static_cast<int>(rect.Width), static_cast<int>(rect.Height), penColor, penWidth, fillColor, static_cast<int>(radius));
    }
}

void CRevisionGraphWnd::DrawOctangle(GraphicsDevice& graphics, const Color& penColor, int penWidth, const Pen* pen, const Color& fillColor, const Brush* brush, const RectF& rect) const
{
    enum
    {
        POINT_COUNT = 8
    };

    // show left & right edges of low boxes as "<===>"

    float minCutAway = min(CDPIAware::Instance().Scale(GetSafeHwnd(), CORNER_SIZE) * m_fZoomFactor, rect.Height / 2);

    // larger boxes: remove 25% of the shorter side

    float suggestedCutAway = min(rect.Height, rect.Width) / 4;

    // use the more visible one of the former two

    PointF points[POINT_COUNT];
    CutawayPoints(rect, max(minCutAway, suggestedCutAway), points);

    // now, draw it

    if (graphics.graphics)
    {
        if (brush != nullptr)
            graphics.graphics->FillPolygon(brush, points, POINT_COUNT);
        if (pen != nullptr)
            graphics.graphics->DrawPolygon(pen, points, POINT_COUNT);
    }
    else if (graphics.pSvg)
    {
        graphics.pSvg->Polygon(points, POINT_COUNT, penColor, penWidth, fillColor);
    }
}

void CRevisionGraphWnd::DrawShape(GraphicsDevice& graphics, const Color& penColor, int penWidth, const Pen* pen, const Color& fillColor, const Brush* brush, const RectF& rect, NodeShape shape) const
{
    switch (shape)
    {
        case TSVNRectangle:
            if (graphics.graphics)
            {
                if (brush != nullptr)
                    graphics.graphics->FillRectangle(brush, rect);
                if (pen != nullptr)
                    graphics.graphics->DrawRectangle(pen, rect);
            }
            else if (graphics.pSvg)
            {
                graphics.pSvg->RoundedRectangle(static_cast<int>(rect.X), static_cast<int>(rect.Y), static_cast<int>(rect.Width), static_cast<int>(rect.Height), penColor, penWidth, fillColor);
            }
            break;
        case TSVNRoundRect:
            DrawRoundedRect(graphics, penColor, penWidth, pen, fillColor, brush, rect);
            break;
        case TSVNOctangle:
            DrawOctangle(graphics, penColor, penWidth, pen, fillColor, brush, rect);
            break;
        case TSVNEllipse:
            if (graphics.graphics)
            {
                if (brush != nullptr)
                    graphics.graphics->FillEllipse(brush, rect);
                if (pen != nullptr)
                    graphics.graphics->DrawEllipse(pen, rect);
            }
            else if (graphics.pSvg)
                graphics.pSvg->Ellipse(static_cast<int>(rect.X), static_cast<int>(rect.Y), static_cast<int>(rect.Width), static_cast<int>(rect.Height), penColor, penWidth, fillColor);
            break;
        default:
            ASSERT(FALSE); //unknown type
            return;
    }
}

inline BYTE LimitedScaleColor(BYTE c1, BYTE c2, float factor)
{
    BYTE scaled = c2 + static_cast<BYTE>((c1 - c2) * factor);
    return c1 < c2
               ? max(c1, scaled)
               : min(c1, scaled);
}

Color LimitedScaleColor(const Color& c1, const Color& c2, float factor)
{
    return Color(LimitedScaleColor(c1.GetA(), c2.GetA(), factor), LimitedScaleColor(c1.GetR(), c2.GetR(), factor), LimitedScaleColor(c1.GetG(), c2.GetG(), factor), LimitedScaleColor(c1.GetB(), c2.GetB(), factor));
}

inline BYTE Darken(BYTE c)
{
    return c < 0xc0
               ? (c / 3) * 2
               : static_cast<BYTE>(static_cast<int>(2 * c) - 0x100);
}

Color Darken(const Color& c)
{
    return Color(0xff, Darken(c.GetR()), Darken(c.GetG()), Darken(c.GetB()));
}

BYTE MaxComponentDiff(const Color& c1, const Color& c2)
{
    int rDiff = abs(static_cast<int>(c1.GetR()) - static_cast<int>(c2.GetR()));
    int gDiff = abs(static_cast<int>(c1.GetG()) - static_cast<int>(c2.GetG()));
    int bDiff = abs(static_cast<int>(c1.GetB()) - static_cast<int>(c2.GetB()));

    return static_cast<BYTE>(max(max(rDiff, gDiff), bDiff));
}

void CRevisionGraphWnd::DrawShadow(GraphicsDevice& graphics, const RectF& rect,
                                   Color shadowColor, NodeShape shape) const
{
    // draw the shadow

    RectF shadow = rect;
    shadow.Offset(2, 2);

    Pen        pen(shadowColor);
    SolidBrush brush(shadowColor);

    DrawShape(graphics, shadowColor, 1, &pen, shadowColor, &brush, shadow, shape);
}

void CRevisionGraphWnd::DrawNode(GraphicsDevice& graphics, const RectF& rect,
                                 Color contour, Color overlayColor,
                                 const CVisibleGraphNode* node, NodeShape shape)
{
    // special case: line deleted but deletion node removed
    // (don't show as "deleted" if the following node has been folded / split)

    enum
    {
        MASK = CGraphNodeStates::COLLAPSED_BELOW | CGraphNodeStates::SPLIT_BELOW
    };

    CNodeClassification nodeClassification = node->GetClassification();
    if ((node->GetNext() == nullptr) && (nodeClassification.Is(CNodeClassification::PATH_ONLY_DELETED)) && ((m_state.GetNodeStates()->GetFlags(node) & MASK) == 0))
    {
        contour = CTheme::Instance().GetThemeColor(m_colors.GetColor(CColors::gdpDeletedNode), true);
    }

    // calculate the GDI+ color values we need to draw the node

    Color background;
    background.SetFromCOLORREF(CTheme::Instance().GetThemeColor(GetSysColor(COLOR_WINDOW)));
    Color textColor;
    if (nodeClassification.Is(CNodeClassification::IS_MODIFIED_WC))
        textColor = CTheme::Instance().GetThemeColor(m_colors.GetColor(CColors::gdpWCNodeBorder), true);
    else
        textColor.SetFromCOLORREF(CTheme::Instance().GetThemeColor(GetSysColor(COLOR_WINDOWTEXT)));

    Color brightColor = LimitedScaleColor(background, contour, 0.9f);

    // Draw the main shape

    bool isWorkingCopy     = nodeClassification.Is(CNodeClassification::IS_WORKINGCOPY);
    bool isModifiedWC      = nodeClassification.Is(CNodeClassification::IS_MODIFIED_WC);
    bool textAsBorderColor = nodeClassification.IsAnyOf(CNodeClassification::IS_LAST | CNodeClassification::IS_MODIFIED_WC) | nodeClassification.Matches(CNodeClassification::IS_COPY_SOURCE, CNodeClassification::IS_OPERATION_MASK) | (contour.GetValue() == brightColor.GetValue());

    Color penColor = textAsBorderColor
                         ? textColor
                         : contour;

    Pen pen(penColor, isWorkingCopy ? 3.0f : 1.0f);
    if (isWorkingCopy && !isModifiedWC)
    {
        CSyncPointer<const CFullHistory> history(m_state.GetFullHistory());
        const CFullHistory::SWCInfo&     wcInfo   = history->GetWCInfo();
        revision_t                       revision = node->GetRevision();

        bool isCommitRev = (wcInfo.minCommit == revision) || (wcInfo.maxCommit == revision);
        bool isMinAtRev  = (wcInfo.minAtRev == revision) && (wcInfo.minAtRev != wcInfo.maxAtRev);

        DashStyle style = wcInfo.maxAtRev == revision
                              ? DashStyleSolid
                          : isCommitRev ? isMinAtRev ? DashStyleDashDot
                                                     : DashStyleDot
                                        : DashStyleDash;

        pen.SetDashStyle(style);
    }

    SolidBrush brush(brightColor);
    DrawShape(graphics, penColor, isWorkingCopy ? 3 : 1, &pen, brightColor, &brush, rect, shape);

    // overlay with some other color

    if (overlayColor.GetValue() != 0)
    {
        SolidBrush brush2(overlayColor);
        DrawShape(graphics, penColor, isWorkingCopy ? 3 : 1, &pen, overlayColor, &brush2, rect, shape);
    }
}

RectF CRevisionGraphWnd::TransformRectToScreen(const CRect& rect, const CSize& offset) const
{
    PointF leftTop(rect.left * m_fZoomFactor, rect.top * m_fZoomFactor);
    return RectF(leftTop.X - offset.cx, leftTop.Y - offset.cy, rect.right * m_fZoomFactor - leftTop.X - 1, rect.bottom * m_fZoomFactor - leftTop.Y);
}

RectF CRevisionGraphWnd::GetNodeRect(const ILayoutNodeList::SNode& node, const CSize& offset) const
{
    // get node and position

    RectF noderect(TransformRectToScreen(node.rect, offset));

    // show two separate lines for touching nodes,
    // unless the scale is too small

    if (noderect.Height > 15.0f)
        noderect.Height -= 1.0f;

    // done

    return noderect;
}

RectF CRevisionGraphWnd::GetBranchCover(const ILayoutNodeList* nodeList, index_t nodeIndex, bool upward, const CSize& offset) const
{
    // construct a rect that covers the respective branch

    CRect cover(0, 0, 0, 0);
    while (nodeIndex != NO_INDEX)
    {
        ILayoutNodeList::SNode node = nodeList->GetNode(nodeIndex);
        cover |= node.rect;

        const CVisibleGraphNode* nextNode = upward
                                                ? node.node->GetPrevious()
                                                : node.node->GetNext();

        nodeIndex = nextNode == nullptr ? NO_INDEX : nextNode->GetIndex();
    }

    // expand it just a little to make it look nicer

    cover.InflateRect(CDPIAware::Instance().Scale(GetSafeHwnd(), 10), CDPIAware::Instance().Scale(GetSafeHwnd(), 2), CDPIAware::Instance().Scale(GetSafeHwnd(), 10), CDPIAware::Instance().Scale(GetSafeHwnd(), 2));

    // and now, transform it

    return TransformRectToScreen(cover, offset);
}

void CRevisionGraphWnd::DrawShadows(GraphicsDevice& graphics, const CRect& logRect, const CSize& offset) const
{
    // shadow color to use

    Color background;
    background.SetFromCOLORREF(CTheme::Instance().GetThemeColor(GetSysColor(COLOR_WINDOW)));
    Color textColor;
    textColor.SetFromCOLORREF(CTheme::Instance().GetThemeColor(GetSysColor(COLOR_WINDOWTEXT)));

    Color shadowColor = LimitedScaleColor(background, static_cast<ARGB>(Color::Black), 0.5f);

    // iterate over all visible nodes

    CSyncPointer<const ILayoutNodeList> nodes(m_state.GetNodes());
    for (index_t index = nodes->GetFirstVisible(logRect); index != NO_INDEX; index = nodes->GetNextVisible(index, logRect))
    {
        // get node and position

        ILayoutNodeList::SNode node = nodes->GetNode(index);
        RectF                  nodeRect(GetNodeRect(node, offset));

        // actual drawing

        switch (node.style)
        {
            case ILayoutNodeList::SNode::STYLE_DELETED:
            case ILayoutNodeList::SNode::STYLE_RENAMED:
                DrawShadow(graphics, nodeRect, shadowColor, TSVNOctangle);
                break;
            case ILayoutNodeList::SNode::STYLE_ADDED:
                DrawShadow(graphics, nodeRect, shadowColor, TSVNRoundRect);
                break;
            case ILayoutNodeList::SNode::STYLE_LAST:
                DrawShadow(graphics, nodeRect, shadowColor, TSVNEllipse);
                break;
            default:
                DrawShadow(graphics, nodeRect, shadowColor, TSVNRectangle);
                break;
        }
    }
}

void CRevisionGraphWnd::DrawSquare(GraphicsDevice& graphics, const PointF& leftTop, const Color& lightColor, const Color& darkColor, const Color& penColor) const
{
    float squareSize = CDPIAware::Instance().Scale(GetSafeHwnd(), MARKER_SIZE) * m_fZoomFactor;

    PointF leftBottom(leftTop.X, leftTop.Y + squareSize);
    RectF  square(leftTop, SizeF(squareSize, squareSize));

    if (graphics.graphics)
    {
        LinearGradientBrush lgBrush(leftTop, leftBottom, lightColor, darkColor);
        graphics.graphics->FillRectangle(&lgBrush, square);
        if (squareSize > 4.0f)
        {
            Pen pen(penColor);
            graphics.graphics->DrawRectangle(&pen, square);
        }
    }
    else if (graphics.pSvg)
    {
        graphics.pSvg->GradientRectangle(static_cast<int>(square.X), static_cast<int>(square.Y), static_cast<int>(square.Width), static_cast<int>(square.Height),
                                         lightColor, darkColor, penColor);
    }
}

void CRevisionGraphWnd::DrawGlyph(GraphicsDevice& graphics, Image* glyphs, const PointF& leftTop, GlyphType glyph, GlyphPosition position) const
{
    // special case

    if (glyph == NoGlyph)
        return;

    // bitmap source area

    REAL x = (static_cast<REAL>(position) + static_cast<REAL>(glyph)) * CDPIAware::Instance().Scale(GetSafeHwnd(), GLYPH_BITMAP_SIZE);

    // screen target area

    float glyphSize = CDPIAware::Instance().Scale(GetSafeHwnd(), GLYPH_SIZE) * m_fZoomFactor;
    RectF target(leftTop, SizeF(glyphSize, glyphSize));

    // scaled copy

    if (graphics.graphics)
    {
        graphics.graphics->DrawImage(glyphs, target, x, 0.0f, static_cast<REAL>(CDPIAware::Instance().Scale(GetSafeHwnd(), GLYPH_BITMAP_SIZE)), static_cast<REAL>(CDPIAware::Instance().Scale(GetSafeHwnd(), GLYPH_BITMAP_SIZE)), UnitPixel, nullptr, nullptr, nullptr);
    }
    else if (graphics.pSvg)
    {
        // instead of inserting a bitmap, draw a
        // round rectangle instead.
        // Embedding images would blow up the resulting
        // svg file a lot, and the round rectangle
        // is enough IMHO.
        // Note:
        // images could be embedded like this:
        // <image y="100" x="100" id="imgId1234" xlink:href="data:image/png;base64,...base64endodeddata..." height="16" width="16" />

        graphics.pSvg->RoundedRectangle(static_cast<int>(target.X), static_cast<int>(target.Y), static_cast<int>(target.Width), static_cast<int>(target.Height),
                                        Color(0, 0, 0), 2, Color(255, 255, 255), static_cast<int>(target.Width / 3.0));
    }
}

void CRevisionGraphWnd::DrawGlyphs(GraphicsDevice& graphics, Image* glyphs, const CVisibleGraphNode* node, const PointF& center, GlyphType glyph1, GlyphType glyph2, GlyphPosition position, DWORD state1, DWORD state2, bool showAll)
{
    // don't show collapse and cut glyths by default

    if (!showAll && ((glyph1 == CollapseGlyph) || (glyph1 == SplitGlyph)))
        glyph1 = NoGlyph;
    if (!showAll && ((glyph2 == CollapseGlyph) || (glyph2 == SplitGlyph)))
        glyph2 = NoGlyph;

    // glyth2 shall be set only if 2 glyphs are in use

    if (glyph1 == NoGlyph)
    {
        std::swap(glyph1, glyph2);
        std::swap(state1, state2);
    }

    // anything to do?

    if (glyph1 == NoGlyph)
        return;

    // 1 or 2 glyphs?

    CSyncPointer<CRevisionGraphState::TVisibleGlyphs>
        visibleGlyphs(m_state.GetVisibleGlyphs());

    float squareSize = CDPIAware::Instance().Scale(GetSafeHwnd(), GLYPH_SIZE) * m_fZoomFactor;
    if (glyph2 == NoGlyph)
    {
        PointF leftTop(center.X - 0.5f * squareSize, center.Y - 0.5f * squareSize);
        DrawGlyph(graphics, glyphs, leftTop, glyph1, position);
        visibleGlyphs->emplace_back(state1, leftTop, node);
    }
    else
    {
        PointF leftTop1(center.X - squareSize, center.Y - 0.5f * squareSize);
        DrawGlyph(graphics, glyphs, leftTop1, glyph1, position);
        visibleGlyphs->emplace_back(state1, leftTop1, node);

        PointF leftTop2(center.X, center.Y - 0.5f * squareSize);
        DrawGlyph(graphics, glyphs, leftTop2, glyph2, position);
        visibleGlyphs->emplace_back(state2, leftTop2, node);
    }
}

void CRevisionGraphWnd::DrawGlyphs(GraphicsDevice& graphics, Image* glyphs, const CVisibleGraphNode* node, const RectF& nodeRect, DWORD state, DWORD allowed, bool upsideDown)
{
    // shortcut

    if ((state == 0) && (allowed == 0))
        return;

    // draw all glyphs

    PointF topCenter(0.5f * nodeRect.GetLeft() + 0.5f * nodeRect.GetRight(), nodeRect.GetTop());
    PointF rightCenter(nodeRect.GetRight(), 0.5f * nodeRect.GetTop() + 0.5f * nodeRect.GetBottom());
    PointF bottomCenter(0.5f * nodeRect.GetLeft() + 0.5f * nodeRect.GetRight(), nodeRect.GetBottom());

    DrawGlyphs(graphics, glyphs, node, upsideDown ? bottomCenter : topCenter, (state & CGraphNodeStates::COLLAPSED_ABOVE) ? ExpandGlyph : CollapseGlyph, (state & CGraphNodeStates::SPLIT_ABOVE) ? JoinGlyph : SplitGlyph, upsideDown ? Below : Above, CGraphNodeStates::COLLAPSED_ABOVE, CGraphNodeStates::SPLIT_ABOVE, (allowed & CGraphNodeStates::COLLAPSED_ABOVE) != 0);

    DrawGlyphs(graphics, glyphs, node, rightCenter, (state & CGraphNodeStates::COLLAPSED_RIGHT) ? ExpandGlyph : CollapseGlyph, (state & CGraphNodeStates::SPLIT_RIGHT) ? JoinGlyph : SplitGlyph, Right, CGraphNodeStates::COLLAPSED_RIGHT, CGraphNodeStates::SPLIT_RIGHT, (allowed & CGraphNodeStates::COLLAPSED_RIGHT) != 0);

    DrawGlyphs(graphics, glyphs, node, upsideDown ? topCenter : bottomCenter, (state & CGraphNodeStates::COLLAPSED_BELOW) ? ExpandGlyph : CollapseGlyph, (state & CGraphNodeStates::SPLIT_BELOW) ? JoinGlyph : SplitGlyph, upsideDown ? Above : Below, CGraphNodeStates::COLLAPSED_BELOW, CGraphNodeStates::SPLIT_BELOW, (allowed & CGraphNodeStates::COLLAPSED_BELOW) != 0);
}

void CRevisionGraphWnd::IndicateGlyphDirection(GraphicsDevice& graphics, const ILayoutNodeList* nodeList, const ILayoutNodeList::SNode& node, const RectF& nodeRect, DWORD glyphs, bool upsideDown, const CSize& offset) const
{
    // shortcut

    if (glyphs == 0)
        return;

    // where to place the indication?

    bool indicateAbove = (glyphs & CGraphNodeStates::COLLAPSED_ABOVE) != 0;
    bool indicateRight = (glyphs & CGraphNodeStates::COLLAPSED_RIGHT) != 0;
    bool indicateBelow = (glyphs & CGraphNodeStates::COLLAPSED_BELOW) != 0;

    // fill indication area a semi-transparent blend of red
    // and the background color

    Color color;
    color.SetFromCOLORREF(CTheme::Instance().GetThemeColor(GetSysColor(COLOR_WINDOW)));
    color.SetValue((color.GetValue() & 0x807f7f7f) + 0x800000);

    SolidBrush brush(color);

    // draw the indication (only one condition should match)

    RectF glyphCenter = (indicateAbove ^ upsideDown)
                            ? RectF(nodeRect.X, nodeRect.Y - 1.0f, 0.0f, 0.0f)
                            : RectF(nodeRect.X, nodeRect.GetBottom() - 1.0f, 0.0f, 0.0f);

    if (indicateAbove)
    {
        const CVisibleGraphNode* firstAffected = node.node->GetSource();

        assert(firstAffected);
        RectF branchCover = GetBranchCover(nodeList, firstAffected->GetIndex(), true, offset);
        RectF::Union(branchCover, branchCover, glyphCenter);

        if (graphics.graphics)
            graphics.graphics->FillRectangle(&brush, branchCover);
        else if (graphics.pSvg)
            graphics.pSvg->RoundedRectangle(static_cast<int>(branchCover.X), static_cast<int>(branchCover.Y), static_cast<int>(branchCover.Width), static_cast<int>(branchCover.Height),
                                            color, 1, color);
    }

    if (indicateRight)
    {
        for (const CVisibleGraphNode::CCopyTarget* branch = node.node->GetFirstCopyTarget(); branch != nullptr; branch = branch->next())
        {
            RectF branchCover = GetBranchCover(nodeList, branch->value()->GetIndex(), false, offset);
            if (graphics.graphics)
                graphics.graphics->FillRectangle(&brush, branchCover);
            else if (graphics.pSvg)
                graphics.pSvg->RoundedRectangle(static_cast<int>(branchCover.X), static_cast<int>(branchCover.Y), static_cast<int>(branchCover.Width), static_cast<int>(branchCover.Height),
                                                color, 1, color);
        }
    }

    if (indicateBelow)
    {
        const CVisibleGraphNode* firstAffected = node.node->GetNext();

        RectF branchCover = GetBranchCover(nodeList, firstAffected->GetIndex(), false, offset);
        RectF::Union(branchCover, branchCover, glyphCenter);

        if (graphics.graphics)
            graphics.graphics->FillRectangle(&brush, branchCover);
        else if (graphics.pSvg)
            graphics.pSvg->RoundedRectangle(static_cast<int>(branchCover.X), static_cast<int>(branchCover.Y), static_cast<int>(branchCover.Width), static_cast<int>(branchCover.Height),
                                            color, 1, color);
    }
}

void CRevisionGraphWnd::DrawMarker(GraphicsDevice& graphics, const RectF& noderect, MarkerPosition position, int relPosition, int colorIndex)
{
    // marker size
    float squareSize = CDPIAware::Instance().Scale(GetSafeHwnd(), MARKER_SIZE) * m_fZoomFactor;
    float squareDist = min((noderect.Height - squareSize) / 2, squareSize / 2);

    // position

    REAL   offset = squareSize * (0.75f + relPosition);
    REAL   left   = position == MpRight
                        ? noderect.GetRight() - offset - squareSize
                        : noderect.GetLeft() + offset;
    PointF leftTop(left, noderect.Y + squareDist);

    // color

    Color lightColor(CTheme::Instance().GetThemeColor(m_colors.GetColor(CColors::CtMarkers, colorIndex), true));
    Color darkColor(Darken(lightColor));
    Color borderColor(0x80000000);

    // draw it

    DrawSquare(graphics, leftTop, lightColor, darkColor, borderColor);
}

void CRevisionGraphWnd::DrawStripes(GraphicsDevice& graphics, const CSize& offset)
{
    // we need to fill this visible area of the the screen
    // (even if there is graph in that part)

    RectF clipRect;
    if (graphics.graphics)
        graphics.graphics->GetVisibleClipBounds(&clipRect);

    // don't show stripes if we don't have multiple roots

    CSyncPointer<const ILayoutRectList> trees(m_state.GetTrees());
    if (trees->GetCount() < 2)
        return;

    // iterate over all trees

    for (index_t i = 0, count = trees->GetCount(); i < count; ++i)
    {
        // screen coordinates covered by the tree

        CRect tree  = trees->GetRect(i);
        REAL  left  = tree.left * m_fZoomFactor;
        REAL  right = tree.right * m_fZoomFactor;
        RectF rect(left - offset.cx, clipRect.Y, i + 1 == count ? clipRect.Width : right - left, clipRect.Height);

        // relevant?

        if (rect.IntersectsWith(clipRect))
        {
            // draw the background stripe

            Color color((i & 1) == 0
                            ? CTheme::Instance().GetThemeColor(m_colors.GetColor(CColors::gdpStripeColor1), true)
                            : CTheme::Instance().GetThemeColor(m_colors.GetColor(CColors::gdpStripeColor2), true));
            if (graphics.graphics)
            {
                SolidBrush brush(color);
                graphics.graphics->FillRectangle(&brush, rect);
            }
            else if (graphics.pSvg)
                graphics.pSvg->RoundedRectangle(static_cast<int>(rect.X), static_cast<int>(rect.Y), static_cast<int>(rect.Width), static_cast<int>(rect.Height),
                                                color, 1, color);
        }
    }
}

void CRevisionGraphWnd::DrawNodes(GraphicsDevice& graphics, Image* glyphs, const CRect& logRect, const CSize& offset)
{
    CSyncPointer<CGraphNodeStates>      nodeStates(m_state.GetNodeStates());
    CSyncPointer<const ILayoutNodeList> nodes(m_state.GetNodes());

    bool upsideDown = m_state.GetOptions()->GetOption<CUpsideDownLayout>()->IsActive();

    // iterate over all visible nodes

    for (index_t index = nodes->GetFirstVisible(logRect); index != NO_INDEX; index = nodes->GetNextVisible(index, logRect))
    {
        // get node and position

        ILayoutNodeList::SNode node = nodes->GetNode(index);
        RectF                  nodeRect(GetNodeRect(node, offset));

        // actual drawing

        Color transparent(0);
        Color overlayColor = transparent;

        SvgGrouper grouper(graphics.pSvg);

        switch (node.style)
        {
            case ILayoutNodeList::SNode::STYLE_DELETED:
                DrawNode(graphics, nodeRect, CTheme::Instance().GetThemeColor(m_colors.GetColor(CColors::gdpDeletedNode), true), transparent, node.node, TSVNOctangle);
                break;

            case ILayoutNodeList::SNode::STYLE_ADDED:
                if (m_bTweakTagsColors && node.node->GetClassification().Is(CNodeClassification::IS_TAG))
                    overlayColor = CTheme::Instance().GetThemeColor(m_colors.GetColor(CColors::gdpTagOverlay), true);
                else if (m_bTweakTrunkColors && node.node->GetClassification().Is(CNodeClassification::IS_TRUNK))
                    overlayColor = CTheme::Instance().GetThemeColor(m_colors.GetColor(CColors::gdpTrunkOverlay), true);
                DrawNode(graphics, nodeRect, CTheme::Instance().GetThemeColor(m_colors.GetColor(CColors::gdpAddedNode), true), overlayColor, node.node, TSVNRoundRect);
                break;

            case ILayoutNodeList::SNode::STYLE_RENAMED:
                DrawNode(graphics, nodeRect, CTheme::Instance().GetThemeColor(m_colors.GetColor(CColors::gdpRenamedNode), true), overlayColor, node.node, TSVNOctangle);
                break;

            case ILayoutNodeList::SNode::STYLE_LAST:
                DrawNode(graphics, nodeRect, CTheme::Instance().GetThemeColor(m_colors.GetColor(CColors::gdpLastCommitNode), true), transparent, node.node, TSVNEllipse);
                break;

            case ILayoutNodeList::SNode::STYLE_MODIFIED:
                DrawNode(graphics, nodeRect, CTheme::Instance().GetThemeColor(m_colors.GetColor(CColors::gdpModifiedNode), true), transparent, node.node, TSVNRectangle);
                break;

            case ILayoutNodeList::SNode::STYLE_MODIFIED_WC:
                DrawNode(graphics, nodeRect, CTheme::Instance().GetThemeColor(m_colors.GetColor(CColors::gdpWCNode), true), transparent, node.node, TSVNEllipse);
                break;

            default:
                DrawNode(graphics, nodeRect, CTheme::Instance().GetThemeColor(m_colors.GetColor(CColors::gdpUnchangedNode), true), transparent, node.node, TSVNRectangle);
                break;
        }

        // Draw the "tagged" icon

        if (node.node->GetFirstTag() != nullptr)
            DrawMarker(graphics, nodeRect, MpRight, 0, 0);

        if ((m_selectedEntry1 == node.node) || (m_selectedEntry2 == node.node))
            DrawMarker(graphics, nodeRect, MpLeft, 0, 1);

        // expansion glypths etc.

        DrawGlyphs(graphics, glyphs, node.node, nodeRect, nodeStates->GetFlags(node.node), 0, upsideDown);
    }
}

void CRevisionGraphWnd::DrawConnections(GraphicsDevice& graphics, const CRect& logRect, const CSize& offset) const
{
    enum
    {
        MAX_POINTS = 100
    };
    CPoint points[MAX_POINTS];

    CPen  newPen(PS_SOLID, 0, CTheme::Instance().GetThemeColor(GetSysColor(COLOR_WINDOWTEXT)));
    CPen* pOldPen = nullptr;
    if (graphics.pDC)
        pOldPen = graphics.pDC->SelectObject(&newPen);

    // iterate over all visible lines

    CSyncPointer<const ILayoutConnectionList> connections(m_state.GetConnections());
    for (index_t index = connections->GetFirstVisible(logRect); index != NO_INDEX; index = connections->GetNextVisible(index, logRect))
    {
        // get connection and point position

        auto connection = connections->GetConnection(index);

        if (connection.numberOfPoints > MAX_POINTS)
            continue;

        for (index_t i = 0; i < connection.numberOfPoints; ++i)
        {
            points[i].x = static_cast<int>(connection.points[i].x * m_fZoomFactor) - offset.cx;
            points[i].y = static_cast<int>(connection.points[i].y * m_fZoomFactor) - offset.cy;
        }

        // draw the connection

        if (graphics.pDC)
            graphics.pDC->PolyBezier(points, connection.numberOfPoints);
        else if (graphics.pSvg)
        {
            Color color;
            color.SetFromCOLORREF(CTheme::Instance().GetThemeColor(GetSysColor(COLOR_WINDOWTEXT)));
            graphics.pSvg->PolyBezier(points, connection.numberOfPoints, color);
        }
    }

    if (graphics.pDC)
        graphics.pDC->SelectObject(pOldPen);
}

void CRevisionGraphWnd::DrawTexts(GraphicsDevice& graphics, const CRect& logRect, const CSize& offset)
{
    COLORREF standardTextColor = CTheme::Instance().GetThemeColor(GetSysColor(COLOR_WINDOWTEXT));
    if (m_nFontSize <= 0)
        return;

    // iterate over all visible nodes

    if (graphics.pDC)
        graphics.pDC->SetTextAlign(TA_LEFT | TA_TOP | TA_NOUPDATECP);
    CSyncPointer<const ILayoutTextList> texts(m_state.GetTexts());
    for (index_t index = texts->GetFirstVisible(logRect); index != NO_INDEX; index = texts->GetNextVisible(index, logRect))
    {
        // get node and position

        ILayoutTextList::SText text = texts->GetText(index, GetSafeHwnd());
        CRect                  textRect(static_cast<int>(text.rect.left * m_fZoomFactor) - offset.cx, static_cast<int>(text.rect.top * m_fZoomFactor) - offset.cy, static_cast<int>(text.rect.right * m_fZoomFactor) - offset.cx, static_cast<int>(text.rect.bottom * m_fZoomFactor) - offset.cy);

        // draw the revision text

        if (graphics.pDC)
        {
            graphics.pDC->SetTextColor(text.style == ILayoutTextList::SText::STYLE_WARNING
                                           ? CTheme::Instance().GetThemeColor(m_colors.GetColor(CColors::gdpWCNodeBorder), true).ToCOLORREF()
                                           : standardTextColor);
            graphics.pDC->SelectObject(GetFont(FALSE, text.style != ILayoutTextList::SText::STYLE_DEFAULT));
            graphics.pDC->DrawText(text.text, &textRect, DT_HIDEPREFIX | DT_NOPREFIX | DT_SINGLELINE | DT_CENTER | DT_TOP);
        }
        else if (graphics.pSvg)
        {
            graphics.pSvg->CenteredText((textRect.left + textRect.right) / 2, textRect.top + m_nFontSize + 3, "Arial", m_nFontSize,
                                        false, text.style != ILayoutTextList::SText::STYLE_DEFAULT,
                                        text.style == ILayoutTextList::SText::STYLE_WARNING
                                            ? CTheme::Instance().GetThemeColor(m_colors.GetColor(CColors::gdpWCNodeBorder), true)
                                            : standardTextColor,
                                        CUnicodeUtils::GetUTF8(text.text));
        }
    }
}

void CRevisionGraphWnd::DrawCurrentNodeGlyphs(GraphicsDevice& graphics, Image* glyphs, const CSize& offset)
{
    CSyncPointer<const ILayoutNodeList> nodeList(m_state.GetNodes());
    bool                                upsideDown = m_state.GetOptions()->GetOption<CUpsideDownLayout>()->IsActive();

    // don't draw glyphs if we are outside the client area
    // (e.g. within a scrollbar)

    CPoint point;
    GetCursorPos(&point);
    ScreenToClient(&point);
    if (!GetClientRect().PtInRect(point))
        return;

    // expansion glypths etc.

    m_hoverIndex  = GetHitNode(point);
    m_hoverGlyphs = GetHoverGlyphs(point);

    if ((m_hoverIndex != NO_INDEX) || (m_hoverGlyphs != 0))
    {
        index_t nodeIndex = m_hoverIndex == NO_INDEX
                                ? GetHitNode(point, CSize(CDPIAware::Instance().Scale(GetSafeHwnd(), GLYPH_SIZE), CDPIAware::Instance().Scale(GetSafeHwnd(), GLYPH_SIZE) / 2))
                                : m_hoverIndex;

        if (nodeIndex >= nodeList->GetCount())
            return;

        ILayoutNodeList::SNode node = nodeList->GetNode(nodeIndex);
        RectF                  nodeRect(GetNodeRect(node, offset));

        DWORD flags = m_state.GetNodeStates()->GetFlags(node.node);

        IndicateGlyphDirection(graphics, nodeList.get(), node, nodeRect, m_hoverGlyphs, upsideDown, offset);
        DrawGlyphs(graphics, glyphs, node.node, nodeRect, flags, m_hoverGlyphs, upsideDown);
    }
}

void CRevisionGraphWnd::DrawGraph(GraphicsDevice& graphics, const CRect& rect, int nVScrollPos, int nHScrollPos, bool bDirectDraw)
{
    CMemDC* memDC = nullptr;
    if (graphics.pDC)
    {
        if (!bDirectDraw)
        {
            memDC        = new CMemDC(*graphics.pDC, rect);
            graphics.pDC = &memDC->GetDC();
        }

        graphics.pDC->FillSolidRect(rect, CTheme::Instance().GetThemeColor(GetSysColor(COLOR_WINDOW)));
        graphics.pDC->SetBkMode(TRANSPARENT);
    }

    // preparation & sync

    CSyncPointer<CAllRevisionGraphOptions> options(m_state.GetOptions());
    ClearVisibleGlyphs(rect);

    // transform visible

    CSize offset(nHScrollPos, nVScrollPos);
    CRect logRect(static_cast<int>(offset.cx / m_fZoomFactor) - 1,
                  static_cast<int>(offset.cy / m_fZoomFactor) - 1,
                  static_cast<int>((rect.Width() + offset.cx) / m_fZoomFactor) + 1,
                  static_cast<int>((rect.Height() + offset.cy) / m_fZoomFactor) + 1);

    // draw the different components

    if (graphics.pDC)
    {
        Graphics* gcs     = Graphics::FromHDC(*graphics.pDC);
        graphics.graphics = gcs;
        gcs->SetPageUnit(UnitPixel);
        gcs->SetInterpolationMode(InterpolationModeHighQualityBicubic);
        gcs->SetSmoothingMode(SmoothingModeAntiAlias);
        gcs->SetClip(RectF(static_cast<Gdiplus::REAL>(rect.left), static_cast<Gdiplus::REAL>(rect.top), static_cast<Gdiplus::REAL>(rect.Width()), static_cast<Gdiplus::REAL>(rect.Height())));
    }

    if (options->GetOption<CShowTreeStripes>()->IsActive())
        DrawStripes(graphics, offset);

    if (m_fZoomFactor > SHADOW_ZOOM_THRESHOLD)
        DrawShadows(graphics, logRect, offset);

    Bitmap glyphs(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_REVGRAPHGLYPHS));

    DrawNodes(graphics, &glyphs, logRect, offset);
    DrawConnections(graphics, logRect, offset);
    DrawTexts(graphics, logRect, offset);

    if (m_showHoverGlyphs)
        DrawCurrentNodeGlyphs(graphics, &glyphs, offset);

    // draw preview

    if ((!bDirectDraw) && (m_preview.GetSafeHandle()) && (m_bShowOverview) && (graphics.pDC))
    {
        // draw the overview image rectangle in the top right corner
        CMyMemDC memDC2(graphics.pDC, true);
        memDC2.SetWindowOrg(0, 0);
        HBITMAP oldHbm = reinterpret_cast<HBITMAP>(memDC2.SelectObject(&m_preview));
        graphics.pDC->BitBlt(rect.Width() - m_previewWidth, 0, m_previewWidth, m_previewHeight,
                             &memDC2, 0, 0, SRCCOPY);
        memDC2.SelectObject(oldHbm);
        // draw the border for the overview rectangle
        m_overviewRect.left   = rect.Width() - m_previewWidth;
        m_overviewRect.top    = 0;
        m_overviewRect.right  = rect.Width();
        m_overviewRect.bottom = m_previewHeight;
        graphics.pDC->DrawEdge(&m_overviewRect, EDGE_BUMP, BF_RECT);
        // now draw a rectangle where the current view is located in the overview

        LONG width  = static_cast<long>(rect.Width() * m_previewZoom / m_fZoomFactor);
        LONG height = static_cast<long>(rect.Height() * m_previewZoom / m_fZoomFactor);
        LONG xPos   = static_cast<long>(nHScrollPos * m_previewZoom / m_fZoomFactor);
        LONG yPos   = static_cast<long>(nVScrollPos * m_previewZoom / m_fZoomFactor);
        RECT tempRect;
        tempRect.left   = rect.Width() - m_previewWidth + xPos;
        tempRect.top    = yPos;
        tempRect.right  = tempRect.left + width;
        tempRect.bottom = tempRect.top + height;
        // make sure the position rect is not bigger than the preview window itself
        ::IntersectRect(&m_overviewPosRect, &m_overviewRect, &tempRect);

        RectF rect2(static_cast<float>(m_overviewPosRect.left), static_cast<float>(m_overviewPosRect.top), static_cast<float>(m_overviewPosRect.Width()), static_cast<float>(m_overviewPosRect.Height()));
        if (graphics.graphics)
        {
            SolidBrush brush(Color(64, 0, 0, 0));
            graphics.graphics->FillRectangle(&brush, rect2);
            graphics.pDC->DrawEdge(&m_overviewPosRect, EDGE_BUMP, BF_RECT);
        }
    }

    // flush changes to screen

    delete graphics.graphics;
    delete memDC;
}

void CRevisionGraphWnd::DrawRubberBand()
{
    CDC* pDC = GetDC();
    pDC->SetROP2(R2_NOT);
    pDC->SelectObject(GetStockObject(NULL_BRUSH));
    pDC->Rectangle(min(m_ptRubberStart.x, m_ptRubberEnd.x), min(m_ptRubberStart.y, m_ptRubberEnd.y),
                   max(m_ptRubberStart.x, m_ptRubberEnd.x), max(m_ptRubberStart.y, m_ptRubberEnd.y));
    ReleaseDC(pDC);
}
