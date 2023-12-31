﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2015, 2021-2022 - TortoiseSVN

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
#include "../TortoiseShell/resource.h"
#include "SVNStatus.h"
#include "UnicodeUtils.h"
#include "SVNConfig.h"
#include "SVNHelpers.h"
#include "SVNTrace.h"
#ifdef _MFC_VER
#    include "SVN.h"
#    include "TSVNPath.h"
#    include "Hooks.h"
#endif

#ifdef _MFC_VER
SVNStatus::SVNStatus(bool* pbCancelled, bool suppressUI)
    : SVNBase()
    , status(nullptr)
    , headrev(-1)
    , m_allstatus(svn_wc_status_none)
    , m_prompt(suppressUI)
    , m_statusHash(nullptr)
    , m_statusArray(nullptr)
    , m_statusHashIndex(0)
    , m_externalHash(nullptr)
#else
SVNStatus::SVNStatus(bool* pbCancelled, bool)
    : SVNBase()
    , status(nullptr)
    , headrev(-1)
    , m_allstatus(svn_wc_status_none)
    , m_statusHash(nullptr)
    , m_statusArray(nullptr)
    , m_statusHashIndex(0)
    , m_externalHash(nullptr)
#endif
{
    m_pool = svn_pool_create(NULL);

    svn_error_clear(svn_client_create_context2(&m_pCtx, SVNConfig::Instance().GetConfig(m_pool), m_pool));

    if (pbCancelled)
    {
        m_pCtx->cancel_func  = cancel;
        m_pCtx->cancel_baton = pbCancelled;
    }
    m_pCtx->client_name = SVNHelper::GetUserAgentString(m_pool);

#ifdef _MFC_VER
    // set up authentication
    m_prompt.Init(m_pool, m_pCtx);

    if (m_err)
    {
        ShowErrorDialog(nullptr);
        svn_error_clear(m_err);
        svn_pool_destroy(m_pool); // free the allocated memory
        exit(-1);
    }
#endif
}

SVNStatus::~SVNStatus()
{
    svn_error_clear(m_err);
    svn_pool_destroy(m_pool); // free the allocated memory
}

void SVNStatus::ClearPool() const
{
    svn_pool_clear(m_pool);
}

// static method
svn_wc_status_kind SVNStatus::GetAllStatus(const CTSVNPath& path, svn_depth_t depth)
{
    svn_client_ctx_t*  ctx;
    svn_wc_status_kind statusKind;
    apr_pool_t*        pool = nullptr;
    svn_error_t*       err  = nullptr;

    pool = svn_pool_create(NULL); // create the memory pool

    svn_error_clear(svn_client_create_context2(&ctx, SVNConfig::Instance().GetConfig(pool), pool));

    svn_revnum_t       youngest = SVN_INVALID_REVNUM;
    svn_opt_revision_t rev{};
    rev.kind   = svn_opt_revision_unspecified;
    statusKind = svn_wc_status_none;

    const char* svnPath = path.GetSVNApiPath(pool);
    if ((svnPath == nullptr) || (svnPath[0] == 0))
        return svn_wc_status_none;
    SVNTRACE(
        err = svn_client_status6(&youngest,
                                 ctx,
                                 svnPath,
                                 &rev,
                                 depth,
                                 TRUE,  // get all
                                 FALSE, // check out-of-date
                                 TRUE,  // check working copy
                                 TRUE,  // no ignore
                                 FALSE, // ignore externals
                                 TRUE,  // depth as sticky
                                 NULL,
                                 getallstatus,
                                 &statusKind,
                                 pool),
        svnPath)

    // Error present
    if (err != nullptr)
    {
        svn_error_clear(err);
        svn_pool_destroy(pool); //free allocated memory
        return svn_wc_status_none;
    }

    svn_pool_destroy(pool); //free allocated memory
    return statusKind;
}

// static method
svn_wc_status_kind SVNStatus::GetAllStatusRecursive(const CTSVNPath& path)
{
    return GetAllStatus(path, svn_depth_infinity);
}

