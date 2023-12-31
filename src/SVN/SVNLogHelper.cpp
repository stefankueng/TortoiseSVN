﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007, 2009, 2011, 2013, 2015-2016, 2021 - TortoiseSVN

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
#include "SVNLogHelper.h"
#include "UnicodeUtils.h"
#include "PathUtils.h"
#include "Access/StrictLogIterator.h"

SVNRev SVNLogHelper::GetCopyFromRev(const CTSVNPath& url, SVNRev pegRev, CString& copyfromURL)
{
    SVNRev result;

    // fill / update a suitable log cache

    if (pegRev.IsHead())
        pegRev = GetHEADRevision(url);
    if (!pegRev.IsNumber())
        return SVNRev();

    std::unique_ptr<const CCacheLogQuery> query(ReceiveLog(CTSVNPathList(url), pegRev, pegRev, 1, 0, TRUE, FALSE, false));
    if (query.get() == nullptr)
        return result;

    // construct the path object
    // (URLs are always escaped, so we must unescape them)

    CStringA svnURLPath = CUnicodeUtils::GetUTF8(url.GetSVNPathString());
    if (svnURLPath.Left(9).CompareNoCase("file:///\\") == 0)
        svnURLPath.Delete(7, 2);

    CStringA relPath = svnURLPath.Mid(query->GetRootURL().GetLength());
    relPath          = CPathUtils::PathUnescape(relPath);

    const CPathDictionary*   paths = &query->GetCache()->GetLogInfo().GetPaths();
    CDictionaryBasedTempPath path(paths, static_cast<const char*>(relPath));

    // follow the log

    LogCache::CStrictLogIterator iterator(query->GetCache(), pegRev, path);

    result = pegRev;
    iterator.Retry();
    while (!iterator.EndOfPath())
    {
        result = iterator.GetRevision();
        iterator.Advance();
    }

    // no copy-from-info?

    if (!iterator.GetPath().IsValid())
        return SVNRev();

    // return the results

    copyfromURL = MakeUIUrlOrPath(query->GetRootURL() + iterator.GetPath().GetPath().c_str());
    if (iterator.GetCopyFromRevision() != NO_REVISION)
        result = iterator.GetCopyFromRevision();
    return result;
}

std::vector<std::pair<CTSVNPath, SVNRev>>
    SVNLogHelper::GetCopyHistory(const CTSVNPath& url, const SVNRev& pegRev)
{
    std::vector<std::pair<CTSVNPath, SVNRev>> result;

    CTSVNPath path = url;
    SVNRev    rev  = pegRev.IsHead() ? static_cast<SVNRev>(GetHEADRevision(url)) : pegRev;

    while (rev.IsNumber() && !path.IsEmpty())
    {
        result.emplace_back(path, rev);

        CString copyFromURL;
        rev = GetCopyFromRev(path, rev, copyFromURL);
        path.SetFromSVN(copyFromURL);
    }

    return result;
}

std::pair<CTSVNPath, SVNRev>
    SVNLogHelper::GetCommonSource(const CTSVNPath& url1, const SVNRev& pegRev1,
                                  const CTSVNPath& url2, const SVNRev& pegRev2)
{
    // get the full copy histories of both urls

    auto copyHistory1 = GetCopyHistory(url1, pegRev1);
    auto copyHistory2 = GetCopyHistory(url2, pegRev2);

    // starting from the oldest paths, look for the first divergence
    // (this may be important in the case of cyclic renames)

    auto iter1 = copyHistory1.rbegin(), end1 = copyHistory1.rend();
    auto iter2 = copyHistory2.rbegin(), end2 = copyHistory2.rend();
    for (; iter1 != end1 && iter2 != end2 && iter1->first.IsEquivalentTo(iter2->first); ++iter1, ++iter2)
    {
    }

    // if even the oldest paths were different, then there is no common source

    if (iter1 == copyHistory1.rbegin())
        return std::pair<CTSVNPath, SVNRev>();

    // return the last revision in the common history

    --iter1;
    --iter2;
    SVNRev commonRev = min(static_cast<LONG>(iter1->second), static_cast<LONG>(iter2->second));
    return std::make_pair(iter1->first, commonRev);
}

SVNRev SVNLogHelper::GetYoungestRev(const CTSVNPath& url)
{
    SVNRev result;

    // fill / update a suitable log cache

    auto pegrev = GetHEADRevision(url);

    std::unique_ptr<const CCacheLogQuery> query(ReceiveLog(CTSVNPathList(url), pegrev, pegrev, 1, 1, TRUE, FALSE, false));
    if (query.get() == nullptr)
        return pegrev;

    // construct the path object
    // (URLs are always escaped, so we must unescape them)

    CStringA svnURLPath = CUnicodeUtils::GetUTF8(url.GetSVNPathString());
    if (svnURLPath.Left(9).CompareNoCase("file:///\\") == 0)
        svnURLPath.Delete(7, 2);

    CStringA relPath = svnURLPath.Mid(query->GetRootURL().GetLength());
    relPath          = CPathUtils::PathUnescape(relPath);

    const CPathDictionary*   paths = &query->GetCache()->GetLogInfo().GetPaths();
    CDictionaryBasedTempPath path(paths, static_cast<const char*>(relPath));

    // follow the log

    LogCache::CStrictLogIterator iterator(query->GetCache(), pegrev, path);

    iterator.Retry();
    if (!iterator.EndOfPath())
        result = iterator.GetRevision();

    return result;
}
