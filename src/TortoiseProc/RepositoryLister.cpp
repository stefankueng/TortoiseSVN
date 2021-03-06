// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009-2015, 2018, 2021 - TortoiseSVN

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
#include "RepositoryLister.h"
#include "UnicodeUtils.h"
#include "PathUtils.h"
#include "resource.h"

#include "SVNProperties.h"
#include "SVNInfo.h"
#include "SVNError.h"
#include "SVNHelpers.h"

#include <functional>

/////////////////////////////////////////////////////////////////////
// CRepositoryLister::CQuery
/////////////////////////////////////////////////////////////////////

// auto-schedule upon construction

CRegDWORD fetchingLocksEnabled(L"Software\\TortoiseSVN\\RepoBrowserShowLocks", TRUE);

CRepositoryLister::CQuery::CQuery(const CTSVNPath& path, const SVNRev& pegRevision, apr_uint32_t dirent, const SRepositoryInfo& repository)
    : path(path)
    , dirent(dirent)
    , pegRevision(pegRevision)
    , repository(repository)
{
}

// parameter access

const CTSVNPath& CRepositoryLister::CQuery::GetPath() const
{
    return path;
}

const SVNRev& CRepositoryLister::CQuery::GetRevision() const
{
    return repository.revision;
}

const SVNRev& CRepositoryLister::CQuery::GetPegRevision() const
{
    return pegRevision.IsValid() ? pegRevision : GetRevision();
}

// result access. Automatically waits for execution to be finished.

const std::deque<CItem>& CRepositoryLister::CQuery::GetResult()
{
    WaitUntilDone(true);
    return result;
}

const CString& CRepositoryLister::CQuery::GetError()
{
    WaitUntilDone(true);

    if (error.IsEmpty() && HasBeenTerminated())
        error.LoadString(IDS_REPOBROWSE_ERR_CANCEL);

    return error;
}

bool CRepositoryLister::CQuery::Succeeded()
{
    return GetError().IsEmpty();
}

/////////////////////////////////////////////////////////////////////
// CRepositoryLister::CListQuery
/////////////////////////////////////////////////////////////////////

// callback from the SVN::List() method which stores all the information

BOOL CRepositoryLister::CListQuery::ReportList(const CString& listPath, svn_node_kind_t kind, svn_filesize_t size, bool hasProps, svn_revnum_t createdRev, apr_time_t time, const CString& author, const CString& lockToken, const CString& lockowner, const CString& lockcomment, bool isDavComment, apr_time_t lockCreationDate, apr_time_t lockExpirationDate, const CString& absolutePath, const CString& externalParentUrl, const CString& externalTarget)
{
    UNREFERENCED_PARAMETER(externalParentUrl);
    UNREFERENCED_PARAMETER(externalTarget);
    // skip the parent path

    if (listPath.IsEmpty())
    {
        // terminate with an error if this was actually a file

        return kind == svn_node_dir ? TRUE : FALSE;
    }

    // store dir entry

    bool    abspathHasSlash = (absolutePath.GetAt(absolutePath.GetLength() - 1) == '/');
    CString relPath         = absolutePath + (abspathHasSlash ? L"" : L"/");
    CItem   entry(listPath, externalTarget, kind, size, hasProps, createdRev, time, author, lockToken, lockowner, lockcomment, isDavComment, lockCreationDate, lockExpirationDate, repository.root + relPath + listPath, repository);

    result.push_back(entry);

    return TRUE;
}

// early termination

BOOL CRepositoryLister::CListQuery::Cancel()
{
    return HasBeenTerminated();
}

// actual job code: just call \ref SVN::List

