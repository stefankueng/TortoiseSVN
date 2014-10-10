// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2014 - TortoiseSVN

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
#pragma warning(push)
#include "SVN.h"
#include "svn_props.h"
#include "svn_sorts.h"
#include "client.h"
#include "svn_compat.h"
#pragma warning(pop)

#include "TortoiseProc.h"
#include "ProgressDlg.h"
#include "UnicodeUtils.h"
#include "DirFileEnum.h"
#include "TSVNPath.h"
#include "ShellUpdater.h"
#include "registry.h"
#include "SVNHelpers.h"
#include "SVNStatus.h"
#include "SVNInfo.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "TempFile.h"
#include "SVNAdminDir.h"
#include "SVNConfig.h"
#include "SVNError.h"
#include "SVNLogQuery.h"
#include "CacheLogQuery.h"
#include "RepositoryInfo.h"
#include "LogCacheSettings.h"
#include "CriticalSection.h"
#include "SVNTrace.h"
#include "FormatMessageWrapper.h"
#include "Hooks.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define IDYESTOALL          19


LCID SVN::s_locale = MAKELCID((DWORD)CRegStdDWORD(L"Software\\TortoiseSVN\\LanguageID", MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)), SORT_DEFAULT);
bool SVN::s_useSystemLocale = !!(DWORD)CRegStdDWORD(L"Software\\TortoiseSVN\\UseSystemLocaleForDates", TRUE);

/* Number of micro-seconds between the beginning of the Windows epoch
* (Jan. 1, 1601) and the Unix epoch (Jan. 1, 1970)
*/
#define APR_DELTA_EPOCH_IN_USEC   APR_TIME_C(11644473600000000);

void AprTimeToFileTime(LPFILETIME pft, apr_time_t t)
{
    LONGLONG ll;
    t += APR_DELTA_EPOCH_IN_USEC;
    ll = t * 10;
    pft->dwLowDateTime = (DWORD)ll;
    pft->dwHighDateTime = (DWORD) (ll >> 32);
    return;
}

SVN::SVNLock::SVNLock()
    : creation_date (0)
    , expiration_date (0)
{
}

SVN::SVNProgress::SVNProgress()
    : progress (0)
    , total (0)
    , overall_total (0)
    , BytesPerSecond (0)
{
}


SVN::SVN(bool suppressUI)
    : SVNBase()
    , m_progressWnd(0)
    , m_pProgressDlg(NULL)
    , m_bShowProgressBar(false)
    , progress_total(0)
    , progress_lastprogress(0)
    , progress_lasttotal(0)
    , logCachePool()
    , m_commitRev(-1)
    , m_prompt(suppressUI)
    , m_pbCancel(nullptr)
    , m_bListComplete(false)
    , m_progressWndIsCProgress(false)
    , progress_averagehelper(0)
    , progress_lastTicks(0)
    , m_resolvekind((svn_wc_conflict_kind_t)-1)
    , m_resolveresult(svn_wc_conflict_choose_postpone)
    , m_bAssumeCacheEnabled(false)
{
    parentpool = svn_pool_create(NULL);
    svn_error_clear(svn_client_create_context2(&m_pctx, SVNConfig::Instance().GetConfig(parentpool), parentpool));

    pool = svn_pool_create (parentpool);
    svn_ra_initialize(pool);

    if (m_pctx->config == nullptr)
    {
        svn_pool_destroy (pool);
        svn_pool_destroy (parentpool);
        exit(-1);
    }

    // set up authentication
    m_prompt.Init(parentpool, m_pctx);

    m_pctx->log_msg_func3 = svn_cl__get_log_message;
    m_pctx->log_msg_baton3 = logMessage(L"");
    m_pctx->notify_func2 = notify;
    m_pctx->notify_baton2 = this;
    m_pctx->notify_func = NULL;
    m_pctx->notify_baton = NULL;
    m_pctx->conflict_func = NULL;
    m_pctx->conflict_baton = NULL;
    m_pctx->conflict_func2 = conflict_resolver;
    m_pctx->conflict_baton2 = this;
    m_pctx->cancel_func = cancel;
    m_pctx->cancel_baton = this;
    m_pctx->progress_func = progress_func;
    m_pctx->progress_baton = this;
    m_pctx->client_name = SVNHelper::GetUserAgentString(pool);
}

SVN::~SVN(void)
{
    ResetLogCachePool();

    svn_error_clear(Err);
    svn_pool_destroy (parentpool);
}

#pragma warning(push)
#pragma warning(disable: 4100)  // unreferenced formal parameter

BOOL SVN::Cancel() { if (m_pbCancel) {return *m_pbCancel; } return FALSE;}

BOOL SVN::Notify(const CTSVNPath& path, const CTSVNPath& url, svn_wc_notify_action_t action,
                svn_node_kind_t kind, const CString& mime_type,
                svn_wc_notify_state_t content_state,
                svn_wc_notify_state_t prop_state, svn_revnum_t rev,
                const svn_lock_t * lock, svn_wc_notify_lock_state_t lock_state,
                const CString& changelistname,
                const CString& propertyName,
                svn_merge_range_t * range,
                svn_error_t * err, apr_pool_t * pool) {return TRUE;};
BOOL SVN::Log(svn_revnum_t rev, const std::string& author, const std::string& message, apr_time_t time, const MergeInfo* mergeInfo) {return TRUE;}
BOOL SVN::BlameCallback(LONG linenumber, bool localchange, svn_revnum_t revision,
                        const CString& author, const CString& date, svn_revnum_t merged_revision,
                        const CString& merged_author, const CString& merged_date,
                        const CString& merged_path, const CStringA& line,
                        const CStringA& log_msg, const CStringA& merged_log_msg) {return TRUE;}
svn_error_t* SVN::DiffSummarizeCallback(const CTSVNPath& path, svn_client_diff_summarize_kind_t kind, bool propchanged, svn_node_kind_t node) {return SVN_NO_ERROR;}
BOOL SVN::ReportList(const CString& path, svn_node_kind_t kind,
                     svn_filesize_t size, bool has_props, bool complete,
                     svn_revnum_t created_rev, apr_time_t time,
                     const CString& author, const CString& locktoken,
                     const CString& lockowner, const CString& lockcomment,
                     bool is_dav_comment, apr_time_t lock_creationdate,
                     apr_time_t lock_expirationdate, const CString& absolutepath,
                     const CString& externalParentUrl, const CString& externalTarget) {return TRUE;}
svn_wc_conflict_choice_t SVN::ConflictResolveCallback(const svn_wc_conflict_description2_t *description, CString& mergedfile)
{
    if (m_resolvekind == description->kind)
        return m_resolveresult;

    return svn_wc_conflict_choose_postpone;
}

#pragma warning(pop)

struct log_msg_baton3
{
  const char *message;  /* the message. */
  const char *message_encoding; /* the locale/encoding of the message. */
  const char *base_dir; /* the base directory for an external edit. UTF-8! */
  const char *tmpfile_left; /* the tmpfile left by an external edit. UTF-8! */
  apr_pool_t *pool; /* a pool. */
};


bool SVN::Checkout(const CTSVNPath& moduleName, const CTSVNPath& destPath, const SVNRev& pegrev,
                   const SVNRev& revision, svn_depth_t depth, bool bIgnoreExternals,
                   bool bAllow_unver_obstructions)
{
    SVNPool subpool(pool);
    Prepare();

    CHooks::Instance().PreConnect(CTSVNPathList(moduleName));
    const char* svnPath = moduleName.GetSVNApiPath(subpool);
    SVNTRACE(
        Err = svn_client_checkout3 (NULL,           // we don't need the resulting revision
                                    svnPath,
                                    destPath.GetSVNApiPath(subpool),
                                    pegrev,
                                    revision,
                                    depth,
                                    bIgnoreExternals,
                                    bAllow_unver_obstructions,
                                    m_pctx,
                                    subpool ),
        svnPath
    );
    ClearCAPIAuthCacheOnError();

    return (Err == NULL);
}

bool SVN::Remove(const CTSVNPathList& pathlist, bool force, bool keeplocal, const CString& message, const RevPropHash& revProps)
{
    // svn_client_delete needs to run on a sub-pool, so that after it's run, the pool
    // cleanups get run.  For example, after a failure do to an unforced delete on
    // a modified file, if you don't do a cleanup, the WC stays locked
    SVNPool subPool(pool);
    Prepare();
    m_pctx->log_msg_baton3 = logMessage(message);

    apr_hash_t * revPropHash = MakeRevPropHash(revProps, subPool);

    CallPreConnectHookIfUrl(pathlist);

    SVNTRACE(
        Err = svn_client_delete4 (pathlist.MakePathArray(subPool),
                                  force,
                                  keeplocal,
                                  revPropHash,
                                  commitcallback2,
                                  this,
                                  m_pctx,
                                  subPool) ,
        NULL
    );

    ClearCAPIAuthCacheOnError();

    if(Err != NULL)
    {
        return FALSE;
    }

    for(int nPath = 0; nPath < pathlist.GetCount(); nPath++)
    {
        if ((!keeplocal)&&(!pathlist[nPath].IsDirectory()))
        {
            SHChangeNotify(SHCNE_DELETE, SHCNF_PATH | SHCNF_FLUSHNOWAIT, pathlist[nPath].GetWinPath(), NULL);
        }
        else
        {
            SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH | SHCNF_FLUSHNOWAIT, pathlist[nPath].GetWinPath(), NULL);
        }
    }

    return true;
}

bool SVN::Revert(const CTSVNPathList& pathlist, const CStringArray& changelists, bool recurse, bool clearchangelists)
{
    TRACE("Reverting list of %d files\n", pathlist.GetCount());
    SVNPool subpool(pool);
    apr_array_header_t * clists = MakeChangeListArray(changelists, subpool);

    Prepare();

    SVNTRACE(
        Err = svn_client_revert3(pathlist.MakePathArray(subpool),
            recurse ? svn_depth_infinity : svn_depth_empty,
            clists,
            clearchangelists,
            m_pctx,
            subpool) ,
        NULL
    );

    return (Err == NULL);
}


bool SVN::Add(const CTSVNPathList& pathList, ProjectProperties * props, svn_depth_t depth, bool force, bool bUseAutoprops, bool no_ignore, bool addparents)
{
    SVNTRACE_BLOCK

    // the add command should use the mime-type file
    const char *mimetypes_file;
    Prepare();

    svn_config_t * opt = (svn_config_t *)apr_hash_get (m_pctx->config, SVN_CONFIG_CATEGORY_CONFIG,
        APR_HASH_KEY_STRING);
    if (opt)
    {
        if (bUseAutoprops)
        {
            svn_config_get(opt, &mimetypes_file,
                SVN_CONFIG_SECTION_MISCELLANY,
                SVN_CONFIG_OPTION_MIMETYPES_FILE, FALSE);
            if (mimetypes_file && *mimetypes_file)
            {
                Err = svn_io_parse_mimetypes_file(&(m_pctx->mimetypes_map),
                    mimetypes_file, pool);
                if (Err)
                    return false;
            }
            if (props)
                props->InsertAutoProps(opt);
        }
    }

    for(int nItem = 0; nItem < pathList.GetCount(); nItem++)
    {
        CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": add file %s\n", pathList[nItem].GetWinPath());
        if (Cancel())
        {
            Err = svn_error_create(SVN_ERR_CANCELLED, NULL, CUnicodeUtils::GetUTF8(CString(MAKEINTRESOURCE(IDS_SVN_USERCANCELLED))));
            return false;
        }
        SVNPool subpool(pool);
        Err = svn_client_add5 (pathList[nItem].GetSVNApiPath(subpool), depth, force, no_ignore, !bUseAutoprops, addparents, m_pctx, subpool);
        if(Err != NULL)
        {
            return false;
        }
    }

    return true;
}

bool SVN::AddToChangeList(const CTSVNPathList& pathList, const CString& changelist, svn_depth_t depth, const CStringArray& changelists)
{
    SVNPool subpool(pool);
    Prepare();

    if (changelist.IsEmpty())
        return RemoveFromChangeList(pathList, CStringArray(), depth);

    apr_array_header_t *clists = MakeChangeListArray(changelists, subpool);

    Err = svn_client_add_to_changelist(pathList.MakePathArray(subpool),
        (LPCSTR)CUnicodeUtils::GetUTF8(changelist),
        depth,
        clists,
        m_pctx,
        subpool);

    return (Err == NULL);
}

bool SVN::RemoveFromChangeList(const CTSVNPathList& pathList, const CStringArray& changelists, svn_depth_t depth)
{
    SVNPool subpool(pool);
    Prepare();
    apr_array_header_t * clists = MakeChangeListArray(changelists, subpool);

    Err = svn_client_remove_from_changelists(pathList.MakePathArray(subpool),
        depth,
        clists,
        m_pctx,
        subpool);

    return (Err == NULL);
}

bool SVN::Update(const CTSVNPathList& pathList, const SVNRev& revision,
                 svn_depth_t depth, bool depthIsSticky,
                 bool ignoreexternals, bool bAllow_unver_obstructions,
                 bool makeParents)
{
    SVNPool localpool(pool);
    Prepare();
    CHooks::Instance().PreConnect(pathList);
    SVNTRACE(
        Err = svn_client_update4(NULL,
                                pathList.MakePathArray(pool),
                                revision,
                                depth,
                                depthIsSticky,
                                ignoreexternals,
                                bAllow_unver_obstructions,
                                true,       // adds_as_modifications (why would anyone want a tree conflict? Set to true unconditionally)
                                makeParents,
                                m_pctx,
                                localpool),
        NULL
    );
    ClearCAPIAuthCacheOnError();
    return (Err == NULL);
}

