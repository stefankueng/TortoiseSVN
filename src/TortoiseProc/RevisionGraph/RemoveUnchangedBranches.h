// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008, 2012 - TortoiseSVN

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

#include "ModificationOptions.h"
#include "resource.h"

/** Remove all branches / tags that have been deleted and have
* no surviving copy.
*/

class CRemoveUnchangedBranches
    : public CModificationOptionImpl< IModificationOption
                                    , 200
                                    , ID_VIEW_REMOVEUNCHANGEDBRANCHES
                                    , true           // remove branches first
                                    , false          // from leaves to root
                                    , true>          // this is a cyclic option
{
public:

    /// construction

    CRemoveUnchangedBranches (CRevisionGraphOptionList& list);

    /// implement IModificationOption

    virtual void Apply (CVisibleGraph* graph, CVisibleGraphNode* node) override;
};