void CRepositoryLister::CListQuery::InternalExecute()
{
    // TODO: let the svn API fetch the externals
    bool fetchLocks = !!static_cast<DWORD>(fetchingLocksEnabled);
    if (!List(path, GetRevision(), GetPegRevision(), svn_depth_immediates, fetchLocks, dirent, false))
    {
        // something went wrong or query was cancelled
        // -> store error, clear results and terminate sub-queries

        if (externalsQuery != nullptr)
            externalsQuery->Terminate();

        result.clear();

        error = GetLastErrorMessage();
        if (error.IsEmpty())
            error.LoadString(IDS_REPOBROWSE_ERR_UNKNOWN);
    }
    else
    {
        // add results from the sub-query
        if (!m_redirectedUrl.IsEmpty())
            redirectedUrl = m_redirectedUrl.GetSVNPathString();

        if (externalsQuery != nullptr)
        {
            if (externalsQuery->Succeeded())
            {
                const std::deque<CItem>& externals = externalsQuery->GetResult();

                for (std::deque<CItem>::const_iterator iter = externals.begin(), end = externals.end(); iter != end; ++iter)
                {
                    if (iter->m_externalPosition == 0)
                        result.push_back(*iter);
                    else
                        subPathExternals.push_back(*iter);
                }
            }
            else
            {
                result.clear();
                error = externalsQuery->GetError();
            }
        }
    }
}

// wait for embedded job to finish

CRepositoryLister::CListQuery::~CListQuery()
{
    if (externalsQuery != nullptr)
        externalsQuery->Delete(true);
}

// auto-schedule upon construction

CRepositoryLister::CListQuery::CListQuery(const CTSVNPath& path, const SVNRev& pegRevision, const SRepositoryInfo& repository, apr_uint32_t dirent, bool includeExternals, bool runSilently, async::CJobScheduler* scheduler)
    : CQuery(path, pegRevision, dirent, repository)
    , SVN(runSilently)
    , externalsQuery(includeExternals
                         ? new CExternalsQuery(path, pegRevision, repository, runSilently, scheduler)
                         : nullptr)
{
    CJobBase::Schedule(false, scheduler);
}

// cancel the svn:externals sub query as well

void CRepositoryLister::CListQuery::Terminate()
{
    CQuery::Terminate();

    if (externalsQuery != nullptr)
        externalsQuery->Terminate();
}

// access additional results

const std::deque<CItem>& CRepositoryLister::CListQuery::GetSubPathExternals()
{
    WaitUntilDone(true);
    return subPathExternals;
}

// true, if this query has been run silently and some error occurred

bool CRepositoryLister::CListQuery::ShouldBeRerun()
{
    return m_prompt.IsSilent() && !Succeeded();
}

/////////////////////////////////////////////////////////////////////
// CRepositoryLister::CExternalsQuery
/////////////////////////////////////////////////////////////////////

// actual job code: fetch externals and parse them

void CRepositoryLister::CExternalsQuery::InternalExecute()
{
    // fetch externals definition for the given path

    static const std::string svnExternals(SVN_PROP_EXTERNALS);

    SVNReadProperties properties(path, GetRevision(), GetPegRevision(), runSilently, false);

    std::string externals;
    for (int i = 0, count = properties.GetCount(); i < count; ++i)
        if (properties.GetItemName(i) == svnExternals)
            externals = properties.GetItemValue(i);

    // none there?

    if (externals.empty())
        return;

    // parse externals

    SVNPool pool;

    apr_array_header_t* parsedExternals = nullptr;
    SVNError            svnError(svn_wc_parse_externals_description3(&parsedExternals, path.GetSVNApiPath(pool), externals.c_str(), TRUE, pool));

    if (svnError.GetCode() == 0)
    {
        SVN svn;

        // copy the parser output

        for (long i = 0; i < parsedExternals->nelts; ++i)
        {
            svn_wc_external_item2_t* external = APR_ARRAY_IDX(parsedExternals, i, svn_wc_external_item2_t*);

            if (external != nullptr)
            {
                // get target repository

                CStringA absoluteURL = CPathUtils::GetAbsoluteURL(external->url, CUnicodeUtils::GetUTF8(repository.root), CUnicodeUtils::GetUTF8(path.GetSVNPathString()));

                SRepositoryInfo externalRepository;

                CTSVNPath url;
                url.SetFromSVN(absoluteURL);
                if (HasBeenTerminated())
                    break;
                SVNInfo            info;
                const SVNInfoData* pInfoData = info.GetFirstFileInfo(url, external->peg_revision, external->revision, svn_depth_empty);
                if (HasBeenTerminated())
                    break;

                if (pInfoData)
                    externalRepository.root = pInfoData->reposRoot;
                externalRepository.revision    = external->revision;
                externalRepository.pegRevision = external->peg_revision;

                // add the new entry

                CString relPath = CUnicodeUtils::GetUnicode(external->target_dir);
                CItem   entry(relPath.Mid(relPath.ReverseFind('/') + 1), relPath

                            // even file externals will be treated as folders because
                            // this is a safe default
                            ,
                            pInfoData ? pInfoData->kind : svn_node_dir, 0

                            // actually, we don't know for sure whether the target
                            // URL has props but its the safe default to say 'yes'
                            ,
                            true
                            // explicit revision, HEAD or DATE
                            ,
                            external->revision.kind == svn_opt_revision_number
                                  ? external->revision.value.number
                                  : SVN_IGNORED_REVNUM

                            // explicit revision, HEAD or DATE
                            ,
                            external->revision.kind == svn_opt_revision_date
                                  ? external->revision.value.date
                                  : 0,
                            CString(), CString(), CString(), CString(), false, 0, 0, CPathUtils::PathUnescape(CUnicodeUtils::GetUnicode(absoluteURL)), externalRepository);

                result.push_back(entry);
            }
        }
    }
    else
    {
        error = CUnicodeUtils::GetUnicode(svnError.GetMessage());
    }
}

