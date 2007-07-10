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
// necessary includes
///////////////////////////////////////////////////////////////

#include "ILogQuery.h"
#include "ILogReceiver.h"

#include "RevisionInfoContainer.h"
#include "DictionaryBasedTempPath.h"

#include "QuickHashMap.h"

///////////////////////////////////////////////////////////////
// forward declarations
///////////////////////////////////////////////////////////////

class SVNInfoData;
class CTSVNPath;

namespace LogCache
{
	class CLogCachePool;
	class CCachedLogInfo;
}

using namespace LogCache;

/**
 * Implements the ILogQuery interface on the log cache. It requires another
 * query to fill the gaps in the cache it may encounter. Those gaps will be
 * filled on-the-fly.
 */
class CCacheLogQuery : public ILogQuery
{
private:

	class CLogFiller : private ILogReceiver
	{
	private:

		/// cache to use & update
		CCachedLogInfo* cache;
		CStringA URL;

		/// connection to the SVN repository
		ILogQuery* svnQuery;

		/// path to log for and the begin of the current gap 
		/// in the log for that path
		std::auto_ptr<CDictionaryBasedTempPath> currentPath;
		revision_t firstNARevision;
		bool followRenames;
        bool revs_only;

		/// the original log receiver (may be NULL)
		ILogReceiver* receiver;

		/// make sure, we can iterator over the given range for the given path
		void MakeRangeIterable ( const CDictionaryBasedPath& path
							   , revision_t startRevision
							   , revision_t count);

		/// implement ILogReceiver
		virtual void ReceiveLog ( LogChangedPathArray* changes
								, svn_revnum_t rev
								, const CString& author
								, const apr_time_t& timeStamp
								, const CString& message);

	public:

		/// actually call SVN
		/// return the last revision sent to the receiver
		revision_t FillLog ( CCachedLogInfo* cache
						   , const CStringA& URL
						   , ILogQuery* svnQuery
						   , revision_t startRevision
						   , revision_t endRevision
						   , const CDictionaryBasedTempPath& startPath
						   , int limit
						   , bool strictNodeHistory
                           , ILogReceiver* receiver
                           , bool revs_only);
	};

	/// we get our cache from here
	CLogCachePool* caches;

	/// cache to use & update
	CCachedLogInfo* cache;
	CStringA URL;

	/// used, if caches is NULL
	CCachedLogInfo* tempCache;

	/// connection to the SVN repository (may be NULL)
	ILogQuery* svnQuery;

    /// efficient map cached string / path -> CString
    typedef quick_hash_map<index_t, CString> TID2String;

    TID2String authorToStringMap;
    TID2String pathToStringMap;

	/// Determine the revision range to pass to SVN.
	revision_t NextAvailableRevision ( const CDictionaryBasedTempPath& path
									 , revision_t firstMissingRevision
								     , revision_t endRevision) const;

	/// Determine an end-revision that would fill many cache gaps efficiently
	revision_t FindOldestGap ( revision_t startRevision
							 , revision_t endRevision) const;

	/// ask SVN to fill the log -- at least a bit
	/// Possibly, it will stop long before endRevision and limit!
	revision_t FillLog ( revision_t startRevision
					   , revision_t endRevision
					   , const CDictionaryBasedTempPath& startPath
					   , int limit
					   , bool strictNodeHistory
					   , ILogReceiver* receiver
					   , bool revs_only);

	/// fill the receiver's change list buffer 
	std::auto_ptr<LogChangedPathArray> GetChanges 
		( CRevisionInfoContainer::CChangesIterator& first
		, CRevisionInfoContainer::CChangesIterator& last);

	/// crawl the history and forward it to the receiver
	void InternalLog ( revision_t startRevision
					 , revision_t endRevision
					 , const CDictionaryBasedTempPath& startPath
					 , int limit
					 , bool strictNodeHistory
					 , ILogReceiver* receiver
                     , bool revs_only);

	/// follow copy history until the startRevision is reached
	CDictionaryBasedTempPath TranslatePegRevisionPath 
		( revision_t pegRevision
		, revision_t startRevision
		, const CDictionaryBasedTempPath& startPath);

	/// extract the repository-relative path of the URL / file name
	/// and open the cache
	CDictionaryBasedTempPath GetRelativeRepositoryPath (SVNInfoData& info);

	/// get UUID & repository-relative path
	SVNInfoData& GetRepositoryInfo ( const CTSVNPath& path
								   , const SVNRev& pegRevision
 								   , SVNInfoData& baseInfo
								   , SVNInfoData& headInfo) const;

	/// decode special revisions:
	/// base / head must be initialized with NO_REVISION
	/// and will be used to cache these values.
	revision_t DecodeRevision ( const CTSVNPath& path
				  			  , const SVNRev& revision
							  , SVNInfoData& baseInfo
							  , SVNInfoData& headInfo) const;

	/// get the (exactly) one path from targets
	/// throw an exception, if there are none or more than one
	CTSVNPath GetPath (const CTSVNPathList& targets) const;

public:

	// construction / destruction

	CCacheLogQuery (CLogCachePool* caches, ILogQuery* svnQuery);
	virtual ~CCacheLogQuery(void);

	/// query a section from log for multiple paths
	/// (special revisions, like "HEAD", are supported)
	virtual void Log ( const CTSVNPathList& targets
					 , const SVNRev& peg_revision
					 , const SVNRev& start
					 , const SVNRev& end
					 , int limit
					 , bool strictNodeHistory
					 , ILogReceiver* receiver
                     , bool revs_only);

	/// access to the cache
	/// (only valid after calling Log())
	CCachedLogInfo* GetCache();
};
