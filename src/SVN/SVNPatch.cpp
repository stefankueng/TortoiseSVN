﻿// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2010-2015, 2017, 2019, 2021-2022 - TortoiseSVN

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
#include "resource.h"
#include "SVNPatch.h"
#include "UnicodeUtils.h"
#include "ProgressDlg.h"
#include "DirFileEnum.h"
#include "SVNAdminDir.h"
#include "SVNConfig.h"
#include "StringUtils.h"

#pragma warning(push)
#include "svn_dso.h"
#include "svn_utf.h"
#include "svn_dirent_uri.h"
#pragma warning(pop)

#define STRIP_LIMIT 10

int SVNPatch::abort_on_pool_failure(int /*retcode*/)
{
    abort();
    //return -1;
}

SVNPatch::SVNPatch()
    : m_pool(nullptr)
    , m_nStrip(0)
    , m_bSuccessfullyPatched(false)
    , m_nRejected(0)
    , m_pProgDlg(nullptr)
{
    apr_initialize();
    svn_dso_initialize2();
    g_SVNAdminDir.Init();
    apr_pool_create_ex(&m_pool, nullptr, abort_on_pool_failure, nullptr);
}

SVNPatch::~SVNPatch()
{
    apr_pool_destroy(m_pool);
    g_SVNAdminDir.Close();
    apr_terminate();
}

void SVNPatch::notify(void* baton, const svn_wc_notify_t* notify, apr_pool_t* /*pool*/)
{
    SVNPatch* pThis = static_cast<SVNPatch*>(baton);
    if (pThis && notify)
    {
        PathRejects* pInfo = nullptr;
        if (!pThis->m_filePaths.empty())
            pInfo = &pThis->m_filePaths[pThis->m_filePaths.size() - 1];
        if ((notify->action == svn_wc_notify_skip) || (notify->action == svn_wc_notify_patch_rejected_hunk))
        {
            if (pInfo)
            {
                pInfo->rejects++;
                pThis->m_nRejected++;
            }
        }
        if (notify->action != svn_wc_notify_add)
        {
            CString abspath = CUnicodeUtils::GetUnicode(notify->path);
            if (abspath.Left(pThis->m_targetPath.GetLength()).Compare(pThis->m_targetPath) == 0)
                pThis->m_testPath = abspath.Mid(pThis->m_targetPath.GetLength());
            else
                pThis->m_testPath = abspath;
        }
        if (pInfo)
        {
            pInfo->content = pInfo->content || (notify->content_state != svn_wc_notify_state_unknown);
            pInfo->props   = pInfo->props || (notify->prop_state != svn_wc_notify_state_unknown);
        }

        if (((notify->action == svn_wc_notify_patch) || (notify->action == svn_wc_notify_add)) && (pThis->m_pProgDlg))
        {
            pThis->m_pProgDlg->FormatPathLine(2, IDS_PATCH_PATHINGFILE, static_cast<LPCWSTR>(CUnicodeUtils::GetUnicode(notify->path)));
        }
    }
}

svn_error_t* SVNPatch::patch_func(void* baton, svn_boolean_t* filtered, const char* canonPathFromPatchfile,
                                  const char* patchAbspath,
                                  const char* rejectAbspath,
                                  apr_pool_t* /*scratch_pool*/)
{
    SVNPatch* pThis = static_cast<SVNPatch*>(baton);
    if (pThis)
    {
        CString     absPath = CUnicodeUtils::GetUnicode(canonPathFromPatchfile);
        PathRejects pr;
        pr.path       = PathIsRelative(absPath) ? absPath : pThis->Strip(absPath);
        pr.rejects    = 0;
        pr.resultPath = CUnicodeUtils::GetUnicode(patchAbspath);
        pr.resultPath.Replace('/', '\\');
        pr.rejectsPath = CUnicodeUtils::GetUnicode(rejectAbspath);
        pr.rejectsPath.Replace('/', '\\');
        pr.content = false;
        pr.props   = false;
        // only add this entry if it hasn't been added already
        bool bExists = false;
        for (auto it = pThis->m_filePaths.rbegin(); it != pThis->m_filePaths.rend(); ++it)
        {
            if (it->path.Compare(pr.path) == 0)
            {
                bExists = true;
                break;
            }
        }
        if (!bExists)
            pThis->m_filePaths.push_back(pr);
        pThis->m_nRejected = 0;
        *filtered          = false;

        CTempFiles::Instance().AddFileToRemove(pr.resultPath);
        CTempFiles::Instance().AddFileToRemove(pr.rejectsPath);
    }
    return nullptr;
}

