// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008 - TortoiseSVN

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
// necessary includes
///////////////////////////////////////////////////////////////

#include "RepositoryInfo.h"
#include "QuickHash.h"

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{

///////////////////////////////////////////////////////////////
// forward declarations
///////////////////////////////////////////////////////////////

class CCachedLogInfo;
class CStringDictionary;
class CIndexPairDictionary;
class CPathDictionary;
class CTokenizedStringContainer;
class CRevisionIndex;
class CRevisionInfoContainer;
class CSkipRevisionInfo;
class CLogCachePool;

/**
 * Contains all data we may get for a repository log cache.
 */

struct CLogCacheStatisticsData
{
public:

	/// all the data we can get ...

	size_t fileSize;
	size_t ramSize;

    ConnectionState connectionState;

	__time64_t headTimeStamp; 
	__time64_t lastWriteAccess;
	__time64_t lastReadAccess;
	bool dirty;

	revision_t revisionCount;
	revision_t maxRevision;

	index_t authorCount;
	index_t pathElementCount;
	index_t pathCount;
	index_t skipDeltaCount;

	index_t wordTokenCount;
	index_t pairTokenCount;
	index_t textSize;
	size_t uncompressedSize;

	revision_t changesRevisionCount;
	revision_t changesMissingRevisionCount;
	size_t changesCount;

	revision_t mergeInfoRevisionCount;
	revision_t mergeInfoMissingRevisionCount;
	size_t mergeInfoCount;

	revision_t userRevPropRevisionCount;
	revision_t userRevPropMissingRevisionCount;
	size_t userRevPropCount;
};

/**
 * Collects size, access and completeness info for a given
 * cached repository log. All values are 0 for unknown
 * (e.g. non-cached) repositories.
 *
 * Needs full (friend) access to cache container internals.
 */

class CLogCacheStatistics : private CLogCacheStatisticsData
{
private:

    /// utilities

	static size_t GetSizeOf (const CStringDictionary& container);
	static size_t GetSizeOf (const CIndexPairDictionary& container);
	static size_t GetSizeOf (const CPathDictionary& container);
	static size_t GetSizeOf (const CTokenizedStringContainer& container);
	static size_t GetSizeOf (const CRevisionInfoContainer& container);
	static size_t GetSizeOf (const CRevisionIndex& container);
	static size_t GetSizeOf (const CSkipRevisionInfo& container);
	static size_t GetSizeOf (const CCachedLogInfo& container);

	template<class T>
	static size_t GetSizeOf (const std::vector<T>& container);
	template<class T>
	static size_t GetSizeOf (const quick_hash<T>& container);

	bool CacheExists() const;

	static __time64_t GetTime (FILETIME& fileTime);

	/// data collection

	void CollectData (CLogCachePool& pool, const CString& uuid, const CString& root);
	void CollectData (const CCachedLogInfo& source);

public:

	/// construction / destruction:
	/// collect data during construction

	CLogCacheStatistics();
	CLogCacheStatistics (CLogCachePool& pool, const CString& uuid, const CString& root);
	~CLogCacheStatistics();

	/// all back to zero

	void Reset();

    /// data access

    const CLogCacheStatisticsData& GetData() const;
};

///////////////////////////////////////////////////////////////
// utilities
///////////////////////////////////////////////////////////////

template<class T>
static size_t CLogCacheStatistics::GetSizeOf (const std::vector<T>& container)
{
	return container.capacity() * sizeof(T[1]) + sizeof (container);
}

template<class T>
static size_t CLogCacheStatistics::GetSizeOf (const quick_hash<T>& container)
{
	typedef T::index_type index_type;

	return container.statistics().capacity * sizeof(index_type[1]) 
		 + sizeof (container);
}

///////////////////////////////////////////////////////////////
// data access
///////////////////////////////////////////////////////////////

inline const CLogCacheStatisticsData& CLogCacheStatistics::GetData() const
{
    return *this;
}

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

