// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2007 - TortoiseSVN

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

///////////////////////////////////////////////////////////////
// include
///////////////////////////////////////////////////////////////

#include "LogCacheGlobals.h"

///////////////////////////////////////////////////////////////
// forward declarations
///////////////////////////////////////////////////////////////

class IHierarchicalInStream;
class IHierarchicalOutStream;

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{

///////////////////////////////////////////////////////////////
//
// CRevisionIndex
//
//		a simple class to map from revision number to revision
//		index.
//
//		To handle high revision counts efficiently, only that
//		range of revisions is stored that actually maps to
//		revision indices (i.e. revisions at the upper and 
//		lower ends may be ommitted).
//
///////////////////////////////////////////////////////////////

class CRevisionIndex
{
private:

	// the mapping and its bias

	revision_t firstRevision;
	std::vector<index_t> indices;

	// sub-stream IDs

	enum
	{
		INDEX_STREAM_ID = 1
	};

public:

	// construction / destruction

	CRevisionIndex(void);
	~CRevisionIndex(void);

	// range of (possibly available) revisions:
	// GetFirstRevision() .. GetLastRevision()-1

	revision_t GetFirstRevision() const
	{
		return firstRevision;
	}
	revision_t GetLastRevision() const
	{
		return firstRevision + (revision_t)indices.size();
	}

	// read

	index_t operator[](revision_t revision) const
	{
		revision -= firstRevision;
		return revision < indices.size()
			? indices[revision]
			: NO_INDEX;
	}

	// insert info (must be NO_INDEX before)

	void SetRevisionIndex (revision_t revision, index_t index);

	// reset content

	void Clear();

	// stream I/O

	friend IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
											 , CRevisionIndex& container);
	friend IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
											  , const CRevisionIndex& container);
};

// stream I/O

IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
								  , CRevisionIndex& container);
IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
								   , const CRevisionIndex& container);

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

