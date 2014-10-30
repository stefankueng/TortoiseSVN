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

#include "stdafx.h"
#pragma warning(push)
#include "SVNDiff.h"
#include "svn_types.h"
#pragma warning(pop)

#include "resource.h"
#include "../TortoiseShell/resource.h"
#include "AppUtils.h"
#include "TempFile.h"
#include "SVNStatus.h"
#include "SVNInfo.h"
#include "SVNProperties.h"
#include "UnicodeUtils.h"
#include "registry.h"
#include "FileDiffDlg.h"
#include "ProgressDlg.h"
#include "Blame.h"

SVNDiff::SVNDiff(SVN * pSVN /* = NULL */, HWND hWnd /* = NULL */, bool bRemoveTempFiles /* = false */)
    : m_bDeleteSVN(false)
    , m_pSVN(NULL)
    , m_hWnd(NULL)
    , m_bRemoveTempFiles(false)
    , m_headPeg(SVNRev::REV_HEAD)
    , m_bAlternativeTool(false)
    , m_JumpLine(0)
{
    if (pSVN)
        m_pSVN = pSVN;
    else
    {
        m_pSVN = new SVN;
        m_bDeleteSVN = true;
    }
    m_hWnd = hWnd;
    m_bRemoveTempFiles = bRemoveTempFiles;
}

SVNDiff::~SVNDiff(void)
{
    if (m_bDeleteSVN)
        delete m_pSVN;
}

bool SVNDiff::DiffWCFile(const CTSVNPath& filePath,
                         bool ignoreprops,
                         svn_wc_status_kind status, /* = svn_wc_status_none */
                         svn_wc_status_kind text_status /* = svn_wc_status_none */,
                         svn_wc_status_kind prop_status /* = svn_wc_status_none */,
                         svn_wc_status_kind remotetext_status /* = svn_wc_status_none */,
                         svn_wc_status_kind remoteprop_status /* = svn_wc_status_none */)
{
    CTSVNPath basePath;
    CTSVNPath remotePath;
    SVNRev remoteRev;
    svn_revnum_t baseRev = 0;

    // first diff the remote properties against the wc props
    // TODO: should we attempt to do a three way diff with the properties too
    // if they're modified locally and remotely?
    if (!ignoreprops && (remoteprop_status > svn_wc_status_normal))
    {
        DiffProps(filePath, SVNRev::REV_HEAD, SVNRev::REV_WC, baseRev);
    }
    if (!ignoreprops && (prop_status > svn_wc_status_normal)&&(filePath.IsDirectory()))
    {
        DiffProps(filePath, SVNRev::REV_WC, SVNRev::REV_BASE, baseRev);
    }
    if (filePath.IsDirectory())
        return true;

    if ((status > svn_wc_status_normal) || (text_status > svn_wc_status_normal))
    {
        basePath = SVN::GetPristinePath(filePath);
        if (baseRev == 0)
        {
            SVNStatus stat;
            CTSVNPath dummy;
            svn_client_status_t * s = stat.GetFirstFileStatus(filePath, dummy);
            if (s)
                baseRev = s->revision >= 0 ? s->revision : s->changed_rev;
        }
        // If necessary, convert the line-endings on the file before diffing
        if ((DWORD)CRegDWORD(L"Software\\TortoiseSVN\\ConvertBase", TRUE))
        {
            CTSVNPath temporaryFile = CTempFiles::Instance().GetTempFilePath(m_bRemoveTempFiles, filePath, SVNRev::REV_BASE);
            if (!m_pSVN->Export(filePath, temporaryFile, SVNRev(SVNRev::REV_BASE), SVNRev(SVNRev::REV_BASE)))
            {
                temporaryFile.Reset();
            }
            else
            {
                basePath = temporaryFile;
                SetFileAttributes(basePath.GetWinPath(), FILE_ATTRIBUTE_READONLY);
            }
        }
    }

    if (remotetext_status > svn_wc_status_normal)
    {
        remotePath = CTempFiles::Instance().GetTempFilePath(false, filePath, SVNRev::REV_HEAD);

        CProgressDlg progDlg;
        progDlg.SetTitle(IDS_APPNAME);
        progDlg.SetTime(false);
        m_pSVN->SetAndClearProgressInfo(&progDlg, true);    // activate progress bar
        progDlg.ShowModeless(GetHWND());
        progDlg.FormatPathLine(1, IDS_PROGRESSGETFILE, (LPCTSTR)filePath.GetUIFileOrDirectoryName());
        remoteRev = SVNRev::REV_HEAD;
        if (!m_pSVN->Export(filePath, remotePath, remoteRev, remoteRev))
        {
            progDlg.Stop();
            m_pSVN->SetAndClearProgressInfo((HWND)NULL);
            m_pSVN->ShowErrorDialog(GetHWND());
            return false;
        }
        progDlg.Stop();
        m_pSVN->SetAndClearProgressInfo((HWND)NULL);
        SetFileAttributes(remotePath.GetWinPath(), FILE_ATTRIBUTE_READONLY);
    }

    CString name = filePath.GetUIFileOrDirectoryName();
    CString n1, n2, n3;
    n1.Format(IDS_DIFF_WCNAME, (LPCTSTR)name);
    if (baseRev)
        n2.FormatMessage(IDS_DIFF_BASENAMEREV, (LPCTSTR)name, baseRev);
    else
        n2.Format(IDS_DIFF_BASENAME, (LPCTSTR)name);
    n3.Format(IDS_DIFF_REMOTENAME, (LPCTSTR)name);

    if ((text_status <= svn_wc_status_normal)&&(prop_status <= svn_wc_status_normal)&&(status <= svn_wc_status_normal))
    {
        // Hasn't changed locally - diff remote against WC
        return CAppUtils::StartExtDiff(
            filePath, remotePath, n1, n3, filePath, filePath, SVNRev::REV_WC, remoteRev, remoteRev, CAppUtils::DiffFlags().AlternativeTool(m_bAlternativeTool),
            m_JumpLine, filePath.GetFileOrDirectoryName(), L"");
    }
    else if (remotePath.IsEmpty())
    {
        return DiffFileAgainstBase(filePath, baseRev, ignoreprops, status, text_status, prop_status);
    }
    else
    {
        // Three-way diff
        CAppUtils::MergeFlags flags;
        flags.bAlternativeTool = m_bAlternativeTool;
        flags.bReadOnly = true;
        return !!CAppUtils::StartExtMerge(flags,
            basePath, remotePath, filePath, CTSVNPath(), false, n2, n3, n1, CString(), filePath.GetFileOrDirectoryName());
    }
}