svn_error_t* SVNPatch::patchfile_func(void* baton, svn_boolean_t* filtered, const char* canonPathFromPatchfile,
                                      const char* /*patch_abspath*/,
                                      const char* /*reject_abspath*/,
                                      apr_pool_t* /*scratch_pool*/)
{
    SVNPatch* pThis = static_cast<SVNPatch*>(baton);
    if (pThis)
    {
        CString relpath = CUnicodeUtils::GetUnicode(canonPathFromPatchfile);
        relpath         = PathIsRelative(relpath) ? relpath : relpath.Mid(pThis->m_targetPath.GetLength());
        *filtered       = relpath.CompareNoCase(pThis->m_fileToPatch) != 0;
    }
    return nullptr;
}

int SVNPatch::Init(const CString& patchFile, const CString& targetPath, CProgressDlg* pPprogDlg)
{
    CTSVNPath target = CTSVNPath(targetPath);
    if (patchFile.IsEmpty() || targetPath.IsEmpty() || !svn_dirent_is_absolute(target.GetSVNApiPath(m_pool)))
    {
        m_errorStr.LoadString(IDS_ERR_PATCHPATHS);
        return -1;
    }
    svn_error_t*      err         = nullptr;
    apr_pool_t*       scratchpool = nullptr;
    svn_client_ctx_t* ctx         = nullptr;

    m_errorStr.Empty();
    m_patchFile  = patchFile;
    m_targetPath = targetPath;
    m_testPath.Empty();

    m_patchFile.Replace('\\', '/');
    m_targetPath.Replace('\\', '/');

    apr_pool_create_ex(&scratchpool, m_pool, abort_on_pool_failure, nullptr);
    svn_error_clear(svn_client_create_context2(&ctx, SVNConfig::Instance().GetConfig(m_pool), scratchpool));
    ctx->notify_func2  = notify;
    ctx->notify_baton2 = this;

    if (pPprogDlg)
    {
        pPprogDlg->SetTitle(IDS_APPNAME);
        pPprogDlg->FormatNonPathLine(1, IDS_PATCH_PROGTITLE);
        pPprogDlg->SetShowProgressBar(false);
        pPprogDlg->ShowModeless(AfxGetMainWnd());
        m_pProgDlg = pPprogDlg;
    }

    m_filePaths.clear();
    m_nRejected              = 0;
    m_nStrip                 = 0;
    CTSVNPath tsvnPatchFile  = CTSVNPath(m_patchFile);
    CTSVNPath tsvnTargetPath = CTSVNPath(m_targetPath);

    err = svn_client_patch(tsvnPatchFile.GetSVNApiPath(scratchpool),  // patch_abspath
                           tsvnTargetPath.GetSVNApiPath(scratchpool), // local_abspath
                           true,                                      // dry_run
                           m_nStrip,                                  // strip_count
                           false,                                     // reverse
                           true,                                      // ignore_whitespace
                           false,                                     // remove_tempfiles
                           patch_func,                                // patch_func
                           this,                                      // patch_baton
                           ctx,                                       // client context
                           scratchpool);
    if (m_filePaths.empty() && (m_nStrip == 0))
    {
        // in case no paths matched, try again with one strip.
        // because if the patch paths are absolute, only stripping
        // the drive letter will get the svn_client_patch to call
        // the patch_func and we then get the data we need to later
        // adjust the strip value correctly.
        m_nStrip++;
        err = svn_client_patch(tsvnPatchFile.GetSVNApiPath(scratchpool),  // patch_abspath
                               tsvnTargetPath.GetSVNApiPath(scratchpool), // local_abspath
                               true,                                      // dry_run
                               m_nStrip,                                  // strip_count
                               false,                                     // reverse
                               true,                                      // ignore_whitespace
                               false,                                     // remove_tempfiles
                               patch_func,                                // patch_func
                               this,                                      // patch_baton
                               ctx,                                       // client context
                               scratchpool);
    }

    m_pProgDlg = nullptr;
    apr_pool_destroy(scratchpool);

    // since we're doing a dry-run, a 'path not found' can happen
    // since new files/dirs aren't added in the patch func.
    if ((err) && (err->apr_err != SVN_ERR_WC_PATH_NOT_FOUND))
    {
        m_errorStr = GetErrorMessage(err);
        m_filePaths.clear();
        svn_error_clear(err);
        return -1;
    }

    if ((m_nRejected > (static_cast<int>(m_filePaths.size()) / 3)) && !m_testPath.IsEmpty())
    {
        auto startStrip = m_nStrip;
        for (m_nStrip = 0; m_nStrip < STRIP_LIMIT; ++m_nStrip)
        {
            int nExisting = 0;
            for (std::vector<PathRejects>::iterator it = m_filePaths.begin(); it != m_filePaths.end(); ++it)
            {
                CString p = Strip(it->path);
                if (p.IsEmpty())
                {
                    m_nStrip = STRIP_LIMIT;
                    break;
                }
                else if (PathFileExists(m_targetPath + L"\\" + p))
                    ++nExisting;
            }
            if (nExisting > static_cast<int>(m_filePaths.size() - m_nRejected))
            {
                m_nStrip += startStrip;
                break;
            }
        }
    }

    if (m_nStrip >= STRIP_LIMIT)
    {
        m_nStrip = 0;
    }
    else if (m_nStrip > 0)
    {
        apr_pool_create_ex(&scratchpool, m_pool, abort_on_pool_failure, nullptr);
        svn_error_clear(svn_client_create_context2(&ctx, SVNConfig::Instance().GetConfig(m_pool), scratchpool));
        ctx->notify_func2  = notify;
        ctx->notify_baton2 = this;

        m_filePaths.clear();
        m_nRejected = 0;
        err         = svn_client_patch(tsvnPatchFile.GetSVNApiPath(scratchpool),  // patch_abspath
                               tsvnTargetPath.GetSVNApiPath(scratchpool), // local_abspath
                               true,                                      // dry_run
                               m_nStrip,                                  // strip_count
                               false,                                     // reverse
                               true,                                      // ignore_whitespace
                               false,                                     // remove_tempfiles
                               patch_func,                                // patch_func
                               this,                                      // patch_baton
                               ctx,                                       // client context
                               scratchpool);

        apr_pool_destroy(scratchpool);

        if (err)
        {
            m_errorStr = GetErrorMessage(err);
            m_filePaths.clear();
            svn_error_clear(err);
            return -1;
        }
    }

    return static_cast<int>(m_filePaths.size());
}

