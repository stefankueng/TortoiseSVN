// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008, 2012-2013 - TortoiseSVN

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

// include base classes

#include "CopyFilterOptions.h"
#include "RevisionGraphOptionsImpl.h"
#include "resource.h"

/** "Pin" all Head nodes, i.e. keep them from being removed by other rules.
*/

class CShowHead
    : public CRevisionGraphOptionImpl< ICopyFilterOption
                                     , 100
                                     , ID_VIEW_SHOWHEAD>
{
public:

    /// construction

    CShowHead (CRevisionGraphOptionList& list);

    /// implement ICopyFilterOption

    virtual EResult ShallRemove (const CFullGraphNode* node) const override;
};