svn_revnum_t SVN::Commit(const CTSVNPathList& pathlist, const CString& message,
                         const CStringArray& changelists, bool keepchangelist, svn_depth_t depth, bool keep_locks, const RevPropHash& revProps)
{
    SVNPool localpool(pool);

    Prepare();

    apr_array_header_t *clists = MakeChangeListArray(changelists, localpool);

    apr_hash_t * revprop_table = MakeRevPropHash(revProps, localpool);

    m_pctx->log_msg_baton3 = logMessage(message);

    CHooks::Instance().PreConnect(pathlist);

    SVNTRACE(
        Err = svn_client_commit6 (
                                  pathlist.MakePathArray(pool),
                                  depth,
                                  keep_locks,
                                  keepchangelist,
                                  true,       // commit_as_operations
                                  true,       // include file externals
                                  false,      // include dir externals
                                  clists,
                                  revprop_table,
                                  commitcallback2,
                                  this,
                                  m_pctx,
                                  localpool),
        NULL
    );
    m_pctx->log_msg_baton3 = logMessage(L"");
    ClearCAPIAuthCacheOnError();
    if(Err != NULL)
    {
        return 0;
    }

    svn_revnum_t finrev = -1;

    return finrev;
}

bool SVN::Copy(const CTSVNPathList& srcPathList, const CTSVNPath& destPath,
               const SVNRev& revision, const SVNRev& pegrev, const CString& logmsg, bool copy_as_child,
               bool make_parents, bool ignoreExternals, const RevPropHash& revProps)
{
    SVNPool subpool(pool);

    Prepare();

    m_pctx->log_msg_baton3 = logMessage(logmsg);
    apr_hash_t * revPropHash = MakeRevPropHash(revProps, subpool);

    CallPreConnectHookIfUrl(srcPathList, destPath);

    SVNTRACE(
        Err = svn_client_copy6 (MakeCopyArray(srcPathList, revision, pegrev),
                                destPath.GetSVNApiPath(subpool),
                                copy_as_child,
                                make_parents,
                                ignoreExternals,
                                revPropHash,
                                commitcallback2,
                                this,
                                m_pctx,
                                subpool),
        NULL
    );
    ClearCAPIAuthCacheOnError();
    if(Err != NULL)
    {
        return false;
    }

    return true;
}

bool SVN::Move(const CTSVNPathList& srcPathList, const CTSVNPath& destPath,
               const CString& message /* = L""*/,
               bool move_as_child /* = false*/, bool make_parents /* = false */,
               bool allow_mixed /* = false */,
               bool metadata_only /* = false */,
               const RevPropHash& revProps /* = RevPropHash() */ )
{
    SVNPool subpool(pool);

    Prepare();
    m_pctx->log_msg_baton3 = logMessage(message);
    apr_hash_t * revPropHash = MakeRevPropHash(revProps, subpool);
    CallPreConnectHookIfUrl(srcPathList, destPath);
    SVNTRACE (
        Err = svn_client_move7 (srcPathList.MakePathArray(subpool),
                                destPath.GetSVNApiPath(subpool),
                                move_as_child,
                                make_parents,
                                allow_mixed,
                                metadata_only,
                                revPropHash,
                                commitcallback2,
                                this,
                                m_pctx,
                                subpool),
        NULL
    );

    ClearCAPIAuthCacheOnError();
    if(Err != NULL)
    {
        return false;
    }

    return true;
}

bool SVN::MakeDir(const CTSVNPathList& pathlist, const CString& message, bool makeParents, const RevPropHash& revProps)
{
    Prepare();
    m_pctx->log_msg_baton3 = logMessage(message);
    apr_hash_t * revPropHash = MakeRevPropHash(revProps, pool);

    CallPreConnectHookIfUrl(pathlist);

    SVNTRACE (
        Err = svn_client_mkdir4 (pathlist.MakePathArray(pool),
                                 makeParents,
                                 revPropHash,
                                 commitcallback2,
                                 this,
                                 m_pctx,
                                 pool) ,
        NULL
    );
    ClearCAPIAuthCacheOnError();
    if(Err != NULL)
    {
        return false;
    }

    return true;
}

bool SVN::CleanUp(const CTSVNPath& path, bool breaklocks, bool fixtimestamps, bool cleardavcache, bool vacuumpristines, bool includeexternals)
{
    Prepare();
    SVNPool subpool(pool);
    const char* svnPath = path.GetSVNApiPath(subpool);
    SVNTRACE (
        Err = svn_client_cleanup2 (svnPath, breaklocks, fixtimestamps, cleardavcache, vacuumpristines, includeexternals, m_pctx, subpool),
        svnPath
    )

    return (Err == NULL);
}

bool SVN::Vacuum(const CTSVNPath& path, bool unversioned, bool ignored, bool fixtimestamps, bool pristines, bool includeExternals)
{
    Prepare();
    SVNPool subpool(pool);
    const char* svnPath = path.GetSVNApiPath(subpool);
    SVNTRACE(
        Err = svn_client_vacuum(svnPath, unversioned, ignored, fixtimestamps, pristines, includeExternals, m_pctx, subpool),
        svnPath
        )

        return (Err == NULL);
}

bool SVN::Resolve(const CTSVNPath& path, svn_wc_conflict_choice_t result, bool recurse, bool typeonly, svn_wc_conflict_kind_t kind)
{
    SVNPool subpool(pool);
    Prepare();

    if (typeonly)
    {
        m_resolvekind = kind;
        m_resolveresult = result;
        // change result to unspecified so the conflict resolve callback is invoked
        result = svn_wc_conflict_choose_unspecified;
    }

    switch (kind)
    {
    case svn_wc_conflict_kind_property:
    case svn_wc_conflict_kind_tree:
        break;
    case svn_wc_conflict_kind_text:
    default:
        {
            // before marking a file as resolved, we move the conflicted parts
            // to the trash bin: just in case the user later wants to get those
            // files back anyway
            SVNInfo info;
            const SVNInfoData * infodata = info.GetFirstFileInfo(path, SVNRev(), SVNRev());
            if (infodata)
            {
                CTSVNPathList conflictedEntries;

                for (auto it = infodata->conflicts.cbegin(); it != infodata->conflicts.cend(); ++it)
                {
                    if ((it->conflict_new.GetLength())&&(result != svn_wc_conflict_choose_theirs_full))
                    {
                        conflictedEntries.AddPath(CTSVNPath(it->conflict_new));
                    }
                    if ((it->conflict_old.GetLength())&&(result != svn_wc_conflict_choose_merged))
                    {
                        conflictedEntries.AddPath(CTSVNPath(it->conflict_old));
                    }
                    if ((it->conflict_wrk.GetLength())&&(result != svn_wc_conflict_choose_mine_full))
                    {
                        conflictedEntries.AddPath(CTSVNPath(it->conflict_wrk));
                    }
                }
                conflictedEntries.DeleteAllPaths(true, false, NULL);
            }
        }
        break;
    }

    const char* svnPath = path.GetSVNApiPath(subpool);
    SVNTRACE (
        Err = svn_client_resolve(svnPath,
                                 recurse ? svn_depth_infinity : svn_depth_empty,
                                 result,
                                 m_pctx,
                                 subpool),
        svnPath
    );

    // reset the conflict resolve callback data
    m_resolvekind = (svn_wc_conflict_kind_t)-1;
    m_resolveresult = svn_wc_conflict_choose_postpone;
    return (Err == NULL);
}

bool SVN::Export(const CTSVNPath& srcPath, const CTSVNPath& destPath, const SVNRev& pegrev, const SVNRev& revision,
                 bool force, bool bIgnoreExternals, bool bIgnoreKeywords, svn_depth_t depth, HWND hWnd,
                 SVNExportType extended, const CString& eol)
{
    Prepare();

    if (revision.IsWorking())
    {
        SVNTRACE_BLOCK

        if (g_SVNAdminDir.IsAdminDirPath(srcPath.GetWinPath()))
            return false;
        // files are special!
        if (!srcPath.IsDirectory())
        {
            CopyFile(srcPath.GetWinPath(), destPath.GetWinPath(), FALSE);
            SetFileAttributes(destPath.GetWinPath(), FILE_ATTRIBUTE_NORMAL);
            return true;
        }
        // our own "export" function with a callback and the ability to export
        // unversioned items too. With our own function, we can show the progress
        // of the export with a progress bar - that's not possible with the
        // Subversion API.
        // BUG?: If a folder is marked as deleted, we export that folder too!
        CProgressDlg progress;
        progress.SetTitle(IDS_PROC_EXPORT_3);
        progress.FormatNonPathLine(1, IDS_SVNPROGRESS_EXPORTINGWAIT);
        progress.SetTime(true);
        progress.ShowModeless(hWnd);
        std::map<CTSVNPath, CTSVNPath> copyMap;
        switch (extended)
        {
        case SVNExportNormal:
        case SVNExportOnlyLocalChanges:
            {
                CTSVNPath statusPath;
                svn_client_status_t * s;
                SVNStatus status;
                if ((s = status.GetFirstFileStatus(srcPath, statusPath, false, svn_depth_infinity, true, !!bIgnoreExternals))!=0)
                {
                    if (SVNStatus::GetMoreImportant(s->node_status, svn_wc_status_unversioned)!=svn_wc_status_unversioned)
                    {
                        CTSVNPath destination = destPath;
                        destination.AppendPathString(statusPath.GetWinPathString().Mid(srcPath.GetWinPathString().GetLength()));
                        copyMap[statusPath] = destination;
                    }
                    while ((s = status.GetNextFileStatus(statusPath))!=0)
                    {
                        if ((s->node_status == svn_wc_status_unversioned)||
                            (s->node_status == svn_wc_status_ignored)||
                            (s->node_status == svn_wc_status_none)||
                            (s->node_status == svn_wc_status_missing)||
                            (s->node_status == svn_wc_status_deleted))
                            continue;
                        if ((extended == SVNExportOnlyLocalChanges) &&
                            (s->node_status == svn_wc_status_normal) )
                            continue;
                        CTSVNPath destination = destPath;
                        destination.AppendPathString(statusPath.GetWinPathString().Mid(srcPath.GetWinPathString().GetLength()));
                        copyMap[statusPath] = destination;
                    }
                }
                else
                {
                    Err = svn_error_create(status.GetSVNError()->apr_err, const_cast<svn_error_t *>(status.GetSVNError()), NULL);
                    return false;
                }
            }
            break;
        case SVNExportIncludeUnversioned:
            {
                CString srcfile;
                CDirFileEnum lister(srcPath.GetWinPathString());
                copyMap[srcPath] = destPath;
                while (lister.NextFile(srcfile, NULL))
                {
                    if (g_SVNAdminDir.IsAdminDirPath(srcfile))
                        continue;
                    CTSVNPath destination = destPath;
                    destination.AppendPathString(srcfile.Mid(srcPath.GetWinPathString().GetLength()));
                    copyMap[CTSVNPath(srcfile)] = destination;
                }
            }
            break;
        }
        size_t count = 0;
        for (std::map<CTSVNPath, CTSVNPath>::iterator it = copyMap.begin(); (it != copyMap.end()) && (!progress.HasUserCancelled()); ++it)
        {
            progress.FormatPathLine(1, IDS_SVNPROGRESS_EXPORTING, it->first.GetWinPath());
            progress.FormatPathLine(2, IDS_SVNPROGRESS_EXPORTINGTO, it->second.GetWinPath());
            progress.SetProgress64(count, copyMap.size());
            count++;
            if (it->first.IsDirectory())
                CPathUtils::MakeSureDirectoryPathExists(it->second.GetWinPath());
            else
            {
                if (!CopyFile(it->first.GetWinPath(), it->second.GetWinPath(), !force))
                {
                    DWORD lastError = GetLastError();
                    if (lastError == ERROR_PATH_NOT_FOUND)
                    {
                        CPathUtils::MakeSureDirectoryPathExists(it->second.GetContainingDirectory().GetWinPath());
                        if (!CopyFile(it->first.GetWinPath(), it->second.GetWinPath(), !force))
                            lastError = GetLastError();
                        else
                            lastError = 0;
                    }
                    if ((lastError == ERROR_ALREADY_EXISTS)||(lastError == ERROR_FILE_EXISTS))
                    {
                        lastError = 0;

                        UINT ret = 0;
                        CString strMessage;
                        strMessage.Format(IDS_PROC_OVERWRITE_CONFIRM, it->second.GetWinPath());
                        CTaskDialog taskdlg(strMessage,
                                            CString(MAKEINTRESOURCE(IDS_PROC_OVERWRITE_CONFIRM_TASK2)),
                                            L"TortoiseSVN",
                                            0,
                                            TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW);
                        taskdlg.AddCommandControl(IDYES, CString(MAKEINTRESOURCE(IDS_PROC_OVERWRITE_CONFIRM_TASK3)));
                        taskdlg.AddCommandControl(IDCANCEL, CString(MAKEINTRESOURCE(IDS_PROC_OVERWRITE_CONFIRM_TASK4)));
                        taskdlg.SetDefaultCommandControl(IDCANCEL);
                        taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
                        taskdlg.SetVerificationCheckboxText(CString(MAKEINTRESOURCE(IDS_PROC_OVERWRITE_CONFIRM_TASK5)));
                        taskdlg.SetMainIcon(TD_WARNING_ICON);
                        ret = (UINT)taskdlg.DoModal(GetExplorerHWND());
                        if (taskdlg.GetVerificationCheckboxState())
                            force = true;

                        if ((ret == IDYESTOALL)||(ret == IDYES))
                        {
                            if (!CopyFile(it->first.GetWinPath(), it->second.GetWinPath(), FALSE))
                            {
                                lastError = GetLastError();
                            }
                            SetFileAttributes(it->second.GetWinPath(), FILE_ATTRIBUTE_NORMAL);
                        }
                    }
                    if (lastError)
                    {
                        CFormatMessageWrapper errorDetails(lastError);
                        if(!errorDetails)
                            return false;
                        Err = svn_error_create(NULL, NULL, CUnicodeUtils::GetUTF8(CString(errorDetails)));
                        return false;
                    }
                }
                SetFileAttributes(it->second.GetWinPath(), FILE_ATTRIBUTE_NORMAL);
            }
        }
        if (progress.HasUserCancelled())
        {
            progress.Stop();
            Err = svn_error_create(SVN_ERR_CANCELLED, NULL, CUnicodeUtils::GetUTF8(CString(MAKEINTRESOURCE(IDS_SVN_USERCANCELLED))));
            return false;
        }
        progress.Stop();
    }
    else
    {
        SVNPool subpool(pool);
        const char* source = srcPath.GetSVNApiPath(subpool);
        CHooks::Instance().PreConnect(CTSVNPathList(srcPath));
        SVNTRACE (
            Err = svn_client_export5(NULL,      //no resulting revision needed
                source,
                destPath.GetSVNApiPath(subpool),
                pegrev,
                revision,
                force,
                bIgnoreExternals,
                bIgnoreKeywords,
                depth,
                eol.IsEmpty() ? NULL : (LPCSTR)CStringA(eol),
                m_pctx,
                subpool),
            source
        );
        ClearCAPIAuthCacheOnError();
        if(Err != NULL)
        {
            return false;
        }
    }
    return true;
}

