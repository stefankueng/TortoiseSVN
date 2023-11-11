// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006-2007, 2009-2015, 2021, 2023 - TortoiseSVN

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
#include "ViewData.h"
#include <map>
#include <list>

class CBaseView;

/**
 * \ingroup TortoiseMerge
 * this struct holds all the information of a single change in TortoiseMerge.
 */
class ViewState
{
public:
    ViewState()
    {
    }

    std::map<int, CString>    diffLines;
    std::map<int, DiffStates> lineStates;
    std::map<int, DWORD>      lineLines;
    std::map<int, EOL>        linesEOL;
    std::map<int, bool>       markedLines;
    std::list<int>            addedLines;

    std::map<int, ViewData>   removedLines;
    std::map<int, ViewData>   replacedLines;
    bool                      modifies = false; ///< this step modifies view (save before and after save differs)

    void                      AddViewLineFromView(CBaseView* pView, int nViewLine, bool bAddEmptyLine);
    void                      Clear();
    bool                      IsEmpty() const { return diffLines.empty() && lineStates.empty() && lineLines.empty() && linesEOL.empty() && markedLines.empty() && addedLines.empty() && removedLines.empty() && replacedLines.empty(); }
};

/**
 * \ingroup TortoiseMerge
 * this struct holds all the information of a single change in TortoiseMerge for all(3) views.
 */
struct AllViewState
{
    ViewState right;
    ViewState bottom;
    ViewState left;

    void      Clear()
    {
        right.Clear();
        bottom.Clear();
        left.Clear();
    }
    bool IsEmpty() const { return right.IsEmpty() && bottom.IsEmpty() && left.IsEmpty(); }
};

/**
 * \ingroup TortoiseMerge
 * Holds all the information of previous changes made to a view content.
 * Of course, can undo those changes.
 */
class CUndo
{
public:
    static CUndo& GetInstance();

    bool          Undo(CBaseView* pLeft, CBaseView* pRight, CBaseView* pBottom);
    bool          Redo(CBaseView* pLeft, CBaseView* pRight, CBaseView* pBottom);
    void          AddState(const AllViewState& allstate, POINT pt);
    bool          CanUndo() const { return !m_undoViewStates.empty(); }
    bool          CanRedo() const { return !m_redoViewStates.empty(); }

    bool          IsGrouping() const { return m_undoGroups.size() % 2 == 1; }
    bool          IsRedoGrouping() const { return m_redoGroups.size() % 2 == 1; }
    void          BeginGrouping()
    {
        if (m_undoGroupCount == 0)
            m_undoGroups.push_back(m_undoCaretPoints.size());
        m_undoGroupCount++;
    }
    void EndGrouping()
    {
        m_undoGroupCount--;
        if (m_undoGroupCount == 0)
            m_undoGroups.push_back(m_undoCaretPoints.size());
    }
    void Clear();
    void MarkAllAsOriginalState() { MarkAsOriginalState(true, true, true); }
    void MarkAsOriginalState(bool left, bool right, bool bottom);

protected:
    static ViewState                     Do(const ViewState& state, CBaseView* pView, const POINT& pt);
    void                                 UndoOne(CBaseView* pLeft, CBaseView* pRight, CBaseView* pBottom);
    void                                 RedoOne(CBaseView* pLeft, CBaseView* pRight, CBaseView* pBottom);
    void                                 updateActiveView(CBaseView* pLeft, CBaseView* pRight, CBaseView* pBottom) const;
    std::list<AllViewState>              m_undoViewStates;
    std::list<POINT>                     m_undoCaretPoints;
    std::list<std::list<int>::size_type> m_undoGroups;
    int                                  m_undoGroupCount;

    std::list<AllViewState>              m_redoViewStates;
    std::list<POINT>                     m_redoCaretPoints;
    std::list<std::list<int>::size_type> m_redoGroups;

    __int64                              m_savedStateIndexLeft;
    __int64                              m_savedStateIndexRight;
    __int64                              m_savedStateIndexBottom;

private:
    CUndo();
    ~CUndo();
};