bool SVNDiff::DiffFileAgainstBase( const CTSVNPath& filePath, svn_revnum_t & baseRev, bool ignoreprops, svn_wc_status_kind status /*= svn_wc_status_none*/, svn_wc_status_kind text_status /*= svn_wc_status_none*/, svn_wc_status_kind prop_status /*= svn_wc_status_none*/ )
{
    bool retvalue = false;
    bool fileexternal = false;
    if ((text_status == svn_wc_status_none)||(prop_status == svn_wc_status_none))
    {
        SVNStatus stat;
        stat.GetStatus(filePath);
        if (stat.status == NULL)
            return false;
        text_status = stat.status->text_status;
        prop_status = stat.status->prop_status;
        fileexternal = stat.status->file_external != 0;
    }
    if (!ignoreprops && (prop_status > svn_wc_status_normal))
    {
        DiffProps(filePath, SVNRev::REV_WC, SVNRev::REV_BASE, baseRev);
    }

    if (filePath.IsDirectory())
        return true;
    if ((status >= svn_wc_status_normal) || (text_status >= svn_wc_status_normal))
    {
        CTSVNPath basePath(SVN::GetPristinePath(filePath));
        if (baseRev == 0)
        {
            SVNInfo info;
            const SVNInfoData * infodata = info.GetFirstFileInfo(filePath, SVNRev(), SVNRev());

            if (infodata)
            {
                if (infodata->copyfromurl && infodata->copyfromurl[0])
                    baseRev = infodata->copyfromrev;
                else
                    baseRev = infodata->lastchangedrev;
            }
        }
        // If necessary, convert the line-endings on the file before diffing
        // note: file externals can not be exported
        if (((DWORD)CRegDWORD(L"Software\\TortoiseSVN\\ConvertBase", TRUE)) && (!fileexternal))
        {
            CTSVNPath temporaryFile = CTempFiles::Instance().GetTempFilePath(m_bRemoveTempFiles, filePath, SVNRev::REV_BASE);
            if (!m_pSVN->Export(filePath, temporaryFile, SVNRev(SVNRev::REV_BASE), SVNRev(SVNRev::REV_BASE)))
            {
                temporaryFile.Reset();
            }
            else
            {
                basePath = temporaryFile;
                SetFileAttributes(basePath.GetWinPath(), FILE_ATTRIBUTE_READONLY);
            }
        }
        // for added/deleted files, we don't have a BASE file.
        // create an empty temp file to be used.
        if (!basePath.Exists())
        {
            basePath = CTempFiles::Instance().GetTempFilePath(m_bRemoveTempFiles, filePath, SVNRev::REV_BASE);
            SetFileAttributes(basePath.GetWinPath(), FILE_ATTRIBUTE_READONLY);
        }
        CString name = filePath.GetFilename();
        CTSVNPath wcFilePath = filePath;
        if (!wcFilePath.Exists())
        {
            wcFilePath = CTempFiles::Instance().GetTempFilePath(m_bRemoveTempFiles, filePath, SVNRev::REV_BASE);
            SetFileAttributes(wcFilePath.GetWinPath(), FILE_ATTRIBUTE_READONLY);
        }
        CString n1, n2;
        n1.Format(IDS_DIFF_WCNAME, (LPCTSTR)name);
        if (baseRev)
            n2.FormatMessage(IDS_DIFF_BASENAMEREV, (LPCTSTR)name, baseRev);
        else
            n2.Format(IDS_DIFF_BASENAME, (LPCTSTR)name);
        retvalue = CAppUtils::StartExtDiff(
            basePath, wcFilePath, n2, n1,
            filePath, filePath, SVNRev::REV_BASE, SVNRev::REV_WC, SVNRev::REV_BASE,
            CAppUtils::DiffFlags().Wait().AlternativeTool(m_bAlternativeTool), m_JumpLine, name, L"");
    }
    return retvalue;
}