// static method
svn_wc_status_kind SVNStatus::GetMoreImportant(svn_wc_status_kind status1, svn_wc_status_kind status2)
{
    if (GetStatusRanking(status1) >= GetStatusRanking(status2))
        return status1;
    return status2;
}
// static private method
int SVNStatus::GetStatusRanking(svn_wc_status_kind status)
{
    switch (status)
    {
        case svn_wc_status_none:
            return 0;
        case svn_wc_status_unversioned:
            return 1;
        case svn_wc_status_ignored:
            return 2;
        case svn_wc_status_incomplete:
            return 4;
        case svn_wc_status_normal:
        case svn_wc_status_external:
            return 5;
        case svn_wc_status_added:
            return 6;
        case svn_wc_status_missing:
            return 7;
        case svn_wc_status_deleted:
            return 8;
        case svn_wc_status_replaced:
            return 9;
        case svn_wc_status_modified:
            return 10;
        case svn_wc_status_merged:
            return 11;
        case svn_wc_status_conflicted:
            return 12;
        case svn_wc_status_obstructed:
            return 13;
    }
    return 0;
}

svn_revnum_t SVNStatus::GetStatus(const CTSVNPath& path, bool update /* = false */, bool noignore /* = false */, bool noexternals /* = false */)
{
    apr_hash_t*         statusHash  = nullptr;
    apr_hash_t*         extHash     = nullptr;
    apr_array_header_t* statusArray = nullptr;
    const SortItem*     item;

    svn_error_clear(m_err);
    statusHash                  = apr_hash_make(m_pool);
    extHash                     = apr_hash_make(m_pool);
    svn_revnum_t       youngest = SVN_INVALID_REVNUM;
    svn_opt_revision_t rev{};
    rev.kind = svn_opt_revision_unspecified;
    HashbatonT hashBaton{};
    hashBaton.hash    = statusHash;
    hashBaton.extHash = extHash;
    hashBaton.pThis   = this;

    const char* svnPath = path.GetSVNApiPath(m_pool);
    if ((svnPath == nullptr) || (svnPath[0] == 0))
    {
        status = nullptr;
        return -2;
    }

#ifdef _MFC_VER
    if (update)
        CHooks::Instance().PreConnect(CTSVNPathList(path));
#endif
    SVNTRACE(
        m_err = svn_client_status6(&youngest,
                                   m_pCtx,
                                   svnPath,
                                   &rev,
                                   svn_depth_empty, // depth
                                   TRUE,            // get all
                                   update,          // check out-of-date
                                   true,            // check working copy
                                   noignore,
                                   noexternals,
                                   TRUE, // depth as sticky
                                   NULL,
                                   getstatushash,
                                   &hashBaton,
                                   m_pool),
        svnPath);
    ClearCAPIAuthCacheOnError();

    // Error present if function is not under version control
    if ((m_err != nullptr) || (apr_hash_count(statusHash) == 0))
    {
        status = nullptr;
        return -2;
    }

    // Convert the unordered hash to an ordered, sorted array
    statusArray = sort_hash(statusHash,
                            sort_compare_items_as_paths,
                            m_pool);

    // only the first entry is needed (no recurse)
    item = &APR_ARRAY_IDX(statusArray, 0, const SortItem);

    status = static_cast<svn_client_status_t*>(item->value);

    return youngest;
}

svn_client_status_t* SVNStatus::GetFirstFileStatus(const CTSVNPath& path, CTSVNPath& retPath, bool update, svn_depth_t depth, bool bNoIgnore /* = true */, bool bNoExternals /* = false */)
{
    const SortItem* item = nullptr;

    svn_error_clear(m_err);
    m_statusHash   = apr_hash_make(m_pool);
    m_externalHash = apr_hash_make(m_pool);
    headrev        = SVN_INVALID_REVNUM;
    svn_opt_revision_t rev{};
    rev.kind = svn_opt_revision_unspecified;
    HashbatonT hashBaton{};
    hashBaton.hash    = m_statusHash;
    hashBaton.extHash = m_externalHash;
    hashBaton.pThis   = this;
    m_statusHashIndex = 0;

    m_pCtx->notify_func2  = notify;
    m_pCtx->notify_baton2 = &hashBaton;

    const char* svnPath = path.GetSVNApiPath(m_pool);
    if ((svnPath == nullptr) || (svnPath[0] == 0))
        return nullptr;
#ifdef _MFC_VER
    if (update)
        CHooks::Instance().PreConnect(CTSVNPathList(path));
#endif
    SVNTRACE(
        m_err = svn_client_status6(&headrev,
                                   m_pCtx,
                                   svnPath,
                                   &rev,
                                   depth,
                                   TRUE,   // get all
                                   update, // check out-of-date
                                   true,   // check working copy
                                   bNoIgnore,
                                   bNoExternals,
                                   TRUE, // depth as sticky
                                   NULL,
                                   getstatushash,
                                   &hashBaton,
                                   m_pool),
        svnPath)
    ClearCAPIAuthCacheOnError();

    // Error present if function is not under version control
    if ((m_err != nullptr) || (apr_hash_count(m_statusHash) == 0))
    {
        return nullptr;
    }

    // Convert the unordered hash to an ordered, sorted array
    m_statusArray = sort_hash(m_statusHash,
                              sort_compare_items_as_paths,
                              m_pool);

    // only the first entry is needed (no recurse)
    m_statusHashIndex = 0;
    item              = &APR_ARRAY_IDX(m_statusArray, m_statusHashIndex, const SortItem);
    retPath.SetFromSVN(static_cast<const char*>(item->key));
    return static_cast<svn_client_status_t*>(item->value);
}