bool SVN::Switch(const CTSVNPath& path, const CTSVNPath& url, const SVNRev& revision, const SVNRev& pegrev, svn_depth_t depth, bool depthIsSticky, bool ignore_externals, bool allow_unver_obstruction, bool ignore_ancestry)
{
    SVNPool subpool(pool);
    Prepare();

    const char* svnPath = path.GetSVNApiPath(subpool);
    CHooks::Instance().PreConnect(CTSVNPathList(path));
    SVNTRACE (
        Err = svn_client_switch3(NULL,
                                 svnPath,
                                 url.GetSVNApiPath(subpool),
                                 pegrev,
                                 revision,
                                 depth,
                                 depthIsSticky,
                                 ignore_externals,
                                 allow_unver_obstruction,
                                 ignore_ancestry,
                                 m_pctx,
                                 subpool),
        svnPath
    );

    ClearCAPIAuthCacheOnError();
    return (Err == NULL);
}

svn_error_t * SVN::import_filter(void * baton,
                                 svn_boolean_t * filtered,
                                 const char * local_abspath,
                                 const svn_io_dirent2_t * dirent,
                                 apr_pool_t * scratch_pool)
{
    // TODO: we could use this when importing files in the repo browser
    // via drag/drop maybe.
    UNREFERENCED_PARAMETER(baton);
    UNREFERENCED_PARAMETER(local_abspath);
    UNREFERENCED_PARAMETER(dirent);
    UNREFERENCED_PARAMETER(scratch_pool);
    if (filtered)
        *filtered = FALSE;
    return SVN_NO_ERROR;
}

bool SVN::Import(const CTSVNPath& path, const CTSVNPath& url, const CString& message,
                 ProjectProperties * props, svn_depth_t depth, bool bUseAutoprops,
                 bool no_ignore, bool ignore_unknown,
                 const RevPropHash& revProps)
{
    // the import command should use the mime-type file
    const char *mimetypes_file = NULL;
    Prepare();
    svn_config_t * opt = (svn_config_t *)apr_hash_get (m_pctx->config, SVN_CONFIG_CATEGORY_CONFIG,
        APR_HASH_KEY_STRING);
    if (bUseAutoprops)
    {
        svn_config_get(opt, &mimetypes_file,
            SVN_CONFIG_SECTION_MISCELLANY,
            SVN_CONFIG_OPTION_MIMETYPES_FILE, FALSE);
        if (mimetypes_file && *mimetypes_file)
        {
            Err = svn_io_parse_mimetypes_file(&(m_pctx->mimetypes_map),
                mimetypes_file, pool);
            if (Err)
                return FALSE;
        }
        if (props)
            props->InsertAutoProps(opt);
    }

    SVNPool subpool(pool);
    m_pctx->log_msg_baton3 = logMessage(message);
    apr_hash_t * revPropHash = MakeRevPropHash(revProps, subpool);

    const char* svnPath = path.GetSVNApiPath(subpool);
    CHooks::Instance().PreConnect(CTSVNPathList(path));
    SVNTRACE (
        Err = svn_client_import5(svnPath,
                                 url.GetSVNApiPath(subpool),
                                 depth,
                                 no_ignore,
                                 !bUseAutoprops,
                                 ignore_unknown,
                                 revPropHash,
                                 import_filter,
                                 this,
                                 commitcallback2,
                                 this,
                                 m_pctx,
                                 subpool),
        svnPath
    );
    m_pctx->log_msg_baton3 = logMessage(L"");
    ClearCAPIAuthCacheOnError();
    if(Err != NULL)
    {
        return false;
    }

    return true;
}

bool SVN::Merge(const CTSVNPath& path1, const SVNRev& revision1, const CTSVNPath& path2, const SVNRev& revision2,
                const CTSVNPath& localPath, bool force, svn_depth_t depth, const CString& options,
                bool ignoreanchestry, bool dryrun, bool record_only)
{
    SVNPool subpool(pool);
    apr_array_header_t *opts;

    opts = svn_cstring_split (CUnicodeUtils::GetUTF8(options), " \t\n\r", TRUE, subpool);

    Prepare();

    const char* svnPath = path1.GetSVNApiPath(subpool);
    CHooks::Instance().PreConnect(CTSVNPathList(path1));
    SVNTRACE (
        Err = svn_client_merge5(svnPath,
                                revision1,
                                path2.GetSVNApiPath(subpool),
                                revision2,
                                localPath.GetSVNApiPath(subpool),
                                depth,
                                ignoreanchestry,
                                ignoreanchestry,
                                force,
                                record_only,
                                dryrun,
                                true,
                                opts,
                                m_pctx,
                                subpool),
        svnPath
    );
    ClearCAPIAuthCacheOnError();

    return (Err == NULL);
}

bool SVN::PegMerge(const CTSVNPath& source, const SVNRevRangeArray& revrangearray, const SVNRev& pegrevision,
                   const CTSVNPath& destpath, bool force, svn_depth_t depth, const CString& options,
                   bool ignoreancestry, bool dryrun, bool record_only)
{
    SVNPool subpool(pool);
    apr_array_header_t *opts;

    opts = svn_cstring_split (CUnicodeUtils::GetUTF8(options), " \t\n\r", TRUE, subpool);

    Prepare();

    const char* svnPath = source.GetSVNApiPath(subpool);
    CHooks::Instance().PreConnect(CTSVNPathList(source));
    SVNTRACE (
        Err = svn_client_merge_peg5 (svnPath,
                                     revrangearray.GetCount() ? revrangearray.GetAprArray(subpool) : NULL,
                                     pegrevision,
                                     destpath.GetSVNApiPath(subpool),
                                     depth,
                                     ignoreancestry,
                                     ignoreancestry,
                                     force,
                                     record_only,
                                     dryrun,
                                     true,
                                     opts,
                                     m_pctx,
                                     subpool),
        svnPath
    );
    ClearCAPIAuthCacheOnError();

    return (Err == NULL);
}

bool SVN::MergeReintegrate(const CTSVNPath& source, const SVNRev& pegrevision, const CTSVNPath& wcpath, bool dryrun, const CString& options)
{
    SVNPool subpool(pool);
    apr_array_header_t *opts;

    opts = svn_cstring_split (CUnicodeUtils::GetUTF8(options), " \t\n\r", TRUE, subpool);

    Prepare();
    const char* svnPath = source.GetSVNApiPath(subpool);
    CHooks::Instance().PreConnect(CTSVNPathList(source));

#pragma warning(disable: 4996)   // disable deprecated warning: we use svn_client_merge_reintegrate specifically for 'old style' merges
    SVNTRACE (
        Err = svn_client_merge_reintegrate(svnPath,
            pegrevision,
            wcpath.GetSVNApiPath(subpool),
            dryrun,
            opts,
            m_pctx,
            subpool),
        svnPath
    );
#pragma warning(default: 4996)

    ClearCAPIAuthCacheOnError();
    return (Err == NULL);
}

bool SVN::SuggestMergeSources(const CTSVNPath& targetpath, const SVNRev& revision, CTSVNPathList& sourceURLs)
{
    SVNPool subpool(pool);
    apr_array_header_t * sourceurls;

    Prepare();
    sourceURLs.Clear();
    const char* svnPath = targetpath.GetSVNApiPath(subpool);
    CHooks::Instance().PreConnect(sourceURLs);
    SVNTRACE (
        Err = svn_client_suggest_merge_sources(&sourceurls,
                                                svnPath,
                                                revision,
                                                m_pctx,
                                                subpool),
        svnPath
    );

    ClearCAPIAuthCacheOnError();
    if (Err != NULL)
    {
        return false;
    }

    for (int i = 0; i < sourceurls->nelts; i++)
    {
        const char *path = (APR_ARRAY_IDX (sourceurls, i, const char*));
        sourceURLs.AddPath(CTSVNPath(CUnicodeUtils::GetUnicode(path)));
    }

    return true;
}

bool SVN::CreatePatch(const CTSVNPath& path1, const SVNRev& revision1,
                      const CTSVNPath& path2, const SVNRev& revision2,
                      const CTSVNPath& relativeToDir, svn_depth_t depth,
                      bool ignoreancestry, bool nodiffadded, bool nodiffdeleted, bool showCopiesAsAdds, bool ignorecontenttype,
                      bool useGitFormat, bool ignoreproperties, bool propertiesonly, const CString& options, bool bAppend, const CTSVNPath& outputfile)
{
    // to create a patch, we need to remove any custom diff tools which might be set in the config file
    svn_config_t * cfg = (svn_config_t *)apr_hash_get (m_pctx->config, SVN_CONFIG_CATEGORY_CONFIG, APR_HASH_KEY_STRING);
    CStringA diffCmd;
    CStringA diff3Cmd;
    if (cfg)
    {
        const char * value;
        svn_config_get(cfg, &value, SVN_CONFIG_SECTION_HELPERS, SVN_CONFIG_OPTION_DIFF_CMD, NULL);
        diffCmd = CStringA(value);
        svn_config_get(cfg, &value, SVN_CONFIG_SECTION_HELPERS, SVN_CONFIG_OPTION_DIFF3_CMD, NULL);
        diff3Cmd = CStringA(value);

        svn_config_set(cfg, SVN_CONFIG_SECTION_HELPERS, SVN_CONFIG_OPTION_DIFF_CMD, NULL);
        svn_config_set(cfg, SVN_CONFIG_SECTION_HELPERS, SVN_CONFIG_OPTION_DIFF3_CMD, NULL);
    }

    bool bRet = Diff(path1, revision1, path2, revision2, relativeToDir, depth, ignoreancestry, nodiffadded, nodiffdeleted, showCopiesAsAdds, ignorecontenttype, useGitFormat, ignoreproperties, propertiesonly, options, bAppend, outputfile, CTSVNPath());
    if (cfg)
    {
        svn_config_set(cfg, SVN_CONFIG_SECTION_HELPERS, SVN_CONFIG_OPTION_DIFF_CMD, (LPCSTR)diffCmd);
        svn_config_set(cfg, SVN_CONFIG_SECTION_HELPERS, SVN_CONFIG_OPTION_DIFF3_CMD, (LPCSTR)diff3Cmd);
    }
    return bRet;
}

bool SVN::Diff(const CTSVNPath& path1, const SVNRev& revision1,
               const CTSVNPath& path2, const SVNRev& revision2,
               const CTSVNPath& relativeToDir, svn_depth_t depth,
               bool ignoreancestry, bool nodiffadded, bool nodiffdeleted, bool showCopiesAsAdds, bool ignorecontenttype,
               bool useGitFormat, bool ignoreproperties, bool propertiesonly, const CString& options, bool bAppend, const CTSVNPath& outputfile)
{
    return Diff(path1, revision1, path2, revision2, relativeToDir, depth, ignoreancestry, nodiffadded, nodiffdeleted, showCopiesAsAdds, ignorecontenttype, useGitFormat, ignoreproperties, propertiesonly, options, bAppend, outputfile, CTSVNPath());
}

