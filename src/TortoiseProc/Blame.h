// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008, 2010-2013, 2016, 2021 - TortoiseSVN

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
#include "SVN.h"
#include "ProgressDlg.h"
#include "SVNRev.h"
#include "StdioFileT.h"

class CTSVNPath;

/**
 * \ingroup TortoiseProc
 * Helper class to get the blame information for a file.
 */
class CBlame : public SVN
{
public:
    CBlame();
    ~CBlame() override;

    /**
     * Determine for every line in a versioned file the author, revision of last change, date of last
     * change. The result is saved to a temporary file.\n
     * Since this operation takes a long time a progress dialog is shown if \a showprogress is set to TRUE
     * \param startRev the starting revision of the operation
     * \param endRev the revision to stop the operation
     * \param pegRev the peg revision
     * \param options
     * \param includeMerge
     * \param showProgress
     * \param ignoreMimeType
     * \param path the path to the file to determine the required information
     * \return The path to the temporary file or an empty string in case of an error.
     */
    CString BlameToTempFile(const CTSVNPath& path, const SVNRev& startRev, const SVNRev& endRev, SVNRev pegRev, const CString& options, BOOL includeMerge, BOOL showProgress, BOOL ignoreMimeType);

    bool BlameToFile(const CTSVNPath& path, const SVNRev& startRev, const SVNRev& endRev, SVNRev peg, const CTSVNPath& toFile, const CString& options, BOOL ignoreMimeType, BOOL includeMerge);
    void SetAndClearProgressInfo(CProgressDlg* pProgressDlg, int infoLine, bool bShowProgressBar = false)
    {
        SVN::SetAndClearProgressInfo(pProgressDlg, bShowProgressBar);
        m_bShowProgressBar = bShowProgressBar;
        m_nFormatLine      = infoLine;
    }

private:
    BOOL BlameCallback(LONG lineNumber, bool localChange, svn_revnum_t revision, const CString& author, const CString& date,
                       svn_revnum_t mergedRevision, const CString& mergedAuthor, const CString& mergedDate, const CString& mergedPath,
                       const CStringA& line, const CStringA& logMsg, const CStringA& mergedLogMsg) override;
    BOOL Cancel() override;
    BOOL Notify(const CTSVNPath& path, const CTSVNPath& url, svn_wc_notify_action_t action,
                svn_node_kind_t kind, const CString& mimeType,
                svn_wc_notify_state_t contentState,
                svn_wc_notify_state_t propState, svn_revnum_t rev,
                const svn_lock_t* lock, svn_wc_notify_lock_state_t lockState,
                const CString&     changeListName,
                const CString&     propertyName,
                svn_merge_range_t* range,
                svn_error_t* err, apr_pool_t* pool) override;

private:
    BOOL m_bCancelled;    ///< TRUE if the operation should be canceled
    LONG m_nCounter;      ///< Counts the number of calls to the Cancel() callback (revisions?)
    LONG m_nHeadRev;      ///< The HEAD revision of the file
    bool m_bNoLineNo;     ///< if true, then the line number isn't written to the file
    bool m_bHasMerges;    ///< If the blame has merge info, this is set to true
    bool m_bIncludeMerge; ///< true if merge info was requested
    int  m_nFormatLine;   ///< the line of the progress dialog where to write the blame progress to
    int  m_bSetProgress;  ///< whether to set the progress bar state

    CString      m_sSavePath;   ///< Where to save the blame data
    CStdioFileT  m_saveFile;    ///< The file object to write to
    CProgressDlg m_progressDlg; ///< The progress dialog shown during operation
    LONG         m_lowestRev;
    LONG         m_highestRev;
    BOOL         extBlame;
};
