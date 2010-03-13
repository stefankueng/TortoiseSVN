// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2010 - TortoiseSVN

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
#include "HistoryCombo.h"
#include "SVNRev.h"

class CTSVNPath;

/**
 * \ingroup TortoiseProc
 * An utility class with static functions.
 */
class CAppUtils
{
public:
	/**
	* Flags for StartExtDiff function.
	*/
	struct DiffFlags
	{
		bool bWait;
		bool bBlame;
		bool bReadOnly;
		bool bAlternativeTool; // If true, invert selection of TortoiseMerge vs. external diff tool

		DiffFlags(): bWait(false), bBlame(false), bReadOnly(false), bAlternativeTool(false)	{}
		DiffFlags& Wait(bool b = true) { bWait = b; return *this; }
		DiffFlags& Blame(bool b = true) { bBlame = b; return *this; }
		DiffFlags& ReadOnly(bool b = true) { bReadOnly = b; return *this; }
		DiffFlags& AlternativeTool(bool b = true) { bAlternativeTool = b; return *this; }
	};

	struct MergeFlags
	{
		bool bWait;
		bool bReadOnly;
		bool bAlternativeTool; // If true, invert selection of TortoiseMerge vs. external merge tool

		MergeFlags(): bWait(false), bReadOnly(false), bAlternativeTool(false)	{}
		MergeFlags& Wait(bool b = true) { bWait = b; return *this; }
		MergeFlags& ReadOnly(bool b = true) { bReadOnly = b; return *this; }
		MergeFlags& AlternativeTool(bool b = true) { bAlternativeTool = b; return *this; }
	};

	/**
	 * Launches the external merge program if there is one.
	 * \return TRUE if the program could be started
	 */
	static BOOL StartExtMerge(const MergeFlags& flags,
		const CTSVNPath& basefile, const CTSVNPath& theirfile, const CTSVNPath& yourfile, const CTSVNPath& mergedfile,
		const CString& basename = CString(), const CString& theirname = CString(), const CString& yourname = CString(),
		const CString& mergedname = CString());

	/**
	 * Starts the external patch program (currently always TortoiseMerge)
	 */
	static BOOL StartExtPatch(const CTSVNPath& patchfile, const CTSVNPath& dir, 
			const CString& sOriginalDescription = CString(), const CString& sPatchedDescription = CString(), 
			BOOL bReversed = FALSE, BOOL bWait = FALSE);

	/**
	 * Starts the external unified diff viewer (the app associated with *.diff or *.patch files).
	 * If no app is associated with those file types, the default text editor is used.
	 */
	static BOOL StartUnifiedDiffViewer(const CTSVNPath& patchfile, const CString& title, BOOL bWait = FALSE);

	/**
	 * Starts the external diff application
	 */
	static bool StartExtDiff(
		const CTSVNPath& file1, const CTSVNPath& file2, 
		const CString& sName1, const CString& sName2, const DiffFlags& flags,
		int line);

	/**
	 * Starts the external diff application for properties
	 */
	static BOOL StartExtDiffProps(const CTSVNPath& file1, const CTSVNPath& file2, 
			const CString& sName1 = CString(), const CString& sName2 = CString(),
			BOOL bWait = FALSE, BOOL bReadOnly = FALSE);

	/**
	 * Finds the standard application to open / process the given file
	 * with the given verb (see ShellOpen for verbs).
	 * \param fileName file path to pass to the application
	 * \param verb verb to use for the registry lookup. 
	 *        Falls back to "open", if the lookup failed.
	 * \param extension if not empty, use this extension instead the
	 *        of fileName for the registry lookup
	 * \param applySecurityHeuristics if set, the function will not  
	 *        return applications that require additional arguments
	 *        (i.e. if %* or %2 are found in the command line)
	 * \param askUserOnFailure if set and the registry lookup did
	 *        not find anything, let the user select an application
	 *		  via "file open" dialog.
	 * \return application command line to execute. An empty string, 
	 *         if lookup failed.
	 */
	static CString GetAppForFile 
		( const CString& fileName
		, const CString& extension
		, const CString& verb
		, bool applySecurityHeuristics
		, bool askUserOnFailure);

	/**
	 * Launches the standard text viewer/editor application which is associated
	 * with txt files.
	 * \return TRUE if the program could be started.
	 */
	static BOOL StartTextViewer(CString file);

	/**
	 * Checks if the given file has a size of less than four, which means
	 * an 'empty' file or just newlines, i.e. an empty diff.
	 */
	static BOOL CheckForEmptyDiff(const CTSVNPath& sDiffPath);

	/**
	 * Create a font which can is used for log messages, etc
	 */
	static void CreateFontForLogs(CFont& fontToCreate);