bool SVN::Diff(const CTSVNPath& path1, const SVNRev& revision1,
               const CTSVNPath& path2, const SVNRev& revision2,
               const CTSVNPath& relativeToDir, svn_depth_t depth,
               bool ignoreancestry, bool nodiffadded, bool nodiffdeleted, bool showCopiesAsAdds, bool ignorecontenttype,
               bool useGitFormat, bool ignoreproperties, bool propertiesonly, const CString& options, bool bAppend, const CTSVNPath& outputfile, const CTSVNPath& errorfile)
{
    bool del = false;
    apr_file_t * outfile = nullptr;
    apr_file_t * errfile = nullptr;
    apr_array_header_t *opts;

    SVNPool localpool(pool);
    Prepare();

    opts = svn_cstring_split (CUnicodeUtils::GetUTF8(options), " \t\n\r", TRUE, localpool);

    apr_int32_t flags = APR_WRITE | APR_CREATE | APR_BINARY;
    if (bAppend)
        flags |= APR_APPEND;
    else
        flags |= APR_TRUNCATE;
    Err = svn_io_file_open (&outfile, outputfile.GetSVNApiPath(localpool),
                            flags,
                            APR_OS_DEFAULT, localpool);
    if (Err)
        return false;

    svn_stream_t * outstream = svn_stream_from_aprfile2(outfile, false, localpool);
    svn_stream_t * errstream = svn_stream_from_aprfile2(errfile, false, localpool);

    CTSVNPath workingErrorFile;
    if (errorfile.IsEmpty())
    {
        workingErrorFile = CTempFiles::Instance().GetTempFilePath(true);
        del = true;
    }
    else
    {
        workingErrorFile = errorfile;
    }

    Err = svn_io_file_open (&errfile, workingErrorFile.GetSVNApiPath(localpool),
                            APR_WRITE | APR_CREATE | APR_TRUNCATE | APR_BINARY,
                            APR_OS_DEFAULT, localpool);
    if (Err)
        return false;

    const char* svnPath = path1.GetSVNApiPath(localpool);
    if (path1.IsUrl() || path2.IsUrl() || !revision1.IsWorking() || !revision2.IsWorking())
        CHooks::Instance().PreConnect(CTSVNPathList(path1));
    SVNTRACE (
        Err = svn_client_diff6 (opts,
                               svnPath,
                               revision1,
                               path2.GetSVNApiPath(localpool),
                               revision2,
                               relativeToDir.IsEmpty() ? NULL : relativeToDir.GetSVNApiPath(localpool),
                               depth,
                               ignoreancestry,
                               nodiffadded,
                               nodiffdeleted,
                               showCopiesAsAdds,
                               ignorecontenttype,
                               ignoreproperties,
                               propertiesonly,
                               useGitFormat,
                               APR_LOCALE_CHARSET,
                               outstream,
                               errstream,
                               NULL,        // we don't deal with change lists when diffing
                               m_pctx,
                               localpool),
        svnPath
    );
    ClearCAPIAuthCacheOnError();
    if (Err)
    {
        return false;
    }
    if (del)
    {
        svn_io_remove_file2 (workingErrorFile.GetSVNApiPath(localpool), true, localpool);
    }
    return true;
}

bool SVN::PegDiff(const CTSVNPath& path, const SVNRev& pegrevision,
                  const SVNRev& startrev, const SVNRev& endrev,
                  const CTSVNPath& relativeToDir, svn_depth_t depth,
                  bool ignoreancestry, bool nodiffadded, bool nodiffdeleted, bool showCopiesAsAdds, bool ignorecontenttype,
                    bool useGitFormat, bool ignoreproperties, bool propertiesonly, const CString& options, bool bAppend, const CTSVNPath& outputfile)
{
    return PegDiff(path, pegrevision, startrev, endrev, relativeToDir, depth, ignoreancestry, nodiffadded, nodiffdeleted, showCopiesAsAdds, ignorecontenttype, useGitFormat, ignoreproperties, propertiesonly, options, bAppend, outputfile, CTSVNPath());
}

bool SVN::PegDiff(const CTSVNPath& path, const SVNRev& pegrevision,
                  const SVNRev& startrev, const SVNRev& endrev,
                  const CTSVNPath& relativeToDir, svn_depth_t depth,
                  bool ignoreancestry, bool nodiffadded, bool nodiffdeleted, bool showCopiesAsAdds, bool ignorecontenttype,
                  bool useGitFormat, bool ignoreproperties, bool propertiesonly, const CString& options, bool bAppend, const CTSVNPath& outputfile, const CTSVNPath& errorfile)
{
    bool del = false;
    apr_file_t * outfile;
    apr_file_t * errfile;
    apr_array_header_t *opts;

    SVNPool localpool(pool);
    Prepare();

    opts = svn_cstring_split (CUnicodeUtils::GetUTF8(options), " \t\n\r", TRUE, localpool);

    apr_int32_t flags = APR_WRITE | APR_CREATE | APR_BINARY;
    if (bAppend)
        flags |= APR_APPEND;
    else
        flags |= APR_TRUNCATE;
    Err = svn_io_file_open (&outfile, outputfile.GetSVNApiPath(localpool),
        flags,
        APR_OS_DEFAULT, localpool);

    if (Err)
        return false;

    CTSVNPath workingErrorFile;
    if (errorfile.IsEmpty())
    {
        workingErrorFile = CTempFiles::Instance().GetTempFilePath(true);
        del = true;
    }
    else
    {
        workingErrorFile = errorfile;
    }

    Err = svn_io_file_open (&errfile, workingErrorFile.GetSVNApiPath(localpool),
        APR_WRITE | APR_CREATE | APR_TRUNCATE | APR_BINARY,
        APR_OS_DEFAULT, localpool);
    if (Err)
        return false;

    svn_stream_t * outstream = svn_stream_from_aprfile2(outfile, false, localpool);
    svn_stream_t * errstream = svn_stream_from_aprfile2(errfile, false, localpool);

    const char* svnPath = path.GetSVNApiPath(localpool);
    CHooks::Instance().PreConnect(CTSVNPathList(path));
    SVNTRACE (
        Err = svn_client_diff_peg6 (opts,
            svnPath,
            pegrevision,
            startrev,
            endrev,
            relativeToDir.IsEmpty() ? NULL : relativeToDir.GetSVNApiPath(localpool),
            depth,
            ignoreancestry,
            nodiffadded,
            nodiffdeleted,
            showCopiesAsAdds,
            ignorecontenttype,
            ignoreproperties,
            propertiesonly,
            useGitFormat,
            APR_LOCALE_CHARSET,
            outstream,
            errstream,
            NULL, // we don't deal with change lists when diffing
            m_pctx,
            localpool),
        svnPath
    );
    ClearCAPIAuthCacheOnError();
    if (Err)
    {
        return false;
    }
    if (del)
    {
        svn_io_remove_file2 (workingErrorFile.GetSVNApiPath(localpool), true, localpool);
    }
    return true;
}

bool SVN::DiffSummarize(const CTSVNPath& path1, const SVNRev& rev1, const CTSVNPath& path2, const SVNRev& rev2, svn_depth_t depth, bool ignoreancestry)
{
    SVNPool localpool(pool);
    Prepare();

    const char* svnPath = path1.GetSVNApiPath(localpool);
    if (path1.IsUrl() || path2.IsUrl() || !rev1.IsWorking() || !rev2.IsWorking())
        CHooks::Instance().PreConnect(CTSVNPathList(path1));
    SVNTRACE (
        Err = svn_client_diff_summarize2(svnPath, rev1,
                                        path2.GetSVNApiPath(localpool), rev2,
                                        depth, ignoreancestry, NULL,
                                        summarize_func, this,
                                        m_pctx, localpool),
        svnPath
    );
    ClearCAPIAuthCacheOnError();

    return (Err == NULL);
}

bool SVN::DiffSummarizePeg(const CTSVNPath& path, const SVNRev& peg, const SVNRev& rev1, const SVNRev& rev2, svn_depth_t depth, bool ignoreancestry)
{
    SVNPool localpool(pool);
    Prepare();

    const char* svnPath = path.GetSVNApiPath(localpool);
    CHooks::Instance().PreConnect(CTSVNPathList(path));
    SVNTRACE (
        Err = svn_client_diff_summarize_peg2(svnPath, peg, rev1, rev2,
                                            depth, ignoreancestry, NULL,
                                            summarize_func, this,
                                            m_pctx, localpool),
        svnPath
    );
    ClearCAPIAuthCacheOnError();

    return (Err == NULL);
}

LogCache::CCachedLogInfo* SVN::GetLogCache (const CTSVNPath& path)
{
    if (!LogCache::CSettings::GetEnabled())
        return NULL;

    CString uuid;
    CString root = GetLogCachePool()->GetRepositoryInfo().GetRepositoryRootAndUUID (path, uuid);
    return GetLogCachePool()->GetCache (uuid, root);
}

std::unique_ptr<const CCacheLogQuery>
SVN::ReceiveLog (const CTSVNPathList& pathlist, const SVNRev& revisionPeg,
                 const SVNRev& revisionStart, const SVNRev& revisionEnd,
                 int limit, bool strict, bool withMerges, bool refresh)
{
    Prepare();
    try
    {
        SVNPool localpool(pool);

        // query used internally to contact the repository if necessary

        CSVNLogQuery svnQuery (m_pctx, localpool);

        // cached-based queries.
        // Use & update existing cache

        std::unique_ptr<CCacheLogQuery> cacheQuery
            (new CCacheLogQuery (GetLogCachePool(), &svnQuery));

        // run query through SVN but collect results in a temporary cache

        std::unique_ptr<CCacheLogQuery> tempQuery
            (new CCacheLogQuery (*this, &svnQuery));

        // select query and run it

        ILogQuery* query = !(GetLogCachePool()->IsEnabled() || m_bAssumeCacheEnabled) || refresh
                         ? tempQuery.get()
                         : cacheQuery.get();

        try
        {
            query->Log ( pathlist
                       , revisionPeg
                       , revisionStart
                       , revisionEnd
                       , limit
                       , strict != FALSE
                       , this
                       , false // changes will be fetched but not forwarded to receiver
                       , withMerges != FALSE
                       , true
                       , false
                       , TRevPropNames());
        }
        catch (SVNError& e)
        {
            // cancellation is no actual error condition

            if (e.GetCode() != SVN_ERR_CANCELLED)
                throw;

            // caller shall be able to detect the cancellation, though

            Err = svn_error_create (e.GetCode(), NULL, e.GetMessage());
        }

        // merge temp results with permanent cache, if applicable

        if (refresh && (GetLogCachePool()->IsEnabled() || m_bAssumeCacheEnabled))
        {
            // handle cache refresh results

            if (tempQuery->GotAnyData())
            {
                tempQuery->UpdateCache (cacheQuery.get());
            }
            else
            {
                // no connection to the repository but also not canceled
                // (no exception thrown) -> re-run from cache

                return ReceiveLog ( pathlist
                                  , revisionPeg, revisionStart, revisionEnd
                                  , limit, strict, withMerges
                                  , false);
            }
        }

        // return the cache that contains the log info

        return std::unique_ptr<const CCacheLogQuery>
            ((GetLogCachePool()->IsEnabled() || m_bAssumeCacheEnabled)
                ? cacheQuery.release()
                : tempQuery.release() );
    }
    catch (SVNError& e)
    {
        Err = svn_error_create (e.GetCode(), NULL, e.GetMessage());
        return std::unique_ptr<const CCacheLogQuery>();
    }
}

bool SVN::Cat(const CTSVNPath& url, const SVNRev& pegrevision, const SVNRev& revision, const CTSVNPath& localpath)
{
    apr_file_t * file;
    svn_stream_t * stream;
    apr_status_t status;
    SVNPool localpool(pool);
    Prepare();

    CTSVNPath fullLocalPath(localpath);
    if (fullLocalPath.IsDirectory())
    {
        fullLocalPath.AppendPathString(url.GetFileOrDirectoryName());
    }
    ::DeleteFile(fullLocalPath.GetWinPath());

    status = apr_file_open(&file, fullLocalPath.GetSVNApiPath(localpool), APR_WRITE | APR_CREATE | APR_TRUNCATE, APR_OS_DEFAULT, localpool);
    if (status)
    {
        Err = svn_error_wrap_apr(status, NULL);
        return FALSE;
    }
    stream = svn_stream_from_aprfile2(file, true, localpool);

    const char* svnPath = url.GetSVNApiPath(localpool);
    if (url.IsUrl() || (!pegrevision.IsWorking() && !pegrevision.IsValid()) || (!revision.IsWorking() && !revision.IsValid()))
        CHooks::Instance().PreConnect(CTSVNPathList(url));
    SVNTRACE (
        Err = svn_client_cat3(NULL, stream, svnPath, pegrevision, revision, true, m_pctx, localpool, localpool),
        svnPath
    );

    apr_file_close(file);
    ClearCAPIAuthCacheOnError();

    return (Err == NULL);
}

bool SVN::CreateRepository(const CTSVNPath& path, const CString& fstype)
{
    svn_repos_t * repo;
    svn_error_t * err;
    apr_hash_t *config;

    SVNPool localpool;

    apr_hash_t *fs_config = apr_hash_make (localpool);

    apr_hash_set (fs_config, SVN_FS_CONFIG_BDB_TXN_NOSYNC,
        APR_HASH_KEY_STRING, "0");

    apr_hash_set (fs_config, SVN_FS_CONFIG_BDB_LOG_AUTOREMOVE,
        APR_HASH_KEY_STRING, "1");

    config = SVNConfig::Instance().GetConfig(localpool);

    const char * fs_type = apr_pstrdup(localpool, CStringA(fstype));
    apr_hash_set (fs_config, SVN_FS_CONFIG_FS_TYPE,
        APR_HASH_KEY_STRING,
        fs_type);
    err = svn_repos_create(&repo, path.GetSVNApiPath(localpool), NULL, NULL, config, fs_config, localpool);

    bool ret = (err == NULL);
    svn_error_clear(err);
    return ret;
}

