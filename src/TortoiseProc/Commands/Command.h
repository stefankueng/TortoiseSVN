// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007, 2011-2012 - TortoiseSVN

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
#include "TortoiseProc.h"
#include "CmdLineParser.h"
#include "TSVNPath.h"




/**
 * \ingroup TortoiseProc
 * Interface for all command line commands TortoiseProc can execute.
 */
class Command
{
public:
    Command()
        : hwndExplorer(NULL)
    {
    }
    /// allow sub-classes to execute code during destruction
    virtual ~Command() {};
    /**
     * Executes the command.
     */
    virtual bool            Execute() = 0;
    virtual bool            CheckPaths();

    void                    SetParser(const CCmdLineParser& p) {parser = p;}
    void                    SetPaths(const CTSVNPathList& plist, const CTSVNPath& path) {pathList = plist; cmdLinePath = path;}
    void                    SetExplorerHwnd(HWND hWnd) {hwndExplorer = hWnd;}
    HWND                    GetExplorerHWND() const { return (::IsWindow(hwndExplorer) ? hwndExplorer : NULL); }
protected:
    CCmdLineParser          parser;
    CTSVNPathList           pathList;
    CTSVNPath               cmdLinePath;
private:
    HWND                    hwndExplorer;
};

/**
 * \ingroup TortoiseProc
 * Factory for the different commands which TortoiseProc executes from the
 * command line.
 */
class CommandServer
{
public:

    Command *               GetCommand(const CString& sCmd);
};