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

#include "apr_tables.h"

#if defined(_MFC_VER)
// CSTRING is always available in an MFC build
#define CSTRING_AVAILABLE
#endif

/**
* \ingroup Utils
* This class represents a path and the corresponding methods to convert the path
* from/to Subversion/Windows format.
*/
class CTSVNPath
{
public:
	CTSVNPath(void);
	~CTSVNPath(void);
	// Create a TSVNPath object from an unknown path type (same as using SetFromUnknown)
	explicit CTSVNPath(const CString& sUnknownPath);

public:
	/**
	 * Set the path as an UTF8 string with forward slashes
	 */
	void SetFromSVN(const char* pPath);
	void SetFromSVN(const char* pPath, bool bIsDirectory);
	void SetFromSVN(const CString& sPath);
	/**
	 * Set the path as UNICODE with backslashes
	 */
	void SetFromWin(LPCTSTR pPath);
	void SetFromWin(const CString& sPath);
	void SetFromWin(const CString& sPath, bool bIsDirectory);
	/**
	 * Set the path from an unknown source.
	 */
	void SetFromUnknown(const CString& sPath);
	/**
	 * Returns the path in Windows format, i.e. with backslashes
	 */
	LPCTSTR GetWinPath() const;
	/**
	 * Returns the path in Windows format, i.e. with backslashes
	 */
	const CString& GetWinPathString() const;
	/**
	 * Returns the path with forward slashes.
	 */
	const CString& GetSVNPathString() const;
	/**
	 * Returns the path completely prepared to be fed the the SVN APIs
	 * It will be in UTF8, with URLs escaped, if necessary
	 */
	const char* GetSVNApiPath(apr_pool_t *pool) const;
	/**
	 * Returns the path for showing in an UI.
	 *
	 * URL's are returned with forward slashes, unescaped if necessary
	 * Paths are returned with backward slashes
	 */
	const CString& GetUIPathString() const;
	/**
	 * Checks if the path is an URL.
	 */
	bool IsUrl() const;
	/**
	 * Returns true if the path points to a directory
	 */
	bool IsDirectory() const;
	/**
	 * Returns the directory. If the path points to a directory, then the path
	 * is returned unchanged. If the path points to a file, the path to the 
	 * parent directory is returned.
	 */
	CTSVNPath GetDirectory() const;
	/**
	* Returns the the directory which contains the item the path refers to.
	* If the path is a directory, then this returns the directory above it.
	* If the path is to a file, then this returns the directory which contains the path
	* parent directory is returned.
	*/
	CTSVNPath GetContainingDirectory() const;
	/**
	 * Get the 'root path' (e.g. "c:\") - Used to pass to GetDriveType 
	 */
	CString GetRootPathString() const;
	/**
	 * Returns the filename part of the full path.
	 * \remark don't call this for directories.
	 */
	CString GetFilename() const;
	/**
	 * Returns the item's name without the full path.
	 */
	CString GetFileOrDirectoryName() const;
	/**
	 * Returns the item's name without the full path, unescaped if necessary.
	 */
	CString GetUIFileOrDirectoryName() const;
	/**
	 * Returns the file extension, including the dot.
	 * \remark Returns an empty string for directories
	 */
	CString GetFileExtension() const;

	bool IsEmpty() const;
	void Reset();
	/**
	 * Checks if two paths are equal. The slashes are taken care of.
	 */
	bool IsEquivalentTo(const CTSVNPath& rhs) const;
	bool IsEquivalentToWithoutCase(const CTSVNPath& rhs) const;
	bool operator==(const CTSVNPath& x) const {return IsEquivalentTo(x);}
	
	/**
	 * Checks if \c possibleDescendant is a child of this path.
	 */
	bool IsAncestorOf(const CTSVNPath& possibleDescendant) const;
	/**
	 * Get a string representing the file path, optionally with a base 
	 * section stripped off the front
	 * Returns a string with fwdslash paths 
	 */
	LPCTSTR GetDisplayString(const CTSVNPath* pOptionalBasePath = NULL) const;
	/**
	 * Compares two paths. Slash format is irrelevant.
	 */
	static int Compare(const CTSVNPath& left, const CTSVNPath& right);

