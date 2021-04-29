﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008, 2010-2011, 2013-2015, 2018, 2021 - TortoiseSVN

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
#include "DiffCommand.h"
#include "PathUtils.h"
#include "AppUtils.h"
#include "ChangedDlg.h"
#include "SVNDiff.h"
#include "SVNStatus.h"

bool DiffCommand::Execute()
{
    bool bRet             = false;
    auto path2            = CPathUtils::GetLongPathname(parser.GetVal(L"path2"));
    bool bAlternativeTool = !!parser.HasKey(L"alternative");
    bool bBlame           = !!parser.HasKey(L"blame");
    bool ignoreProps      = !!parser.HasKey(L"ignoreprops");
    if (path2.empty())
    {
        SVNDiff diff(nullptr, GetExplorerHWND());
        diff.SetAlternativeTool(bAlternativeTool);
        diff.SetJumpLine(parser.GetLongVal(L"line"));
        if (parser.HasKey(L"startrev") && parser.HasKey(L"endrev"))
        {
            SVNRev startRevision = SVNRev(parser.GetVal(L"startrev"));
            SVNRev endRevision   = SVNRev(parser.GetVal(L"endrev"));
            SVNRev pegRevision;
            if (parser.HasVal(L"pegrevision"))
                pegRevision = SVNRev(parser.GetVal(L"pegrevision"));
            CString diffOptions;
            if (parser.HasVal(L"diffoptions"))
                diffOptions = parser.GetVal(L"diffoptions");
            bRet = diff.ShowCompare(cmdLinePath, startRevision, cmdLinePath, endRevision, pegRevision, ignoreProps, parser.HasKey(L"prettyprint"), diffOptions, false, bBlame);
        }
        else
        {
            svn_revnum_t baseRev = 0;
            if (parser.HasKey(L"unified"))
            {
                SVNRev pegRevision;
                if (parser.HasVal(L"pegrevision"))
                    pegRevision = SVNRev(parser.GetVal(L"pegrevision"));
                CString diffOptions;
                if (parser.HasVal(L"diffoptions"))
                    diffOptions = parser.GetVal(L"diffoptions");
                diff.ShowUnifiedDiff(cmdLinePath, SVNRev::REV_BASE, cmdLinePath, SVNRev::REV_WC, pegRevision, !!parser.HasKey(L"prettyprint"), diffOptions, false, bBlame, false);
            }
            else
            {
                if (cmdLinePath.IsDirectory())
                {
                    if (!ignoreProps)
                        bRet = diff.DiffProps(cmdLinePath, SVNRev::REV_WC, SVNRev::REV_BASE, baseRev);
                    if (bRet == false)
                    {
                        CChangedDlg dlg;
                        dlg.m_pathList = CTSVNPathList(cmdLinePath);
                        dlg.DoModal();
                        bRet = true;
                    }
                }
                else
                {
                    bRet = diff.DiffFileAgainstBase(cmdLinePath, baseRev, ignoreProps);
                }
            }
        }
    }
    else
        bRet = CAppUtils::StartExtDiff(
            CTSVNPath(path2.c_str()), cmdLinePath, CString(), CString(),
            CAppUtils::DiffFlags().AlternativeTool(bAlternativeTool), parser.GetLongVal(L"line"), L"");
    return bRet;
}