// auto-schedule upon construction

CRepositoryLister::CExternalsQuery::CExternalsQuery(const CTSVNPath& path, const SVNRev& pegRevision, const SRepositoryInfo& repository, bool runSilently, async::CJobScheduler* scheduler)
    : CQuery(path, pegRevision, true, repository)
    , runSilently(runSilently)
{
    CJobBase::Schedule(false, scheduler);
}

/////////////////////////////////////////////////////////////////////
// CRepositoryLister
/////////////////////////////////////////////////////////////////////

// cleanup utilities

void CRepositoryLister::CompactDumpster()
{
    auto target = dumpster.begin();
    auto end    = dumpster.end();

    for (auto iter = target; iter != end; ++iter)
        if ((*iter)->GetStatus() == async::IJob::done)
            (*iter)->Delete(true);
        else
            *(target++) = *iter;

    dumpster.erase(target, end);
}

void CRepositoryLister::ClearDumpster()
{
    std::for_each(dumpster.begin(), dumpster.end(), [](auto obj) { obj->WaitUntilDone(true); });

    CompactDumpster();
}

// parameter encoding utility

CTSVNPath CRepositoryLister::EscapeUrl(const CString& url)
{
    CStringA prepUrl = CUnicodeUtils::GetUTF8(CTSVNPath(url).GetSVNPathString());
    prepUrl.Replace("%", "%25");
    return CTSVNPath(CUnicodeUtils::GetUnicode(CPathUtils::PathEscape(prepUrl)));
}

// simple construction

CRepositoryLister::CRepositoryLister()
    : scheduler(8, 0, true, false)
{
}

// wait for all jobs to finish before returning

CRepositoryLister::~CRepositoryLister()
{
    Refresh();
    ClearDumpster();
}

// we probably will call \ref GetList() on that \ref url soon

