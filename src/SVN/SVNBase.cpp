// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010 - TortoiseSVN

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
#include "StdAfx.h"
#include "SVNBase.h"
#include "TSVNPath.h"
#include "StringUtils.h"
#include "UnicodeUtils.h"
#include "resource.h"

#pragma warning(push)
#include "svn_config.h"
#include "svn_pools.h"
#include "svn_client.h"
#pragma warning(pop)

SVNBase::SVNBase() 
    : Err(NULL)
    , m_pctx(NULL)
{
}


SVNBase::~SVNBase()
{
}

#ifdef CSTRING_AVAILABLE
CString SVNBase::GetLastErrorMessage(int wrap /* = 80 */)
{
    CString msg = GetErrorString(Err, wrap);
    if (!PostCommitErr.IsEmpty())
    {
#ifdef _MFC_VER
        msg += _T("\n") + CStringUtils::LinesWrap(PostCommitErr, wrap);
#else
        msg += _T("\n") + CStringUtils::LinesWrap(PostCommitErr, wrap);
#endif
    }
    return msg;
}

CString SVNBase::GetErrorString(svn_error_t * Err, int wrap /* = 80 */)
{
    CString msg;
    CString temp;
    char errbuf[256];

    if (Err != NULL)
    {
        svn_error_t * ErrPtr = Err;
        if (ErrPtr->message)
            msg = CUnicodeUtils::GetUnicode(ErrPtr->message);
        else
        {
            /* Is this a Subversion-specific error code? */
            if ((ErrPtr->apr_err > APR_OS_START_USEERR)
                && (ErrPtr->apr_err <= APR_OS_START_CANONERR))
                msg = svn_strerror (ErrPtr->apr_err, errbuf, _countof (errbuf));
            /* Otherwise, this must be an APR error code. */
            else
            {
                svn_error_t *temp_err = NULL;
                const char * err_string = NULL;
                temp_err = svn_utf_cstring_to_utf8(&err_string, apr_strerror (ErrPtr->apr_err, errbuf, _countof (errbuf)-1), ErrPtr->pool);
                if (temp_err)
                {
                    svn_error_clear (temp_err);
                    msg = _T("Can't recode error string from APR");
                }
                else
                {
                    msg = CUnicodeUtils::GetUnicode(err_string);
                }
            }
        }
        msg = CStringUtils::LinesWrap(msg, wrap);
        while (ErrPtr->child)
        {
            ErrPtr = ErrPtr->child;
            msg += _T("\n");
            if (ErrPtr->message)
                temp = CUnicodeUtils::GetUnicode(ErrPtr->message);
            else
            {
                /* Is this a Subversion-specific error code? */
                if ((ErrPtr->apr_err > APR_OS_START_USEERR)
                    && (ErrPtr->apr_err <= APR_OS_START_CANONERR))
                    temp = svn_strerror (ErrPtr->apr_err, errbuf, _countof (errbuf));
                /* Otherwise, this must be an APR error code. */
                else
                {
                    svn_error_t *temp_err = NULL;
                    const char * err_string = NULL;
                    temp_err = svn_utf_cstring_to_utf8(&err_string, apr_strerror (ErrPtr->apr_err, errbuf, _countof (errbuf)-1), ErrPtr->pool);
                    if (temp_err)
                    {
                        svn_error_clear (temp_err);
                        temp = _T("Can't recode error string from APR");
                    }
                    else
                    {
                        temp = CUnicodeUtils::GetUnicode(err_string);
                    }
                }
            }
            temp = CStringUtils::LinesWrap(temp, wrap);
            msg += temp;
        }
        temp.Empty();
#ifdef IDS_SVNERR_CHECKPATHORURL
        // add some hint text for some of the error messages
        switch (Err->apr_err)
        {
        case SVN_ERR_BAD_FILENAME:
        case SVN_ERR_BAD_URL:
            // please check the path or URL you've entered.
            temp.LoadString(IDS_SVNERR_CHECKPATHORURL);
            break;
        case SVN_ERR_WC_CLEANUP_REQUIRED:
            // do a "cleanup"
            temp.LoadString(IDS_SVNERR_RUNCLEANUP);
            break;
        case SVN_ERR_WC_NOT_UP_TO_DATE:
        case SVN_ERR_FS_TXN_OUT_OF_DATE:
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
        if ((Err->apr_err == SVN_ERR_FS_PATH_NOT_LOCKED)||
            (Err->apr_err == SVN_ERR_FS_NO_SUCH_LOCK)||
            (Err->apr_err == SVN_ERR_RA_NOT_LOCKED))
        {
            // the lock has already been broken from another working copy
            temp.LoadString(IDS_SVNERR_UNLOCKFAILEDNOLOCK);
        }
        else if (SVN_ERR_IS_UNLOCK_ERROR(Err))
        {
            // if you want to break the lock, use the "check for modifications" dialog
            temp.LoadString(IDS_SVNERR_UNLOCKFAILED);
        }
        if (!temp.IsEmpty())
        {
            msg += _T("\n") + temp;
        }
#endif
        return msg;
    }
    return _T("");
}

#endif

