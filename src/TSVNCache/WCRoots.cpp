// TortoiseSVN - a Windows shell extension for easy version control

// External Cache Copyright (C) 2010, 2014-2015, 2021 - TortoiseSVN

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
#include "WCRoots.h"
#include "SVNAdminDir.h"

#define DBTIMEOUT 10000

CWCRoots::CWCRoots()
{
}

CWCRoots::~CWCRoots()
{
}

__int64 CWCRoots::GetDBFileTime(const CTSVNPath& path)
{
    AutoLocker                                  lock(m_critSec);
    std::map<CTSVNPath, WCRootsTimes>::iterator it = m_wcdBs.lower_bound(path);
    if (it != m_wcdBs.end())
    {
        if (it->first.IsAncestorOf(path))
        {
            auto ticks = GetTickCount64();
            if (ticks - it->second.lastTicks > DBTIMEOUT)
            {
                // refresh the file time
                CTSVNPath wcDbFile(it->first);
                wcDbFile.AppendPathString(g_SVNAdminDir.GetAdminDirName() + L"\\wc.db");
                if (wcDbFile.Exists())
                {
                    WCRootsTimes dbTimes;
                    dbTimes.lastTicks = ticks;
                    dbTimes.fileTime  = wcDbFile.GetLastWriteTime();
                    it->second        = dbTimes;
                }
                else
                {
                    // remove the path from the map
                    m_wcdBs.erase(it);
                    return 0;
                }
            }
            return it->second.fileTime;
        }
        else
        {
            it = AddPathInternal(path);
            if (it != m_wcdBs.end())
                return it->second.fileTime;
        }
    }
    else
    {
        it = AddPathInternal(path);
        if (it != m_wcdBs.end())
            return it->second.fileTime;
    }
    return 0;
}

std::map<CTSVNPath, WCRootsTimes>::iterator CWCRoots::AddPathInternal(const CTSVNPath& path)
{
    AutoLocker lock(m_critSec);
    CTSVNPath  p(path);
    do
    {
        CTSVNPath dbPath(p);
        dbPath.AppendPathString(g_SVNAdminDir.GetAdminDirName() + L"\\wc.db");
        if (!dbPath.Exists())
            p = p.GetContainingDirectory();
        else
        {
            WCRootsTimes dbTimes;
            dbTimes.lastTicks = GetTickCount64();
            dbTimes.fileTime  = dbPath.GetLastWriteTime();
            return m_wcdBs.emplace(p, dbTimes).first;
        }
    } while (!p.IsEmpty());

    return m_wcdBs.end();
}

bool CWCRoots::AddPath(const CTSVNPath& path)
{
    return AddPathInternal(path) != m_wcdBs.end();
}

bool CWCRoots::NotifyChange(const CTSVNPath& path)
{
    AutoLocker lock(m_critSec);
    CTSVNPath  p(path);
    bool       changed = true;
    while (p.IsAdminDir())
    {
        p = p.GetContainingDirectory();
    }
    std::map<CTSVNPath, WCRootsTimes>::iterator it = m_wcdBs.lower_bound(p);
    if (it != m_wcdBs.end())
    {
        if (it->first.IsAncestorOf(p))
        {
            // refresh the file time
            CTSVNPath wcDbFile(it->first);
            wcDbFile.AppendPathString(g_SVNAdminDir.GetAdminDirName() + L"\\wc.db");
            if (wcDbFile.Exists())
            {
                WCRootsTimes dbTimes;
                dbTimes.lastTicks = GetTickCount64();
                dbTimes.fileTime  = wcDbFile.GetLastWriteTime();
                changed           = (dbTimes.fileTime != it->second.fileTime);
                it->second        = dbTimes;
            }
            else
            {
                // remove the path from the map
                m_wcdBs.erase(it);
            }
        }
    }
    return changed;
}