unsigned int SVNStatus::GetVersionedCount() const
{
    unsigned int    count = 0;
    const SortItem* item  = nullptr;
    for (unsigned int i = 0; i < apr_hash_count(m_statusHash); ++i)
    {
        item = &APR_ARRAY_IDX(m_statusArray, i, const SortItem);
        if (item)
        {
            if (SVNStatus::GetMoreImportant(static_cast<svn_client_status_t*>(item->value)->node_status, svn_wc_status_ignored) != svn_wc_status_ignored)
                count++;
        }
    }
    return count;
}

svn_client_status_t* SVNStatus::GetNextFileStatus(CTSVNPath& retPath)
{
    const SortItem* item = nullptr;

    if ((m_statusHashIndex + 1) >= apr_hash_count(m_statusHash))
        return nullptr;
    m_statusHashIndex++;

    item = &APR_ARRAY_IDX(m_statusArray, m_statusHashIndex, const SortItem);
    retPath.SetFromSVN(static_cast<const char*>(item->key));
    return static_cast<svn_client_status_t*>(item->value);
}

bool SVNStatus::IsExternal(const CTSVNPath& path) const
{
    if (apr_hash_get(m_externalHash, path.GetSVNApiPath(m_pool), APR_HASH_KEY_STRING))
        return true;
    return false;
}

bool SVNStatus::IsInExternal(const CTSVNPath& path) const
{
    if (apr_hash_count(m_statusHash) == 0)
        return false;

    SVNPool           localPool(m_pool);
    apr_hash_index_t* hi = nullptr;
    const char*       key = nullptr;
    for (hi = apr_hash_first(localPool, m_externalHash); hi; hi = apr_hash_next(hi))
    {
        apr_hash_this(hi, reinterpret_cast<const void**>(&key), nullptr, nullptr);
        if (key)
        {
            if (CTSVNPath(CUnicodeUtils::GetUnicode(key)).IsAncestorOf(path))
                return true;
        }
    }
    return false;
}

void SVNStatus::GetExternals(std::set<CTSVNPath>& externals) const
{
    if (apr_hash_count(m_statusHash) == 0)
        return;

    SVNPool           localPool(m_pool);
    apr_hash_index_t* hi = nullptr;
    const char*       key = nullptr;
    for (hi = apr_hash_first(localPool, m_externalHash); hi; hi = apr_hash_next(hi))
    {
        apr_hash_this(hi, reinterpret_cast<const void**>(&key), nullptr, nullptr);
        if (key)
        {
            externals.insert(CTSVNPath(CUnicodeUtils::GetUnicode(key)));
        }
    }
}

void SVNStatus::GetStatusString(svn_wc_status_kind status, size_t bufLen, wchar_t* string)
{
    wcsncpy_s(string, bufLen, L"\0", bufLen);
    switch (status)
    {
        case svn_wc_status_none:
            wcsncpy_s(string, bufLen, L"none\0", bufLen);
            break;
        case svn_wc_status_unversioned:
            wcsncpy_s(string, bufLen, L"unversioned\0", bufLen);
            break;
        case svn_wc_status_normal:
            wcsncpy_s(string, bufLen, L"normal\0", bufLen);
            break;
        case svn_wc_status_added:
            wcsncpy_s(string, bufLen, L"added\0", bufLen);
            break;
        case svn_wc_status_missing:
            wcsncpy_s(string, bufLen, L"missing\0", bufLen);
            break;
        case svn_wc_status_deleted:
            wcsncpy_s(string, bufLen, L"deleted\0", bufLen);
            break;
        case svn_wc_status_replaced:
            wcsncpy_s(string, bufLen, L"replaced\0", bufLen);
            break;
        case svn_wc_status_modified:
            wcsncpy_s(string, bufLen, L"modified\0", bufLen);
            break;
        case svn_wc_status_merged:
            wcsncpy_s(string, bufLen, L"merged\0", bufLen);
            break;
        case svn_wc_status_conflicted:
            wcsncpy_s(string, bufLen, L"conflicted\0", bufLen);
            break;
        case svn_wc_status_obstructed:
            wcsncpy_s(string, bufLen, L"obstructed\0", bufLen);
            break;
        case svn_wc_status_ignored:
            wcsncpy_s(string, bufLen, L"ignored\0", bufLen);
            break;
        case svn_wc_status_external:
            wcsncpy_s(string, bufLen, L"external\0", bufLen);
            break;
        case svn_wc_status_incomplete:
            wcsncpy_s(string, bufLen, L"incomplete\0", bufLen);
            break;
        default:
            break;
    }
}

