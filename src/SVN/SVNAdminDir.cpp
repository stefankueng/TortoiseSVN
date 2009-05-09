// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2009 - TortoiseSVN

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
#include "UnicodeUtils.h"
#include "SVNAdminDir.h"

SVNAdminDir g_SVNAdminDir;

SVNAdminDir::SVNAdminDir()
	: m_nInit(0)
	, m_bVSNETHack(false)
	, m_pool(NULL)
{
}

SVNAdminDir::~SVNAdminDir()
{
	if (m_nInit)
		svn_pool_destroy(m_pool);
}

bool SVNAdminDir::Init()
{
	if (m_nInit==0)
	{
		m_bVSNETHack = false;
		m_pool = svn_pool_create(NULL);
		size_t ret = 0;
		getenv_s(&ret, NULL, 0, "SVN_ASP_DOT_NET_HACK");
		if (ret)
		{
			svn_error_clear(svn_wc_set_adm_dir("_svn", m_pool));
			m_bVSNETHack = true;
		}
	}
	m_nInit++;
	return true;
}

bool SVNAdminDir::Close()
{
	m_nInit--;
	if (m_nInit>0)
		return false;
	svn_pool_destroy(m_pool);
	return true;
}

bool SVNAdminDir::IsAdminDirName(const CString& name) const
{
	CStringA nameA = CUnicodeUtils::GetUTF8(name).MakeLower();
	return !!svn_wc_is_adm_dir(nameA, m_pool);
}

bool SVNAdminDir::HasAdminDir(const CString& path) const
{
	if (PathIsUNCServer(path))
		return false;
	return HasAdminDir(path, !!PathIsDirectory(path));
}

bool SVNAdminDir::HasAdminDir(const CString& path, bool bDir) const
{
	if (path.IsEmpty())
		return false;
	if (PathIsUNCServer(path))
		return false;
	bool bHasAdminDir = false;
	CString sDirName = path;
	if (!bDir)
	{
		sDirName = path.Left(path.ReverseFind('\\'));
	}
	bHasAdminDir = !!PathFileExists(sDirName + _T("\\.svn"));
	if (!bHasAdminDir && m_bVSNETHack)
		bHasAdminDir = !!PathFileExists(sDirName + _T("\\_svn"));
	return bHasAdminDir;
}

bool SVNAdminDir::IsAdminDirPath(const CString& path) const
{
	if (path.IsEmpty())
		return false;
	bool bIsAdminDir = false;
	CString lowerpath = path;
	lowerpath.MakeLower();
	int ind = -1;
	int ind1 = 0;
	while ((ind1 = lowerpath.Find(_T("\\.svn"), ind1))>=0)
	{
		ind = ind1++;
		if (ind == (lowerpath.GetLength() - 5))
		{
			bIsAdminDir = true;
			break;
		}
		else if (lowerpath.Find(_T("\\.svn\\"), ind)>=0)
		{
			bIsAdminDir = true;
			break;
		}
	}
	if (!bIsAdminDir && m_bVSNETHack)
	{
		ind = -1;
		ind1 = 0;
		while ((ind1 = lowerpath.Find(_T("\\_svn"), ind1))>=0)
		{
			ind = ind1++;
			if (ind == (lowerpath.GetLength() - 5))
			{
				bIsAdminDir = true;
				break;
			}
			else if (lowerpath.Find(_T("\\_svn\\"), ind)>=0)
			{
				bIsAdminDir = true;
				break;
			}
		}
	}
	return bIsAdminDir;
}


