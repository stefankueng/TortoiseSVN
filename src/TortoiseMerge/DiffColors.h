// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2007-2008, 2011, 2013 - TortoiseSVN

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
#include "DiffStates.h"
#include "registry.h"

#define INLINEADDED_COLOR                           RGB(255, 255, 150)
#define INLINEREMOVED_COLOR                         RGB(200, 100, 100)
#define MODIFIED_COLOR                              RGB(220, 220, 255)


#define DIFFSTATE_UNKNOWN_DEFAULT_FG                ::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_NORMAL_DEFAULT_FG                 ::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_REMOVED_DEFAULT_FG                ::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_REMOVEDWHITESPACE_DEFAULT_FG      ::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_ADDED_DEFAULT_FG                  ::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_ADDEDWHITESPACE_DEFAULT_FG        ::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_WHITESPACE_DEFAULT_FG             ::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_WHITESPACE_DIFF_DEFAULT_FG        ::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_EMPTY_DEFAULT_FG                  ::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_CONFLICTED_DEFAULT_FG             ::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_CONFLICTED_IGNORED_DEFAULT_FG     ::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_CONFLICTADDED_DEFAULT_FG          ::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_CONFLICTEMPTY_DEFAULT_FG          ::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_MOVEDFROM_DEFAULT_FG              ::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_MOVEDTO_DEFAULT_FG                ::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_IDENTICAL_MOVEDFROM_DEFAULT_FG    ::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_IDENTICAL_MOVEDTO_DEFAULT_FG      ::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_IDENTICALREMOVED_DEFAULT_FG       ::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_IDENTICALADDED_DEFAULT_FG         ::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_THEIRSREMOVED_DEFAULT_FG          ::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_THEIRSADDED_DEFAULT_FG            ::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_YOURSREMOVED_DEFAULT_FG           ::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_YOURSADDED_DEFAULT_FG             ::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_CONFLICTRESOLVED_DEFAULT_FG       ::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_EDITED_DEFAULT_FG                 ::GetSysColor(COLOR_WINDOWTEXT)

#define DIFFSTATE_UNKNOWN_DEFAULT_BG                ::GetSysColor(COLOR_WINDOW)
#define DIFFSTATE_NORMAL_DEFAULT_BG                 ::GetSysColor(COLOR_WINDOW)
#define DIFFSTATE_REMOVED_DEFAULT_BG                RGB(255,200,100)
#define DIFFSTATE_REMOVEDWHITESPACE_DEFAULT_BG      DIFFSTATE_NORMAL_DEFAULT_BG
#define DIFFSTATE_ADDED_DEFAULT_BG                  RGB(255,255,0)
#define DIFFSTATE_ADDEDWHITESPACE_DEFAULT_BG        DIFFSTATE_NORMAL_DEFAULT_BG
#define DIFFSTATE_WHITESPACE_DEFAULT_BG             DIFFSTATE_NORMAL_DEFAULT_BG
#define DIFFSTATE_WHITESPACE_DIFF_DEFAULT_BG        DIFFSTATE_NORMAL_DEFAULT_BG
#define DIFFSTATE_EMPTY_DEFAULT_BG                  RGB(200,200,200)
#define DIFFSTATE_CONFLICTED_DEFAULT_BG             RGB(255,100,100)
#define DIFFSTATE_CONFLICTED_IGNORED_DEFAULT_BG     DIFFSTATE_CONFLICTED_DEFAULT_BG
#define DIFFSTATE_CONFLICTADDED_DEFAULT_BG          DIFFSTATE_CONFLICTED_DEFAULT_BG
#define DIFFSTATE_CONFLICTEMPTY_DEFAULT_BG          DIFFSTATE_CONFLICTED_DEFAULT_BG
#define DIFFSTATE_MOVEDFROM_DEFAULT_BG              DIFFSTATE_REMOVED_DEFAULT_BG
#define DIFFSTATE_MOVEDTO_DEFAULT_BG                DIFFSTATE_ADDED_DEFAULT_BG
#define DIFFSTATE_IDENTICAL_MOVEDFROM_DEFAULT_BG    DIFFSTATE_NORMAL_DEFAULT_BG
#define DIFFSTATE_IDENTICAL_MOVEDTO_DEFAULT_BG      DIFFSTATE_NORMAL_DEFAULT_BG
#define DIFFSTATE_IDENTICALREMOVED_DEFAULT_BG       DIFFSTATE_REMOVED_DEFAULT_BG
#define DIFFSTATE_IDENTICALADDED_DEFAULT_BG         DIFFSTATE_ADDED_DEFAULT_BG
#define DIFFSTATE_THEIRSREMOVED_DEFAULT_BG          DIFFSTATE_REMOVED_DEFAULT_BG
#define DIFFSTATE_THEIRSADDED_DEFAULT_BG            DIFFSTATE_ADDED_DEFAULT_BG
#define DIFFSTATE_YOURSREMOVED_DEFAULT_BG           DIFFSTATE_REMOVED_DEFAULT_BG
#define DIFFSTATE_YOURSADDED_DEFAULT_BG             DIFFSTATE_ADDED_DEFAULT_BG
#define DIFFSTATE_CONFLICTRESOLVED_DEFAULT_BG       RGB(200,255,200)
#define DIFFSTATE_EDITED_DEFAULT_BG                 MODIFIED_COLOR

/**
 * \ingroup TortoiseMerge
 * Singleton class which handles the colors for the different diff states
 * used in TortoiseMerge.
 */
class CDiffColors
{
public:
    static CDiffColors&         GetInstance();

    void                        GetColors(DiffStates state, COLORREF &crBkgnd, COLORREF &crText);
    void                        SetColors(DiffStates state, const COLORREF &crBkgnd, const COLORREF &crText);
    void                        LoadRegistry();

protected:
    CRegDWORD                   m_regForegroundColors[DIFFSTATE_END];
    CRegDWORD                   m_regBackgroundColors[DIFFSTATE_END];
private:
    CDiffColors(void);
    ~CDiffColors(void);
};