void SVNStatus::GetStatusString(HINSTANCE hInst, svn_wc_status_kind status, wchar_t* string, int size, WORD lang)
{
    enum
    {
        MAX_STATUS_LENGTH = 240
    };

    struct SCacheEntry
    {
        wchar_t buffer[MAX_STATUS_LENGTH];

        HINSTANCE          hInst;
        svn_wc_status_kind status;
        WORD               lang;
    };

    static std::vector<SCacheEntry> cache;
    for (size_t count = cache.size(), i = 0; i < count; ++i)
    {
        const SCacheEntry& entry = cache[i];
        if ((entry.status == status) && (entry.hInst == hInst) && (entry.lang == lang))
        {
            wcsncpy_s(string, size, entry.buffer, min(size, static_cast<int>(MAX_STATUS_LENGTH)) - 1);
            return;
        }
    }

    cache.push_back(SCacheEntry());
    SCacheEntry& entry = cache.back();
    entry.status       = status;
    entry.hInst        = hInst;
    entry.lang         = lang;

    switch (status)
    {
        case svn_wc_status_none:
            LoadStringEx(hInst, IDS_STATUSNONE, entry.buffer, MAX_STATUS_LENGTH, lang);
            break;
        case svn_wc_status_unversioned:
            LoadStringEx(hInst, IDS_STATUSUNVERSIONED, entry.buffer, MAX_STATUS_LENGTH, lang);
            break;
        case svn_wc_status_normal:
            LoadStringEx(hInst, IDS_STATUSNORMAL, entry.buffer, MAX_STATUS_LENGTH, lang);
            break;
        case svn_wc_status_added:
            LoadStringEx(hInst, IDS_STATUSADDED, entry.buffer, MAX_STATUS_LENGTH, lang);
            break;
        case svn_wc_status_missing:
            LoadStringEx(hInst, IDS_STATUSABSENT, entry.buffer, MAX_STATUS_LENGTH, lang);
            break;
        case svn_wc_status_deleted:
            LoadStringEx(hInst, IDS_STATUSDELETED, entry.buffer, MAX_STATUS_LENGTH, lang);
            break;
        case svn_wc_status_replaced:
            LoadStringEx(hInst, IDS_STATUSREPLACED, entry.buffer, MAX_STATUS_LENGTH, lang);
            break;
        case svn_wc_status_modified:
            LoadStringEx(hInst, IDS_STATUSMODIFIED, entry.buffer, MAX_STATUS_LENGTH, lang);
            break;
        case svn_wc_status_merged:
            LoadStringEx(hInst, IDS_STATUSMERGED, entry.buffer, MAX_STATUS_LENGTH, lang);
            break;
        case svn_wc_status_conflicted:
            LoadStringEx(hInst, IDS_STATUSCONFLICTED, entry.buffer, MAX_STATUS_LENGTH, lang);
            break;
        case svn_wc_status_ignored:
            LoadStringEx(hInst, IDS_STATUSIGNORED, entry.buffer, MAX_STATUS_LENGTH, lang);
            break;
        case svn_wc_status_obstructed:
            LoadStringEx(hInst, IDS_STATUSOBSTRUCTED, entry.buffer, MAX_STATUS_LENGTH, lang);
            break;
        case svn_wc_status_external:
            LoadStringEx(hInst, IDS_STATUSEXTERNAL, entry.buffer, MAX_STATUS_LENGTH, lang);
            break;
        case svn_wc_status_incomplete:
            LoadStringEx(hInst, IDS_STATUSINCOMPLETE, entry.buffer, MAX_STATUS_LENGTH, lang);
            break;
        default:
            LoadStringEx(hInst, IDS_STATUSNONE, entry.buffer, MAX_STATUS_LENGTH, lang);
            break;
    }

    wcsncpy_s(string, size, entry.buffer, min(size, static_cast<int>(MAX_STATUS_LENGTH)) - 1);
}