bool SVNPatch::PatchPath(const CString& path)
{
    svn_error_t*      err         = nullptr;
    apr_pool_t*       scratchPool = nullptr;
    svn_client_ctx_t* ctx         = nullptr;

    m_errorStr.Empty();

    m_patchFile.Replace('\\', '/');
    m_targetPath.Replace('\\', '/');

    m_fileToPatch = path.Mid(m_targetPath.GetLength() + 1);
    m_fileToPatch.Replace('\\', '/');

    apr_pool_create_ex(&scratchPool, m_pool, abort_on_pool_failure, nullptr);
    svn_error_clear(svn_client_create_context2(&ctx, SVNConfig::Instance().GetConfig(m_pool), scratchPool));

    const char* patchAbspath = nullptr;
    err                      = svn_dirent_canonicalize_safe(&patchAbspath, nullptr,
                                    CUnicodeUtils::GetUTF8(m_patchFile),
                                    scratchPool, scratchPool);
    if (err)
    {
        m_errorStr = GetErrorMessage(err);
        svn_error_clear(err);
        apr_pool_destroy(scratchPool);
        return false;
    }

    const char* localAbspath = nullptr;
    err                      = svn_dirent_canonicalize_safe(&localAbspath, nullptr,
                                    CUnicodeUtils::GetUTF8(m_targetPath),
                                    scratchPool, scratchPool);
    if (err)
    {
        m_errorStr = GetErrorMessage(err);
        svn_error_clear(err);
        apr_pool_destroy(scratchPool);
        return false;
    }

    m_nRejected = 0;
    err         = svn_client_patch(patchAbspath,   // patch_abspath
                           localAbspath,   // local_abspath
                           false,          // dry_run
                           m_nStrip,       // strip_count
                           false,          // reverse
                           true,           // ignore_whitespace
                           true,           // remove_tempfiles
                           patchfile_func, // patch_func
                           this,           // patch_baton
                           ctx,            // client context
                           scratchPool);

    if (err)
    {
        m_errorStr = GetErrorMessage(err);
        svn_error_clear(err);
        apr_pool_destroy(scratchPool);
        return false;
    }
    apr_pool_destroy(scratchPool);

    return true;
}