bool SVN::Blame(const CTSVNPath& path, const SVNRev& startrev, const SVNRev& endrev, const SVNRev& peg, const CString& diffoptions, bool ignoremimetype, bool includemerge)
{
    Prepare();
    SVNPool subpool(pool);
    apr_array_header_t *opts;
    svn_diff_file_options_t * options = svn_diff_file_options_create(subpool);
    opts = svn_cstring_split (CUnicodeUtils::GetUTF8(diffoptions), " \t\n\r", TRUE, subpool);
    svn_error_clear(svn_diff_file_options_parse(options, opts, subpool));

    // Subversion < 1.4 silently changed a revision WC to BASE. Due to a bug
    // report this was changed: now Subversion returns an error 'not implemented'
    // since it actually blamed the BASE file and not the working copy file.
    // Until that's implemented, we 'fall back' here to the old behavior and
    // just change and REV_WC to REV_BASE.
    SVNRev rev1 = startrev;
    SVNRev rev2 = endrev;
    if (rev1.IsWorking())
        rev1 = SVNRev::REV_BASE;
    if (rev2.IsWorking())
        rev2 = SVNRev::REV_BASE;

    const char* svnPath = path.GetSVNApiPath(subpool);
    CHooks::Instance().PreConnect(CTSVNPathList(path));
    SVNTRACE (
        Err = svn_client_blame5 ( svnPath,
                                 peg,
                                 rev1,
                                 rev2,
                                 options,
                                 ignoremimetype,
                                 includemerge,
                                 blameReceiver,
                                 (void *)this,
                                 m_pctx,
                                 subpool),
        svnPath
    );
    ClearCAPIAuthCacheOnError();

    if ((Err != 0)&&((Err->apr_err == SVN_ERR_UNSUPPORTED_FEATURE)||(Err->apr_err == SVN_ERR_FS_NOT_FOUND)||(Err->apr_err == SVN_ERR_CLIENT_UNRELATED_RESOURCES))&&(includemerge))
    {
        Prepare();
        SVNTRACE (
            Err = svn_client_blame5 (   svnPath,
                                        peg,
                                        rev1,
                                        rev2,
                                        options,
                                        ignoremimetype,
                                        false,
                                        blameReceiver,
                                        (void *)this,
                                        m_pctx,
                                        subpool),
            svnPath
        )
    }
    ClearCAPIAuthCacheOnError();

    return (Err == NULL);
}

svn_error_t* SVN::blameReceiver(void *baton,
                                svn_revnum_t /*start_revnum*/,
                                svn_revnum_t /*end_revnum*/,
                                apr_int64_t line_no,
                                svn_revnum_t revision,
                                apr_hash_t *rev_props,
                                svn_revnum_t merged_revision,
                                apr_hash_t *merged_rev_props,
                                const char *merged_path,
                                const char *line,
                                svn_boolean_t local_change,
                                apr_pool_t *pool)
{
    svn_error_t * error = NULL;
    CString author_native, merged_author_native;
    CString merged_path_native;
    CStringA log_msg;
    CStringA merged_log_msg;
    CStringA line_native;
    TCHAR date_native[SVN_DATE_BUFFER] = {0};
    TCHAR merged_date_native[SVN_DATE_BUFFER] = {0};

    SVN * svn = (SVN *)baton;

    const char * prop = svn_prop_get_value(rev_props, SVN_PROP_REVISION_AUTHOR);
    if (prop)
        author_native = CUnicodeUtils::GetUnicode(prop);
    prop = svn_prop_get_value(merged_rev_props, SVN_PROP_REVISION_AUTHOR);
    if (prop)
        merged_author_native = CUnicodeUtils::GetUnicode(prop);

    prop = svn_prop_get_value(rev_props, SVN_PROP_REVISION_LOG);
    if (prop)
        log_msg = prop;
    prop = svn_prop_get_value(merged_rev_props, SVN_PROP_REVISION_LOG);
    if (prop)
        merged_log_msg = prop;


    if (merged_path)
        merged_path_native = CUnicodeUtils::GetUnicode(merged_path);

    if (line)
        line_native = line;


    prop = svn_prop_get_value(rev_props, SVN_PROP_REVISION_DATE);
    if (prop)
    {
        // Convert date to a format for humans.
        apr_time_t time_temp;

        error = svn_time_from_cstring (&time_temp, prop, pool);
        if (error)
            return error;

        formatDate(date_native, time_temp, true);
    }
    else
        wcscat_s(date_native, L"(no date)");

    prop = svn_prop_get_value(merged_rev_props, SVN_PROP_REVISION_DATE);
    if (prop)
    {
        // Convert date to a format for humans.
        apr_time_t time_temp;

        error = svn_time_from_cstring (&time_temp, prop, pool);
        if (error)
            return error;

        formatDate(merged_date_native, time_temp, true);
    }
    else
        wcscat_s(merged_date_native, L"(no date)");


    if (!svn->BlameCallback((LONG)line_no, !!local_change, revision, author_native, date_native, merged_revision, merged_author_native, merged_date_native, merged_path_native, line_native, log_msg, merged_log_msg))
    {
        return svn_error_create(SVN_ERR_CANCELLED, NULL, "error in blame callback");
    }
    return error;
}

bool SVN::Lock(const CTSVNPathList& pathList, bool bStealLock, const CString& comment /* = CString( */)
{
    SVNTRACE_BLOCK

    Prepare();
    CHooks::Instance().PreConnect(pathList);
    Err = svn_client_lock(pathList.MakePathArray(pool), CUnicodeUtils::GetUTF8(comment), !!bStealLock, m_pctx, pool);
    ClearCAPIAuthCacheOnError();
    return (Err == NULL);
}

bool SVN::Unlock(const CTSVNPathList& pathList, bool bBreakLock)
{
    SVNTRACE_BLOCK

    Prepare();
    CHooks::Instance().PreConnect(pathList);
    Err = svn_client_unlock(pathList.MakePathArray(pool), bBreakLock, m_pctx, pool);
    ClearCAPIAuthCacheOnError();
    return (Err == NULL);
}

svn_error_t* SVN::summarize_func(const svn_client_diff_summarize_t *diff, void *baton, apr_pool_t * /*pool*/)
{
    SVN * svn = (SVN *)baton;
    if (diff)
    {
        CTSVNPath path = CTSVNPath(CUnicodeUtils::GetUnicode(diff->path));
        return svn->DiffSummarizeCallback(path, diff->summarize_kind, !!diff->prop_changed, diff->node_kind);
    }
    return SVN_NO_ERROR;
}
svn_error_t* SVN::listReceiver(void* baton, const char* path,
                               const svn_dirent_t *dirent,
                               const svn_lock_t *lock,
                               const char *abs_path,
                               const char *external_parent_url,
                               const char *external_target,
                               apr_pool_t * /*pool*/)
{
    SVN * svn = (SVN *)baton;
    bool result = !!svn->ReportList(CUnicodeUtils::GetUnicode(path),
                                  dirent->kind,
                                  dirent->size,
                                  !!dirent->has_props,
                                  svn->m_bListComplete,
                                  dirent->created_rev,
                                  dirent->time,
                                  CUnicodeUtils::GetUnicode(dirent->last_author),
                                  lock ? CUnicodeUtils::GetUnicode(lock->token) : CString(),
                                  lock ? CUnicodeUtils::GetUnicode(lock->owner) : CString(),
                                  lock ? CUnicodeUtils::GetUnicode(lock->comment) : CString(),
                                  lock ? !!lock->is_dav_comment : false,
                                  lock ? lock->creation_date : 0,
                                  lock ? lock->expiration_date : 0,
                                  CUnicodeUtils::GetUnicode(abs_path),
                                  CUnicodeUtils::GetUnicode(external_parent_url),
                                  CUnicodeUtils::GetUnicode(external_target)
                                  );
    svn_error_t * err = NULL;
    if ((result == false) || svn->Cancel())
    {
        CString temp;
        temp.LoadString (result ? IDS_SVN_USERCANCELLED : IDS_ERR_ERROR);
        err = svn_error_create(SVN_ERR_CANCELLED, NULL, CUnicodeUtils::GetUTF8(temp));
    }
    return err;
}

// implement ILogReceiver

void SVN::ReceiveLog ( TChangedPaths* /* changes */
                     , svn_revnum_t rev
                     , const StandardRevProps* stdRevProps
                     , UserRevPropArray* /* userRevProps*/
                     , const MergeInfo* mergeInfo)
{
    // check for user pressing "Cancel" somewhere

    cancel();

    // use the log info (in a derived class specific way)

    static const std::string emptyString;
    Log ( rev
        , stdRevProps == NULL ? emptyString : stdRevProps->GetAuthor()
        , stdRevProps == NULL ? emptyString : stdRevProps->GetMessage()
        , stdRevProps == NULL ? apr_time_t(0) : stdRevProps->GetTimeStamp()
        , mergeInfo);
}

void SVN::notify( void *baton,
                 const svn_wc_notify_t *notify,
                 apr_pool_t *pool)
{
    SVN * svn = (SVN *)baton;

    CTSVNPath tsvnPath;
    tsvnPath.SetFromSVN(notify->path);
    CTSVNPath url;
    url.SetFromSVN(notify->url);
    if (notify->action == svn_wc_notify_url_redirect)
        svn->m_redirectedUrl = url;

    CString mime;
    if (notify->mime_type)
        mime = CUnicodeUtils::GetUnicode(notify->mime_type);

    CString changelistname;
    if (notify->changelist_name)
        changelistname = CUnicodeUtils::GetUnicode(notify->changelist_name);

    CString propertyName;
    if (notify->prop_name)
        propertyName = CUnicodeUtils::GetUnicode(notify->prop_name);

    svn->Notify(tsvnPath, url, notify->action, notify->kind,
                mime, notify->content_state,
                notify->prop_state, notify->revision,
                notify->lock, notify->lock_state, changelistname, propertyName, notify->merge_range,
                notify->err, pool);
}

static svn_wc_conflict_choice_t
trivial_conflict_choice(const svn_wc_conflict_description2_t *cd)
{
  if ((cd->operation == svn_wc_operation_update
      || cd->operation == svn_wc_operation_switch) &&
      cd->reason == svn_wc_conflict_reason_moved_away &&
      cd->action == svn_wc_conflict_action_edit)
    {
      /* local move vs. incoming edit -> update move */
      return svn_wc_conflict_choose_mine_conflict;
    }

  return svn_wc_conflict_choose_unspecified;
}

svn_error_t* SVN::conflict_resolver(svn_wc_conflict_result_t **result,
                               const svn_wc_conflict_description2_t *description,
                               void *baton,
                               apr_pool_t * resultpool,
                               apr_pool_t * /*scratchpool*/)
{
    SVN * svn = (SVN *)baton;
    CString file;
    svn_wc_conflict_choice_t choice;

    // Automatically resolve trivial conflicts.
    choice = trivial_conflict_choice(description);

    // Call the callback if still unresolved.
    if (choice  == svn_wc_conflict_choose_unspecified)
    {
        choice = svn->ConflictResolveCallback(description, file);
    }

    CTSVNPath f(file);
    *result = svn_wc_create_conflict_result(choice, file.IsEmpty() ? NULL : apr_pstrdup(resultpool, f.GetSVNApiPath(resultpool)), resultpool);
    if (svn->Cancel())
    {
        CString temp;
        temp.LoadString(IDS_SVN_USERCANCELLED);
        return svn_error_create(SVN_ERR_CANCELLED, NULL, CUnicodeUtils::GetUTF8(temp));
    }
    return SVN_NO_ERROR;
}

svn_error_t* SVN::cancel(void *baton)
{
    SVN * svn = (SVN *)baton;
    if ((svn->Cancel())||((svn->m_pProgressDlg)&&(svn->m_pProgressDlg->HasUserCancelled())))
    {
        CString message(MAKEINTRESOURCE(IDS_SVN_USERCANCELLED));
        return svn_error_create(SVN_ERR_CANCELLED, NULL, CUnicodeUtils::GetUTF8(message));
    }
    return SVN_NO_ERROR;
}

void SVN::cancel()
{
    if (Cancel() || ((m_pProgressDlg != NULL) && (m_pProgressDlg->HasUserCancelled())))
    {
        CString message(MAKEINTRESOURCE(IDS_SVN_USERCANCELLED));
        throw SVNError (SVN_ERR_CANCELLED, CUnicodeUtils::GetUTF8(message));
    }
}

void * SVN::logMessage (CString message, char * baseDirectory)
{
    message.Remove('\r');

    log_msg_baton3* baton = (log_msg_baton3 *) apr_palloc (pool, sizeof (*baton));
    baton->message = apr_pstrdup(pool, (LPCSTR)CUnicodeUtils::GetUTF8(message));
    baton->base_dir = baseDirectory ? baseDirectory : "";

    baton->message_encoding = NULL;
    baton->tmpfile_left = NULL;
    baton->pool = pool;

    return baton;
}

