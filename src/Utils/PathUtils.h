// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2011, 2013-2014, 2018-2021 - TortoiseSVN

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
#include <string>
#if defined(_MFC_VER)
// CSTRING is always available in an MFC build
#    define CSTRING_AVAILABLE
#endif

/**
 * \ingroup Utils
 * helper class to handle path strings.
 */
class CPathUtils
{
public:
    static BOOL MakeSureDirectoryPathExists(LPCWSTR path);
    static void ConvertToBackslash(LPWSTR dest, LPCWSTR src, size_t len);

    /**
     * Returns false if calling \ref Unescape is not necessary.
     */
    static bool ContainsEscapedChars(const char* psz, size_t length);

    /**
     * Replaces escaped sequences with the corresponding characters in a string.
     * \return Position of the terminating \0 char.
     */
    static char* Unescape(char* psz);

    /**
     * Returns the long pathname of a path which may be in 8.3 format.
     */
    static std::wstring GetLongPathname(const std::wstring& path);
    static std::wstring GetLongPathname(LPCWSTR path);

    /**
     * Returns the version string from the VERSION resource of a dll or exe.
     * \param pStrFilename path to the dll or exe
     * \return the version string
     */
    static std::wstring GetVersionFromFile(LPCWSTR pStrFilename);

#ifdef CSTRING_AVAILABLE
    /**
     * Replaces non-URI chars with the corresponding escape sequences.
     */
    static CStringA PathEscape(const CStringA& path);

    /**
     * Returns the path to the applications exe file
     */
    static CString GetAppPath(HMODULE hMod = NULL);

    /**
     * Returns the path to the installation folder, in our case the TortoiseSVN/bin folder.
     * \remark the path returned has a trailing backslash
     */
    static CString GetAppDirectory(HMODULE hMod = NULL);

    /**
     * Returns the path to the installation parent folder, in our case the TortoiseSVN folder.
     * \remark the path returned has a trailing backslash
     */
    static CString GetAppParentDirectory(HMODULE hMod = NULL);

    /**
     * returns the filename of a full path
     */
    static CString GetFileNameFromPath(CString sPath);

    /**
     * returns the file extension from a full path
     */
    static CString GetFileExtFromPath(const CString& sPath);

    /**
     * Return an absolute URL for the given URL. If the
     * latter is already absolute, this function will return
     * it in canonical form. Otherwise, scheme, server,
     * repository or path-relative URLs will be expanded.
     * Returns an empty string upon errors.
     */
    static CStringA GetAbsoluteURL(const CStringA& URL, const CStringA& repositoryRootURL, const CStringA& parentPathURL);

    /**
     * Copies a file or a folder from \a srcPath to \a destpath, creating
     * intermediate folders if necessary. If \a force is TRUE, then files
     * are overwritten if they already exist.
     * Folders are just created at the new location, no files in them get
     * copied.
     */
    static BOOL FileCopy(CString srcPath, CString destPath, BOOL force = TRUE);

    /**
     * parses a string for a path or url. If no path or url is found,
     * an empty string is returned.
     * \remark if more than one path or url is inside the string, only
     * the first one is returned.
     */
    static CString ParsePathInString(const CString& Str);

    /**
     * Returns the path to the application data folder, in our case the %APPDATA%TortoiseSVN folder.
     * \remark the path returned has a trailing backslash
     */
    static CString GetAppDataDirectory();
    static CString GetLocalAppDataDirectory();

    /**
     * Replaces escaped sequences with the corresponding characters in a string.
     */
    static CStringA PathUnescape(const CStringA& path);
    static CStringW PathUnescape(const CStringW& path);

    static CString PathUnescape(const char* path);

    /**
    * Escapes regexp-specific chars.
    */
    static CString PathPatternEscape(const CString& path);
    /**
     * Unescapes regexp-specific chars.
     */
    static CString PathPatternUnEscape(const CString& path);

    /**
     * Combines two url parts, taking care of slashes.
     */
    static CString CombineUrls(CString first, CString second);

    /**
     * Sets the last-write-time of the file to the current time
     */
    static bool Touch(const CString& path);

private:
    static bool DoesPercentNeedEscaping(LPCSTR str);
#endif
};