int SVNPatch::GetPatchResult(const CString& sPath, CString& sSavePath, CString& sRejectPath) const
{
    for (std::vector<PathRejects>::const_iterator it = m_filePaths.begin(); it != m_filePaths.end(); ++it)
    {
        if (it->path.CompareNoCase(sPath) == 0)
        {
            sSavePath = it->resultPath;
            if (it->rejects > 0)
                sRejectPath = it->rejectsPath;
            else
                sRejectPath.Empty();
            return it->rejects;
        }
    }
    return -1;
}

CString SVNPatch::CheckPatchPath(const CString& path) const
{
    // first check if the path already matches
    int origMatches = CountMatches(path);
    if (origMatches > (GetNumberOfFiles() / 3))
        return path;

    CProgressDlg progress;
    CString      tmp;
    progress.SetTitle(IDS_PATCH_SEARCHPATHTITLE);
    progress.SetShowProgressBar(false);
    tmp.LoadString(IDS_PATCH_SEARCHPATHLINE1);
    progress.SetLine(1, tmp);
    progress.ShowModeless(AfxGetMainWnd());

    // now go up the tree and try again
    CString upperPath = path;
    while (upperPath.ReverseFind('\\') > 0)
    {
        upperPath = upperPath.Left(upperPath.ReverseFind('\\'));
        progress.SetLine(2, upperPath, true);
        if (progress.HasUserCancelled())
            return path;
        if (CountMatches(upperPath) > origMatches)
            return upperPath;
    }
    // still no match found. So try sub folders
    bool         isDir = false;
    CString      subPath;
    CDirFileEnum fileFinder(path);
    while (fileFinder.NextFile(subPath, &isDir))
    {
        if (progress.HasUserCancelled())
            return path;
        if (!isDir)
            continue;
        if (g_SVNAdminDir.IsAdminDirPath(subPath))
            continue;
        progress.SetLine(2, subPath, true);
        if (CountMatches(subPath) > origMatches)
            return subPath;
    }

    // if a patch file only contains newly added files
    // we can't really find the correct path.
    // But: we can compare paths strings without the filenames
    // and check if at least those match
    upperPath = path;
    while (upperPath.ReverseFind('\\') > 0)
    {
        upperPath = upperPath.Left(upperPath.ReverseFind('\\'));
        progress.SetLine(2, upperPath, true);
        if (progress.HasUserCancelled())
            return path;
        if (CountDirMatches(upperPath) > origMatches)
            return upperPath;
    }

    return path;
}

int SVNPatch::CountMatches(const CString& path) const
{
    int matches = 0;
    for (int i = 0; i < GetNumberOfFiles(); ++i)
    {
        CString temp = GetFilePath(i);
        temp.Replace('/', '\\');
        if ((PathIsRelative(temp)) ||
            ((temp.GetLength() > 1) && (temp[0] == '\\') && (temp[1] != '\\')))
            temp = path + L"\\" + temp;
        if (PathFileExists(temp))
            matches++;
    }
    return matches;
}

int SVNPatch::CountDirMatches(const CString& path) const
{
    int matches = 0;
    for (int i = 0; i < GetNumberOfFiles(); ++i)
    {
        CString temp = GetFilePath(i);
        temp.Replace('/', '\\');
        if (PathIsRelative(temp))
            temp = path + L"\\" + temp;
        // remove the filename
        temp = temp.Left(temp.ReverseFind('\\'));
        if (PathFileExists(temp))
            matches++;
    }
    return matches;
}

