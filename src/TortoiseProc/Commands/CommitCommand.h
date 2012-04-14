// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008, 2010-2012 - TortoiseSVN

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

class CCommitDlg;
class CSVNProgressDlg;

/**
 * \ingroup TortoiseProc
 * Shows the commit dialog, executes the commit and retries (if necessary)
 */
class CommitCommand : public Command
{
private:
    CString LoadLogMessage();
    void InitProgressDialog (CCommitDlg& commitDlg, CSVNProgressDlg& progDlg);
    bool IsOutOfDate(const svn_error_t* err ) const;

public:
    /**
     * Executes the command.
     */
    virtual bool            Execute() override;
};