bool SVNDiff::UnifiedDiff(CTSVNPath& tempfile, const CTSVNPath& url1, const SVNRev& rev1, const CTSVNPath& url2, const SVNRev& rev2, const SVNRev& peg, const CString& options, bool bIgnoreAncestry /* = false */, bool bIgnoreProperties /* = true */)
{
    tempfile = CTempFiles::Instance().GetTempFilePath(m_bRemoveTempFiles, CTSVNPath(L"Test.diff"));
    bool bIsUrl = !!SVN::PathIsURL(url1);

    CProgressDlg progDlg;
    progDlg.SetTitle(IDS_APPNAME);
    progDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROGRESS_UNIFIEDDIFF)));
    progDlg.SetTime(false);
    m_pSVN->SetAndClearProgressInfo(&progDlg);
    progDlg.ShowModeless(GetHWND());
    // find the root of the files
    CTSVNPathList plist;
    plist.AddPath(url1);
    plist.AddPath(url2);
    CTSVNPath relativeTo = plist.GetCommonRoot();
    if (!relativeTo.IsUrl())
    {
        if (!relativeTo.IsDirectory())
            relativeTo = relativeTo.GetContainingDirectory();
    }
    if (relativeTo.IsEmpty() && url1.Exists() && url2.IsUrl())
    {
        // the source path exists, i.e. it's a local path, so
        // use this as the relative url
        relativeTo = url1.GetDirectory();
    }
    // the 'relativeTo' path must be a path: svn throws an error if it's used for urls.
    else if ((!url2.IsEquivalentTo(url1) && (relativeTo.IsEquivalentTo(url1) || relativeTo.IsEquivalentTo(url2))) || url1.IsUrl() || url2.IsUrl())
        relativeTo.Reset();
    if ((!url1.IsEquivalentTo(url2))||((rev1.IsWorking() || rev1.IsBase())&&(rev2.IsWorking() || rev2.IsBase())))
    {
        if (!m_pSVN->Diff(url1, rev1, url2, rev2, relativeTo, svn_depth_infinity, true, false, false, false, false, false, bIgnoreProperties, false, options, bIgnoreAncestry, tempfile))
        {
            progDlg.Stop();
            m_pSVN->SetAndClearProgressInfo((HWND)NULL);
            m_pSVN->ShowErrorDialog(GetHWND());
            return false;
        }
    }
    else
    {
        if (!m_pSVN->PegDiff(url1, (peg.IsValid() ? peg : (bIsUrl ? m_headPeg : SVNRev::REV_WC)), rev1, rev2, relativeTo, svn_depth_infinity, true, false, false, false, false, false, bIgnoreProperties, false, options, false, tempfile))
        {
            if (!m_pSVN->Diff(url1, rev1, url2, rev2, relativeTo, svn_depth_infinity, true, false, false, false, false, false, bIgnoreProperties, false, options, false, tempfile))
            {
                progDlg.Stop();
                m_pSVN->SetAndClearProgressInfo((HWND)NULL);
                m_pSVN->ShowErrorDialog(GetHWND());
                return false;
            }
        }
    }
    if (CAppUtils::CheckForEmptyDiff(tempfile))
    {
        progDlg.Stop();
        m_pSVN->SetAndClearProgressInfo((HWND)NULL);
        TaskDialog(GetHWND(), AfxGetResourceHandle(), MAKEINTRESOURCE(IDS_APPNAME), MAKEINTRESOURCE(IDS_ERR_ERROROCCURED), MAKEINTRESOURCE(IDS_ERR_EMPTYDIFF), TDCBF_OK_BUTTON, TD_ERROR_ICON, NULL);
        return false;
    }
    progDlg.Stop();
    m_pSVN->SetAndClearProgressInfo((HWND)NULL);
    return true;
}

bool SVNDiff::ShowUnifiedDiff(const CTSVNPath& url1, const SVNRev& rev1,
                              const CTSVNPath& url2, const SVNRev& rev2,
                              SVNRev peg,
                              const CString& options,
                              bool bIgnoreAncestry /* = false */,
                              bool /*blame*/,
                              bool bIgnoreProperties /* = true */)
{
    CTSVNPath tempfile;
    if (UnifiedDiff(tempfile, url1, rev1, url2, rev2, peg, options, bIgnoreAncestry, bIgnoreProperties))
    {
        CString title;
        CTSVNPathList list;
        list.AddPath(url1);
        list.AddPath(url2);
        if (url1.IsEquivalentTo(url2))
            title.FormatMessage(IDS_SVNDIFF_ONEURL, (LPCTSTR)rev1.ToString(), (LPCTSTR)rev2.ToString(), (LPCTSTR)url1.GetUIFileOrDirectoryName());
        else
        {
            CTSVNPath root = list.GetCommonRoot();
            CString u1 = url1.GetUIPathString().Mid(root.GetUIPathString().GetLength());
            CString u2 = url2.GetUIPathString().Mid(root.GetUIPathString().GetLength());
            title.FormatMessage(IDS_SVNDIFF_TWOURLS, (LPCTSTR)rev1.ToString(), (LPCTSTR)u1, (LPCTSTR)rev2.ToString(), (LPCTSTR)u2);
        }
        return !!CAppUtils::StartUnifiedDiffViewer(tempfile.GetWinPathString(), title);
    }
    return false;
}

