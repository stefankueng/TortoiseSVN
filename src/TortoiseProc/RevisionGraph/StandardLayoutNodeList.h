// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008,2010 - TortoiseSVN

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

#include "StandardLayout.h"
#include "FullHistory.h"

class CStandardLayoutNodeList : public ILayoutNodeList
{
private:

    const CCachedLogInfo* cache;
    const std::vector<CStandardLayoutNodeInfo>& nodes;
	CFullHistory::SWCInfo wcInfo;

	/// utilities

	index_t GetStyle (const CVisibleGraphNode* node) const;
	DWORD GetStyleFlags (const CVisibleGraphNode* node) const;

public:

    /// construction

    CStandardLayoutNodeList ( const std::vector<CStandardLayoutNodeInfo>& nodes
                            , const CCachedLogInfo* cache
							, const CFullHistory::SWCInfo& wcInfo);

    /// implement ILayoutItemList

    virtual index_t GetCount() const;

    virtual CString GetToolTip (index_t index) const;

    virtual index_t GetFirstVisible (const CRect& viewRect) const;
    virtual index_t GetNextVisible (index_t prev, const CRect& viewRect) const;
    virtual index_t GetAt (const CPoint& point, CSize delta) const;

    /// implement ILayoutNodeList

    virtual SNode GetNode (index_t index) const;
};
