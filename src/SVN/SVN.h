// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003 - Tim Kemp and Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#pragma once
#include "atlsimpstr.h"
#include "afxtempl.h"

#include "apr_general.h"
#include "svn_pools.h"
#include "svn_client.h"
#include "svn_sorts.h"
#include "svn_path.h"
#include "svn_wc.h"
#include "svn_utf.h"
#include "svn_repos.h"
#include "svn_string.h"
#include "svn_config.h"
#include "svn_time.h"
#include "svn_subst.h"
#include "svn_auth.h"

#include "PromptDlg.h"

svn_error_t * svn_cl__get_log_message (const char **log_msg,
									const char **tmp_file,
									apr_array_header_t * commit_items,
									void *baton, apr_pool_t * pool);

/**
 * \ingroup TortoiseProc
 * This class provides all Subversion commands as methods and adds some helper
 * methods to the Subversion commands. Also provides virtual methods for the
 * callbacks Subversion uses. This class can't be used directly but must be
 * inherited.
 *
 * \par requirements
 * win95 or later
 * winNT4 or later
 * MFC
 *
 * \version 1.0
 * first version
 *
 * \date 10-20-2002
 *
 * \author kueng
 *
 * \par license
 * This code is absolutely free to use and modify. The code is provided "as is" with
 * no expressed or implied warranty. The author accepts no liability if it causes
 * any damage to your computer, causes your pet to fall ill, increases baldness
 * or makes your car start emitting strange noises when you start it up.
 * This code has no bugs, just undocumented features!
 * 
 * \todo 
 *
 * \bug 
 *
 */
class SVN
{
public:
	SVN(void);
	~SVN(void);

	virtual BOOL Prompt(CString& info, CString prompt, BOOL hide);
	virtual BOOL Cancel();
	virtual BOOL Notify(CString path, svn_wc_notify_action_t action, svn_node_kind_t kind, CString myme_type, svn_wc_notify_state_t content_state, svn_wc_notify_state_t prop_state, LONG rev);
	virtual BOOL Log(LONG rev, CString author, CString date, CString message, CString cpaths);