	/** As PredLeftLessThanRight, but for checking if paths are equivalent 
	 */
	static bool PredLeftEquivalentToRight(const CTSVNPath& left, const CTSVNPath& right);

	/** Checks if the left path is pointing to the same working copy path as the right.
	 * The same wc path means the paths are equivalent once all the admin dir path parts
	 * are removed. This is used in the TSVNCache crawler to filter out all the 'duplicate'
	 * paths to crawl.
	 */
	static bool PredLeftSameWCPathAsRight(const CTSVNPath& left, const CTSVNPath& right);
	
	static bool CheckChild(const CTSVNPath &parent, const CTSVNPath& child);

	/**
	 * appends a string to this path. 
	 *\remark - missing slashes are not added - this is just a string concatenation, but with
	 * preservation of the proper caching behavior.
	 * If you want to join a file- or directory-name onto the path, you should use AppendPathString
	 */
	void AppendRawString(const CString& sAppend);

	/**
	* appends a part of a path to this path. 
	*\remark - missing slashes are dealt with properly. Don't use this to append a file extension, for example
	* 
	*/
	void AppendPathString(const CString& sAppend);

	/**
	 * Get the file modification time - returns zero for files which don't exist
	 * Returns a FILETIME structure cast to an __int64, for easy comparisons
	 */
	__int64 GetLastWriteTime() const;

	/**
	 * Get the file size. Returns zero for directories or files that don't exist.
	 */
	__int64 GetFileSize() const;

	
	bool IsReadOnly() const;
	
	/**
	 * Checks if the path really exists.
	 */
	bool Exists() const;
	
	/**
	 * Deletes the file/folder
	 * \param bTrash if true, uses the Windows trash bin when deleting.
	 */
	bool Delete(bool bTrash) const;

	/**
	 * Checks if a Subversion admin directory is present. For files, the check
	 * is done in the same directory. For folders, it checks if the folder itself
	 * contains an admin directory.
	 */
	bool HasAdminDir() const;
	
	/**
	 * Checks if the path point to or below a Subversion admin directory (.svn).
	 */
	bool IsAdminDir() const;

	void SetCustomData(LPARAM lp) {m_customData = lp;}
	LPARAM GetCustomData() {return m_customData;}

	/**
	 * Checks if the path or URL is valid on Windows.
	 * A path is valid if conforms to the specs in the windows API.
	 * An URL is valid if the path checked out from it is valid
	 * on windows. That means an URL which is valid according to the WWW specs
	 * isn't necessarily valid as a windows path (e.g. http://myserver.com/repos/file:name 
	 * is a valid URL, but the path is illegal on windows ("file:name" is illegal), so
	 * this function would return \c false for that URL).
	 */
	bool IsValidOnWindows() const;

	/**
	 * Checks to see if the path or URL represents one of the special directories
	 * (branches, tags, or trunk).
	 */
	bool IsSpecialDirectory() const;
private:
	// All these functions are const, and all the data
	// is mutable, in order that the hidden caching operations
	// can be carried out on a const CTSVNPath object, which is what's 
	// likely to be passed between functions
	// The public 'SetFromxxx' functions are not const, and so the proper 
	// const-correctness semantics are preserved
	void SetFwdslashPath(const CString& sPath) const;
	void SetBackslashPath(const CString& sPath) const;
	void SetUTF8FwdslashPath(const CString& sPath) const;
	void EnsureBackslashPathSet() const;
	void EnsureFwdslashPathSet() const;
	/**
	 * Checks if two path strings are equal. No conversion of slashes is done!
	 * \remark for slash-independent comparison, use IsEquivalentTo()
	 */
	static bool ArePathStringsEqual(const CString& sP1, const CString& sP2);
	static bool ArePathStringsEqualWithCase(const CString& sP1, const CString& sP2);
	
	/**
	 * Adds the required trailing slash to local root paths such as 'C:'
	 */
	void SanitizeRootPath(CString& sPath, bool bIsForwardPath) const;

