// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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
	virtual ~CBlame();

	/**
	 * Determine for every line in a versioned file the author, revision of last change, date of last
	 * change. The result is saved to a temporary file.\n
	 * Since this operation takes a long time a progress dialog is shown if \a showprogress is set to TRUE
	 * \param startrev the starting revision of the operation
	 * \param endrev the revision to stop the operation
	 * \param pegrev the peg revision
	 * \param path the path to the file to determine the required information
	 * \return The path to the temporary file or an empty string in case of an error.
	 */
	CString		BlameToTempFile(const CTSVNPath& path, SVNRev startrev, SVNRev endrev, SVNRev pegrev, CString& logfile, const CString& options, BOOL includemerge, BOOL showprogress, BOOL ignoremimetype);

	bool		BlameToFile(const CTSVNPath& path, SVNRev startrev, SVNRev endrev, SVNRev peg, const CTSVNPath& tofile, const CString& options, BOOL ignoremimetype, BOOL includemerge);
private:
	BOOL		BlameCallback(LONG linenumber, svn_revnum_t revision, const CString& author, const CString& date,
								svn_revnum_t merged_revision, const CString& merged_author, const CString& merged_date, const CString& merged_path,
								const CStringA& line);
	BOOL		Cancel();
	BOOL		Notify(const CTSVNPath& path, svn_wc_notify_action_t action, 
						svn_node_kind_t kind, const CString& mime_type, 
						svn_wc_notify_state_t content_state, 
						svn_wc_notify_state_t prop_state, LONG rev,
						const svn_lock_t * lock, svn_wc_notify_lock_state_t lock_state,
						svn_error_t * err, apr_pool_t * pool);
	BOOL		Log(svn_revnum_t rev, const CString& author, const CString& message, apr_time_t time, BOOL haschildren);
private:
	BOOL		m_bCancelled;			///< TRUE if the operation should be canceled
	LONG		m_nCounter;				///< Counts the number of calls to the Cancel() callback (revisions?)
	LONG		m_nHeadRev;				///< The HEAD revision of the file
	bool		m_bNoLineNo;			///< if true, then the line number isn't written to the file
	bool		m_bHasMerges;			///< If the blame has merge info, this is set to true

	CString		m_sSavePath;			///< Where to save the blame data
	CStdioFileT	m_saveFile;				///< The file object to write to
	CFile		m_saveLog;
	CProgressDlg m_progressDlg;			///< The progress dialog shown during operation
	LONG		m_lowestrev;
	LONG		m_highestrev;
	BOOL		extBlame;
};
