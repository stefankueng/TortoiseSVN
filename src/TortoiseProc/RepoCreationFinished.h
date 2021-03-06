// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010-2011, 2021 - TortoiseSVN

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
#include "TSVNPath.h"
#include "PathEdit.h"
#include "StandAloneDlg.h"

class CRepoCreationFinished : public CStandAloneDialog
{
    DECLARE_DYNAMIC(CRepoCreationFinished)

public:
    CRepoCreationFinished(CWnd* pParent = nullptr); // standard constructor
    ~CRepoCreationFinished() override;

    void SetRepoPath(const CTSVNPath& repoPath) { m_repoPath = repoPath; }

    // Dialog Data
    enum
    {
        IDD = IDD_REPOCREATEFINISHED
    };

protected:
    void         DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL         OnInitDialog() override;
    afx_msg void OnBnClickedCreatefolders();
    afx_msg void OnBnClickedRepobrowser();

    DECLARE_MESSAGE_MAP()

private:
    CTSVNPath m_repoPath;
    CPathEdit m_repoUrl;
};