	/**
	* Launch an external application (usually the diff viewer)
	*/
	static bool LaunchApplication(
		const CString& sCommandLine, 
		UINT idErrMessageFormat, 
		bool bWaitForStartup,
		bool bWaitForExit = false);

	/**
	* Launch the external blame viewer
	*/
	static bool LaunchTortoiseBlame(
		const CString& sBlameFile, const CString& sLogFile, const CString& sOriginalFile, const CString& sParams = CString(), 
		const SVNRev& startrev = SVNRev(), const SVNRev& endrev = SVNRev());
	
	/**
	 * Resizes all columns in a list control. Considers also icons in columns
	 * with no text.
	 */
	static void ResizeAllListCtrlCols(CListCtrl * pListCtrl);

	/**
	 * Formats text in a rich edit control (version 2).
	 * text in between * chars is formatted bold
	 * text in between ^ chars is formatted italic
	 * text in between _ chars is underlined
	 */
	static bool FormatTextInRichEditControl(CWnd * pWnd);
	static bool UnderlineRegexMatches(CWnd * pWnd, const CString& matchstring, const CString& matchsubstring = _T(".*"));

	static bool FindStyleChars(const CString& sText, TCHAR stylechar, int& start, int& end);

	static bool BrowseRepository(CHistoryCombo& combo, CWnd * pParent, SVNRev& rev, bool multiSelection = false);
	static bool BrowseRepository(const CString& repoRoot, CHistoryCombo& combo, CWnd * pParent, SVNRev& rev);

	static bool FileOpenSave(CString& path, int * filterindex, UINT title, UINT filter, bool bOpen, HWND hwndOwner = NULL);

	static bool SetListCtrlBackgroundImage(HWND hListCtrl, UINT nID, int width = 128, int height = 128);

	/**
	 * guesses a name of the project from a repository URL
	 */
	static 	CString	GetProjectNameFromURL(CString url);

	/**
	 * Replacement for SVNDiff::ShowUnifiedDiff(), but started as a separate process.
	 */
	static bool StartShowUnifiedDiff(HWND hWnd, const CTSVNPath& url1, const SVNRev& rev1, 
									const CTSVNPath& url2, const SVNRev& rev2, 
									const SVNRev& peg = SVNRev(), const SVNRev& headpeg = SVNRev(),
									bool bAlternateDiff = false,
									bool bIgnoreAncestry = false,
                                    bool /* blame */ = false);

	/**
	 * Replacement for SVNDiff::ShowCompare(), but started as a separate process.
	 */
	static bool StartShowCompare(HWND hWnd, const CTSVNPath& url1, const SVNRev& rev1, 
								const CTSVNPath& url2, const SVNRev& rev2, 
								const SVNRev& peg = SVNRev(), const SVNRev& headpeg = SVNRev(),
								bool bAlternateDiff = false, bool ignoreancestry = false,
								bool blame = false, svn_node_kind_t nodekind = svn_node_unknown,
								int line = 0);

	/**
	 * Creates a .lnk file (a windows shortcut file)
	 */
	static HRESULT CreateShortCut(LPCTSTR pszTargetfile, LPCTSTR pszTargetargs, 
								LPCTSTR pszLinkfile, LPCTSTR pszDescription, 
								int iShowmode, LPCTSTR pszCurdir, LPCTSTR pszIconfile, int iIconindex);
	/**
	 * Creates an url shortcut file (.url)
	 */
	static HRESULT CreateShortcutToURL(LPCTSTR pszUrl, LPCTSTR pszLinkFile);

	/**
	 * Sets up all the default diff and merge scripts.
	 * \param force if true, overwrite all existing entries
	 * \param either "Diff", "Merge" or an empty string
	 */
	static bool SetupDiffScripts(bool force, const CString& type);

	/**
	 * Sets the Accessibility property for the specified window.
	 * \param hWnd the handle of the control to set the property for
	 * \param propid the id of the property to set, e.g., PROPID_ACC_DESCRIPTION
	 * \param text the text for the property
	 */
	static bool SetAccProperty(HWND hWnd, MSAAPROPID propid, const CString& text);
	static bool SetAccProperty(HWND hWnd, MSAAPROPID propid, long value);

	/**
	 * finds the accelerator char from a dialog control
	 */
	static TCHAR FindAcceleratorKey(CWnd * pWnd, UINT id);

	static CString GetAbsoluteUrlFromRelativeUrl(const CString& root, const CString& url);

private:
	static CString PickDiffTool(const CTSVNPath& file1, const CTSVNPath& file2);
	static bool GetMimeType(const CTSVNPath& file, CString& mimetype);
	static void SetCharFormat(CWnd* window, DWORD mask, DWORD effects );
	CAppUtils(void);
	~CAppUtils(void);
};
