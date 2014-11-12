// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006-2007, 2010-2011, 2013-2014 - TortoiseSVN

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
#include "Undo.h"

#include "BaseView.h"

void viewstate::AddViewLineFromView(CBaseView *pView, int nViewLine, bool bAddEmptyLine)
{
    // is undo good place for this ?
    if (!pView || !pView->m_pViewData)
        return;
    replacedlines[nViewLine] = pView->m_pViewData->GetData(nViewLine);
    if (bAddEmptyLine)
    {
        addedlines.push_back(nViewLine + 1);
        pView->AddEmptyViewLine(nViewLine);
    }
}

void viewstate::Clear()
{
    difflines.clear();
    linestates.clear();
    linelines.clear();
    linesEOL.clear();
    markedlines.clear();
    addedlines.clear();

    removedlines.clear();
    replacedlines.clear();
    modifies = false;
}

void CUndo::MarkAsOriginalState(bool bLeft, bool bRight, bool bBottom)
{
    // TODO reduce code dumplication
    // find higest index of changing step
    if (bLeft) // left is selected for mark
    {
        m_originalstateLeft = m_viewstates.size();
        std::list<allviewstate>::reverse_iterator i = m_viewstates.rbegin();
        while (i != m_viewstates.rend() && !i->left.modifies)
        {
            ++i;
            --m_originalstateLeft;
        }
    }
    if (bRight) // right is selected for mark
    {
        m_originalstateRight = m_viewstates.size();
        std::list<allviewstate>::reverse_iterator i = m_viewstates.rbegin();
        while (i != m_viewstates.rend() && !i->right.modifies)
        {
            ++i;
            --m_originalstateRight;
        }
    }
    if (bBottom) // bottom is selected for mark
    {
        m_originalstateBottom = m_viewstates.size();
        std::list<allviewstate>::reverse_iterator i = m_viewstates.rbegin();
        while (i != m_viewstates.rend() && !i->bottom.modifies)
        {
            ++i;
            --m_originalstateBottom;
        }
    }
}

CUndo& CUndo::GetInstance()
{
    static CUndo instance;
    return instance;
}

CUndo::CUndo()
{
    Clear();
}

CUndo::~CUndo()
{
}

void CUndo::AddState(const allviewstate& allstate, POINT pt)
{
    m_viewstates.push_back(allstate);
    m_caretpoints.push_back(pt);
}

bool CUndo::Undo(CBaseView * pLeft, CBaseView * pRight, CBaseView * pBottom)
{
    if (!CanUndo())
        return false;

    if (m_groups.size() && m_groups.back() == m_caretpoints.size())
    {
        m_groups.pop_back();
        std::list<int>::size_type b = m_groups.back();
        m_groups.pop_back();
        while (b < m_caretpoints.size())
            UndoOne(pLeft, pRight, pBottom);
    }
    else
        UndoOne(pLeft, pRight, pBottom);

    CBaseView * pActiveView = NULL;

    if (pBottom && pBottom->IsTarget())
    {
        pActiveView = pBottom;
    }
    else
    if (pRight && pRight->IsTarget())
    {
        pActiveView = pRight;
    }
    else
    //if (pLeft && pLeft->IsTarget())
    {
        pActiveView = pLeft;
    }


    if (pActiveView) {
        pActiveView->ClearSelection();
        pActiveView->BuildAllScreen2ViewVector();
        pActiveView->RecalcAllVertScrollBars();
        pActiveView->RecalcAllHorzScrollBars();
        pActiveView->EnsureCaretVisible();
        pActiveView->UpdateCaret();

        // TODO reduce code dumplication
        if (m_viewstates.size() < m_originalstateLeft)
        {
            // Left can never get back to original state now
            m_originalstateLeft = (size_t)-1;
        }
        if (pLeft)
        {
            bool bModified = (m_originalstateLeft==(size_t)-1);
            if (!bModified)
            {
                std::list<allviewstate>::iterator i = m_viewstates.begin();
                std::advance(i, m_originalstateLeft);
                for (; i!=m_viewstates.end(); ++i)
                {
                    if (i->left.modifies)
                    {
                        bModified = true;
                        break;
                    }
                }
            }
            pLeft->SetModified(bModified);
            pLeft->ClearStepModifiedMark();
        }
        if (m_viewstates.size() < m_originalstateRight)
        {
            // Right can never get back to original state now
            m_originalstateRight = (size_t)-1;
        }
        if (pRight)
        {
            bool bModified = (m_originalstateRight==(size_t)-1);
            if (!bModified)
            {
                std::list<allviewstate>::iterator i = m_viewstates.begin();
                std::advance(i, m_originalstateRight);
                for (; i!=m_viewstates.end() && !i->right.modifies; ++i) ;
                bModified = i!=m_viewstates.end();
            }
            pRight->SetModified(bModified);
            pRight->ClearStepModifiedMark();
        }
        if (m_viewstates.size() < m_originalstateBottom)
        {
            // Bottom can never get back to original state now
            m_originalstateBottom = (size_t)-1;
        }
        if (pBottom)
        {
            bool bModified = (m_originalstateBottom==(size_t)-1);
            if (!bModified)
            {
                std::list<allviewstate>::iterator i = m_viewstates.begin();
                std::advance(i, m_originalstateBottom);
                for (; i!=m_viewstates.end(); ++i)
                {
                    if (i->bottom.modifies)
                    {
                        bModified = true;
                        break;
                    }
                }
            }
            pBottom->SetModified(bModified);
            pBottom->ClearStepModifiedMark();
        }
        pActiveView->RefreshViews();
    }

    return true;
}