void SVN::PathToUrl(CString &path)
{
    bool bUNC = false;
    path.Trim();
    if (path.Left(2).Compare(L"\\\\")==0)
        bUNC = true;
    // convert \ to /
    path.Replace('\\','/');
    path.TrimLeft('/');
    // prepend file://
    if (bUNC)
        path.Insert(0, L"file://");
    else
        path.Insert(0, L"file:///");
    path.TrimRight(L"/\\");          //remove trailing slashes
}

void SVN::UrlToPath(CString &url)
{
    // we have to convert paths like file:///c:/myfolder
    // to c:/myfolder
    // and paths like file:////mymachine/c/myfolder
    // to //mymachine/c/myfolder
    url.Trim();
    url.Replace('\\','/');
    url = url.Mid(7);   // "file://" has seven chars
    url.TrimLeft('/');
    // if we have a ':' that means the file:// url points to an absolute path
    // like file:///D:/Development/test
    // if we don't have a ':' we assume it points to an UNC path, and those
    // actually _need_ the slashes before the paths
    if ((url.Find(':')<0) && (url.Find('|')<0))
        url.Insert(0, L"\\\\");
    SVN::preparePath(url);
    // now we need to unescape the url
    url = CPathUtils::PathUnescape(url);
}

void SVN::preparePath(CString &path)
{
    path.Trim();
    path.TrimRight(L"/\\");          //remove trailing slashes
    path.Replace('\\','/');

    if (path.Left(10).CompareNoCase(L"file://///")==0)
    {
        if (path.Find('%')<0)
            path.Replace(L"file://///", L"file://");
        else
            path.Replace(L"file://///", L"file:////");
    }
    else if (path.Left(9).CompareNoCase(L"file:////")==0)
    {
        if (path.Find('%')<0)
            path.Replace(L"file:////", L"file://");
    }
}

svn_error_t* svn_cl__get_log_message(const char **log_msg,
                                    const char **tmp_file,
                                    const apr_array_header_t * /*commit_items*/,
                                    void *baton,
                                    apr_pool_t * pool)
{
    log_msg_baton3 *lmb = (log_msg_baton3 *) baton;
    *tmp_file = NULL;
    if (lmb->message)
    {
        *log_msg = apr_pstrdup (pool, lmb->message);
    }

    return SVN_NO_ERROR;
}

CString SVN::GetURLFromPath(const CTSVNPath& path)
{
    const char * URL;
    if (path.IsUrl())
        return path.GetSVNPathString();
    Prepare();
    SVNPool subpool(pool);
    Err = svn_client_url_from_path2 (&URL, path.GetSVNApiPath(subpool), m_pctx, subpool, subpool);
    if (Err)
        return L"";
    if (URL==NULL)
        return L"";
    return CString(URL);
}

CTSVNPath SVN::GetWCRootFromPath(const CTSVNPath& path)
{
    const char * wcroot = NULL;
    Prepare();
    SVNPool subpool(pool);
    const char* svnPath = path.GetSVNApiPath(subpool);
    if ((svnPath==nullptr) || svn_path_is_url(svnPath) || (svnPath[0]==0))
        return CTSVNPath();
    SVNTRACE (
        Err = svn_client_get_wc_root (&wcroot, svnPath, m_pctx, subpool, subpool),
        svnPath
        )

   if (Err)
        return CTSVNPath();
    if (wcroot == NULL)
        return CTSVNPath();
    CTSVNPath ret;
    ret.SetFromSVN(wcroot);
    return ret;
}

bool SVN::List(const CTSVNPath& url, const SVNRev& revision, const SVNRev& pegrev, svn_depth_t depth, bool fetchlocks, bool complete, bool includeExternals)
{
    SVNPool subpool(pool);
    Prepare();

    const char* svnPath = url.GetSVNApiPath(subpool);
    CHooks::Instance().PreConnect(CTSVNPathList(url));
    m_bListComplete = complete;
    SVNTRACE (
        Err = svn_client_list3(svnPath,
                               pegrev,
                               revision,
                               depth,
                               complete ? SVN_DIRENT_ALL : 0,
                               fetchlocks,
                               includeExternals,
                               listReceiver,
                               this,
                               m_pctx,
                               subpool),
        svnPath
    )
    ClearCAPIAuthCacheOnError();
    return (Err == NULL);
}

bool SVN::Relocate(const CTSVNPath& path, const CTSVNPath& from, const CTSVNPath& to, bool includeexternals)
{
    Prepare();
    SVNPool subpool(pool);
    CString uuid;

    const CString root = GetRepositoryRootAndUUID(path, false, uuid);

    const char* svnPath = path.GetSVNApiPath(subpool);
    CHooks::Instance().PreConnect(CTSVNPathList(path));
    SVNTRACE (
        Err = svn_client_relocate2(
                    svnPath,
                    from.GetSVNApiPath(subpool),
                    to.GetSVNApiPath(subpool),
                    !includeexternals,
                    m_pctx, subpool),
        svnPath
    );

    ClearCAPIAuthCacheOnError();
    if (Err == NULL)
    {
        GetLogCachePool()->DropCache(uuid, root);
    }

    return (Err == NULL);
}

bool SVN::IsRepository(const CTSVNPath& path)
{
    Prepare();
    SVNPool subPool(pool);

    // The URL we get here is per definition properly encoded and escaped.
    const char * rootPath = svn_repos_find_root_path(path.GetSVNApiPath(subPool), subPool);
    if (rootPath)
    {
        svn_repos_t* pRepos = NULL;
        Err = svn_repos_open3 (&pRepos, rootPath, NULL, subPool, subPool);
        if ((Err)&&(Err->apr_err == SVN_ERR_FS_BERKELEY_DB))
            return true;
        if (Err == NULL)
            return true;
    }

    return false;
}

CString SVN::GetRepositoryRootAndUUID(const CTSVNPath& path, bool useLogCache, CString& sUUID)
{
    if (useLogCache && (GetLogCachePool()->IsEnabled() || m_bAssumeCacheEnabled))
        return GetLogCachePool()->GetRepositoryInfo().GetRepositoryRootAndUUID (path, sUUID);

    const char * returl = nullptr;
    const char * uuid   = nullptr;

    SVNPool localpool(pool);
    Prepare();

    // empty the sUUID first
    sUUID.Empty();

    if (path.IsUrl())
        CHooks::Instance().PreConnect(CTSVNPathList(path));

    // make sure the url is canonical.
    const char * goodurl = path.GetSVNApiPath(localpool);
    SVNTRACE (
        Err = svn_client_get_repos_root(&returl, &uuid, goodurl, m_pctx, localpool, localpool),
        goodurl
    );
    ClearCAPIAuthCacheOnError();
    if (Err == NULL)
    {
        sUUID = CString(uuid);
    }

    return CString(returl);
}


CString SVN::GetRepositoryRoot( const CTSVNPath& url )
{
    CString sUUID;
    return GetRepositoryRootAndUUID(url, true, sUUID);
}


CString SVN::GetUUIDFromPath( const CTSVNPath& path )
{
    CString sUUID;
    GetRepositoryRootAndUUID(path, true, sUUID);
    return sUUID;
}

svn_revnum_t SVN::GetHEADRevision(const CTSVNPath& path, bool cacheAllowed)
{
    svn_ra_session_t *ra_session;
    const char * urla;
    svn_revnum_t rev = -1;

    SVNPool localpool(pool);
    Prepare();

    // make sure the url is canonical.
    const char * svnPath = path.GetSVNApiPath(localpool);
    if (!path.IsUrl())
    {
        SVNTRACE (
            Err = svn_client_url_from_path2 (&urla, svnPath, m_pctx, localpool, localpool),
            svnPath
        );
    }
    else
    {
        urla = svnPath;
    }
    if (Err)
        return -1;

    // use cached information, if allowed

    if (cacheAllowed && LogCache::CSettings::GetEnabled())
    {
        // look up in cached repository properties
        // (missing entries will be added automatically)

        CTSVNPath canonicalURL;
        canonicalURL.SetFromSVN (urla);

        CRepositoryInfo& cachedProperties = GetLogCachePool()->GetRepositoryInfo();
        CString uuid = cachedProperties.GetRepositoryUUID (canonicalURL);
        return cachedProperties.GetHeadRevision (uuid, canonicalURL);
    }
    else
    {
        // non-cached access

        CHooks::Instance().PreConnect(CTSVNPathList(path));
        /* use subpool to create a temporary RA session */
        SVNTRACE (
            Err = svn_client_open_ra_session2 (&ra_session, urla, path.IsUrl() ? NULL : svnPath, m_pctx, localpool, localpool),
            urla
        );
        ClearCAPIAuthCacheOnError();
        if (Err)
            return -1;

        SVNTRACE (
            Err = svn_ra_get_latest_revnum(ra_session, &rev, localpool),
            urla
        );
        ClearCAPIAuthCacheOnError();
        if (Err)
            return -1;
        return rev;
    }
}

bool SVN::GetRootAndHead(const CTSVNPath& path, CTSVNPath& url, svn_revnum_t& rev)
{
    svn_ra_session_t *ra_session;
    const char * urla;
    const char * returl;

    SVNPool localpool(pool);
    Prepare();

    // make sure the url is canonical.
    const char * svnPath = path.GetSVNApiPath(localpool);
    if (!path.IsUrl())
    {
        SVNTRACE (
            Err = svn_client_url_from_path2 (&urla, svnPath, m_pctx, localpool, localpool),
            svnPath
        );
    }
    else
    {
        urla = svnPath;
    }

    if (Err)
        return false;

    // use cached information, if allowed

    if (LogCache::CSettings::GetEnabled())
    {
        // look up in cached repository properties
        // (missing entries will be added automatically)

        CRepositoryInfo& cachedProperties = GetLogCachePool()->GetRepositoryInfo();
        CString uuid;
        url.SetFromSVN (cachedProperties.GetRepositoryRootAndUUID (path, uuid));
        if (url.IsEmpty())
        {
            assert (Err != NULL);
        }
        else
        {
            rev = cachedProperties.GetHeadRevision (uuid, path);
            if ((rev == NO_REVISION) && (Err == NULL))
            {
                CHooks::Instance().PreConnect(CTSVNPathList(path));
                SVNTRACE (
                    Err = svn_client_open_ra_session2 (&ra_session, urla, path.IsUrl() ? NULL : svnPath, m_pctx, localpool, localpool),
                    urla
                );
                ClearCAPIAuthCacheOnError();
                if (Err)
                    return false;

                SVNTRACE (
                    Err = svn_ra_get_latest_revnum(ra_session, &rev, localpool),
                    urla
                );
                ClearCAPIAuthCacheOnError();
                if (Err)
                    return false;
            }
        }

        return (Err == NULL);
    }
    else
    {
        // non-cached access

        CHooks::Instance().PreConnect(CTSVNPathList(path));
        /* use subpool to create a temporary RA session */

        SVNTRACE (
            Err = svn_client_open_ra_session2 (&ra_session, urla, path.IsUrl() ? NULL : svnPath, m_pctx, localpool, localpool),
            urla
        );
        ClearCAPIAuthCacheOnError();
        if (Err)
            return FALSE;

        SVNTRACE (
            Err = svn_ra_get_latest_revnum(ra_session, &rev, localpool),
            urla
        );
        ClearCAPIAuthCacheOnError();
        if (Err)
            return FALSE;

        SVNTRACE (
            Err = svn_ra_get_repos_root2(ra_session, &returl, localpool),
            urla
        );
        ClearCAPIAuthCacheOnError();
        if (Err)
            return FALSE;

        url.SetFromSVN(CUnicodeUtils::GetUnicode(returl));
    }

    return true;
}

bool SVN::GetLocks(const CTSVNPath& url, std::map<CString, SVNLock> * locks)
{
    svn_ra_session_t *ra_session;

    SVNPool localpool(pool);
    Prepare();

    apr_hash_t * hash = apr_hash_make(localpool);

    /* use subpool to create a temporary RA session */
    const char* svnPath = url.GetSVNApiPath(localpool);
    CHooks::Instance().PreConnect(CTSVNPathList(url));
    SVNTRACE (
        Err = svn_client_open_ra_session2 (&ra_session, svnPath, NULL, m_pctx, localpool, localpool),
        svnPath
    );
    ClearCAPIAuthCacheOnError();
    if (Err != NULL)
        return false;

    SVNTRACE (
        Err = svn_ra_get_locks2(ra_session, &hash, "", svn_depth_infinity, localpool),
        svnPath
    )
    ClearCAPIAuthCacheOnError();
    if (Err != NULL)
        return false;
    apr_hash_index_t *hi;
    svn_lock_t* val;
    const char* key;
    for (hi = apr_hash_first(localpool, hash); hi; hi = apr_hash_next(hi))
    {
        apr_hash_this(hi, (const void**)&key, NULL, (void**)&val);
        if (val)
        {
            SVNLock lock;
            lock.comment = CUnicodeUtils::GetUnicode(val->comment);
            lock.creation_date = val->creation_date/1000000L;
            lock.expiration_date = val->expiration_date/1000000L;
            lock.owner = CUnicodeUtils::GetUnicode(val->owner);
            lock.path = CUnicodeUtils::GetUnicode(val->path);
            lock.token = CUnicodeUtils::GetUnicode(val->token);
            CString sKey = CUnicodeUtils::GetUnicode(key);
            (*locks)[sKey] = lock;
        }
    }
    return true;
}