CString SVNPatch::GetStrippedPath(int nIndex) const
{
    if (nIndex < 0)
        return L"";
    if (nIndex < static_cast<int>(m_filePaths.size()))
    {
        CString filepath = Strip(GetFilePath(nIndex));
        return filepath;
    }

    return L"";
}

CString SVNPatch::Strip(const CString& filename) const
{
    CString s = filename;
    if (m_nStrip > 0)
    {
        // Remove windows drive letter "c:"
        if (s.GetLength() > 2 && s[1] == ':')
        {
            s = s.Mid(2);
        }

        for (int nStrip = 1; nStrip <= m_nStrip; nStrip++)
        {
            // "/home/ts/my-working-copy/dir/file.txt"
            //  "home/ts/my-working-copy/dir/file.txt"
            //       "ts/my-working-copy/dir/file.txt"
            //          "my-working-copy/dir/file.txt"
            //                          "dir/file.txt"
            int p = s.FindOneOf(L"/\\");
            if (p < 0)
            {
                s.Empty();
                break;
            }
            s = s.Mid(p + 1);
        }
    }
    return s;
}

CString SVNPatch::GetErrorMessage(svn_error_t* err)
{
    CString msg;

    if (err != nullptr)
    {
        svn_error_t* errPtr = err;
        msg                 = GetErrorMessageForNode(errPtr);
        while (errPtr->child)
        {
            errPtr = errPtr->child;
            msg += L"\n";
            msg += GetErrorMessageForNode(errPtr);
        }
    }
    return msg;
}

CString SVNPatch::GetErrorMessageForNode(svn_error_t* err)
{
    CString msg;
    if (err != nullptr)
    {
        svn_error_t* errPtr = err;
        if (errPtr->message)
            msg = CUnicodeUtils::GetUnicode(errPtr->message);
        else
        {
            char errbuf[256] = {0};
            /* Is this a Subversion-specific error code? */
            if ((errPtr->apr_err > APR_OS_START_USEERR) && (errPtr->apr_err <= APR_OS_START_CANONERR))
            {
                msg = svn_strerror(errPtr->apr_err, errbuf, _countof(errbuf));
            }
            /* Otherwise, this must be an APR error code. */
            else
            {
                svn_error_t* tempErr   = nullptr;
                const char*  errString = nullptr;
                tempErr                = svn_utf_cstring_to_utf8(&errString, apr_strerror(errPtr->apr_err, errbuf, _countof(errbuf) - 1), errPtr->pool);
                if (tempErr)
                {
                    svn_error_clear(tempErr);
                    msg = L"Can't recode error string from APR";
                }
                else
                {
                    msg = CUnicodeUtils::GetUnicode(errString);
                }
            }
        }
        msg = CStringUtils::LinesWrap(msg, 80);
    }
    return msg;
}

bool SVNPatch::RemoveFile(const CString& path) const
{
    svn_error_t*      err         = nullptr;
    apr_pool_t*       scratchPool = nullptr;
    svn_client_ctx_t* ctx         = nullptr;

    apr_pool_create_ex(&scratchPool, m_pool, abort_on_pool_failure, nullptr);
    svn_error_clear(svn_client_create_context2(&ctx, SVNConfig::Instance().GetConfig(m_pool), scratchPool));

    apr_array_header_t* targets = apr_array_make(scratchPool, 1, sizeof(const char*));

    const char* canonicalizedPath = nullptr;
    svn_error_clear(svn_dirent_canonicalize_safe(&canonicalizedPath, nullptr, CUnicodeUtils::GetUTF8(path), scratchPool, scratchPool));
    if (canonicalizedPath == nullptr)
    {
        apr_pool_destroy(scratchPool);
        return false;
    }

    (*static_cast<const char**>(apr_array_push(targets))) = canonicalizedPath;

    err = svn_client_delete4(targets, true, false, nullptr, nullptr, nullptr, ctx, scratchPool);

    if (err)
    {
        svn_error_clear(err);
        apr_pool_destroy(scratchPool);
        return false;
    }
    apr_pool_destroy(scratchPool);
    return true;
}