	void UpdateAttributes() const;

private:
	mutable CString m_sBackslashPath;
	mutable CString m_sLongBackslashPath;
	mutable CString m_sFwdslashPath;
	mutable CString m_sUIPath;
	mutable	CStringA m_sUTF8FwdslashPath;
	mutable CStringA m_sUTF8FwdslashPathEscaped;
	// Have we yet determined if this is a directory or not?
	mutable bool m_bDirectoryKnown;
	mutable bool m_bIsDirectory;
	mutable bool m_bLastWriteTimeKnown;
	mutable bool m_bURLKnown;
	mutable bool m_bIsURL;
	mutable __int64 m_lastWriteTime;
	mutable __int64 m_fileSize;
	mutable bool m_bIsReadOnly;
	mutable bool m_bHasAdminDirKnown;
	mutable bool m_bHasAdminDir;
	mutable bool m_bIsValidOnWindowsKnown;
	mutable bool m_bIsValidOnWindows;
	mutable bool m_bIsAdminDirKnown;
	mutable bool m_bIsAdminDir;
	mutable bool m_bExists;
	mutable bool m_bExistsKnown;
	mutable LPARAM m_customData;
	mutable bool m_bIsSpecialDirectoryKnown;
	mutable bool m_bIsSpecialDirectory;

	friend bool operator<(const CTSVNPath& left, const CTSVNPath& right);
};

/**
 * Compares two paths and return true if left is earlier in sort order than right
 * (Uses CTSVNPath::Compare logic, but is suitable for std::sort and similar)
 */
 bool operator<(const CTSVNPath& left, const CTSVNPath& right);


//////////////////////////////////////////////////////////////////////////

/**
 * \ingroup Utils
 * This class represents a list of paths
 */
class CTSVNPathList 
{
public:
	CTSVNPathList();
	// A constructor which allows a path list to be easily built with one initial entry in
	explicit CTSVNPathList(const CTSVNPath& firstEntry);

public:
	void AddPath(const CTSVNPath& newPath);
#if defined(_MFC_VER)
	bool LoadFromFile(const CTSVNPath& filename);
	bool WriteToFile(const CString& sFilename, bool bANSI = false) const;
#endif
	/**
	 * Load from the path argument string, when the 'path' parameter is used
	 * This is a list of paths, with '*' between them
	 */
	void LoadFromAsteriskSeparatedString(const CString& sPathString);
	CString CreateAsteriskSeparatedString() const;

	int GetCount() const;
	void Clear();
	CTSVNPath& operator[](INT_PTR index);
	const CTSVNPath& operator[](INT_PTR index) const;
	bool AreAllPathsFiles() const;
	bool AreAllPathsFilesInOneDirectory() const;
	CTSVNPath GetCommonDirectory() const;
	CTSVNPath GetCommonRoot() const;
	void SortByPathname(bool bReverse = false);
	/** 
	 * Delete all the files and opt. directories in the list, then clear the list.
	 * \param bTrash if true, the items are deleted using the Windows trash bin
	 * \param bFilesOnly if true, delete file paths only
	 */
	void DeleteAllPaths(bool bTrash, bool bFilesOnly);
	/** Remove duplicate entries from the list (sorts the list as a side-effect */
	void RemoveDuplicates();
	/** Removes all paths which are on or in a Subversion admin directory */
	void RemoveAdminPaths();
	void RemovePath(const CTSVNPath& path);
	/** 
	 * Removes all child items and leaves only the top folders. Useful if you
	 * create the list to remove them (i.e. if you remove a parent folder, the
	 * child files and folders don't have to be deleted anymore)
	 */
	void RemoveChildren();

	/** Checks if two CTSVNPathLists are the same */
	bool IsEqual(const CTSVNPathList& list);

	/** Convert into the SVN API parameter format */
	apr_array_header_t * MakePathArray (apr_pool_t *pool) const;

	static bool DeleteViaShell(LPCTSTR path, bool useTrashbin);

private:
	typedef std::vector<CTSVNPath> PathVector;
	PathVector m_paths;
	// If the list contains just files in one directory, then
	// this contains the directory name
	mutable CTSVNPath m_commonBaseDirectory;
};