#ifdef _MFC_VER
const CString& SVNStatus::GetDepthString(svn_depth_t depth)
{
    static const CString sUnknown(MAKEINTRESOURCE(IDS_SVN_DEPTH_UNKNOWN));
    static const CString sExclude(MAKEINTRESOURCE(IDS_SVN_DEPTH_EXCLUDE));
    static const CString sEmpty(MAKEINTRESOURCE(IDS_SVN_DEPTH_EMPTY));
    static const CString sFiles(MAKEINTRESOURCE(IDS_SVN_DEPTH_FILES));
    static const CString sImmediate(MAKEINTRESOURCE(IDS_SVN_DEPTH_IMMEDIATE));
    static const CString sInfinite(MAKEINTRESOURCE(IDS_SVN_DEPTH_INFINITE));
    static const CString sDefault;

    switch (depth)
    {
        case svn_depth_unknown:
            return sUnknown;
        case svn_depth_exclude:
            return sExclude;
        case svn_depth_empty:
            return sEmpty;
        case svn_depth_files:
            return sFiles;
        case svn_depth_immediates:
            return sImmediate;
        case svn_depth_infinity:
            return sInfinite;
    }

    return sDefault;
}
#endif

void SVNStatus::GetDepthString(HINSTANCE hInst, svn_depth_t depth, wchar_t* string, int size, WORD lang)
{
    switch (depth)
    {
        case svn_depth_unknown:
            LoadStringEx(hInst, IDS_SVN_DEPTH_UNKNOWN, string, size, lang);
            break;
        case svn_depth_exclude:
            LoadStringEx(hInst, IDS_SVN_DEPTH_EXCLUDE, string, size, lang);
            break;
        case svn_depth_empty:
            LoadStringEx(hInst, IDS_SVN_DEPTH_EMPTY, string, size, lang);
            break;
        case svn_depth_files:
            LoadStringEx(hInst, IDS_SVN_DEPTH_FILES, string, size, lang);
            break;
        case svn_depth_immediates:
            LoadStringEx(hInst, IDS_SVN_DEPTH_IMMEDIATE, string, size, lang);
            break;
        case svn_depth_infinity:
            LoadStringEx(hInst, IDS_SVN_DEPTH_INFINITE, string, size, lang);
            break;
    }
}

int SVNStatus::LoadStringEx(HINSTANCE hInstance, UINT uID, LPWSTR lpBuffer, int nBufferMax, WORD wLanguage)
{
    const STRINGRESOURCEIMAGE* pImage        = nullptr;
    const STRINGRESOURCEIMAGE* pImageEnd     = nullptr;
    ULONG                      nResourceSize = 0;
    HGLOBAL                    hGlobal       = nullptr;
    UINT                       iIndex        = 0;
    int                        ret           = 0;

    HRSRC hResource = FindResourceEx(hInstance, RT_STRING, MAKEINTRESOURCE(((uID >> 4) + 1)), wLanguage);
    if (!hResource)
    {
        // try the default language before giving up!
        hResource = FindResource(hInstance, MAKEINTRESOURCE(((uID >> 4) + 1)), RT_STRING);
        if (!hResource)
            return 0;
    }
    hGlobal = LoadResource(hInstance, hResource);
    if (!hGlobal)
        return 0;
    pImage = static_cast<const STRINGRESOURCEIMAGE*>(::LockResource(hGlobal));
    if (!pImage)
        return 0;

    nResourceSize = ::SizeofResource(hInstance, hResource);
    pImageEnd     = reinterpret_cast<const STRINGRESOURCEIMAGE*>((reinterpret_cast<LPBYTE>(const_cast<STRINGRESOURCEIMAGE*>(pImage)) + nResourceSize));
    iIndex        = uID & 0x000f;

    while ((iIndex > 0) && (pImage < pImageEnd))
    {
        pImage = reinterpret_cast<const STRINGRESOURCEIMAGE*>((reinterpret_cast<LPBYTE>(const_cast<STRINGRESOURCEIMAGE*>(pImage)) + (sizeof(STRINGRESOURCEIMAGE) + (pImage->nLength * sizeof(WCHAR)))));
        iIndex--;
    }
    if (pImage >= pImageEnd)
        return 0;
    if (pImage->nLength == 0)
        return 0;

    ret = pImage->nLength;
    if (pImage->nLength >= nBufferMax)
    {
        wcsncpy_s(lpBuffer, nBufferMax, pImage->achString, pImage->nLength);
        lpBuffer[nBufferMax - 1] = 0;
    }
    else
    {
        wcsncpy_s(lpBuffer, nBufferMax, pImage->achString, pImage->nLength);
        lpBuffer[ret] = 0;
    }
    return ret;
}