void CRepositoryLister::Enqueue(const CString& url, const SVNRev& pegRev, const SRepositoryInfo& repository, apr_uint32_t dirent, bool includeExternals, bool runSilently)
{
    CTSVNPath escapedURL = EscapeUrl(url);

    async::CCriticalSectionLock lock(mutex);

    SPathAndRev key(escapedURL, pegRev, repository.revision);
    auto        iter = queries.find(key);
    if (iter != queries.end())
    {
        // exists but may we want to rerun failed prefetch
        // queries if this is not a mere prefetch request

        if (runSilently || !iter->second->ShouldBeRerun())
            return;

        // remove and & rerun

        dumpster.push_back(iter->second);
        queries.erase(iter);
    }

    // trim list of SVN requests

    if (scheduler.GetQueueDepth() > MAX_QUEUE_DEPTH)
    {
        // reduce the number of SVN requests to half the limit by
        // extracting the oldest (and least probably needed) ones

        std::vector<async::IJob*> removed = scheduler.RemoveJobFromQueue(MAX_QUEUE_DEPTH / 2, true);

        // we can only delete CListQuery instances
        // -> (temp.) re-add all others.
        // Terminate the list queries, so the embedded externals
        // queries get terminated as well and we don't have
        // to re-add them again.

        std::vector<CListQuery*> deletedQueries;
        deletedQueries.reserve(removed.size());

        for (size_t i = 0, count = removed.size(); i < count; ++i)
        {
            CListQuery* query = dynamic_cast<CListQuery*>(removed[i]);
            if (query != nullptr)
            {
                query->Terminate();
                deletedQueries.push_back(query);
            }
        }

        // return the externals queries that have not been cancelled

        for (size_t i = 0, count = removed.size(); i < count; ++i)
        {
            auto rQuery = dynamic_cast<CQuery*>(removed[i]);
            if (rQuery && !rQuery->HasBeenTerminated())
                scheduler.Schedule(removed[i], false);
        }

        // remove terminated queries from our lookup tables

        for (size_t i = 0, count = deletedQueries.size(); i < count; ++i)
        {
            // remove query from list of "valid" queries
            // (note: there might be a new query for the same key)

            CListQuery* query = deletedQueries[i];

            SPathAndRev keydel(query->GetPath(), query->GetPegRevision(), query->GetRevision());

            auto iterDel = queries.find(keydel);
            if ((iterDel != queries.end()) && (iterDel->second == query))
                queries.erase(iterDel);
        }

        // if the dumpster has not been empty before for some reason,
        // then the removed queries might already have been dumped.
        // Now, they would be dumped a second time -> prevent that.

        std::sort(dumpster.begin(), dumpster.end());
        std::sort(deletedQueries.begin(), deletedQueries.end());

        std::vector<CListQuery*> toDump;
        std::set_difference(deletedQueries.begin(), deletedQueries.end(), dumpster.begin(), dumpster.end(), std::back_inserter(toDump));

        dumpster.insert(dumpster.end(), toDump.begin(), toDump.end());
    }

    // add the query whose result we will probably need

    queries[key] = new CListQuery(escapedURL, pegRev, repository, dirent, includeExternals, runSilently, &scheduler);
}

// remove all unfinished entries from the job queue

void CRepositoryLister::Cancel()
{
    async::CCriticalSectionLock lock(mutex);

    // move all unfinished queries to the dumpster

    for (auto iter = queries.begin(); iter != queries.end();)
    {
        if (iter->second->GetStatus() != async::IJob::done)
        {
            iter->second->Terminate();
            dumpster.push_back(iter->second);
            iter = queries.erase(iter);
        }
        else
        {
            ++iter;
        }
    }

    // finally destroy all entries that have already been processed

    CompactDumpster();
}

// wait for all jobs to be finished

void CRepositoryLister::WaitForJobsToFinish()
{
    scheduler.WaitForEmptyQueue();
}

// wait for all jobs to be finished

std::unique_ptr<async::CSchedulerSuspension> CRepositoryLister::SuspendJobs()
{
    return std::make_unique<async::CSchedulerSuspension>(scheduler);
}

// don't return results from previous or still running requests
// the next time \ref GetList() gets called

void CRepositoryLister::Refresh()
{
    async::CCriticalSectionLock lock(mutex);

    // move all revision-specific queries to the dumpster

    for (auto iter = queries.begin(), end = queries.end(); iter != end; ++iter)
    {
        iter->second->Terminate();
        dumpster.push_back(iter->second);
    }

    queries.clear();

    // finally destroy all entries that have already been processed

    CompactDumpster();
}

// invalidate only those entries that belong to the given \ref revision

void CRepositoryLister::Refresh(const SVNRev& revision)
{
    async::CCriticalSectionLock lock(mutex);

    // move all HEAD queries for a specific revision the dumpster

    for (auto iter = queries.begin(); iter != queries.end();)
    {
        if (iter->first.rev == revision)
        {
            iter->second->Terminate();
            dumpster.push_back(iter->second);
            iter = queries.erase(iter);
        }
        else
        {
            ++iter;
        }
    }

    // finally destroy all entries that have already been processed

    CompactDumpster();
}

