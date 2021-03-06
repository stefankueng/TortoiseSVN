// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008, 2015, 2021 - TortoiseSVN

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
#include "StandAloneDlg.h"

///////////////////////////////////////////////////////////////
// forward declarations
///////////////////////////////////////////////////////////////

namespace LogCache
{
struct CLogCacheStatisticsData;
}

class CLogCacheStatisticsDlg : public CStandAloneDialog
{
    DECLARE_DYNAMIC(CLogCacheStatisticsDlg)

public:
    CLogCacheStatisticsDlg(const LogCache::CLogCacheStatisticsData& data, CWnd* pParentWnd = nullptr);
    ~CLogCacheStatisticsDlg() override;

    enum
    {
        IDD = IDD_LOGCACHESTATISTICS
    };

protected:
    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    DECLARE_MESSAGE_MAP()

private:
    CString sizeRAM;
    CString sizeDisk;
    CString connectionState;
    CString lastRead;
    CString lastWrite;
    CString lastHeadUpdate;
    CString authors;
    CString paths;
    CString pathElements;
    CString skipRanges;
    CString wordTokens;
    CString pairTokens;
    CString textSize;
    CString uncompressedSize;
    CString maxRevision;
    CString revisionCount;
    CString changesTotal;
    CString changedRevisions;
    CString changesMissing;
    CString mergesTotal;
    CString mergesRevisions;
    CString mergesMissing;
    CString userRevpropsTotal;
    CString userRevpropsRevisions;
    CString userRevpropsMissing;

    CString        DateToString(__time64_t time) const;
    static CString ToString(__int64 value);
};