bool SVNDiff::ShowCompare( const CTSVNPath& url1, const SVNRev& rev1, const CTSVNPath& url2, const SVNRev& rev2, SVNRev peg, bool ignoreprops, const CString& options, bool ignoreancestry /*= false*/, bool blame /*= false*/, svn_node_kind_t nodekind /*= svn_node_unknown*/ )
{
    CTSVNPath tempfile;
    CString mimetype;
    CProgressDlg progDlg;
    progDlg.SetTitle(IDS_APPNAME);
    progDlg.SetTime(false);
    m_pSVN->SetAndClearProgressInfo(&progDlg);
    CAppUtils::DiffFlags diffFlags;
    diffFlags.ReadOnly().AlternativeTool(m_bAlternativeTool);

    if ((m_pSVN->PathIsURL(url1))||(!rev1.IsWorking())||(!url1.IsEquivalentTo(url2)))
    {
        // no working copy path!
        progDlg.ShowModeless(GetHWND());

        tempfile = CTempFiles::Instance().GetTempFilePath(false, url1);
        // first find out if the url points to a file or dir
        CString sRepoRoot;
        if ((nodekind != svn_node_dir)&&(nodekind != svn_node_file))
        {
            progDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROGRESS_INFO)));
            SVNInfo info;
            const SVNInfoData * data = info.GetFirstFileInfo(url1, (peg.IsValid() ? peg : m_headPeg), rev1, svn_depth_empty);
            if (data == NULL)
            {
                data = info.GetFirstFileInfo(url1, (peg.IsValid() ? peg : rev1), rev1, svn_depth_empty);
                if (data == NULL)
                {
                    data = info.GetFirstFileInfo(url1, (peg.IsValid() ? peg : rev2), rev1, svn_depth_empty);
                    if (data == NULL)
                    {
                        progDlg.Stop();
                        m_pSVN->SetAndClearProgressInfo((HWND)NULL);
                        info.ShowErrorDialog(GetHWND());
                        return false;
                    }
                    else
                    {
                        sRepoRoot = data->reposRoot;
                        nodekind = data->kind;
                        peg = peg.IsValid() ? peg : rev2;
                    }
                }
                else
                {
                    sRepoRoot = data->reposRoot;
                    nodekind = data->kind;
                    peg = peg.IsValid() ? peg : rev1;
                }
            }
            else
            {
                sRepoRoot = data->reposRoot;
                nodekind = data->kind;
                peg = peg.IsValid() ? peg : m_headPeg;
            }
        }
        else
        {
            sRepoRoot = m_pSVN->GetRepositoryRoot(url1);
            peg = peg.IsValid() ? peg : m_headPeg;
        }
        if (nodekind == svn_node_dir)
        {
            if (rev1.IsWorking())
            {
                if (UnifiedDiff(tempfile, url1, rev1, url2, rev2, (peg.IsValid() ? peg : SVNRev::REV_WC), options))
                {
                    CString sWC;
                    sWC.LoadString(IDS_DIFF_WORKINGCOPY);
                    progDlg.Stop();
                    m_pSVN->SetAndClearProgressInfo((HWND)NULL);
                    return !!CAppUtils::StartExtPatch(tempfile, url1.GetDirectory(), sWC, url2.GetSVNPathString(), TRUE);
                }
            }
            else
            {
                progDlg.Stop();
                m_pSVN->SetAndClearProgressInfo((HWND)NULL);
                CFileDiffDlg fdlg;
                fdlg.DoBlame(blame);
                if (url1.IsEquivalentTo(url2))
                {
                    fdlg.SetDiff(url1, (peg.IsValid() ? peg : m_headPeg), rev1, rev2, svn_depth_infinity, ignoreancestry);
                    fdlg.DoModal();
                }
                else
                {
                    fdlg.SetDiff(url1, rev1, url2, rev2, svn_depth_infinity, ignoreancestry);
                    fdlg.DoModal();
                }
            }
        }
        else
        {
            if (url1.IsEquivalentTo(url2) && !ignoreprops)
            {
                svn_revnum_t baseRev = 0;
                DiffProps(url1, rev2, rev1, baseRev);
            }
            // diffing two revs of a file, so export two files
            CTSVNPath tempfile1 = CTempFiles::Instance().GetTempFilePath(m_bRemoveTempFiles, blame ? CTSVNPath() : url1, rev1);
            CTSVNPath tempfile2 = CTempFiles::Instance().GetTempFilePath(m_bRemoveTempFiles, blame ? CTSVNPath() : url2, rev2);

            m_pSVN->SetAndClearProgressInfo(&progDlg, true);    // activate progress bar
            progDlg.FormatPathLine(1, IDS_PROGRESSGETFILEREVISION, (LPCTSTR)url1.GetUIFileOrDirectoryName(), (LPCTSTR)rev1.ToString());
            CAppUtils::GetMimeType(url1, mimetype, rev1);
            CBlame blamer;
            blamer.SetAndClearProgressInfo(&progDlg, true);
            if (blame)
            {
                if (!blamer.BlameToFile(url1, 1, rev1, peg.IsValid() ? peg : rev1, tempfile1, options, TRUE, TRUE))
                {
                    if ((peg.IsValid())&&(blamer.GetSVNError()->apr_err != SVN_ERR_CLIENT_IS_BINARY_FILE))
                    {
                        if (!blamer.BlameToFile(url1, 1, rev1, rev1, tempfile1, options, TRUE, TRUE))
                        {
                            progDlg.Stop();
                            m_pSVN->SetAndClearProgressInfo((HWND)NULL);
                            blamer.ShowErrorDialog(GetHWND());
                            return false;
                        }
                    }
                    else
                    {
                        if (blamer.GetSVNError()->apr_err != SVN_ERR_CLIENT_IS_BINARY_FILE)
                        {
                            progDlg.Stop();
                            m_pSVN->SetAndClearProgressInfo((HWND)NULL);
                        }
                        blamer.ShowErrorDialog(GetHWND());
                        if (blamer.GetSVNError()->apr_err == SVN_ERR_CLIENT_IS_BINARY_FILE)
                            blame = false;
                        else
                            return false;
                    }
                }
            }
            if (!blame)
            {
                bool tryWorking = (!m_pSVN->PathIsURL(url1) && rev1.IsWorking() && PathFileExists(url1.GetWinPath()));
                if (!m_pSVN->Export(url1, tempfile1, peg.IsValid() && !tryWorking ? peg : rev1, rev1))
                {
                    if (peg.IsValid())
                    {
                        if (!m_pSVN->Export(url1, tempfile1, rev1, rev1))
                        {
                            progDlg.Stop();
                            m_pSVN->SetAndClearProgressInfo((HWND)NULL);
                            m_pSVN->ShowErrorDialog(GetHWND());
                            return false;
                        }
                    }
                    else
                    {
                        progDlg.Stop();
                        m_pSVN->SetAndClearProgressInfo((HWND)NULL);
                        m_pSVN->ShowErrorDialog(GetHWND());
                        return false;
                    }
                }
            }
            SetFileAttributes(tempfile1.GetWinPath(), FILE_ATTRIBUTE_READONLY);

            progDlg.FormatPathLine(1, IDS_PROGRESSGETFILEREVISION, (LPCTSTR)url2.GetUIFileOrDirectoryName(), (LPCTSTR)rev2.ToString());
            progDlg.SetProgress(50,100);
            if (blame)
            {
                if (!blamer.BlameToFile(url2, 1, rev2, peg.IsValid() ? peg : rev2, tempfile2, options, TRUE, TRUE))
                {
                    if (peg.IsValid())
                    {
                        if (!blamer.BlameToFile(url2, 1, rev2, rev2, tempfile2, options, TRUE, TRUE))
                        {
                            progDlg.Stop();
                            m_pSVN->SetAndClearProgressInfo((HWND)NULL);
                            m_pSVN->ShowErrorDialog(GetHWND());
                            return false;
                        }
                    }
                    else
                    {
                        progDlg.Stop();
                        m_pSVN->SetAndClearProgressInfo((HWND)NULL);
                        m_pSVN->ShowErrorDialog(GetHWND());
                        return false;
                    }
                }
            }
            else
            {
                if (!m_pSVN->Export(url2, tempfile2, peg.IsValid() ? peg : rev2, rev2))
                {
                    if (peg.IsValid())
                    {
                        if (!m_pSVN->Export(url2, tempfile2, rev2, rev2))
                        {
                            progDlg.Stop();
                            m_pSVN->SetAndClearProgressInfo((HWND)NULL);
                            m_pSVN->ShowErrorDialog(GetHWND());
                            return false;
                        }
                    }
                    else
                    {
                        progDlg.Stop();
                        m_pSVN->SetAndClearProgressInfo((HWND)NULL);
                        m_pSVN->ShowErrorDialog(GetHWND());
                        return false;
                    }
                }
            }
            SetFileAttributes(tempfile2.GetWinPath(), FILE_ATTRIBUTE_READONLY);

            progDlg.SetProgress(100,100);
            progDlg.Stop();
            m_pSVN->SetAndClearProgressInfo((HWND)NULL);

            CString revname1, revname2;
            if (url1.IsEquivalentTo(url2))
            {
                revname1.Format(L"%s Revision %s", (LPCTSTR)url1.GetUIFileOrDirectoryName(), (LPCTSTR)rev1.ToString());
                revname2.Format(L"%s Revision %s", (LPCTSTR)url2.GetUIFileOrDirectoryName(), (LPCTSTR)rev2.ToString());
            }
            else
            {
                if (sRepoRoot.IsEmpty())
                {
                    revname1.Format(L"%s Revision %s", (LPCTSTR)url1.GetSVNPathString(), (LPCTSTR)rev1.ToString());
                    revname2.Format(L"%s Revision %s", (LPCTSTR)url2.GetSVNPathString(), (LPCTSTR)rev2.ToString());
                }
                else
                {
                    if (url1.IsUrl())
                        revname1.Format(L"%s Revision %s", (LPCTSTR)url1.GetSVNPathString().Mid(sRepoRoot.GetLength()), (LPCTSTR)rev1.ToString());
                    else
                        revname1.Format(L"%s Revision %s", (LPCTSTR)url1.GetSVNPathString(), (LPCTSTR)rev1.ToString());
                    if (url2.IsUrl() && (url2.GetSVNPathString().Left(sRepoRoot.GetLength()).Compare(sRepoRoot) == 0))
                        revname2.Format(L"%s Revision %s", (LPCTSTR)url2.GetSVNPathString().Mid(sRepoRoot.GetLength()), (LPCTSTR)rev2.ToString());
                    else
                        revname2.Format(L"%s Revision %s", (LPCTSTR)url2.GetSVNPathString(), (LPCTSTR)rev2.ToString());
                }
            }
            return CAppUtils::StartExtDiff(tempfile1, tempfile2, revname1, revname2, url1, url2, rev1, rev2, peg, diffFlags.Blame(blame), m_JumpLine, L"", mimetype);
        }
    }
    else
    {
        // compare with working copy
        if (PathIsDirectory(url1.GetWinPath()))
        {
            if (UnifiedDiff(tempfile, url1, rev1, url1, rev2, (peg.IsValid() ? peg : SVNRev::REV_WC), options))
            {
                CString sWC, sRev;
                sWC.LoadString(IDS_DIFF_WORKINGCOPY);
                sRev.Format(IDS_DIFF_REVISIONPATCHED, (LONG)rev2);
                progDlg.Stop();
                m_pSVN->SetAndClearProgressInfo((HWND)NULL);
                return !!CAppUtils::StartExtPatch(tempfile, url1.GetDirectory(), sWC, sRev, TRUE);
            }
        }
        else
        {
            ASSERT(rev1.IsWorking());

            if (url1.IsEquivalentTo(url2) && !ignoreprops)
            {
                svn_revnum_t baseRev = 0;
                DiffProps(url1, rev1, rev2, baseRev);
            }

            m_pSVN->SetAndClearProgressInfo(&progDlg, true);    // activate progress bar
            progDlg.ShowModeless(GetHWND());
            progDlg.FormatPathLine(1, IDS_PROGRESSGETFILEREVISION, (LPCTSTR)url1.GetUIFileOrDirectoryName(), (LPCTSTR)rev2.ToString());

            tempfile = CTempFiles::Instance().GetTempFilePath(m_bRemoveTempFiles, url1, rev2);
            if (blame)
            {
                CBlame blamer;
                if (!blamer.BlameToFile(url1, 1, rev2, (peg.IsValid() ? peg : SVNRev::REV_WC), tempfile, options, TRUE, TRUE))
                {
                    if (peg.IsValid())
                    {
                        if (!blamer.BlameToFile(url1, 1, rev2, SVNRev::REV_WC, tempfile, options, TRUE, TRUE))
                        {
                            progDlg.Stop();
                            m_pSVN->SetAndClearProgressInfo((HWND)NULL);
                            m_pSVN->ShowErrorDialog(GetHWND());
                            return false;
                        }
                    }
                    else
                    {
                        progDlg.Stop();
                        m_pSVN->SetAndClearProgressInfo((HWND)NULL);
                        m_pSVN->ShowErrorDialog(GetHWND());
                        return false;
                    }
                }
                progDlg.Stop();
                m_pSVN->SetAndClearProgressInfo((HWND)NULL);
                SetFileAttributes(tempfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
                CTSVNPath tempfile2 = CTempFiles::Instance().GetTempFilePath(false, url1);
                if (!blamer.BlameToFile(url1, 1, SVNRev::REV_WC, SVNRev::REV_WC, tempfile2, options, TRUE, TRUE))
                {
                    progDlg.Stop();
                    m_pSVN->SetAndClearProgressInfo((HWND)NULL);
                    m_pSVN->ShowErrorDialog(GetHWND());
                    return false;
                }
                CString revname, wcname;
                revname.Format(L"%s Revision %ld", (LPCTSTR)url1.GetFilename(), (LONG)rev2);
                wcname.Format(IDS_DIFF_WCNAME, (LPCTSTR)url1.GetFilename());
                m_pSVN->SetAndClearProgressInfo((HWND)NULL);
                return CAppUtils::StartExtDiff(tempfile, tempfile2, revname, wcname, url1, url2, rev1, rev2, peg, diffFlags, m_JumpLine, url1.GetFileOrDirectoryName(), L"");
            }
            else
            {
                if (!m_pSVN->Export(url1, tempfile, (peg.IsValid() ? peg : SVNRev::REV_WC), rev2))
                {
                    if (peg.IsValid())
                    {
                        if (!m_pSVN->Export(url1, tempfile, SVNRev::REV_WC, rev2))
                        {
                            progDlg.Stop();
                            m_pSVN->SetAndClearProgressInfo((HWND)NULL);
                            m_pSVN->ShowErrorDialog(GetHWND());
                            return false;
                        }
                    }
                    else
                    {
                        progDlg.Stop();
                        m_pSVN->SetAndClearProgressInfo((HWND)NULL);
                        m_pSVN->ShowErrorDialog(GetHWND());
                        return false;
                    }
                }
                progDlg.Stop();
                m_pSVN->SetAndClearProgressInfo((HWND)NULL);
                SetFileAttributes(tempfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
                CString revname, wcname;
                revname.Format(L"%s Revision %s", (LPCTSTR)url1.GetFilename(), (LPCTSTR)rev2.ToString());
                wcname.Format(IDS_DIFF_WCNAME, (LPCTSTR)url1.GetFilename());
                return CAppUtils::StartExtDiff(tempfile, url1, revname, wcname, url1, url2, rev1, rev2, peg, diffFlags, m_JumpLine, url1.GetFileOrDirectoryName(), L"");
            }
        }
    }
    m_pSVN->SetAndClearProgressInfo((HWND)NULL);
    return false;
}

bool SVNDiff::DiffProps(const CTSVNPath& filePath, const SVNRev& rev1, const SVNRev& rev2, svn_revnum_t &baseRev) const
{
    bool retvalue = false;
    // diff the properties
    SVNProperties propswc(filePath, rev1, false, false);
    SVNProperties propsbase(filePath, rev2, false, false);

#define MAX_PATH_LENGTH 80
    WCHAR pathbuf1[MAX_PATH] = {0};
    if (filePath.GetWinPathString().GetLength() >= MAX_PATH)
    {
        std::wstring str = filePath.GetWinPath();
        std::wregex rx(L"^(\\w+:|(?:\\\\|/+))((?:\\\\|/+)[^\\\\/]+(?:\\\\|/)[^\\\\/]+(?:\\\\|/)).*((?:\\\\|/)[^\\\\/]+(?:\\\\|/)[^\\\\/]+)$");
        std::wstring replacement = L"$1$2...$3";
        std::wstring str2 = std::regex_replace(str, rx, replacement);
        if (str2.size() >= MAX_PATH)
            str2 = str2.substr(0, MAX_PATH-2);
        PathCompactPathEx(pathbuf1, str2.c_str(), MAX_PATH_LENGTH, 0);
    }
    else
        PathCompactPathEx(pathbuf1, filePath.GetWinPath(), MAX_PATH_LENGTH, 0);

    if ((baseRev == 0) && (!filePath.IsUrl()) && (rev1.IsBase() || rev2.IsBase()))
    {
        SVNStatus stat;
        CTSVNPath dummy;
        svn_client_status_t * s = stat.GetFirstFileStatus(filePath, dummy);
        if (s)
            baseRev = s->revision;
    }
    // check for properties that got removed
    for (int baseindex = 0; baseindex < propsbase.GetCount(); ++baseindex)
    {
        std::string basename = propsbase.GetItemName(baseindex);
        tstring basenameU = CUnicodeUtils::StdGetUnicode(basename);
        tstring basevalue = (LPCTSTR)CUnicodeUtils::GetUnicode(propsbase.GetItemValue(baseindex).c_str());
        bool bFound = false;
        for (int wcindex = 0; wcindex < propswc.GetCount(); ++wcindex)
        {
            if (basename.compare (propswc.GetItemName(wcindex))==0)
            {
                bFound = true;
                break;
            }
        }
        if (!bFound)
        {
            // write the old property value to temporary file
            CTSVNPath wcpropfile = CTempFiles::Instance().GetTempFilePath(false);
            CTSVNPath basepropfile = CTempFiles::Instance().GetTempFilePath(false);
            FILE * pFile;
            _tfopen_s(&pFile, wcpropfile.GetWinPath(), L"wb");
            if (pFile)
            {
                fclose(pFile);
                FILE * pFile2;
                _tfopen_s(&pFile2, basepropfile.GetWinPath(), L"wb");
                if (pFile2)
                {
                    fputs(CUnicodeUtils::StdGetUTF8(basevalue).c_str(), pFile2);
                    fclose(pFile2);
                }
                else
                    return false;
            }
            else
                return false;
            SetFileAttributes(wcpropfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
            SetFileAttributes(basepropfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
            CString n1, n2;
            bool bSwitch = false;
            if (rev1.IsWorking())
                n1.Format(IDS_DIFF_PROP_WCNAME, basenameU.c_str());
            if (rev1.IsBase())
            {
                if (baseRev)
                    n1.FormatMessage(IDS_DIFF_PROP_BASENAMEREV, basenameU.c_str(), baseRev);
                else
                    n1.Format(IDS_DIFF_PROP_BASENAME, basenameU.c_str());
            }
            if (rev1.IsHead())
                n1.Format(IDS_DIFF_PROP_REMOTENAME, basenameU.c_str());
            if (n1.IsEmpty())
            {
                CString temp;
                temp.Format(IDS_DIFF_REVISIONPATCHED, (LONG)rev1);
                n1 = basenameU.c_str();
                n1 += L" " + temp;
                bSwitch = true;
            }
            else
            {
                n1 = CString(pathbuf1) + L" - " + n1;
            }
            if (rev2.IsWorking())
                n2.Format(IDS_DIFF_PROP_WCNAME, basenameU.c_str());
            if (rev2.IsBase())
            {
                if (baseRev)
                    n2.FormatMessage(IDS_DIFF_PROP_BASENAMEREV, basenameU.c_str(), baseRev);
                else
                    n2.Format(IDS_DIFF_PROP_BASENAME, basenameU.c_str());
            }
            if (rev2.IsHead())
                n2.Format(IDS_DIFF_PROP_REMOTENAME, basenameU.c_str());
            if (n2.IsEmpty())
            {
                CString temp;
                temp.Format(IDS_DIFF_REVISIONPATCHED, (LONG)rev2);
                n2 = basenameU.c_str();
                n2 += L" " + temp;
                bSwitch = true;
            }
            else
            {
                n2 = CString(pathbuf1) + L" - " + n2;
            }
            if (bSwitch)
            {
                retvalue = !!CAppUtils::StartExtDiffProps(wcpropfile, basepropfile, n1, n2, TRUE, TRUE);
            }
            else
            {
                retvalue = !!CAppUtils::StartExtDiffProps(basepropfile, wcpropfile, n2, n1, TRUE, TRUE);
            }
        }
    }

    for (int wcindex = 0; wcindex < propswc.GetCount(); ++wcindex)
    {
        std::string wcname = propswc.GetItemName(wcindex);
        tstring wcnameU = CUnicodeUtils::StdGetUnicode(wcname);
        tstring wcvalue = (LPCTSTR)CUnicodeUtils::GetUnicode(propswc.GetItemValue(wcindex).c_str());
        tstring basevalue;
        bool bDiffRequired = true;
        for (int baseindex = 0; baseindex < propsbase.GetCount(); ++baseindex)
        {
            if (propsbase.GetItemName(baseindex).compare(wcname)==0)
            {
                basevalue = CUnicodeUtils::GetUnicode(propsbase.GetItemValue(baseindex).c_str());
                if (basevalue.compare(wcvalue)==0)
                {
                    // name and value are identical
                    bDiffRequired = false;
                    break;
                }
            }
        }
        if (bDiffRequired)
        {
            // write both property values to temporary files
            CTSVNPath wcpropfile = CTempFiles::Instance().GetTempFilePath(false);
            CTSVNPath basepropfile = CTempFiles::Instance().GetTempFilePath(false);
            FILE * pFile;
            _tfopen_s(&pFile, wcpropfile.GetWinPath(), L"wb");
            if (pFile)
            {
                fputs(CUnicodeUtils::StdGetUTF8(wcvalue).c_str(), pFile);
                fclose(pFile);
                FILE * pFile2;
                _tfopen_s(&pFile2, basepropfile.GetWinPath(), L"wb");
                if (pFile2)
                {
                    fputs(CUnicodeUtils::StdGetUTF8(basevalue).c_str(), pFile2);
                    fclose(pFile2);
                }
                else
                    return false;
            }
            else
                return false;
            SetFileAttributes(wcpropfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
            SetFileAttributes(basepropfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
            CString n1, n2;
            if (rev1.IsWorking())
                n1.Format(IDS_DIFF_WCNAME, wcnameU.c_str());
            if (rev1.IsBase())
                n1.Format(IDS_DIFF_BASENAME, wcnameU.c_str());
            if (rev1.IsHead())
                n1.Format(IDS_DIFF_REMOTENAME, wcnameU.c_str());
            if (n1.IsEmpty())
                n1.FormatMessage(IDS_DIFF_PROP_REVISIONNAME, wcnameU.c_str(), (LPCTSTR)rev1.ToString());
            else
                n1 = CString(pathbuf1) + L" - " + n1;
            if (rev2.IsWorking())
                n2.Format(IDS_DIFF_WCNAME, wcnameU.c_str());
            if (rev2.IsBase())
                n2.Format(IDS_DIFF_BASENAME, wcnameU.c_str());
            if (rev2.IsHead())
                n2.Format(IDS_DIFF_REMOTENAME, wcnameU.c_str());
            if (n2.IsEmpty())
                n2.FormatMessage(IDS_DIFF_PROP_REVISIONNAME, wcnameU.c_str(), (LPCTSTR)rev2.ToString());
            else
                n2 = CString(pathbuf1) + L" - " + n2;
            retvalue = !!CAppUtils::StartExtDiffProps(basepropfile, wcpropfile, n2, n1, TRUE, TRUE);
        }
    }
    return retvalue;
}