// like \ref Refresh() but may only affect those nodes in the
// sub-tree starting at the specified \ref url.

void CRepositoryLister::RefreshSubTree(const SVNRev& revision, const CString& url)
{
    CTSVNPath escapedURL = EscapeUrl(url);

    async::CCriticalSectionLock lock(mutex);

    // move all HEAD queries for a specific revision the dumpster

    for (auto iter = queries.begin(); iter != queries.end();)
    {
        if ((iter->first.rev == revision) && ((escapedURL == iter->first.path) || escapedURL.IsAncestorOf(iter->first.path)))
        {
            iter->second->Terminate();
            dumpster.push_back(iter->second);
            iter = queries.erase(iter);
        }
        else
        {
            ++iter;
        }
    }

    // finally destroy all entries that have already been processed

    CompactDumpster();
}

CRepositoryLister::CListQuery* CRepositoryLister::FindQuery(const CString& url, const SVNRev& pegRev, const SRepositoryInfo& repository, apr_uint32_t dirent, bool includeExternals)
{
    // ensure there is a suitable query

    Enqueue(url, pegRev, repository, dirent, includeExternals, false);

    // return that query

    SPathAndRev key(EscapeUrl(url), pegRev, repository.revision);
    return queries[key];
}

CString CRepositoryLister::GetList(const CString& url, const SVNRev& pegRev, const SRepositoryInfo& repository, apr_uint32_t dirent, bool includeExternals, std::deque<CItem>& items, CString& redirUrl)
{
    async::CCriticalSectionLock lock(mutex);

    // find that query

    CListQuery* query = FindQuery(url, pegRev, repository, dirent, includeExternals);
    if (query == nullptr)
    {
        // something went very wrong.
        // Report than and let the user do a refresh

        return CString(MAKEINTRESOURCE(IDS_REPOBROWSE_QUERYFAILURE));
    }

    // wait for the results to come in and return them
    // get "ordinary" list plus direct externals

    items    = query->GetResult();
    redirUrl = query->GetRedirectedUrl();
    return query->GetError();
}

CString CRepositoryLister::AddSubTreeExternals(const CString& url, const SVNRev& pegRev, const SRepositoryInfo& repository, const CString& externalsRelPath, std::deque<CItem>& items)
{
    async::CCriticalSectionLock lock(mutex);

    // auto-create / find that query

    CListQuery* query = FindQuery(url, pegRev, repository, true, true);

    // add unfiltered externals?

    auto begin = query->GetSubPathExternals().begin();
    auto end   = query->GetSubPathExternals().end();

    int levels = CItem::Levels(externalsRelPath);
    for (auto iter = begin; iter != end; ++iter)
    {
        if ((iter->m_externalPosition == levels) && (iter->m_externalRelPath.Find(externalsRelPath) == 0))
        {
            items.push_back(*iter);
        }
        // externals can be placed in subfolders which are not versioned:
        // insert such unversioned folders here.
        if (iter->m_externalRelPath.Find(externalsRelPath) == 0)
        {
            int startslash = 0;
            for (int i = 0; i < levels; ++i)
            {
                startslash = iter->m_externalRelPath.Find('/', startslash) + 1;
            }
            int endslash = iter->m_externalRelPath.Find('/', startslash);
            if (endslash < 0)
                endslash = iter->m_externalRelPath.GetLength();
            CString subpath = iter->m_externalRelPath.Mid(startslash, endslash - startslash);
            if (!subpath.IsEmpty())
            {
                bool bFound = false;
                for (const auto& p : items)
                {
                    if (p.m_path.Compare(subpath) == 0)
                    {
                        bFound = true;
                        break;
                    }
                }
                if (!bFound)
                {
                    CItem item          = *iter;
                    item.m_kind         = svn_node_dir;
                    item.m_path         = subpath;
                    item.m_unversioned  = true;
                    item.m_isExternal   = false;
                    item.m_absolutePath = url + L"/" + subpath;
                    items.push_back(item);
                }
            }
        }
    }

    return query->GetError();
}