	/**
	 * If a method of this class returns FALSE then you can
	 * get the detailed error message with this method.
	 * \return the error message string
	 */
	CString GetLastErrorMessage();
	/** 
	 * Checkout a working copy of moduleName at revision, using destPath as the root
	 * directory of the newly checked out working copy
	 * \param moduleName the path/url of the repository
	 * \param destPath the path to the local working copy
	 * \param revision the revision number to check out
	 * \param recurse TRUE if you want to check out all subdirs and files (recommended)
	 */
	BOOL Checkout(CString moduleName, CString destPath, LONG revision, BOOL recurse);
	/**
	 * If path is a URL, use the MESSAGE to immediately attempt 
	 * to commit a deletion of the URL from the repository. 
	 * Else, schedule a working copy path for removal from the repository.
	 * path's parent must be under revision control. This is just a
	 * \b scheduling operation.  No changes will happen to the repository until
	 * a commit occurs.  This scheduling can be removed with
	 * Revert(). If path is a file it is immediately removed from the
	 * working copy. If path is a directory it will remain in the working copy
	 * but all the files, and all unversioned items, it contains will be
	 * removed. If force is not set then this operation will fail if path
	 * contains locally modified and/or unversioned items. If force is set such
	 * items will be deleted.
	 *
	 * \param path the file/directory to delete
	 * \param force if TRUE, all files including those not versioned are deleted. If FALSE the operation
	 * will fail if a directory contains unversioned files or if the file itself is not versioned.
	 */
	BOOL Remove(CString path, BOOL force);
	/**
	 * Reverts a file/directory to its pristine state. I.e. its reverted to the state where it
	 * was last updated with the repository.
	 * \param path the file/directory to revert
	 * \param recurse 
	 */
	BOOL Revert(CString path, BOOL recurse);
	/**
	 * Schedule a working copy path for addition to the repository.
	 * path's parent must be under revision control already, but path is
	 * not. If recursive is set, then assuming path is a directory, all
	 * of its contents will be scheduled for addition as well.
	 * Important:  this is a \b scheduling operation.  No changes will
	 * happen to the repository until a commit occurs.  This scheduling
	 * can be removed with Revert().
	 *
	 * \param path the file/directory to add 
	 * \param recurse 
	 */
	BOOL Add(CString path, BOOL recurse);
	/**
	 * Update working tree path to revision.
	 * \param path the file/directory to update
	 * \param revision the revision the local copy should be updated to or -1 for HEAD
	 * \param recurse 
	 */
	BOOL Update(CString path, LONG revision, BOOL recurse);
	/**
	 * Commit file or directory path into repository, using message as
	 * the log message.
	 * 
	 * If no error is returned and revision is set to -1, then the 
	 * commit was a no-op; nothing needed to be committed.
	 *
	 * If 0 is returned then the commit failed. Use GetLastErrorMessage()
	 * to get detailed error information.
	 *
	 * \remark to specify multiple sources separate the paths with a ";"
	 *
	 * \param path the file/directory to commit
	 * \param message a log message describing the changes you made
	 * \param recurse 
	 * \param revision the resulting revision number. return value.
	 */
	LONG Commit(CString path, CString message, BOOL recurse);
	/**
	 * Copy srcPath to destPath.
	 * 
	 * srcPath must be a file or directory under version control, or the
	 * URL of a versioned item in the repository.  If srcPath is a URL,
	 * revision is used to choose the revision from which to copy the
	 * srcPath. destPath must be a file or directory under version
	 * control, or a repository URL, existent or not.
	 *
	 * If either srcPath or destPath are URLs, immediately attempt 
	 * to commit the copy action in the repository. 
	 * 
	 * If neither srcPath nor destPath is a URL, then this is just a
	 * variant of Add(), where the destPath items are scheduled
	 * for addition as copies.  No changes will happen to the repository
	 * until a commit occurs.  This scheduling can be removed with
	 * Revert().
	 *
	 * \param srcPath source path
	 * \param destPath destination path
	 * \return the new revision number
	 */
	BOOL Copy(CString srcPath, CString destPath, LONG revision);
	/**
	 * Move srcPath to destPath.
	 * 
	 * srcPath must be a file or directory under version control and 
	 * destPath must also be a working copy path (existent or not).
	 * This is a scheduling operation.  No changes will happen to the
	 * repository until a commit occurs.  This scheduling can be removed
	 * with Revert().  If srcPath is a file it is removed from
	 * the working copy immediately.  If srcPath is a directory it will
	 * remain in the working copy but all the files and unversioned items
	 * it contains will be removed.
	 *
	 * If srcPath contains locally modified and/or unversioned items and
	 * force is not set, the copy will fail. If force is set such items
	 * will be removed.
	 * 
	 * \param srcPath source path
	 * \param destPath destination path
	 * \param revision 
	 * \param force 
	 */
	BOOL Move(CString srcPath, CString destPath, BOOL force);
	/**
	 * If path is a URL, use the message to immediately
	 * attempt to commit the creation of the directory URL in the
	 * repository.
	 * Else, create the directory on disk, and attempt to schedule it for
	 * addition.
	 * 
	 * \param path 
	 * \param message 
	 */
	BOOL MakeDir(CString path, CString message);
	/**
	 * Recursively cleanup a working copy directory DIR, finishing any
	 * incomplete operations, removing lockfiles, etc.
	 * \param path the file/directory to clean up
	 */
	BOOL CleanUp(CString path);
	/**
	 * Remove the 'conflicted' state on a working copy PATH.  This will
	 * not semantically resolve conflicts;  it just allows path to be
	 * committed in the future.
	 * If recurse is set, recurse below path, looking for conflicts to
	 * resolve.
	 * If path is not in a state of conflict to begin with, do nothing.
	 * 
	 * \param path the path to resolve
	 * \param recurse 
	 */
	BOOL Resolve(CString path, BOOL recurse);
	/**
	 * Export the contents of either a subversion repository or a subversion 
	 * working copy into a 'clean' directory (meaning a directory with no 
	 * administrative directories).

	 * \param srcPath	either the path the working copy on disk, or a url to the
	 *					repository you wish to export.
	 * \param destPath	the path to the directory where you wish to create the exported tree.
	 * \param revision	the revision that should be exported, which is only used 
	 *					when exporting from a repository.
	 */
	BOOL Export(CString srcPath, CString destPath, LONG revision);
	/**
	 * Switch working tree path to URL at revision
	 *
	 * Summary of purpose: this is normally used to switch a working
	 * directory over to another line of development, such as a branch or
	 * a tag.  Switching an existing working directory is more efficient
	 * than checking out URL from scratch.
	 *
	 * \param path the path of the working directory
	 * \param url the url of the repository
	 * \param revision the revision number to switch to
	 * \param recurse 
	 */
	BOOL Switch(CString path, CString url, LONG revision, BOOL recurse);
	/**
	 * Import file or directory path into repository directory url at
	 * head and using LOG_MSG as the log message for the (implied)
	 * commit.
	 * If path is a directory, the contents of that directory are
	 * imported, under a new directory named newEntry under url; or if
	 * newEntry is null, then the contents of path are imported directly
	 * into the directory identified by url.  Note that the directory path
	 * itself is not imported -- that is, the basename of path is not part
	 * of the import.
	 *
	 * If path is a file, that file is imported as newEntry (which may
	 * not be null).
	 *
	 * In all cases, if newEntry already exists in url, return error.
	 *
	 * \param path		the file/directory to import
	 * \param url		the url to import to
	 * \param newEntry	the new entry created in the repository directory
	 *					identified by URL.
	 * \param message	log message used for the 'commit'
	 * \param recurse 
	 */
	BOOL Import(CString path, CString url, CString newEntry, CString message, BOOL recurse);
	/**
	 * Merge changes from path1/revision1 to path2/revision2 into the
	 * working-copy path localPath.  path1 and path2 can be either
	 * working-copy paths or URLs.
	 *
	 * By "merging", we mean:  apply file differences and schedule 
	 * additions & deletions when appopriate.
	 *
	 * path1 and path2 must both represent the same node kind -- that is,
	 * if path1 is a directory, path2 must also be, and if path1 is a
	 * file, path2 must also be.
	 * 
	 * If recurse is true (and the paths are directories), apply changes
	 * recursively; otherwise, only apply changes in the current
	 * directory.
	 *
	 * If force is not set and the merge involves deleting locally modified or
	 * unversioned items the operation will fail.  If force is set such items
	 * will be deleted.
	 *
	 * \param path1		first path
	 * \param revision1	first revision
	 * \param path2		second path
	 * \param revision2 second revision
	 * \param localPath destination path
	 * \param force		see description
	 * \param recurse 
	 */
	BOOL Merge(CString path1, LONG revision1, CString path2, LONG revision2, CString localPath, BOOL force, BOOL recurse, BOOL ignoreanchestry = FALSE);
	/**
	 * fires the Log-event on each log message from revisionStart
	 * to revisionEnd inclusive (but never fires the event
	 * on a given log message more than once).
	 * To receive the messages you need to listen to Log() events.
	 *
	 * \param path the file/directory to get the log of
	 * \param revisionStart	the revision to start the logs from
	 * \param revisionEnd the revision to stop the logs
	 * \param changed TRUE if the log should follow changed paths 
	 */
	BOOL ReceiveLog(CString path, LONG revisionStart, LONG revisionEnd, BOOL changed);
	
