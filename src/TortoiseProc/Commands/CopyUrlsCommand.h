// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2014, 2021 - TortoiseSVN

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
#include "Command.h"
#include "SVN.h"
#include "StringUtils.h"

/**
 * \ingroup TortoiseProc
 * Retrieves the urls of the paths, then copies those urls to the clipboard
 */
class CopyUrlsCommand : public Command
{
public:
    /**
     * Executes the command.
     */
    bool Execute() override
    {
        SVN     svn;
        CString sUrls;
        for (int i = 0; i < pathList.GetCount(); ++i)
        {
            sUrls += svn.GetURLFromPath(pathList[i]);
            sUrls += L"\n";
        }
        CStringUtils::WriteAsciiStringToClipboard(sUrls);
        return true;
    }
};
