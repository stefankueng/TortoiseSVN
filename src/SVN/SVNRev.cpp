// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2005 - Tim Kemp and Stefan Kueng

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
#include "SVNRev.h"
#include "svn_time.h"
#include "SVNHelpers.h"

SVNRev::SVNRev(CString sRev)
{
	ZeroMemory(&rev, sizeof(svn_opt_revision_t));
	Create(sRev);
}

void SVNRev::Create(CString sRev)
{
	sDate.Empty();
	m_bIsValid = FALSE;
	memset (&rev, 0, sizeof (rev));
	if (sRev.Left(1).Compare(_T("{"))==0)
	{
		// brackets denote a date
		SVNPool pool;
		svn_boolean_t matched;
		apr_time_t tm;
//BUBGUG?  We don't bother testing for good pool creation anywhere else - should we bother here?
		if ((apr_pool_t*)pool)
		{
			CStringA sRevA = CStringA(sRev);
			sRevA = sRevA.Mid(1, sRevA.GetLength()-2);
			if (svn_parse_date(&matched, &tm, sRevA, apr_time_now(), pool) == NULL)
			{
				if (!matched)
					return;
				rev.kind = svn_opt_revision_date;
				rev.value.date = tm;
				m_bIsValid = TRUE;
				sDate = sRev;
			}
		}
	} // if (sRev.Left(1).Compare(_T("{"))==0)
	else if (sRev.CompareNoCase(_T("HEAD"))==0)
	{
		rev.kind = svn_opt_revision_head;
		m_bIsValid = TRUE;
	}
	else if (sRev.CompareNoCase(_T("BASE"))==0)
	{
		rev.kind = svn_opt_revision_base;
		m_bIsValid = TRUE;
	}
	else if (sRev.CompareNoCase(_T("WC"))==0)
	{
		rev.kind = svn_opt_revision_working;
		m_bIsValid = TRUE;
	}
	else if (sRev.CompareNoCase(_T("PREV"))==0)
	{
		rev.kind = svn_opt_revision_previous;
		m_bIsValid = TRUE;
	}
	else if (sRev.CompareNoCase(_T("COMMITTED"))==0)
	{
		rev.kind = svn_opt_revision_committed;
		m_bIsValid = TRUE;
	}
	else if (sRev.IsEmpty())
	{
		rev.kind = svn_opt_revision_head;
		m_bIsValid = TRUE;
	}
	else
	{
		LONG nRev = _ttol(sRev);
		if (nRev > 0)
		{
			rev.kind = svn_opt_revision_number;
			rev.value.number = nRev;
			m_bIsValid = TRUE;
		}
	}
}

SVNRev::SVNRev(LONG nRev)
{
	Create(nRev);
}

void SVNRev::Create(LONG nRev)
{
	m_bIsValid = TRUE;
	memset (&rev, 0, sizeof (rev));
	if(nRev == SVNRev::REV_HEAD)
	{
		rev.kind = svn_opt_revision_head;
	} 
	else if (nRev == SVNRev::REV_BASE)
	{
		rev.kind = svn_opt_revision_base;
	}
	else if (nRev == SVNRev::REV_WC)
	{
		rev.kind = svn_opt_revision_working;
	}
	else
	{
		rev.kind = svn_opt_revision_number;
		rev.value.number = nRev;
	}
}

SVNRev::~SVNRev()
{
}

SVNRev::operator LONG() const
{
	switch (rev.kind)
	{
	case svn_opt_revision_head:		return SVNRev::REV_HEAD;
	case svn_opt_revision_base:		return SVNRev::REV_BASE;
	case svn_opt_revision_working:	return SVNRev::REV_WC;
	case svn_opt_revision_number:	return rev.value.number;
	}

	ASSERT(true);
	return SVNRev::REV_HEAD;
}

SVNRev::operator svn_opt_revision_t * ()
{
	return &rev;
}