svn_error_t* SVNStatus::getallstatus(void* baton, const char* /*path*/, const svn_client_status_t* status, apr_pool_t* /*pool*/)
{
    svn_wc_status_kind* s = static_cast<svn_wc_status_kind*>(baton);
    *s                    = SVNStatus::GetMoreImportant(*s, status->node_status);
    return nullptr;
}

svn_error_t* SVNStatus::getstatushash(void* baton, const char* path, const svn_client_status_t* status, apr_pool_t* /*pool*/)
{
    HashbatonT* hash = static_cast<HashbatonT*>(baton);
    if (status->node_status == svn_wc_status_external)
    {
        apr_hash_set(hash->extHash, apr_pstrdup(hash->pThis->m_pool, path), APR_HASH_KEY_STRING, reinterpret_cast<const void*>(1));
        return nullptr;
    }
    if (status->file_external)
    {
        apr_hash_set(hash->extHash, apr_pstrdup(hash->pThis->m_pool, path), APR_HASH_KEY_STRING, reinterpret_cast<const void*>(1));
    }
    svn_client_status_t* statusCopy = svn_client_status_dup(status, hash->pThis->m_pool);
    apr_hash_set(hash->hash, apr_pstrdup(hash->pThis->m_pool, path), APR_HASH_KEY_STRING, statusCopy);
    return nullptr;
}

void SVNStatus::notify(void* baton, const svn_wc_notify_t* notify, apr_pool_t* /*pool*/)
{
    HashbatonT* hash = static_cast<HashbatonT*>(baton);

    if (notify->action == svn_wc_notify_status_external)
    {
        apr_hash_set(hash->extHash, apr_pstrdup(hash->pThis->m_pool, notify->path), APR_HASH_KEY_STRING, reinterpret_cast<const void*>(1));
    }
}

apr_array_header_t* SVNStatus::sort_hash(apr_hash_t* ht,
                                         int (*comparisonFunc)(const SVNStatus::SortItem*, const SVNStatus::SortItem*),
                                         apr_pool_t* pool)
{
    apr_hash_index_t*   hi  = nullptr;
    apr_array_header_t* ary = nullptr;

    /* allocate an array with only one element to begin with. */
    ary = apr_array_make(pool, 1, sizeof(SortItem));

    /* loop over hash table and push all keys into the array */
    for (hi = apr_hash_first(pool, ht); hi; hi = apr_hash_next(hi))
    {
        SortItem* item = static_cast<SortItem*>(apr_array_push(ary));

        apr_hash_this(hi, &item->key, &item->kLen, &item->value);
    }

    /* now quick sort the array.  */
    qsort(ary->elts, ary->nelts, ary->elt_size,
          reinterpret_cast<int (*)(const void*, const void*)>(comparisonFunc));

    return ary;
}

int SVNStatus::sort_compare_items_as_paths(const SortItem* a, const SortItem* b)
{
    const char* astr = static_cast<const char*>(a->key);
    const char* bstr = static_cast<const char*>(b->key);
    return svn_path_compare_paths(astr, bstr);
}

svn_error_t* SVNStatus::cancel(void* baton)
{
    volatile bool* canceled = static_cast<bool*>(baton);
    if (*canceled)
    {
        CString temp;
        temp.LoadString(IDS_SVN_USERCANCELLED);
        return svn_error_create(SVN_ERR_CANCELLED, nullptr, CUnicodeUtils::GetUTF8(temp));
    }
    return nullptr;
}

#ifdef _MFC_VER

//// Set-up a filter to restrict the files which will have their status stored by a get-status
//void SVNStatus::SetFilter(const CTSVNPathList& fileList)
//{
//    m_filterFileList.clear();
//    for(int fileIndex = 0; fileIndex < fileList.GetCount(); fileIndex++)
//    {
//        m_filterFileList.push_back(fileList[fileIndex].GetSVNApiPath(m_pool));
//    }
//    // Sort the list so that we can do binary searches
//    std::sort(m_filterFileList.begin(), m_filterFileList.end());
//}
//
//void SVNStatus::ClearFilter()
//{
//    m_filterFileList.clear();
//}

#endif // _MFC_VER