bool SVN::GetWCRevisionStatus(const CTSVNPath& wcpath, bool bCommitted, svn_revnum_t& minrev, svn_revnum_t& maxrev, bool& switched, bool& modified, bool& sparse)
{
    SVNPool localpool(pool);
    Prepare();

    svn_wc_revision_status_t * revstatus = NULL;
    const char* svnPath = wcpath.GetSVNApiPath(localpool);
    SVNTRACE (
        Err = svn_wc_revision_status2(&revstatus, m_pctx->wc_ctx, svnPath, NULL, bCommitted, SVN::cancel, this, localpool, localpool),
        svnPath
    )

    if ((Err)||(revstatus == NULL))
    {
        minrev = 0;
        maxrev = 0;
        switched = false;
        modified = false;
        sparse = false;
        return false;
    }
    minrev = revstatus->min_rev;
    maxrev = revstatus->max_rev;
    switched = !!revstatus->switched;
    modified = !!revstatus->modified;
    sparse = !!revstatus->sparse_checkout;
    return true;
}


bool SVN::GetWCMinMaxRevs( const CTSVNPath& wcpath, bool committed, svn_revnum_t& minrev, svn_revnum_t& maxrev )
{
    SVNPool localpool(pool);
    Prepare();

    const char* svnPath = wcpath.GetSVNApiPath(localpool);
    SVNTRACE (
        Err = svn_client_min_max_revisions (&minrev, &maxrev, svnPath, committed, m_pctx, localpool),
        svnPath
        )

    return (Err == NULL);
}

bool SVN::Upgrade(const CTSVNPath& wcpath)
{
    SVNPool localpool(pool);
    Prepare();

    Err = svn_client_upgrade (wcpath.GetSVNApiPath(localpool), m_pctx, localpool);

    return (Err == NULL);
}

svn_revnum_t SVN::RevPropertySet(const CString& sName, const CString& sValue, const CString& sOldValue, const CTSVNPath& URL, const SVNRev& rev)
{
    svn_revnum_t set_rev;
    svn_string_t*   pval = NULL;
    svn_string_t*   pval2 = NULL;
    Prepare();

    CStringA sValueA = CUnicodeUtils::GetUTF8(sValue);
    sValueA.Remove('\r');
    pval = svn_string_create(sValueA, pool);
    if (!sOldValue.IsEmpty())
        pval2 = svn_string_create (CUnicodeUtils::GetUTF8(sOldValue), pool);

    const char* svnPath = URL.GetSVNApiPath(pool);
    CHooks::Instance().PreConnect(CTSVNPathList(URL));
    SVNTRACE (
        Err = svn_client_revprop_set2(CUnicodeUtils::GetUTF8(sName),
                                        pval,
                                        pval2,
                                        svnPath,
                                        rev,
                                        &set_rev,
                                        FALSE,
                                        m_pctx,
                                        pool),
        svnPath
    );
    ClearCAPIAuthCacheOnError();
    if (Err)
        return 0;
    return set_rev;
}

CString SVN::RevPropertyGet(const CString& sName, const CTSVNPath& URL, const SVNRev& rev)
{
    svn_string_t *propval;
    svn_revnum_t set_rev;
    Prepare();

    const char* svnPath = URL.GetSVNApiPath(pool);
    CHooks::Instance().PreConnect(CTSVNPathList(URL));
    SVNTRACE (
        Err = svn_client_revprop_get(CUnicodeUtils::GetUTF8(sName), &propval, svnPath, rev, &set_rev, m_pctx, pool),
        svnPath
    );
    ClearCAPIAuthCacheOnError();
    if (Err)
        return L"";
    if (propval==NULL)
        return L"";
    if (propval->len == 0)
        return L"";
    return CUnicodeUtils::GetUnicode(propval->data);
}

CTSVNPath SVN::GetPristinePath(const CTSVNPath& wcPath)
{
    svn_error_t * err;
    SVNPool localpool;

    const char* pristinePath = NULL;
    CTSVNPath returnPath;

#pragma warning(push)
#pragma warning(disable: 4996)  // deprecated warning
    // note: the 'new' function would be svn_wc_get_pristine_contents(), but that
    // function returns a stream instead of a path. Since we don't need a stream
    // but really a path here, that function is of no use and would require us
    // to create a temp file and copy the original contents to that temp file.
    //
    // We can't pass a stream to e.g. TortoiseMerge for diffing, that's why we
    // need a *path* and not a stream.
    err = svn_wc_get_pristine_copy_path(svn_path_internal_style(wcPath.GetSVNApiPath(localpool), localpool), &pristinePath, localpool);
#pragma warning(pop)

    if (err != NULL)
    {
        svn_error_clear(err);
        return returnPath;
    }
    if (pristinePath != NULL)
    {
        returnPath.SetFromSVN(pristinePath);
    }
    return returnPath;
}

bool SVN::PathIsURL(const CTSVNPath& path)
{
    return !!svn_path_is_url(CUnicodeUtils::GetUTF8(path.GetSVNPathString()));
}

void SVN::formatDate(TCHAR date_native[], apr_time_t date_svn, bool force_short_fmt)
{
    if (date_svn == 0)
    {
        wcscpy_s(date_native, SVN_DATE_BUFFER, L"(no date)");
        return;
    }

    FILETIME ft = {0};
    AprTimeToFileTime(&ft, date_svn);
    formatDate(date_native, ft, force_short_fmt);
}

void SVN::formatDate(TCHAR date_native[], FILETIME& filetime, bool force_short_fmt)
{
    enum {CACHE_SIZE = 16};
    static async::CCriticalSection mutex;
    static FILETIME lastTime[CACHE_SIZE] = { {0, 0}, {0, 0}, {0, 0}, {0, 0}
                                           , {0, 0}, {0, 0}, {0, 0}, {0, 0}
                                           , {0, 0}, {0, 0}, {0, 0}, {0, 0}
                                           , {0, 0}, {0, 0}, {0, 0}, {0, 0} };
    static TCHAR lastResult[CACHE_SIZE][SVN_DATE_BUFFER];
    static bool formats[CACHE_SIZE];

    // we have to serialize access to the cache

    async::CCriticalSectionLock lock (mutex);

    // cache lookup

    TCHAR* result = NULL;
    for (size_t i = 0; i < CACHE_SIZE; ++i)
        if (   (lastTime[i].dwHighDateTime == filetime.dwHighDateTime)
            && (lastTime[i].dwLowDateTime == filetime.dwLowDateTime)
            && (formats[i] == force_short_fmt))
        {
            result = lastResult[i];
            break;
        }

    // set cache to new data, if old is no hit

    if (result == NULL)
    {
        // evict an entry from the cache

        static size_t victim = 0;
        lastTime[victim] = filetime;
        result = lastResult[victim];
        formats[victim] = force_short_fmt;
        if (++victim == CACHE_SIZE)
            victim = 0;

        result[0] = '\0';

        // Convert UTC to local time
        SYSTEMTIME systemtime;
        VERIFY ( FileTimeToSystemTime(&filetime,&systemtime) );

        static TIME_ZONE_INFORMATION timeZone = {-1};
        if (timeZone.Bias == -1)
            GetTimeZoneInformation (&timeZone);

        SYSTEMTIME localsystime;
        VERIFY ( SystemTimeToTzSpecificLocalTime(&timeZone, &systemtime,&localsystime));

        TCHAR timebuf[SVN_DATE_BUFFER] = {0};
        TCHAR datebuf[SVN_DATE_BUFFER] = {0};

        LCID locale = s_useSystemLocale ? MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), SORT_DEFAULT) : s_locale;

        /// reusing this instance is vital for \ref formatDate performance

        static CRegDWORD logDateFormat (500, L"Software\\TortoiseSVN\\LogDateFormat");
        DWORD flags = force_short_fmt || (logDateFormat == 1)
                    ? DATE_SHORTDATE
                    : DATE_LONGDATE;

        GetDateFormat(locale, flags, &localsystime, NULL, datebuf, SVN_DATE_BUFFER);
        GetTimeFormat(locale, 0, &localsystime, NULL, timebuf, SVN_DATE_BUFFER);
        wcsncat_s(result, SVN_DATE_BUFFER, datebuf, SVN_DATE_BUFFER);
        wcsncat_s(result, SVN_DATE_BUFFER, L" ", SVN_DATE_BUFFER);
        wcsncat_s(result, SVN_DATE_BUFFER, timebuf, SVN_DATE_BUFFER);
    }

    // copy formatted string to result

    wcsncpy_s (date_native, SVN_DATE_BUFFER, result, SVN_DATE_BUFFER);
}

CString SVN::formatDate(apr_time_t date_svn)
{
    apr_time_exp_t exploded_time = {0};

    SYSTEMTIME systime = {0,0,0,0,0,0,0,0};
    TCHAR datebuf[SVN_DATE_BUFFER] = {0};

    LCID locale = s_useSystemLocale ? MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), SORT_DEFAULT) : s_locale;

    try
    {
        apr_time_exp_lt (&exploded_time, date_svn);

        systime.wDay = (WORD)exploded_time.tm_mday;
        systime.wDayOfWeek = (WORD)exploded_time.tm_wday;
        systime.wMonth = (WORD)exploded_time.tm_mon+1;
        systime.wYear = (WORD)exploded_time.tm_year+1900;

        GetDateFormat(locale, DATE_SHORTDATE, &systime, NULL, datebuf, SVN_DATE_BUFFER);
    }
    catch ( ... )
    {
        wcscpy_s(datebuf, L"(no date)");
    }

    return datebuf;
}

CString SVN::formatTime (apr_time_t date_svn)
{
    apr_time_exp_t exploded_time = {0};

    SYSTEMTIME systime = {0,0,0,0,0,0,0,0};
    TCHAR timebuf[SVN_DATE_BUFFER] = {0};

    LCID locale = s_useSystemLocale ? MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), SORT_DEFAULT) : s_locale;

    try
    {
        apr_time_exp_lt (&exploded_time, date_svn);

        systime.wDay = (WORD)exploded_time.tm_mday;
        systime.wDayOfWeek = (WORD)exploded_time.tm_wday;
        systime.wHour = (WORD)exploded_time.tm_hour;
        systime.wMilliseconds = (WORD)(exploded_time.tm_usec/1000);
        systime.wMinute = (WORD)exploded_time.tm_min;
        systime.wMonth = (WORD)exploded_time.tm_mon+1;
        systime.wSecond = (WORD)exploded_time.tm_sec;
        systime.wYear = (WORD)exploded_time.tm_year+1900;

        GetTimeFormat(locale, 0, &systime, NULL, timebuf, SVN_DATE_BUFFER);
    }
    catch ( ... )
    {
        wcscpy_s(timebuf, L"(no time)");
    }

    return timebuf;
}

CString SVN::MakeUIUrlOrPath(const CStringA& UrlOrPath)
{
    CString url;
    if (svn_path_is_url(UrlOrPath))
    {
        size_t length = UrlOrPath.GetLength();
        url = CUnicodeUtils::GetUnicode
            (CPathUtils::ContainsEscapedChars (UrlOrPath, length)
                ? CPathUtils::PathUnescape(UrlOrPath)
                : UrlOrPath
            );
    }
    else
        url = CUnicodeUtils::GetUnicode(UrlOrPath);

    return url;
}

std::string SVN::MakeUIUrlOrPath(const char* UrlOrPath)
{
    if (!svn_path_is_url (UrlOrPath))
        return UrlOrPath;

    std::string result = UrlOrPath;
    if (CPathUtils::ContainsEscapedChars (result.c_str(), result.length()))
        CPathUtils::Unescape (&result[0]);

    return result;
}

bool SVN::EnsureConfigFile()
{
    svn_error_t * err;
    SVNPool localpool;
    err = svn_config_ensure(NULL, localpool);
    if (err)
    {
        svn_error_clear(err);
        return false;
    }
    return true;
}

CString SVN::GetOptionsString(bool bIgnoreEOL, bool bIgnoreSpaces, bool bIgnoreAllSpaces)
{
    CString opts;
    if (bIgnoreEOL)
        opts += L"--ignore-eol-style ";
    if (bIgnoreAllSpaces)
        opts += L"-w";
    else if (bIgnoreSpaces)
        opts += L"-b";
    opts.Trim();
    return opts;
}

CString SVN::GetOptionsString(bool bIgnoreEOL, svn_diff_file_ignore_space_t space)
{
    CString opts;
    if (bIgnoreEOL)
        opts += L"--ignore-eol-style ";
    switch (space)
    {
    case svn_diff_file_ignore_space_change:
        opts += L"-b";
        break;
    case svn_diff_file_ignore_space_all:
        opts += L"-w";
        break;
    }
    opts.Trim();
    return opts;
}

/**
 * Note that this is TSVN's global CLogCachePool creation /
 * destruction mutex. Don't access logCachePool or create a
 * CLogCachePool directly anywhere else in TSVN.
 */
async::CCriticalSection& SVN::GetLogCachePoolMutex()
{
    static async::CCriticalSection mutex;
    return mutex;
}

/**
 * Returns the log cache pool singleton. You will need that to
 * create \c CCacheLogQuery instances.
 */
LogCache::CLogCachePool* SVN::GetLogCachePool()
{
    // modifying the log cache is not thread safe.
    // In particular, we must synchronize the loading & file check.

    async::CCriticalSectionLock lock (GetLogCachePoolMutex());

    if (logCachePool.get() == NULL)
    {
        CString cacheFolder = CPathUtils::GetAppDataDirectory() + L"logcache\\";
        logCachePool.reset (new LogCache::CLogCachePool (*this, cacheFolder));
    }

    return logCachePool.get();
}

