// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2012, 2014-2016, 2021 - TortoiseSVN

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

#pragma once

#ifdef _MFC_VER
#    include "SVNPrompt.h"
#endif
#include "TSVNPath.h"
#include "SVNRev.h"
#include "SVNBase.h"

#include <deque>

class SVNConflictData
{
public:
    SVNConflictData();

    svn_wc_conflict_kind_t kind;

    CString conflictOld;
    CString conflictNew;
    CString conflictWrk;
    CString prejFile;

    // property conflict data
    std::string propName;
    std::string propValueBase;
    std::string propValueWorking;
    std::string propValueIncomingOld;
    std::string propValueIncomingNew;

    // tree conflict data
    CString                  treeConflictPath;
    svn_node_kind_t          treeConflictNodeKind;
    CString                  treeConflictPropertyName;
    bool                     treeConflictBinary;
    CString                  treeConflictMimeType;
    svn_wc_conflict_action_t treeConflictAction;
    svn_wc_conflict_reason_t treeConflictReason;
    CString                  treeConflictBaseFile;
    CString                  treeConflictTheirFile;
    CString                  treeConflictMyFile;
    CString                  treeConflictMergedFile;
    svn_wc_operation_t       treeConflictOperation;

    CString         srcRightVersionUrl;
    CString         srcRightVersionPath;
    SVNRev          srcRightVersionRev;
    svn_node_kind_t srcRightVersionKind;
    CString         srcLeftVersionUrl;
    CString         srcLeftVersionPath;
    SVNRev          srcLeftVersionRev;
    svn_node_kind_t srcLeftVersionKind;
};

/**
 * \ingroup SVN
 * data object which holds all information returned from an svn_client_info() call.
 */
class SVNInfoData
{
public:
    SVNInfoData();

    CTSVNPath       path;
    CString         url;
    SVNRev          rev;
    svn_node_kind_t kind;
    CString         reposRoot;
    CString         reposUuid;
    SVNRev          lastChangedRev;
    __time64_t      lastChangedTime;
    CString         author;
    CString         wcRoot;

    CString        lockPath;
    CString        lockToken;
    CString        lockOwner;
    CString        lockComment;
    bool           lockDavComment;
    __time64_t     lockCreateTime;
    __time64_t     lockExpirationTime;
    svn_filesize_t size64;

    bool              hasWcInfo;
    svn_wc_schedule_t schedule;
    CString           copyFromUrl;
    SVNRev            copyFromRev;
    __time64_t        textTime;
    CString           checksum;

    CString        changeList;
    svn_depth_t    depth;
    svn_filesize_t workingSize64;

    CString movedToAbspath;
    CString movedFromAbspath;

    std::deque<SVNConflictData> conflicts;
    // convenience methods:

    bool IsValid() const { return rev.IsValid() != FALSE; }
};

/**
 * \ingroup SVN
 * Wrapper for the svn_client_info() API.
 */
class SVNInfo : public SVNBase
{
public:
    SVNInfo(const SVNInfo&) = delete;
    SVNInfo& operator=(SVNInfo&) = delete;
    SVNInfo(bool suppressUI = false);
    ~SVNInfo();

    /**
     * returns the info for the \a path.
     * \param path a path or an url
     * \param pegRev the peg revision to use
     * \param revision the revision to get the info for
     * \param depth how deep to fetch the info
     * \param fetchExcluded if true, also also fetch excluded nodes in the working copy
     * \param fetchActualOnly if true, also fetch nodes that don't exist as versioned but are still tree conflicted
     * for all children of \a path.
     */
    const SVNInfoData* GetFirstFileInfo(const CTSVNPath& path, SVNRev pegRev, SVNRev revision, svn_depth_t depth = svn_depth_empty, bool fetchExcluded = true, bool fetchActualOnly = true, bool includeExternals = false);
    size_t             GetFileCount() const { return m_arInfo.size(); }
    /**
     * Returns the info of the next file in the file list. If no more files are in the list then NULL is returned.
     * See GetFirstFileInfo() for details.
     */
    const SVNInfoData* GetNextFileInfo();

    virtual BOOL Cancel();
    virtual void Receiver(SVNInfoData* data);

    /// convenience methods

    static bool IsFile(const CTSVNPath& path, const SVNRev& revision);

    /**
    * Set the parent window of an authentication prompt dialog
    */
#ifdef _MFC_VER
    void SetPromptParentWindow(HWND hWnd);
#endif

    friend class SVN; // So that SVN can get to our m_err
protected:
    apr_pool_t*              m_pool;   ///< the memory pool
    std::vector<SVNInfoData> m_arInfo; ///< contains all gathered info structs.
private:
    unsigned int m_pos; ///< the current position of the vector

#ifdef _MFC_VER
    SVNPrompt m_prompt;
#endif
    static svn_error_t* cancel(void* baton);
    static svn_error_t* infoReceiver(void* baton, const char* path, const svn_client_info2_t* info, apr_pool_t* pool);
};
