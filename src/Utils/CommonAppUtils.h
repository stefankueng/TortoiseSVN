﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010-2012, 2014, 2016-2018, 2021 - TortoiseSVN

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

/**
 * \ingroup TortoiseProc
 * An utility class with static functions.
 */
class CCommonAppUtils
{
public:
    /**
     * Starts the external unified diff viewer (the app associated with *.diff or *.patch files).
     * If no app is associated with those file types, the default text editor is used.
     */
    static BOOL        StartUnifiedDiffViewer(const CString& patchfile, const CString& title, BOOL bWait = FALSE);

    /**
     * Replaces shell variables (like %SOMENAME%) in \ref s with
     * their respective values. If the replacements should fail,
     * the string will be returned unaltered.
     */
    static CString     ExpandEnvironmentStrings(const CString& s);

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
     * \return application command line to execute. An empty string,
     *         if lookup failed.
     */
    static CString     GetAppForFile(const CString& fileName, const CString& extension, const CString& verb, bool applySecurityHeuristics);

    /**
     * Launches the standard text viewer/editor application which is associated
     * with txt files.
     * \return TRUE if the program could be started.
     */
    static BOOL        StartTextViewer(CString file);

    /**
    * Launch an external application (usually the diff viewer)
    */
    static bool        LaunchApplication(const CString& sCommandLine,
                                         UINT           idErrMessageFormat,
                                         bool           bWaitForStartup,
                                         bool           bWaitForExit = false,
                                         HANDLE         hWaitHandle  = nullptr);

    static bool        RunTortoiseProc(const CString& sCommandLine);

    /**
     * Resizes all columns in a list control. Considers also icons in columns
     * with no text.
     */
    static void        ResizeAllListCtrlCols(CListCtrl* pListCtrl);

    static bool        SetListCtrlBackgroundImage(HWND hListCtrl, UINT nID);
    static bool        SetListCtrlBackgroundImage(HWND hListCtrl, UINT nID, int width, int height);

    /**
     * Creates a .lnk file (a windows shortcut file)
     */
    static HRESULT     CreateShortCut(LPCWSTR pszTargetfile, LPCWSTR pszTargetargs,
                                      LPCWSTR pszLinkfile, LPCWSTR pszDescription,
                                      int iShowmode, LPCWSTR pszCurdir, LPCWSTR pszIconfile, int iIconIndex);
    /**
     * Creates an url shortcut file (.url)
     */
    static HRESULT     CreateShortcutToURL(LPCWSTR pszUrl, LPCWSTR pszLinkFile);

    /**
     * Sets the Accessibility property for the specified window.
     * \param hWnd the handle of the control to set the property for
     * \param propId the id of the property to set, e.g., PROPID_ACC_DESCRIPTION
     * \param text the text for the property
     */
    static bool        SetAccProperty(HWND hWnd, const MSAAPROPID& propId, const CString& text);
    static bool        SetAccProperty(HWND hWnd, const MSAAPROPID& propId, long value);

    /**
     * finds the accelerator char from a dialog control
     */
    static wchar_t     FindAcceleratorKey(CWnd* pWnd, UINT id);

    static CString     GetAbsoluteUrlFromRelativeUrl(const CString& root, const CString& url);

    static void        ExtendControlOverHiddenControl(CWnd* parent, UINT controlToExtend, UINT hiddenControl);

    static bool        FileOpenSave(CString& path, int* filterindex, UINT title, UINT filter, bool bOpen, const CString& initialDir = CString(), HWND hwndOwner = nullptr);

    static bool        AddClipboardUrlToWindow(HWND hWnd);

    static CString     FormatWindowTitle(const CString& urlOrPath, const CString& dialogName);
    static void        SetWindowTitle(HWND hWnd, const CString& urlOrPath, const CString& dialogName);

    static void        MarkWindowAsUnpinnable(HWND hWnd);

    static HRESULT     EnableAutoComplete(HWND hWndEdit, LPWSTR szCurrentWorkingDirectory = nullptr, AUTOCOMPLETELISTOPTIONS acloOptions = ACLO_NONE, AUTOCOMPLETEOPTIONS acoOptions = ACO_AUTOSUGGEST, REFCLSID clsid = CLSID_ACListISF);

    // Wrapper for LoadImage(IMAGE_ICON)
    static HICON       LoadIconEx(UINT resourceId, UINT cx, UINT cy);

    static const char* GetResourceData(const wchar_t* resName, int id, DWORD& resLen);

    static bool        StartHtmlHelp(DWORD_PTR id);

protected:
    CCommonAppUtils(){};
    ~CCommonAppUtils(){};
};