/**
 * Close the logCachePool.
 */
void SVN::ResetLogCachePool()
{
    // this should be called by the ~SVN only but we make sure that
    // (illegal) concurrent access won't see zombi objects.

    async::CCriticalSectionLock lock (GetLogCachePoolMutex());
    if (logCachePool.get() != NULL)
        logCachePool->Flush();

    logCachePool.reset();
}

/**
 * Returns the status of the encapsulated \ref SVNPrompt instance.
 */
bool SVN::PromptShown() const
{
    return m_prompt.PromptShown();
}

/**
 * Set the parent window of an authentication prompt dialog
 */
void SVN::SetPromptParentWindow(HWND hWnd)
{
    m_prompt.SetParentWindow(hWnd);
}
/**
 * Set the MFC Application object for a prompt dialog
 */
void SVN::SetPromptApp(CWinApp* pWinApp)
{
    m_prompt.SetApp(pWinApp);
}

apr_array_header_t * SVN::MakeCopyArray(const CTSVNPathList& pathList, const SVNRev& rev, const SVNRev& pegrev)
{
    apr_array_header_t * sources = apr_array_make(pool, pathList.GetCount(),
        sizeof(svn_client_copy_source_t *));

    for (int nItem = 0; nItem < pathList.GetCount(); ++nItem)
    {
        const char *target = apr_pstrdup (pool, pathList[nItem].GetSVNApiPath(pool));
        svn_client_copy_source_t *source = (svn_client_copy_source_t*)apr_palloc(pool, sizeof(*source));
        source->path = target;
        source->revision = rev;
        source->peg_revision = pegrev;
        APR_ARRAY_PUSH(sources, svn_client_copy_source_t *) = source;
    }
    return sources;
}

apr_array_header_t * SVN::MakeChangeListArray(const CStringArray& changelists, apr_pool_t * pool)
{
    // passing NULL if there are no change lists will work only partially: the subversion library
    // in that case executes the command, but fails to remove the existing change lists from the files
    // if 'keep change lists is set to false.
    // We therefore have to create an empty array instead of passing NULL, only then the
    // change lists are removed properly.
    int count = (int)changelists.GetCount();
    // special case: the changelist array contains one empty string
    if ((changelists.GetCount() == 1)&&(changelists[0].IsEmpty()))
        count = 0;

    apr_array_header_t * arr = apr_array_make (pool, count, sizeof(const char *));

    if (count == 0)
        return arr;

    if (!changelists.IsEmpty())
    {
        for (int nItem = 0; nItem < changelists.GetCount(); nItem++)
        {
            const char * c = apr_pstrdup(pool, (LPCSTR)CUnicodeUtils::GetUTF8(changelists[nItem]));
            (*((const char **) apr_array_push(arr))) = c;
        }
    }
    return arr;
}

apr_hash_t * SVN::MakeRevPropHash(const RevPropHash& revProps, apr_pool_t * pool)
{
    apr_hash_t * revprop_table = NULL;
    if (!revProps.empty())
    {
        revprop_table = apr_hash_make(pool);
        for (RevPropHash::const_iterator it = revProps.begin(); it != revProps.end(); ++it)
        {
            svn_string_t *propval = svn_string_create((LPCSTR)CUnicodeUtils::GetUTF8(it->second), pool);
            apr_hash_set (revprop_table, apr_pstrdup(pool, (LPCSTR)CUnicodeUtils::GetUTF8(it->first)), APR_HASH_KEY_STRING, (const void*)propval);
        }
    }

    return revprop_table;
}

svn_error_t * SVN::commitcallback2(const svn_commit_info_t * commit_info, void * baton, apr_pool_t * localpool)
{
    if (commit_info)
    {
        SVN * pThis = (SVN*)baton;
        pThis->m_commitRev = commit_info->revision;
        if (SVN_IS_VALID_REVNUM(commit_info->revision))
        {
            pThis->Notify(CTSVNPath(), CTSVNPath(), svn_wc_notify_update_completed, svn_node_none, L"",
                    svn_wc_notify_state_unknown, svn_wc_notify_state_unknown,
                    commit_info->revision, NULL, svn_wc_notify_lock_state_unchanged,
                    L"", L"", NULL, NULL, localpool);
        }
        if (commit_info->post_commit_err)
        {
            pThis->PostCommitErr = CUnicodeUtils::GetUnicode(commit_info->post_commit_err);
        }
    }
    return NULL;
}

void SVN::SetAndClearProgressInfo(HWND hWnd)
{
    m_progressWnd = hWnd;
    m_pProgressDlg = NULL;
    progress_total = 0;
    progress_lastprogress = 0;
    progress_lasttotal = 0;
    progress_lastTicks = GetTickCount64();
}

void SVN::SetAndClearProgressInfo(CProgressDlg * pProgressDlg, bool bShowProgressBar/* = false*/)
{
    m_progressWnd = NULL;
    m_pProgressDlg = pProgressDlg;
    progress_total = 0;
    progress_lastprogress = 0;
    progress_lasttotal = 0;
    progress_lastTicks = GetTickCount64();
    m_bShowProgressBar = bShowProgressBar;
}

CString SVN::GetSummarizeActionText(svn_client_diff_summarize_kind_t kind)
{
    CString sAction;
    switch (kind)
    {
    case svn_client_diff_summarize_kind_normal:
        sAction.LoadString(IDS_SVN_SUMMARIZENORMAL);
        break;
    case svn_client_diff_summarize_kind_added:
        sAction.LoadString(IDS_SVN_SUMMARIZEADDED);
        break;
    case svn_client_diff_summarize_kind_modified:
        sAction.LoadString(IDS_SVN_SUMMARIZEMODIFIED);
        break;
    case svn_client_diff_summarize_kind_deleted:
        sAction.LoadString(IDS_SVN_SUMMARIZEDELETED);
        break;
    }
    return sAction;
}

void SVN::progress_func(apr_off_t progress, apr_off_t total, void *baton, apr_pool_t * /*pool*/)
{
    SVN * pSVN = (SVN*)baton;
    if ((pSVN==0)||((pSVN->m_progressWnd == 0)&&(pSVN->m_pProgressDlg == 0)))
        return;
    apr_off_t delta = progress;
    if ((progress >= pSVN->progress_lastprogress)&&((total == pSVN->progress_lasttotal) || (total < 0)))
        delta = progress - pSVN->progress_lastprogress;

    pSVN->progress_lastprogress = progress;
    pSVN->progress_lasttotal = total;

    ULONGLONG ticks = GetTickCount64();
    pSVN->progress_vector.push_back(delta);
    pSVN->progress_total += delta;
    //ATLTRACE("progress = %I64d, total = %I64d, delta = %I64d, overall total is : %I64d\n", progress, total, delta, pSVN->progress_total);
    if ((pSVN->progress_lastTicks + 1000UL) < ticks)
    {
        double divby = (double(ticks - pSVN->progress_lastTicks)/1000.0);
        if (divby < 0.0001)
            divby = 1;
        pSVN->m_SVNProgressMSG.overall_total = pSVN->progress_total;
        pSVN->m_SVNProgressMSG.progress = progress;
        pSVN->m_SVNProgressMSG.total = total;
        pSVN->progress_lastTicks = ticks;
        apr_off_t average = 0;
        for (std::vector<apr_off_t>::iterator it = pSVN->progress_vector.begin(); it != pSVN->progress_vector.end(); ++it)
        {
            average += *it;
        }
        average = apr_off_t(double(average) / divby);
        if (average >= 0x100000000LL)
        {
            // it seems that sometimes we get ridiculous numbers here.
            // Anyone *really* having more than 4GB/sec throughput?
            average = 0;
        }

        pSVN->m_SVNProgressMSG.BytesPerSecond = average;
        if (average < 1024LL)
            pSVN->m_SVNProgressMSG.SpeedString.Format(IDS_SVN_PROGRESS_BYTES_SEC, average);
        else
        {
            double averagekb = (double)average / 1024.0;
            pSVN->m_SVNProgressMSG.SpeedString.Format(IDS_SVN_PROGRESS_KBYTES_SEC, averagekb);
        }
        CString s;
        s.Format(IDS_SVN_PROGRESS_SPEED, (LPCWSTR)pSVN->m_SVNProgressMSG.SpeedString);
        pSVN->m_SVNProgressMSG.SpeedString = s;
        if (pSVN->m_progressWnd)
            SendMessage(pSVN->m_progressWnd, WM_SVNPROGRESS, 0, (LPARAM)&pSVN->m_SVNProgressMSG);
        else if (pSVN->m_pProgressDlg)
        {
            if ((pSVN->m_bShowProgressBar && (progress > 1000LL) && (total > 0LL)))
                pSVN->m_pProgressDlg->SetProgress64(progress, total);

            CString sTotal;
            CString temp;
            if (pSVN->m_SVNProgressMSG.overall_total < 1024LL)
                sTotal.Format(IDS_SVN_PROGRESS_TOTALBYTESTRANSFERRED, pSVN->m_SVNProgressMSG.overall_total);
            else if (pSVN->m_SVNProgressMSG.overall_total < 1200000LL)
                sTotal.Format(IDS_SVN_PROGRESS_TOTALTRANSFERRED, pSVN->m_SVNProgressMSG.overall_total / 1024LL);
            else
                sTotal.Format(IDS_SVN_PROGRESS_TOTALMBTRANSFERRED, (double)((double)pSVN->m_SVNProgressMSG.overall_total / 1024000.0));
            temp.FormatMessage(IDS_SVN_PROGRESS_TOTALANDSPEED, (LPCTSTR)sTotal, (LPCTSTR)pSVN->m_SVNProgressMSG.SpeedString);

            pSVN->m_pProgressDlg->SetLine(2, temp);
        }
        pSVN->progress_vector.clear();
    }
    return;
}

void SVN::CallPreConnectHookIfUrl( const CTSVNPathList& pathList, const CTSVNPath& path /* = CTSVNPath()*/ )
{
    if (pathList.GetCount())
    {
        if (pathList[0].IsUrl())
            CHooks::Instance().PreConnect(pathList);
        else if (path.IsUrl())
            CHooks::Instance().PreConnect(pathList);
    }
}

CString SVN::GetChecksumString( svn_checksum_kind_t type, const CString& s, apr_pool_t * localpool )
{
    svn_checksum_t *checksum;
    CStringA sa = CUnicodeUtils::GetUTF8(s);
    svn_checksum(&checksum, type, s, s.GetLength(), localpool);
    const char * hexname = svn_checksum_to_cstring(checksum, localpool);
    CString hex = CUnicodeUtils::GetUnicode(hexname);
    return hex;
}

CString SVN::GetChecksumString( svn_checksum_kind_t type, const CString& s )
{
    return GetChecksumString(type, s, pool);
}

void SVN::Prepare()
{
    svn_error_clear(Err);
    Err = NULL;
    m_redirectedUrl.Reset();
    PostCommitErr.Empty();
}

void SVN::SetAuthInfo(const CString& username, const CString& password)
{
    if (m_pctx)
    {
        if (!username.IsEmpty())
        {
            svn_auth_set_parameter(m_pctx->auth_baton,
                                   SVN_AUTH_PARAM_DEFAULT_USERNAME, apr_pstrdup(pool, CUnicodeUtils::GetUTF8(username)));
        }
        if (!password.IsEmpty())
        {
            svn_auth_set_parameter(m_pctx->auth_baton,
                                   SVN_AUTH_PARAM_DEFAULT_PASSWORD, apr_pstrdup(pool, CUnicodeUtils::GetUTF8(password)));
        }
    }
}

svn_error_t * svn_error_handle_malfunction(svn_boolean_t can_return,
                                           const char *file, int line,
                                           const char *expr)
{
    // we get here every time Subversion encounters something very unexpected.
    // in previous versions, Subversion would just call abort() - now we can
    // show the user some information before we return.
    svn_error_t * err = svn_error_raise_on_malfunction(TRUE, file, line, expr);

    CString sErr(MAKEINTRESOURCE(IDS_ERR_SVNEXCEPTION));
    if (err)
    {
        sErr += L"\n\n" + SVN::GetErrorString(err);
        ::MessageBox(NULL, sErr, L"Subversion Exception!", MB_ICONERROR);
        if (can_return)
            return err;
        if (CRegDWORD(L"Software\\TortoiseSVN\\Debug", FALSE)==FALSE)
        {
            CCrashReport::Instance().AddUserInfoToReport(L"SVNException", sErr);
            CCrashReport::Instance().Uninstall();
            abort();    // ugly, ugly! But at least we showed a messagebox first
        }
    }

    CString sFormatErr;
    sFormatErr.FormatMessage(IDS_ERR_SVNFORMATEXCEPTION, CUnicodeUtils::GetUnicode(file), line, CUnicodeUtils::GetUnicode(expr));
    ::MessageBox(NULL, sFormatErr, L"Subversion Exception!", MB_ICONERROR);
    if (CRegDWORD(L"Software\\TortoiseSVN\\Debug", FALSE)==FALSE)
    {
        CCrashReport::Instance().AddUserInfoToReport(L"SVNException", sFormatErr);
        CCrashReport::Instance().Uninstall();
        abort();    // ugly, ugly! But at least we showed a messagebox first
    }
    return NULL;    // never reached, only to silence compiler warning
}