void CUndo::UndoOne(CBaseView * pLeft, CBaseView * pRight, CBaseView * pBottom)
{
    allviewstate allstate = m_viewstates.back();
    POINT pt = m_caretpoints.back();

    Undo(allstate.left, pLeft, pt);
    Undo(allstate.right, pRight, pt);
    Undo(allstate.bottom, pBottom, pt);

    m_viewstates.pop_back();
    m_caretpoints.pop_back();
}

void CUndo::Undo(const viewstate& state, CBaseView * pView, const POINT& pt)
{
    if (!pView)
        return;

    CViewData* viewData = pView->m_pViewData;
    if (!viewData)
        return;

    for (std::list<int>::const_reverse_iterator it = state.addedlines.rbegin(); it != state.addedlines.rend(); ++it)
    {
        viewData->RemoveData(*it);
    }
    for (std::map<int, DWORD>::const_iterator it = state.linelines.begin(); it != state.linelines.end(); ++it)
    {
        viewData->SetLineNumber(it->first, it->second);
    }
    for (std::map<int, DWORD>::const_iterator it = state.linestates.begin(); it != state.linestates.end(); ++it)
    {
        viewData->SetState(it->first, (DiffStates)it->second);
    }
    for (std::map<int, EOL>::const_iterator it = state.linesEOL.begin(); it != state.linesEOL.end(); ++it)
    {
        viewData->SetLineEnding(it->first, it->second);
    }
    for (std::map<int, bool>::const_iterator it = state.markedlines.begin(); it != state.markedlines.end(); ++it)
    {
        viewData->SetMarked(it->first, it->second);
    }
    for (std::map<int, CString>::const_iterator it = state.difflines.begin(); it != state.difflines.end(); ++it)
    {
        viewData->SetLine(it->first, it->second);
    }
    for (std::map<int, viewdata>::const_iterator it = state.removedlines.begin(); it != state.removedlines.end(); ++it)
    {
        viewData->InsertData(it->first, it->second);
    }
    for (std::map<int, viewdata>::const_iterator it = state.replacedlines.begin(); it != state.replacedlines.end(); ++it)
    {
        viewData->SetData(it->first, it->second);
    }

    if (pView->IsTarget())
    {
        pView->SetCaretViewPosition(pt);
        pView->EnsureCaretVisible();
    }
}

void CUndo::Clear()
{
    m_viewstates.clear();
    m_caretpoints.clear();
    m_groups.clear();
    m_originalstateLeft = 0;
    m_originalstateRight = 0;
    m_originalstateBottom = 0;
    m_groupCount = 0;
}