	/**
	 * Checks out a file with \a revision to \a localpath.
	 * \param revision the revision of the file to checkout
	 * \param localpath the place to store the file
	 */
	BOOL Cat(CString url, LONG revision, CString localpath);

	/**
	 * Lists the sub directories under a repository URL. The returned strings start with either
	 * a "d" if the entry is a directory, a "f" if it is a file or a "u" if the type of the 
	 * entry is unknown. So to retrieve only the name of the entry you have to cut off the
	 * first character of the string.
	 * \param url url to the repository you wish to ls.
	 * \param revision	the revision that you want to explore
	 * \param entries CStringArray of subdirectories
	 */
	BOOL Ls(CString url, LONG revision, CStringArray& entries);

	/**
	 * Checks if a windows path is a local repository
	 */
	BOOL IsRepository(const CString& strPath);

	/**
	 * Returns a text representation of an action enum.
	 */
	static CString GetActionText(svn_wc_notify_action_t action, svn_wc_notify_state_t content_state, svn_wc_notify_state_t prop_state);

	/**
	 * Creates a repository at the specified location.
	 * \param path where the repository should be created
	 * \return TRUE if operation was successful
	 */
	static BOOL CreateRepository(CString path);

	/**
	 * Convert Windows Path to Local Repository URL
	 */
	static void PathToUrl(CString &path);

	/**
	 * Convert Local Repository URL to Windows Path
	 */
	static void UrlToPath(CString &url);

	/** 
	 * Returns the path to the text-base file of the working copy file.
	 * If no text base exists for the file then the returned string is empty.
	 */
	static CString GetPristinePath(CString wcPath);

private:
	svn_auth_baton_t *			auth_baton;
	svn_client_ctx_t 			ctx;
	apr_hash_t *				statushash;
	apr_array_header_t *		statusarray;
	svn_wc_status_t *			status;
	apr_pool_t *				parentpool;
	apr_pool_t *				pool;			///< memory pool
	svn_opt_revision_t			rev;			///< subversion revision. used by getRevision()
	svn_error_t *				Err;			///< Global error object struct


	svn_opt_revision_t *	getRevision (long revNumber);
	void * logMessage (const char * message, char * baseDirectory = NULL);
	void	preparePath(CString &path);
	apr_array_header_t * target (LPCTSTR path);
	svn_error_t * get_url_from_target (const char **URL, const char *target);

	static svn_error_t* prompt(char **info, 
					const char *prompt, 
					svn_boolean_t hide, 
					void *baton, 
					apr_pool_t *pool);
	static svn_error_t* cancel(void *baton);
	static void notify( void *baton,
					const char *path,
					svn_wc_notify_action_t action,
					svn_node_kind_t kind,
					const char *mime_type,
					svn_wc_notify_state_t content_state,
					svn_wc_notify_state_t prop_state,
					svn_revnum_t revision);
	static svn_error_t* logReceiver(void* baton, 
					apr_hash_t* ch_paths, 
					svn_revnum_t rev, 
					const char* author, 
					const char* date, 
					const char* msg, 
					apr_pool_t* pool);

public:
	const char *m_username, *m_password;
	void get_simple_provider (svn_auth_provider_object_t **provider, apr_pool_t *pool);

};
	svn_error_t * tsvn_simple_save_creds (svn_boolean_t *saved,
		void *credentials,
		void *provider_baton,
		apr_hash_t *parameters,
		apr_pool_t *pool);
	svn_error_t *
		tsvn_simple_first_creds (void **credentials,
		void **iter_baton,
		void *provider_baton,
		apr_hash_t *parameters,
		const char *realmstring,
		apr_pool_t *pool);

extern const svn_auth_provider_t tsvn_simple_provider;



