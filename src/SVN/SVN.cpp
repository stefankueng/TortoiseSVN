// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Tim Kemp and Stefan Kueng

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
//

#include "StdAfx.h"
#include "TortoiseProc.h"
#include "svn.h"
#include "svn_error_codes.h"
#include "client.h"
#include "UnicodeUtils.h"
#include "DirFileEnum.h"
#include <shlwapi.h>
#include "wininet.h"

SVN::SVN(void) : SVNPrompt()
{
	m_app = NULL;
	hWnd = NULL;
	memset (&ctx, 0, sizeof (ctx));
	parentpool = svn_pool_create(NULL);

	const char * deststr = NULL;
	svn_utf_cstring_to_utf8(&deststr, "dummy", parentpool);
	svn_utf_cstring_from_utf8(&deststr, "dummy", parentpool);

	Err = svn_config_ensure(NULL, parentpool);
	pool = svn_pool_create (parentpool);
	// set up the configuration
	if (Err == 0)
		Err = svn_config_get_config (&(ctx.config), NULL, pool);

	if (Err != 0)
	{
		::MessageBox(NULL, this->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		svn_pool_destroy (pool);
		svn_pool_destroy (parentpool);
		exit(-1);
	} // if (Err != 0) 

	// set up authentication
	Init(pool);

	ctx.log_msg_func = svn_cl__get_log_message;
	ctx.log_msg_baton = logMessage("");
	ctx.notify_func = notify;
	ctx.notify_baton = this;
	ctx.cancel_func = cancel;
	ctx.cancel_baton = this;

	//set up the SVN_SSH param
	CString tsvn_ssh = CRegString(_T("Software\\TortoiseSVN\\SSH"));
	tsvn_ssh.Replace('\\', '/');
	if (!tsvn_ssh.IsEmpty())
	{
		svn_config_t * cfg = (svn_config_t *)apr_hash_get ((apr_hash_t *)ctx.config, SVN_CONFIG_CATEGORY_CONFIG,
			APR_HASH_KEY_STRING);
		svn_config_set(cfg, SVN_CONFIG_SECTION_TUNNELS, "ssh", CUnicodeUtils::GetUTF8(tsvn_ssh));
	}
	//UseIEProxySettings(ctx.config);
}

SVN::~SVN(void)
{
	svn_pool_destroy (parentpool);
}

void SVN::ReleasePool()
{
	svn_pool_clear (pool);
}

BOOL SVN::Cancel() {return FALSE;};
BOOL SVN::Notify(CString path, svn_wc_notify_action_t action, svn_node_kind_t kind, CString myme_type, svn_wc_notify_state_t content_state, svn_wc_notify_state_t prop_state, LONG rev) {return TRUE;};
BOOL SVN::Log(LONG rev, CString author, CString date, CString message, CString& cpaths) {return TRUE;};
BOOL SVN::BlameCallback(LONG linenumber, LONG revision, CString author, CString date, CStringA line) {return TRUE;}

struct log_msg_baton
{
  const char *message;  /* the message. */
  const char *message_encoding; /* the locale/encoding of the message. */
  const char *base_dir; /* the base directory for an external edit. UTF-8! */
  const char *tmpfile_left; /* the tmpfile left by an external edit. UTF-8! */
  apr_pool_t *pool; /* a pool. */
};

CString SVN::GetLastErrorMessage()
{
	CString msg;
	CString temp;
	if (Err != NULL)
	{
		svn_error_t * ErrPtr = Err;
		msg = CUnicodeUtils::GetUnicode(ErrPtr->message);
		while (ErrPtr->child)
		{
			ErrPtr = ErrPtr->child;
			msg += _T("\n");
			msg += CUnicodeUtils::GetUnicode(ErrPtr->message);
		}
		switch (Err->apr_err)
		{
		case SVN_ERR_BAD_FILENAME:
		case SVN_ERR_BAD_URL:
			// please check the path or URL you've entered.
			temp.LoadString(IDS_SVNERR_CHECKPATHORURL);
			break;
		case SVN_ERR_WC_LOCKED:
		case SVN_ERR_WC_NOT_LOCKED:
			// do a "cleanup"
			temp.LoadString(IDS_SVNERR_RUNCLEANUP);
			break;
		case SVN_ERR_WC_NOT_UP_TO_DATE:
		case SVN_ERR_RA_OUT_OF_DATE:
			// do an update first
			temp.LoadString(IDS_SVNERR_UPDATEFIRST);
			break;
		case SVN_ERR_WC_CORRUPT:
		case SVN_ERR_WC_CORRUPT_TEXT_BASE:
			// do a "cleanup". If that doesn't work you need to do a fresh checkout.
			temp.LoadString(IDS_SVNERR_CLEANUPORFRESHCHECKOUT);
			break;
		default:
			break;
		}
		if (!temp.IsEmpty())
		{
			msg += _T("\n") + temp;
		}
		return msg;
	}
	return _T("");
}

BOOL SVN::Checkout(CString moduleName, CString destPath, SVNRev revision, BOOL recurse)
{
	preparePath(destPath);
	preparePath(moduleName);

	Err = svn_client_checkout (	NULL,			// we don't need the resulting revision
								MakeSVNUrlOrPath(moduleName),
								MakeSVNUrlOrPath(destPath),
								revision,
								recurse,
								&ctx,
								pool );

	if(Err != NULL)
	{
		return FALSE;
	}

	UpdateShell(destPath);

	return TRUE;
}

BOOL SVN::Remove(CString path, BOOL force, CString message)
{
	preparePath(path);

	svn_client_commit_info_t *commit_info = NULL;
	message.Replace(_T("\r"), _T(""));
	ctx.log_msg_baton = logMessage(CUnicodeUtils::GetUTF8(message));
	Err = svn_client_delete (&commit_info, target(path), force,
							&ctx,
							pool);
	if(Err != NULL)
	{
		return FALSE;
	}

	if (commit_info && SVN_IS_VALID_REVNUM (commit_info->revision))
		Notify(path, svn_wc_notify_update_completed, svn_node_none, _T(""), svn_wc_notify_state_unknown, svn_wc_notify_state_unknown, commit_info->revision);

	UpdateShell(path);

	return TRUE;
}

BOOL SVN::Revert(CString path, BOOL recurse)
{
	preparePath(path);
	Err = svn_client_revert (target(path), recurse, &ctx, pool);

	if(Err != NULL)
	{
		return FALSE;
	}

	UpdateShell(path);

	return TRUE;
}


BOOL SVN::Add(CString path, BOOL recurse, BOOL force)
{
	preparePath(path);
	Err = svn_client_add2 (MakeSVNUrlOrPath(path), recurse, force, &ctx, pool);

	if(Err != NULL)
	{
		return FALSE;
	}

	UpdateShell(path);

	return TRUE;
}

BOOL SVN::Update(CString path, SVNRev revision, BOOL recurse)
{
	preparePath(path);
	Err = svn_client_update(NULL,
							MakeSVNUrlOrPath(path),
							revision,
							recurse,
							&ctx,
							pool);

	if(Err != NULL)
	{
		return FALSE;
	}

	UpdateShell(path);

	return TRUE;
}

LONG SVN::Commit(CString path, CString message, BOOL recurse)
{
	preparePath(path);
	svn_client_commit_info_t *commit_info = NULL;

	message.Replace(_T("\r"), _T(""));
	ctx.log_msg_baton = logMessage(CUnicodeUtils::GetUTF8(message));
	Err = svn_client_commit (&commit_info, 
							target ((LPCTSTR)path), 
							!recurse,
							&ctx,
							pool);
	ctx.log_msg_baton = logMessage("");
	if(Err != NULL)
	{
		return 0;
	}
	
	if (commit_info && SVN_IS_VALID_REVNUM (commit_info->revision))
		Notify(path, svn_wc_notify_update_completed, svn_node_none, _T(""), svn_wc_notify_state_unknown, svn_wc_notify_state_unknown, commit_info->revision);

	if(commit_info && SVN_IS_VALID_REVNUM (commit_info->revision))
		return commit_info->revision;

	UpdateShell(path);
	return -1;
}

BOOL SVN::Copy(CString srcPath, CString destPath, SVNRev revision, CString logmsg)
{
	preparePath(srcPath);
	preparePath(destPath);
	svn_client_commit_info_t *commit_info = NULL;
	logmsg.Replace(_T("\r"), _T(""));
	if (logmsg.IsEmpty())
		ctx.log_msg_baton = logMessage(CUnicodeUtils::GetUTF8(_T("made a copy")));
	else
		ctx.log_msg_baton = logMessage(CUnicodeUtils::GetUTF8(logmsg));
	Err = svn_client_copy (&commit_info,
							MakeSVNUrlOrPath(srcPath),
							revision,
							MakeSVNUrlOrPath(destPath),
							&ctx,
							pool);
	if(Err != NULL)
	{
		return FALSE;
	}
	if (commit_info && SVN_IS_VALID_REVNUM (commit_info->revision))
		Notify(destPath, svn_wc_notify_update_completed, svn_node_none, _T(""), svn_wc_notify_state_unknown, svn_wc_notify_state_unknown, commit_info->revision);
	return TRUE;
}

BOOL SVN::Move(CString srcPath, CString destPath, BOOL force, CString message, SVNRev rev)
{
	preparePath(srcPath);
	preparePath(destPath);
	svn_client_commit_info_t *commit_info = NULL;
	message.Replace(_T("\r"), _T(""));
	ctx.log_msg_baton = logMessage(CUnicodeUtils::GetUTF8(message));
	Err = svn_client_move (&commit_info,
							MakeSVNUrlOrPath(srcPath),
							rev,
							MakeSVNUrlOrPath(destPath),
							force,
							&ctx,
							pool);
	if(Err != NULL)
	{
		return FALSE;
	}
	if (commit_info && SVN_IS_VALID_REVNUM (commit_info->revision))
		Notify(destPath, svn_wc_notify_update_completed, svn_node_none, _T(""), svn_wc_notify_state_unknown, svn_wc_notify_state_unknown, commit_info->revision);

	UpdateShell(srcPath);
	UpdateShell(destPath);

	return TRUE;
}

BOOL SVN::MakeDir(CString path, CString message)
{
	preparePath(path);

	svn_client_commit_info_t *commit_info = NULL;
	message.Replace(_T("\r"), _T(""));
	ctx.log_msg_baton = logMessage(CUnicodeUtils::GetUTF8(message));
	Err = svn_client_mkdir (&commit_info,
							target(path),
							&ctx,
							pool);
	if(Err != NULL)
	{
		return FALSE;
	}
	if (commit_info && SVN_IS_VALID_REVNUM (commit_info->revision))
		Notify(path, svn_wc_notify_update_completed, svn_node_none, _T(""), svn_wc_notify_state_unknown, svn_wc_notify_state_unknown, commit_info->revision);

	UpdateShell(path);

	return TRUE;
}

BOOL SVN::CleanUp(CString path)
{
	preparePath(path);
	Err = svn_client_cleanup (MakeSVNUrlOrPath(path), &ctx, pool);

	if(Err != NULL)
	{
		return FALSE;
	}

	UpdateShell(path);

	return TRUE;
}

BOOL SVN::Resolve(CString path, BOOL recurse)
{
	preparePath(path);
	Err = svn_client_resolved (MakeSVNUrlOrPath(path),
								recurse,
								&ctx,
								pool);
	if(Err != NULL)
	{
		return FALSE;
	}

	UpdateShell(path);

	return TRUE;
}

BOOL SVN::Export(CString srcPath, CString destPath, SVNRev revision, BOOL force, CProgressDlg * pProgDlg, BOOL extended)
{

	if (revision.IsWorking()&&(pProgDlg))
	{
		// our own "export" function with a callback and the ability to export
		// unversioned items too
		if (extended)
		{
			CDirFileEnum lister1(srcPath);
			DWORD maxval = 0;
			DWORD current = 0;
			// first, count all the items we have to copy
			CString srcfile;
			while (lister1.NextFile(srcfile))
			{
				if (srcfile.Find(_T(SVN_WC_ADM_DIR_NAME))<0)
					maxval++;
			}
			CDirFileEnum lister2(srcPath);
			while (lister2.NextFile(srcfile))
			{
				if (srcfile.Find(_T(SVN_WC_ADM_DIR_NAME))>=0)
					continue;	// exclude everything inside an admin directory
				current++;
				CString destination = destPath + _T("\\") + srcfile.Mid(srcPath.GetLength());
				pProgDlg->SetProgress(current, maxval);
				pProgDlg->SetLine(2, srcfile, TRUE);
				if (CUtils::FileCopy(srcfile, destination, force)==FALSE)
				{
					LPVOID lpMsgBuf;
					if (!FormatMessageA( 
						FORMAT_MESSAGE_ALLOCATE_BUFFER | 
						FORMAT_MESSAGE_FROM_SYSTEM | 
						FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL,
						GetLastError(),
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
						(LPSTR) &lpMsgBuf,
						0,
						NULL ))
					{
						// Handle the error.
						return FALSE;
					}

					Err = svn_error_create(NULL, NULL, (const char *)lpMsgBuf);
					// Free the buffer.
					LocalFree( lpMsgBuf );
					return FALSE;
				}
				if (pProgDlg->HasUserCancelled())
				{
					Err = svn_error_create(NULL, NULL, "user cancelled");
					return FALSE;
				}
			} // while (lister2.NextFile(srcfile))
		}
		else
		{
			const TCHAR * strbuf = NULL;
			svn_wc_status_t * s;
			SVNStatus status;
			if (s = status.GetFirstFileStatus(srcPath, &strbuf))
			{
				DWORD maxval = status.GetVersionedCount();
				DWORD current = 0;
				if (SVNStatus::GetMoreImportant(s->text_status, svn_wc_status_unversioned)!=svn_wc_status_unversioned)
				{
					current++;
					CString src = strbuf;
					CString destination = destPath + _T("\\") + src.Mid(srcPath.GetLength());
					pProgDlg->SetProgress(current, maxval);
					pProgDlg->SetLine(2, src, TRUE);
					if (CUtils::FileCopy(src, destination, force)==FALSE)
					{
						LPVOID lpMsgBuf;
						if (!FormatMessageA( 
							FORMAT_MESSAGE_ALLOCATE_BUFFER | 
							FORMAT_MESSAGE_FROM_SYSTEM | 
							FORMAT_MESSAGE_IGNORE_INSERTS,
							NULL,
							GetLastError(),
							MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
							(LPSTR) &lpMsgBuf,
							0,
							NULL ))
						{
							// Handle the error.
							return FALSE;
						}

						Err = svn_error_create(NULL, NULL, (const char *)lpMsgBuf);
						// Free the buffer.
						LocalFree( lpMsgBuf );
						return FALSE;
					}
				}
				while (s = status.GetNextFileStatus(&strbuf))
				{
					if ((s->text_status == svn_wc_status_unversioned)||
						(s->text_status == svn_wc_status_ignored)||
						(s->text_status == svn_wc_status_none))
						continue;
					current++;
					if ((s->text_status == svn_wc_status_missing)||
						(s->text_status == svn_wc_status_deleted))
						continue;
					
					CString src = strbuf;
					CString destination = destPath + _T("\\") + src.Mid(srcPath.GetLength());
					pProgDlg->SetProgress(current, maxval);
					pProgDlg->SetLine(2, src, TRUE);
					if (CUtils::FileCopy(src, destination, force)==FALSE)
					{
						LPVOID lpMsgBuf;
						if (!FormatMessageA( 
							FORMAT_MESSAGE_ALLOCATE_BUFFER | 
							FORMAT_MESSAGE_FROM_SYSTEM | 
							FORMAT_MESSAGE_IGNORE_INSERTS,
							NULL,
							GetLastError(),
							MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
							(LPSTR) &lpMsgBuf,
							0,
							NULL ))
						{
							// Handle the error.
							return FALSE;
						}

						Err = svn_error_create(NULL, NULL, (const char *)lpMsgBuf);
						// Free the buffer.
						LocalFree( lpMsgBuf );
						return FALSE;
					}
					if (pProgDlg->HasUserCancelled())
					{
						Err = svn_error_create(NULL, NULL, "user cancelled");
						return FALSE;
					}
				} // while (s = status.GetNextFileStatus(&strbuf))
			} // if (s = status.GetFirstFileStatus(srcPath, &strbuf))
			else
			{
				Err = svn_error_create(status.m_err->apr_err, status.m_err, NULL);
				return FALSE;
			}
		} // else from if (extended)
	}
	else
	{

		preparePath(srcPath);
		preparePath(destPath);

		Err = svn_client_export2(NULL,		//no resulting revision needed
			MakeSVNUrlOrPath(srcPath),
			MakeSVNUrlOrPath(destPath),
			revision,
			force,
			NULL,
			&ctx,
			pool);
		if(Err != NULL)
		{
			return FALSE;
		}
	}
	return TRUE;
}

BOOL SVN::Switch(CString path, CString url, SVNRev revision, BOOL recurse)
{
	preparePath(path);
	preparePath(url);

	Err = svn_client_switch(NULL,
							MakeSVNUrlOrPath(path),
							MakeSVNUrlOrPath(url),
							revision,
							recurse,
							&ctx,
							pool);
	if(Err != NULL)
	{
		return FALSE;
	}
	
	UpdateShell(path);

	return TRUE;
}

BOOL SVN::Import(CString path, CString url, CString message, BOOL recurse)
{
	preparePath(path);
	preparePath(url);

	svn_client_commit_info_t *commit_info = NULL;
	message.Replace(_T("\r"), _T(""));
	ctx.log_msg_baton = logMessage(CUnicodeUtils::GetUTF8(message));
	Err = svn_client_import (&commit_info,
							MakeSVNUrlOrPath(path),
							MakeSVNUrlOrPath(url),
							!recurse,
							&ctx,
							pool);
	ctx.log_msg_baton = logMessage("");
	if(Err != NULL)
	{
		return FALSE;
	}
	if (commit_info && SVN_IS_VALID_REVNUM (commit_info->revision))
		Notify(path, svn_wc_notify_update_completed, svn_node_none, _T(""), svn_wc_notify_state_unknown, svn_wc_notify_state_unknown, commit_info->revision);
	return TRUE;
}

BOOL SVN::Merge(CString path1, SVNRev revision1, CString path2, SVNRev revision2, CString localPath, BOOL force, BOOL recurse, BOOL ignoreanchestry)
{
	preparePath(path1);
	preparePath(path2);
	preparePath(localPath);

	Err = svn_client_merge (MakeSVNUrlOrPath(path1),
							revision1,
							MakeSVNUrlOrPath(path2),
							revision2,
							MakeSVNUrlOrPath(localPath),
							recurse,
							ignoreanchestry,
							force,
							false,		//no 'dry-run'
							&ctx,
							pool);
	if(Err != NULL)
	{
		return FALSE;
	}

	UpdateShell(localPath);

	return TRUE;
}

BOOL SVN::PegMerge(CString source, SVNRev revision1, SVNRev revision2, SVNRev pegrevision, CString destpath, BOOL force, BOOL recurse, BOOL ignoreancestry)
{
	preparePath(source);
	preparePath(destpath);

	Err = svn_client_merge_peg (MakeSVNUrlOrPath(source),
		revision1,
		revision2,
		pegrevision,
		MakeSVNUrlOrPath(destpath),
		recurse,
		ignoreancestry,
		force,
		false,		//no 'dry-run'
		&ctx,
		pool);
	if(Err != NULL)
	{
		return FALSE;
	}

	UpdateShell(destpath);

	return TRUE;
}

BOOL SVN::Diff(CString path1, SVNRev revision1, CString path2, SVNRev revision2, BOOL recurse, BOOL ignoreancestry, BOOL nodiffdeleted, CString options, CString outputfile, CString errorfile)
{
	BOOL del = FALSE;
	apr_file_t * outfile;
	apr_file_t * errfile;
	apr_array_header_t *opts;

	apr_pool_t * localpool = svn_pool_create(pool);

	opts = svn_cstring_split (CUnicodeUtils::GetUTF8(options), " \t\n\r", TRUE, localpool);

	preparePath(path1);
	preparePath(path2);
	preparePath(outputfile);
	preparePath(errorfile);

	Err = svn_io_file_open (&outfile, MakeSVNUrlOrPath(outputfile),
							APR_WRITE | APR_CREATE | APR_TRUNCATE | APR_BINARY,
							APR_OS_DEFAULT, localpool);
	if (Err)
		return FALSE;

	if (errorfile.IsEmpty())
	{
		TCHAR path[MAX_PATH];
		TCHAR tempF[MAX_PATH];
		::GetTempPath (MAX_PATH, path);
		::GetTempFileName (path, _T("svn"), 0, tempF);
		errorfile = CString(tempF);
		del = TRUE;
	}

	Err = svn_io_file_open (&errfile, MakeSVNUrlOrPath(errorfile),
							APR_WRITE | APR_CREATE | APR_TRUNCATE | APR_BINARY,
							APR_OS_DEFAULT, localpool);
	if (Err)
		return FALSE;

	Err = svn_client_diff (opts,
						   MakeSVNUrlOrPath(path1),
						   revision1,
						   MakeSVNUrlOrPath(path2),
						   revision2,
						   recurse,
						   ignoreancestry,
						   nodiffdeleted,
						   outfile,
						   errfile,
						   &ctx,
						   localpool);
	if (Err)
	{
		svn_pool_clear(localpool);
		return FALSE;
	}
	if (del)
	{
		svn_io_remove_file (MakeSVNUrlOrPath(errorfile), localpool);
	}
	svn_pool_clear(localpool);
	return TRUE;
}

BOOL SVN::PegDiff(CString path, SVNRev pegrevision, SVNRev startrev, SVNRev endrev, BOOL recurse, BOOL ignoreancestry, BOOL nodiffdeleted, CString options, CString outputfile, CString errorfile)
{
	BOOL del = FALSE;
	apr_file_t * outfile;
	apr_file_t * errfile;
	apr_array_header_t *opts;

	apr_pool_t * localpool = svn_pool_create(pool);

	opts = svn_cstring_split (CUnicodeUtils::GetUTF8(options), " \t\n\r", TRUE, localpool);

	preparePath(path);
	preparePath(outputfile);
	preparePath(errorfile);

	Err = svn_io_file_open (&outfile, MakeSVNUrlOrPath(outputfile),
		APR_WRITE | APR_CREATE | APR_TRUNCATE | APR_BINARY,
		APR_OS_DEFAULT, localpool);
	if (Err)
		return FALSE;

	if (errorfile.IsEmpty())
	{
		TCHAR path[MAX_PATH];
		TCHAR tempF[MAX_PATH];
		::GetTempPath (MAX_PATH, path);
		::GetTempFileName (path, _T("svn"), 0, tempF);
		errorfile = CString(tempF);
		del = TRUE;
	}

	Err = svn_io_file_open (&errfile, MakeSVNUrlOrPath(errorfile),
		APR_WRITE | APR_CREATE | APR_TRUNCATE | APR_BINARY,
		APR_OS_DEFAULT, localpool);
	if (Err)
		return FALSE;

	Err = svn_client_diff_peg (opts,
		MakeSVNUrlOrPath(path),
		pegrevision,
		startrev,
		endrev,
		recurse,
		ignoreancestry,
		nodiffdeleted,
		outfile,
		errfile,
		&ctx,
		localpool);
	if (Err)
	{
		svn_pool_clear(localpool);
		return FALSE;
	}
	if (del)
	{
		svn_io_remove_file (MakeSVNUrlOrPath(errorfile), localpool);
	}
	svn_pool_clear(localpool);
	return TRUE;
}

BOOL SVN::ReceiveLog(CString path, SVNRev revisionStart, SVNRev revisionEnd, BOOL changed, BOOL strict /* = FALSE */)
{
	preparePath(path);
	Err = svn_client_log (target ((LPCTSTR)path), 
						revisionStart, 
						revisionEnd, 
						changed,
						strict,
						logReceiver,	// log_message_receiver
						(void *)this, &ctx, pool);

	if(Err != NULL)
	{
		return FALSE;
	}
	return TRUE;
}

BOOL SVN::Cat(CString url, SVNRev revision, CString localpath)
{
	apr_file_t * file;
	svn_stream_t * stream;

	preparePath(url);

	if (PathIsDirectory(localpath))
	{
		localpath += _T("\\");
		localpath += url.Mid(url.ReverseFind('/'));
	}
	DeleteFile(localpath);

	preparePath(localpath);
	apr_file_open(&file, MakeSVNUrlOrPath(localpath), APR_WRITE | APR_CREATE, APR_OS_DEFAULT, pool);
	stream = svn_stream_from_aprfile(file, pool);

	Err = svn_client_cat(stream, MakeSVNUrlOrPath(url), revision, &ctx, pool);

	apr_file_close(file);
	if (Err != NULL)
		return FALSE;
	return TRUE;
}

CString SVN::GetActionText(svn_wc_notify_action_t action, svn_wc_notify_state_t content_state, svn_wc_notify_state_t prop_state)
{
	CString temp = _T(" ");
	switch (action)
	{
		case svn_wc_notify_add:
		case svn_wc_notify_update_add:
		case svn_wc_notify_commit_added:
			temp.LoadString(IDS_SVNACTION_ADD);
			break;
		case svn_wc_notify_copy:
			temp.LoadString(IDS_SVNACTION_COPY);
			break;
		case svn_wc_notify_delete:
		case svn_wc_notify_update_delete:
		case svn_wc_notify_commit_deleted:
			temp.LoadString(IDS_SVNACTION_DELETE);
			break;
		case svn_wc_notify_restore:
			temp.LoadString(IDS_SVNACTION_RESTORE);
			break;
		case svn_wc_notify_revert:
			temp.LoadString(IDS_SVNACTION_REVERT);
			break;
		case svn_wc_notify_resolved:
			temp.LoadString(IDS_SVNACTION_RESOLVE);
			break;
		case svn_wc_notify_update_update:
			if ((content_state == svn_wc_notify_state_conflicted) || (prop_state == svn_wc_notify_state_conflicted))
				temp.LoadString(IDS_STATUSCONFLICTED);
			else if ((content_state == svn_wc_notify_state_merged) || (prop_state == svn_wc_notify_state_merged))
				temp.LoadString(IDS_STATUSMERGED);
			else
				temp.LoadString(IDS_SVNACTION_UPDATE);
			break;
		case svn_wc_notify_update_completed:
			temp.LoadString(IDS_SVNACTION_COMPLETED);
			break;
		case svn_wc_notify_update_external:
			temp.LoadString(IDS_SVNACTION_EXTERNAL);
			break;
		case svn_wc_notify_commit_modified:
			temp.LoadString(IDS_SVNACTION_MODIFIED);
			break;
		case svn_wc_notify_commit_replaced:
			temp.LoadString(IDS_SVNACTION_REPLACED);
			break;
		case svn_wc_notify_commit_postfix_txdelta:
			temp.LoadString(IDS_SVNACTION_POSTFIX);
			break;
		case svn_wc_notify_failed_revert:
			temp.LoadString(IDS_SVNACTION_FAILEDREVERT);
			break;
		case svn_wc_notify_status_completed:
		case svn_wc_notify_status_external:
			temp.LoadString(IDS_SVNACTION_STATUS);
			break;
		case svn_wc_notify_skip:
			temp.LoadString(IDS_SVNACTION_SKIP);
			break;
		default:
			return _T("???");
	}
	return temp;
}

BOOL SVN::CreateRepository(CString path, CString fstype)
{
	apr_pool_t * localpool;
	svn_repos_t * repo;
	svn_error_t * err;
	apr_hash_t *config;
	localpool = svn_pool_create (NULL);

	const char * deststr = NULL;
	svn_utf_cstring_to_utf8(&deststr, "dummy", localpool);
	svn_utf_cstring_from_utf8(&deststr, "dummy", localpool);
	
	apr_hash_t *fs_config = apr_hash_make (localpool);;

	apr_hash_set (fs_config, SVN_FS_CONFIG_BDB_TXN_NOSYNC,
		APR_HASH_KEY_STRING, "0");

	apr_hash_set (fs_config, SVN_FS_CONFIG_BDB_LOG_AUTOREMOVE,
		APR_HASH_KEY_STRING, "1");

	err = svn_config_get_config (&config, NULL, localpool);
	if (err != NULL)
	{
		svn_pool_destroy(localpool);
		return FALSE;
	}
	const char * fs_type = apr_pstrdup(localpool, CStringA(fstype));
	apr_hash_set (fs_config, SVN_FS_CONFIG_FS_TYPE,
		APR_HASH_KEY_STRING,
		fs_type);
	err = svn_repos_create(&repo, MakeSVNUrlOrPath(path), NULL, NULL, config, fs_config, localpool);
	if (err != NULL)
	{
		svn_pool_destroy(localpool);
		return FALSE;
	} // if (err != NULL) 
	svn_pool_destroy(localpool);
	return TRUE;
}

BOOL SVN::Blame(CString path, SVNRev startrev, SVNRev endrev)
{
	preparePath(path);
	Err = svn_client_blame ( MakeSVNUrlOrPath(path),
							 startrev,  
							 endrev,  
							 blameReceiver,  
							 (void *)this,  
							 &ctx,  
							 pool );

	if(Err != NULL)
	{
		return FALSE;
	}
	return TRUE;
}

svn_error_t* SVN::blameReceiver(void* baton,
								apr_off_t line_no,
								svn_revnum_t revision,
								const char * author,
								const char * date,
								const char * line,
								apr_pool_t * pool)
{
	svn_error_t * error = NULL;
	CString author_native;
	CStringA line_native;
	TCHAR date_native[MAX_PATH] = {0};

	SVN * svn = (SVN *)baton;

	if (author)
		author_native = CUnicodeUtils::GetUnicode(author);
	if (line)
		line_native = line;

	if (date && date[0])
	{
		//Convert date to a format for humans.
		apr_time_t time_temp;

		error = svn_time_from_cstring (&time_temp, date, pool);
		if (error)
			return error;

		formatDate(date_native, time_temp, true);
	}
	else
		_tcscat(date_native, _T("(no date)"));

	if (!svn->BlameCallback((LONG)line_no, revision, author_native, date_native, line_native))
	{
		return svn_error_create(SVN_ERR_CANCELLED, NULL, "error in blame callback");
	}
	return error;
}

svn_error_t* SVN::logReceiver(void* baton, 
								apr_hash_t* ch_paths, 
								svn_revnum_t rev, 
								const char* author, 
								const char* date, 
								const char* msg, 
								apr_pool_t* pool)
{
	svn_error_t * error = NULL;
	TCHAR date_native[MAX_PATH] = {0};
	stdstring author_native;
	stdstring msg_native;

	SVN * svn = (SVN *)baton;
	author_native = CUnicodeUtils::GetUnicode(author);

	if (date && date[0])
	{
		//Convert date to a format for humans.
		apr_time_t time_temp;

		error = svn_time_from_cstring (&time_temp, date, pool);
		if (error)
			return error;

		formatDate(date_native, time_temp);
	}
	else
		_tcscat(date_native, _T("(no date)"));

	if (msg == NULL)
		msg = "";

	msg_native = CUnicodeUtils::GetUnicode(msg);

	CString cpaths;
	try
	{
		cpaths.Preallocate(1000000);		//allocate 1M memory
		if (ch_paths)
		{
			apr_array_header_t *sorted_paths;
			sorted_paths = svn_sort__hash(ch_paths, svn_sort_compare_items_as_paths, pool);
			for (int i = 0; i < sorted_paths->nelts; i++)
			{
				svn_sort__item_t *item = &(APR_ARRAY_IDX (sorted_paths, i, svn_sort__item_t));
				stdstring path_native;
				const char *path = (const char *)item->key;
				svn_log_changed_path_t *log_item = (svn_log_changed_path_t *)apr_hash_get (ch_paths, item->key, item->klen);
				path_native = MakeUIUrlOrPath(path);
				CString temp;
				switch (log_item->action)
				{
				case 'M':
					temp.LoadString(IDS_SVNACTION_MODIFIED);
					break;
				case 'R':
					temp.LoadString(IDS_SVNACTION_REPLACED);
					break;
				case 'A':
					temp.LoadString(IDS_SVNACTION_ADD);
					break;
				case 'D':
					temp.LoadString(IDS_SVNACTION_DELETE);
				default:
					break;
				} // switch (temppath->action)
				if (!cpaths.IsEmpty())
					cpaths += _T("\r\n");
				cpaths += temp;
				cpaths += _T("  ");
				cpaths += path_native.c_str();
				if (log_item->copyfrom_path && SVN_IS_VALID_REVNUM (log_item->copyfrom_rev))
				{
					CString copyfrompath = MakeUIUrlOrPath(log_item->copyfrom_path);
					CString copyfromrev;
					copyfromrev.Format(_T(" (from %s:%ld)"), copyfrompath, log_item->copyfrom_rev);
					cpaths += copyfromrev;
				}
				if (i == 500)
				{
					temp.Format(_T("\r\n.... and %d more ..."), sorted_paths->nelts - 500);
					cpaths += temp;
					break;
				}
			} // for (int i = 0; i < sorted_paths->nelts; i++) 
		} // if (changed_paths)
	}
	catch (CMemoryException * e)
	{
		cpaths = _T("Memory Exception!");
		e->Delete();
	}
	cpaths.FreeExtra();
	SVN_ERR (svn->cancel(baton));

	if (svn->Log(rev, CString(author_native.c_str()), CString(date_native), CString(msg_native.c_str()), cpaths))
	{
		return error;
	}
	return error;
}

void SVN::notify( void *baton,
					const char *path,
					svn_wc_notify_action_t action,
					svn_node_kind_t kind,
					const char *mime_type,
					svn_wc_notify_state_t content_state,
					svn_wc_notify_state_t prop_state,
					svn_revnum_t revision)
{
	SVN * svn = (SVN *)baton;
	WCHAR buf[MAX_PATH*4];
	if (!MultiByteToWideChar(CP_UTF8, 0, path, -1, buf, MAX_PATH*4))
		buf[0] = 0;
	CString mime;
	if (mime_type)
		mime = CUnicodeUtils::GetUnicode(mime_type);
	svn->Notify(CString(buf), (svn_wc_notify_action_t)action, kind, mime, content_state, prop_state, revision);
}

svn_error_t* SVN::cancel(void *baton)
{
	SVN * svn = (SVN *)baton;
	if (svn->Cancel())
	{
		CStringA temp;
		temp.LoadString(IDS_SVN_USERCANCELLED);
		return svn_error_create(SVN_ERR_CANCELLED, NULL, temp);
	}
	return SVN_NO_ERROR;
}

void * SVN::logMessage (const char * message, char * baseDirectory)
{
	log_msg_baton* baton = (log_msg_baton *) apr_palloc (pool, sizeof (*baton));
	baton->message = apr_pstrdup(pool, message);
	baton->base_dir = baseDirectory ? baseDirectory : "";

	baton->message_encoding = NULL;
	baton->tmpfile_left = NULL;
	baton->pool = pool;

	return baton;
}

void SVN::PathToUrl(CString &path)
{
	path.Trim();
	// convert \ to /
	path.Replace('\\','/');
	// prepend file:///
	if (path.GetAt(0) == '/')
		path.Insert(0, _T("file://"));
	else
		path.Insert(0, _T("file:///"));
	path.TrimRight(_T("/\\"));			//remove trailing slashes
}

void SVN::UrlToPath(CString &url)
{
	//we have to convert paths like file:///c:/myfolder
	//to c:/myfolder
	//and paths like file:////mymachine/c/myfolder
	//to //mymachine/c/myfolder
	url.Trim();
	url.Replace('\\','/');
	url = url.Mid(7);
	if (url.GetAt(1) != '/')
		url = url.Mid(1);
	url.Replace('/','\\');
	url.TrimRight(_T("/\\"));			//remove trailing slashes
}

void	SVN::preparePath(CString &path)
{
	path.Trim();
	path.TrimRight(_T("/\\"));			//remove trailing slashes
	path.Replace('\\','/');
	if ((path.Left(7).CompareNoCase(_T("http://"))==0) ||
		(path.Left(8).CompareNoCase(_T("https://"))==0) ||
		(path.Left(6).CompareNoCase(_T("svn://"))==0) ||
		(path.Left(10).CompareNoCase(_T("svn-ssh://"))==0))
	{
		TCHAR buf[8192];
		DWORD len = 8192;
		UrlCanonicalize(path, buf, &len, URL_ESCAPE_UNSAFE);
		path = CString(buf);
	}
}

svn_error_t* svn_cl__get_log_message (const char **log_msg,
									const char **tmp_file,
									apr_array_header_t * commit_items,
									void *baton, 
									apr_pool_t * pool)
{
	log_msg_baton *lmb = (log_msg_baton *) baton;
	*tmp_file = NULL;
	if (lmb->message)
	{
		*log_msg = apr_pstrdup (pool, lmb->message);
	}

	return SVN_NO_ERROR;
}

apr_array_header_t * SVN::target (LPCTSTR path)
{
	CString p = CString(path);
	apr_array_header_t *targets = apr_array_make (pool,
												5,
												sizeof (const char *));

	int curPos = 0;
	int curPosOld = 0;
	CString targ;
	while (p.Find('*', curPos)>=0)
	{
		curPosOld = curPos;
		curPos = p.Find('*', curPosOld);
		targ = p.Mid(curPosOld, curPos-curPosOld);
		targ.Trim(_T("*"));
		const char * target = apr_pstrdup (pool, MakeSVNUrlOrPath(targ));
		(*((const char **) apr_array_push (targets))) = target;
		curPos++;
	} // while (p.Find('*', curPos)>=0) 
	targ = p.Mid(curPos);
	targ.Trim(_T("*"));
	const char * target = apr_pstrdup (pool, MakeSVNUrlOrPath(targ));
	(*((const char **) apr_array_push (targets))) = target;

	return targets;
}

CString SVN::GetURLFromPath(CString path)
{
	preparePath(path);
	const char * URL;
	Err = get_url_from_target(&URL, MakeSVNUrlOrPath(path));
	if (Err)
		return _T("");
	if (URL==NULL)
		return _T("");
	CString ret = MakeUIUrlOrPath(URL);
	return ret;
}

svn_error_t * SVN::get_url_from_target (const char **URL, const char *target)
{
	svn_wc_adm_access_t *adm_access;          
	const svn_wc_entry_t *entry;  
	svn_boolean_t is_url = svn_path_is_url (target);

	if (is_url)
		*URL = apr_pstrdup(pool, target);

	else
	{
		SVN_ERR (svn_wc_adm_probe_open2 (&adm_access, NULL, target,
			FALSE, 0, pool));
		SVN_ERR (svn_wc_entry (&entry, target, adm_access, FALSE, pool));
		SVN_ERR (svn_wc_adm_close (adm_access));

		*URL = entry ? entry->url : NULL;
	}

	return SVN_NO_ERROR;
}

BOOL SVN::Ls(CString url, SVNRev revision, CStringArray& entries, BOOL extended, BOOL recursive)
{
	entries.RemoveAll();

	preparePath(url);

	apr_hash_t* hash = apr_hash_make(pool);

	Err = svn_client_ls(&hash, 
						MakeSVNUrlOrPath(url),
						revision,
						recursive, 
						&ctx,
						pool);
	if (Err != NULL)
		return FALSE;

	apr_hash_index_t *hi;
    svn_dirent_t* val;
	const char* key;
    for (hi = apr_hash_first(pool, hash); hi; hi = apr_hash_next(hi)) 
	{
        apr_hash_this(hi, (const void**)&key, NULL, (void**)&val);
		CString temp;
		if (val->kind == svn_node_dir)
			temp = "d";
		else if (val->kind == svn_node_file)
			temp = "f";
		else
			temp = "u";
		temp = temp + MakeUIUrlOrPath(key);
		if (extended)
		{
			CString author, revnum, size, dateval;
			TCHAR date_native[_MAX_PATH];
			author = CUnicodeUtils::GetUnicode(val->last_author);
			revnum.Format(_T("%u"), val->created_rev);
			if (val->kind != svn_node_dir)
				size.Format(_T("%u KB"), (val->size+1023)/1024);
			formatDate(date_native, val->time, true);
			dateval.Format(_T("%I64u"), val->time);
			temp = temp + _T("\t") + revnum + _T("\t") + author + _T("\t") + size + _T("\t") + date_native + _T("\t") + dateval;;
		}
		entries.Add(temp);
    }
	return Err == NULL;
}

BOOL SVN::Relocate(CString path, CString from, CString to, BOOL recurse)
{
	preparePath(path);
	preparePath(from);
	preparePath(to);
	Err = svn_client_relocate(MakeSVNUrlOrPath(path), MakeSVNUrlOrPath(from), MakeSVNUrlOrPath(to), recurse, &ctx, pool);
	if (Err != NULL)
		return FALSE;
	return TRUE;
}

BOOL SVN::IsRepository(const CString& strUrl)
{
	svn_repos_t* pRepos;
	CString url = strUrl;
	preparePath(url);
	url += _T("/");
	int pos = url.GetLength();
	while ((pos = url.ReverseFind('/'))>=0)
	{
		url = url.Left(pos);
		Err = svn_repos_open (&pRepos, MakeSVNUrlOrPath(url), pool);
		if ((Err)&&(Err->apr_err == SVN_ERR_FS_BERKELEY_DB))
			return TRUE;
		if (Err == NULL)
			return TRUE;
	}

	return Err == NULL;
}

CString SVN::GetRepositoryRoot(CString url)
{
	svn_ra_plugin_t *ra_lib;
	void *ra_baton, *session;
	const char * returl;
	CString retURL;
	url.Replace('\\', '/');
	CStringA urla = MakeSVNUrlOrPath(url);

	apr_pool_t * localpool = svn_pool_create(pool);
	/* Get the RA library that handles URL. */
	if (svn_ra_init_ra_libs (&ra_baton, localpool))
		return _T("");
	if (svn_ra_get_ra_library (&ra_lib, ra_baton, urla, localpool))
		return _T("");

	/* Open a repository session to the URL. */
	if (svn_client__open_ra_session (&session, ra_lib, urla, NULL, NULL, NULL, FALSE, FALSE, &ctx, localpool))
		return _T("");

	if (ra_lib->get_repos_root(session, &returl, localpool))
		return _T("");
	retURL = CString(returl);
	svn_pool_clear(localpool);
	return retURL;
}

LONG SVN::GetHEADRevision(CString url)
{
	svn_ra_plugin_t *ra_lib;
	void *ra_baton, *session;
	const char * urla;
	LONG rev;
	url.Replace('\\', '/');
	CStringA tempurl = MakeSVNUrlOrPath(url);

	apr_pool_t * localpool = svn_pool_create(pool);
	if (!svn_path_is_url(tempurl))
		SVN::get_url_from_target(&urla, tempurl);
	else
		urla = tempurl;

	if (urla == NULL)
		return -1;

	/* Get the RA library that handles URL. */
	if (svn_ra_init_ra_libs (&ra_baton, localpool))
		return -1;
	if (svn_ra_get_ra_library (&ra_lib, ra_baton, urla, localpool))
		return -1;

	/* Open a repository session to the URL. */
	if (svn_client__open_ra_session (&session, ra_lib, urla, NULL, NULL, NULL, FALSE, FALSE, &ctx, localpool))
		return -1;

	if (ra_lib->get_latest_revnum(session, &rev, localpool))
		return -1;
	svn_pool_clear(localpool);
	return rev;
}

LONG SVN::RevPropertySet(CString sName, CString sValue, CString sURL, SVNRev rev)
{
	svn_revnum_t set_rev;
	svn_string_t*	pval;
	sValue.Replace(_T("\r"), _T(""));
	pval = svn_string_create (MakeSVNUrlOrPath(sValue), pool);
	Err = svn_client_revprop_set(MakeSVNUrlOrPath(sName), 
									pval, 
									MakeSVNUrlOrPath(sURL), 
									rev, 
									&set_rev, 
									FALSE, 
									&ctx, 
									pool);
	if (Err)
		return 0;
	return set_rev;
}

CString SVN::RevPropertyGet(CString sName, CString sURL, SVNRev rev)
{
	svn_string_t *propval;
	svn_revnum_t set_rev;
	Err = svn_client_revprop_get(MakeSVNUrlOrPath(sName), &propval, MakeSVNUrlOrPath(sURL), rev, &set_rev, &ctx, pool);
	if (Err)
		return _T("");
	if (propval==NULL)
		return _T("");
	if (propval->len <= 0)
		return _T("");
	return CUnicodeUtils::GetUnicode(propval->data);
}

CString SVN::GetPristinePath(CString wcPath)
{
	apr_pool_t * localpool;
	svn_error_t * err;
	localpool = svn_pool_create (NULL);

	const char * deststr = NULL;
	svn_utf_cstring_to_utf8(&deststr, "dummy", localpool);
	svn_utf_cstring_from_utf8(&deststr, "dummy", localpool);

	const char* pristinePath = NULL;
	CString temp;

	err = svn_wc_get_pristine_copy_path(svn_path_internal_style(MakeSVNUrlOrPath(wcPath), localpool), (const char **)&pristinePath, localpool);

	if (err != NULL)
	{
		svn_pool_destroy(localpool);
		return temp;
	}
	if (pristinePath != NULL)
		temp = MakeUIUrlOrPath(pristinePath);
	svn_pool_destroy(localpool);
	return temp;
}

BOOL SVN::GetTranslatedFile(CString& sTranslatedFile, CString sFile, BOOL bForceRepair /*= TRUE*/)
{
	svn_wc_adm_access_t *adm_access;          
	apr_pool_t * localpool;
	svn_error_t * err;
	localpool = svn_pool_create(NULL);

	const char * deststr = NULL;
	svn_utf_cstring_to_utf8(&deststr, "dummy", localpool);
	svn_utf_cstring_from_utf8(&deststr, "dummy", localpool);

	const char * translatedPath = NULL;
	preparePath(sFile);
	CStringA temp = MakeSVNUrlOrPath(sFile);
	const char * originPath = temp;
	err = svn_wc_adm_probe_open2 (&adm_access, NULL, originPath, FALSE, 0, localpool);
	if (err)
		goto error;
	err = svn_wc_translated_file((const char **)&translatedPath, originPath, adm_access, bForceRepair, localpool);
	svn_wc_adm_close(adm_access);
	if (err)
		goto error;
	
	sTranslatedFile = MakeUIUrlOrPath(translatedPath);
	return (translatedPath != originPath);

error:
	svn_pool_destroy(localpool);
	return FALSE;
}

void SVN::UpdateShell(CString path)
{
	//updating the left pane (tree view) of the explorer
	//is more difficult (if not impossible) than I thought.
	//Using SHChangeNotify() doesn't work at all. I found that
	//the shell receives the message, but then checks the files/folders
	//itself for changes. And since the folders which are shown
	//in the tree view haven't changed the icon-overlay is
	//not updated!
	//a workaround for this problem would be if this method would
	//rename the folders, do a SHChangeNotify(SHCNE_RMDIR, ...),
	//rename the folders back and do an SHChangeNotify(SHCNE_UPDATEDIR, ...)
	//
	//But I'm not sure if that is really a good workaround - it'll possibly
	//slows down the explorer and also causes more HD usage.
	//
	//So this method only updates the files and folders in the normal
	//explorer view by telling the explorer that the folder icon itself
	//has changed.
	preparePath(path);
	CString temp;
	int pos = -1;
	do
	{
		pos = path.Find('*');
		if (pos>=0)
			temp = path.Left(pos);
		else
			temp = path;
		SHFILEINFO    sfi;
		SHGetFileInfo(
			(LPCTSTR)temp, 
			FILE_ATTRIBUTE_DIRECTORY,
			&sfi, 
			sizeof(SHFILEINFO), 
			SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
		SHFILEINFO    sfiopen;
		SHGetFileInfo(
			(LPCTSTR)temp, 
			FILE_ATTRIBUTE_DIRECTORY,
			&sfiopen, 
			sizeof(SHFILEINFO), 
			SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES | SHGFI_OPENICON);

		SHChangeNotify(SHCNE_UPDATEIMAGE | SHCNF_FLUSH, SHCNF_DWORD, NULL, reinterpret_cast<LPCVOID>((__int64)sfi.iIcon));
		SHChangeNotify(SHCNE_UPDATEIMAGE | SHCNF_FLUSH, SHCNF_DWORD, NULL, reinterpret_cast<LPCVOID>((__int64)sfiopen.iIcon));
		path = path.Mid(pos+1);
	} while (pos >= 0);
}

BOOL SVN::PathIsURL(CString path)
{
	return svn_path_is_url(MakeSVNUrlOrPath(path));
}

void SVN::formatDate(TCHAR date_native[], apr_time_t& date_svn, bool force_short_fmt)
{
	date_native[0] = '\0';
	__time64_t ttime = date_svn/1000000L;

	struct tm * newtime;
	SYSTEMTIME systime;
	TCHAR timebuf[MAX_PATH];
	TCHAR datebuf[MAX_PATH];

	LCID locale = (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT));
	locale = MAKELCID(locale, SORT_DEFAULT);

	newtime = _localtime64(&ttime);

	systime.wDay = newtime->tm_mday;
	systime.wDayOfWeek = newtime->tm_wday;
	systime.wHour = newtime->tm_hour;
	systime.wMilliseconds = 0;
	systime.wMinute = newtime->tm_min;
	systime.wMonth = newtime->tm_mon+1;
	systime.wSecond = newtime->tm_sec;
	systime.wYear = newtime->tm_year+1900;
	if (force_short_fmt || CRegDWORD(_T("Software\\TortoiseSVN\\LogDateFormat")) == 1)
	{
		GetDateFormat(locale, DATE_SHORTDATE, &systime, NULL, datebuf, MAX_PATH);
		GetTimeFormat(locale, 0, &systime, NULL, timebuf, MAX_PATH);
		_tcsncat(date_native, datebuf, MAX_PATH);
		_tcsncat(date_native, _T(" "), MAX_PATH);
		_tcsncat(date_native, timebuf, MAX_PATH);
	}
	else
	{
		GetDateFormat(locale, DATE_LONGDATE, &systime, NULL, datebuf, MAX_PATH);
		GetTimeFormat(locale, 0, &systime, NULL, timebuf, MAX_PATH);
		_tcsncat(date_native, timebuf, MAX_PATH);
		_tcsncat(date_native, _T(", "), MAX_PATH);
		_tcsncat(date_native, datebuf, MAX_PATH);
	}
}

CStringA SVN::MakeSVNUrlOrPath(CString UrlOrPath)
{
	CStringA url = CUnicodeUtils::GetUTF8(UrlOrPath);
	if (svn_path_is_url(url))
	{
		if (!CUtils::IsEscaped(url))
			url = CUtils::PathEscape(url);
	}
	return url;
}

CString SVN::MakeUIUrlOrPath(CStringA UrlOrPath)
{
	if (svn_path_is_url(UrlOrPath))
	{
		CUtils::Unescape(UrlOrPath.GetBuffer());
		UrlOrPath.ReleaseBuffer();
	}
	CString url = CUnicodeUtils::GetUnicode(UrlOrPath);
	return url;
}

void SVN::UseIEProxySettings(apr_hash_t * cfg)
{
	CStringA exceptions;
	CStringA port;
	CStringA proxy;
	CStringA temp;
	const char * valuep;
	INTERNET_PER_CONN_OPTION_LIST    List;
	INTERNET_PER_CONN_OPTION         Option[5];
	unsigned long                    nSize = sizeof(INTERNET_PER_CONN_OPTION_LIST);

	Option[0].dwOption = INTERNET_PER_CONN_AUTOCONFIG_URL;
	Option[1].dwOption = INTERNET_PER_CONN_AUTODISCOVERY_FLAGS;
	Option[2].dwOption = INTERNET_PER_CONN_FLAGS;
	Option[3].dwOption = INTERNET_PER_CONN_PROXY_BYPASS;
	Option[4].dwOption = INTERNET_PER_CONN_PROXY_SERVER;

	List.dwSize = sizeof(INTERNET_PER_CONN_OPTION_LIST);
	List.pszConnection = NULL;
	List.dwOptionCount = 5;
	List.dwOptionError = 0;
	List.pOptions = Option;

	CString server = CRegString(_T("Software\\Tigris.org\\Subversion\\Servers\\global\\http-proxy-host"), _T(""));
	if (!server.IsEmpty())
		return;

	if(!InternetQueryOption(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &List, &nSize))
		return;

	if((Option[2].Value.dwValue & PROXY_TYPE_AUTO_PROXY_URL) == PROXY_TYPE_AUTO_PROXY_URL)
		goto ERROR_LABEL;

	if((Option[2].Value.dwValue & PROXY_TYPE_AUTO_DETECT) == PROXY_TYPE_AUTO_DETECT)
		goto ERROR_LABEL;

	exceptions = CStringA(Option[3].Value.pszValue);
	exceptions.Replace(';', ',');
	proxy = CStringA(Option[4].Value.pszValue);
	if (proxy.Find(';')>=0)
	{
		if (proxy.Find("http=")>=0)
		{
			temp = proxy.Mid(proxy.Find("http=")+5);
			temp = temp.Left(temp.Find(';'));
		}
		if ((temp.IsEmpty())&&(proxy.Find("https=")>=0))
		{
			temp = proxy.Mid(proxy.Find("https=")+6);
			temp = temp.Left(temp.Find(';'));
		}
		proxy = temp;
	}
	if (proxy.Find(':')>=0)
	{
		port = proxy.Mid(proxy.Find(':')+1);
		proxy = proxy.Left(proxy.Find(':'));
	}
	if (proxy.IsEmpty())
		goto ERROR_LABEL;

	svn_config_t * opt = (svn_config_t *)apr_hash_get (cfg, SVN_CONFIG_CATEGORY_SERVERS,
		APR_HASH_KEY_STRING);
	svn_config_get(opt, &valuep, SVN_CONFIG_SECTION_GLOBAL, "http-proxy-exceptions", "");
	exceptions += ",";
	exceptions += valuep;
	svn_config_set(opt, SVN_CONFIG_SECTION_GLOBAL, "http-proxy-exceptions", exceptions);
	
	svn_config_set(opt, SVN_CONFIG_SECTION_GLOBAL, "http-proxy-host", proxy);
	
	svn_config_set(opt, SVN_CONFIG_SECTION_GLOBAL, "http-proxy-port", port);



ERROR_LABEL:
	if(Option[0].Value.pszValue != NULL)
		GlobalFree(Option[0].Value.pszValue);

	if(Option[3].Value.pszValue != NULL)
		GlobalFree(Option[3].Value.pszValue);

	if(Option[4].Value.pszValue != NULL)
		GlobalFree(Option[4].Value.pszValue);
	return;
}
