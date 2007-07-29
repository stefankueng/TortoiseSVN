// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006-2007 - TortoiseSVN

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
#include "StdAfx.h"
#include "SVNLineDiff.h"

const svn_diff_fns_t SVNLineDiff::SVNLineDiff_vtable =
{
	SVNLineDiff::datasource_open,
	SVNLineDiff::datasource_close,
	SVNLineDiff::next_token,
	SVNLineDiff::compare_token,
	SVNLineDiff::discard_token,
	SVNLineDiff::discard_all_token
};

svn_error_t * SVNLineDiff::datasource_open(void * baton, svn_diff_datasource_e datasource)
{
	SVNLineDiff * linediff = (SVNLineDiff *)baton;
	if (linediff->m_bWordDiff)
	{
		std::wstring token;
		switch (datasource)
		{
		case svn_diff_datasource_original:
			{
				int chartype = SVNLINEDIFF_CHARTYPE_NONE;
				for (unsigned long i=0; i<linediff->m_line1length; ++i)
				{
					if (IsCharAlphaNumeric(linediff->m_line1[i]))
					{
						if ((chartype == SVNLINEDIFF_CHARTYPE_ALPHANUMERIC)||
							(chartype == SVNLINEDIFF_CHARTYPE_NONE))
						{
							token += linediff->m_line1[i];
						}
						else
						{
							if (!token.empty())
								linediff->m_line1tokens.push_back(token);
							token = linediff->m_line1[i];
						}
						chartype = SVNLINEDIFF_CHARTYPE_ALPHANUMERIC;
					}
					else if (IsCharWhiteSpace(linediff->m_line1[i]))
					{
						if ((chartype == SVNLINEDIFF_CHARTYPE_SPACE)||
							(chartype == SVNLINEDIFF_CHARTYPE_NONE))
						{
							token += linediff->m_line1[i];
						}
						else
						{
							if (!token.empty())
								linediff->m_line1tokens.push_back(token);
							token = linediff->m_line1[i];
						}
						chartype = SVNLINEDIFF_CHARTYPE_SPACE;
					}
					else
					{
						if (!token.empty())
							linediff->m_line1tokens.push_back(token);
						token = linediff->m_line1[i];
						linediff->m_line1tokens.push_back(token);
						token.clear();
						chartype = SVNLINEDIFF_CHARTYPE_OTHER;
					}
				}
				if (!token.empty())
				{
					linediff->m_line1tokens.push_back(token);
					token.clear();
				}
			}
			break;
		case svn_diff_datasource_modified:
			{
				int chartype = SVNLINEDIFF_CHARTYPE_NONE;
				for (unsigned long i=0; i<linediff->m_line2length; ++i)
				{
					if (IsCharAlphaNumeric(linediff->m_line2[i]))
					{
						if ((chartype == SVNLINEDIFF_CHARTYPE_ALPHANUMERIC)||
							(chartype == SVNLINEDIFF_CHARTYPE_NONE))
						{
							token += linediff->m_line2[i];
						}
						else
						{
							if (!token.empty())
								linediff->m_line2tokens.push_back(token);
							token = linediff->m_line2[i];
						}
						chartype = SVNLINEDIFF_CHARTYPE_ALPHANUMERIC;
					}
					else if (IsCharWhiteSpace(linediff->m_line2[i]))
					{
						if ((chartype == SVNLINEDIFF_CHARTYPE_SPACE)||
							(chartype == SVNLINEDIFF_CHARTYPE_NONE))
						{
							token += linediff->m_line2[i];
						}
						else
						{
							if (!token.empty())
								linediff->m_line2tokens.push_back(token);
							token = linediff->m_line2[i];
						}
						chartype = SVNLINEDIFF_CHARTYPE_SPACE;
					}
					else
					{
						if (!token.empty())
							linediff->m_line2tokens.push_back(token);
						token = linediff->m_line2[i];
						linediff->m_line2tokens.push_back(token);
						token.clear();
						chartype = SVNLINEDIFF_CHARTYPE_OTHER;
					}
				}

				if (!token.empty())
				{
					linediff->m_line2tokens.push_back(token);
					token.clear();
				}
			}
			break;
		}
	}
	return SVN_NO_ERROR;
}

svn_error_t * SVNLineDiff::datasource_close(void * /*baton*/, svn_diff_datasource_e /*datasource*/)
{
	return SVN_NO_ERROR;
}

svn_error_t * SVNLineDiff::next_token(apr_uint32_t * hash, void ** token, void * baton, svn_diff_datasource_e datasource)
{
	SVNLineDiff * linediff = (SVNLineDiff *)baton;
	*token = NULL;
	switch (datasource)
	{
	case svn_diff_datasource_original:
		if (linediff->m_bWordDiff)
		{
			if (linediff->m_line1pos < linediff->m_line1tokens.size())
			{
				*token = (void *)linediff->m_line1tokens[linediff->m_line1pos].c_str();
				*hash = linediff->Adler32(0, linediff->m_line1tokens[linediff->m_line1pos].c_str(), linediff->m_line1tokens[linediff->m_line1pos].size());
				linediff->m_line1pos++;
			}
		}
		else
		{
			if (linediff->m_line1pos < linediff->m_line1length)
			{
				*token = (void *)&linediff->m_line1[linediff->m_line1pos];
				*hash = linediff->m_line1[linediff->m_line1pos];
				linediff->m_line1pos++;
			}
		}
		break;
	case svn_diff_datasource_modified:
		if (linediff->m_bWordDiff)
		{
			if (linediff->m_line2pos < linediff->m_line2tokens.size())
			{
				*token = (void *)linediff->m_line2tokens[linediff->m_line2pos].c_str();
				*hash = linediff->Adler32(0, linediff->m_line2tokens[linediff->m_line2pos].c_str(), linediff->m_line2tokens[linediff->m_line2pos].size());
				linediff->m_line2pos++;
			}
		}
		else
		{
			if (linediff->m_line2pos < linediff->m_line2length)
			{
				*token = (void *)&linediff->m_line2[linediff->m_line2pos];
				*hash = linediff->m_line2[linediff->m_line2pos];
				linediff->m_line2pos++;
			}
		}
		break;
	}
	return SVN_NO_ERROR;
}

svn_error_t * SVNLineDiff::compare_token(void * baton, void * token1, void * token2, int * compare)
{
	SVNLineDiff * linediff = (SVNLineDiff *)baton;
	if (linediff->m_bWordDiff)
	{
		LPCTSTR s1 = (LPCTSTR)token1;
		LPCTSTR s2 = (LPCTSTR)token2;
		if (s1 && s2)
		{
			*compare = _tcscmp(s1, s2);
		}
	}
	else
	{
		TCHAR * c1 = (TCHAR *)token1;
		TCHAR * c2 = (TCHAR *)token2;
		if (c1 && c2)
		{
			if (*c1 == *c2)
				*compare = 0;
			else if (*c1 < *c2)
				*compare = -1;
			else
				*compare = 1;
		}
	}
	return SVN_NO_ERROR;
}

void SVNLineDiff::discard_token(void * /*baton*/, void * /*token*/)
{
}

void SVNLineDiff::discard_all_token(void * /*baton*/)
{
}

SVNLineDiff::SVNLineDiff() : m_pool(NULL)
	, m_subpool(NULL)
{
	m_pool = svn_pool_create(NULL);
}

SVNLineDiff::~SVNLineDiff()
{
	svn_pool_destroy(m_pool);
};

bool SVNLineDiff::Diff(svn_diff_t **diff, LPCTSTR line1, int len1, LPCTSTR line2, int len2, bool bWordDiff)
{
	if (m_subpool)
		svn_pool_clear(m_subpool);
	else
		m_subpool = svn_pool_create(m_pool);

	if (m_subpool == NULL)
		return false;

	m_bWordDiff = bWordDiff;
	m_line1 = line1;
	m_line2 = line2;
	if (len1)
		m_line1length = len1;
	else
		m_line1length = _tcslen(m_line1);
	if (len2)
		m_line2length = len2;
	else
		m_line2length = _tcslen(m_line2);

	m_line1pos = 0;
	m_line2pos = 0;
	m_line1tokens.clear();
	m_line2tokens.clear();
	svn_error_t * err = svn_diff_diff(diff, this, &SVNLineDiff_vtable, m_subpool);
	if (err)
	{
		svn_error_clear(err);
		svn_pool_clear(m_subpool);
		return false;
	}
	return true;
}

#define ADLER_MOD_BASE 65521
#define ADLER_MOD_BLOCK_SIZE 5552

apr_uint32_t SVNLineDiff::Adler32(apr_uint32_t checksum, const WCHAR *data, apr_size_t len)
{
	const unsigned char * input = (const unsigned char *)data;
	apr_uint32_t s1 = checksum & 0xFFFF;
	apr_uint32_t s2 = checksum >> 16;
	apr_uint32_t b;
	len *= 2;
	apr_size_t blocks = len / ADLER_MOD_BLOCK_SIZE;

	len %= ADLER_MOD_BLOCK_SIZE;

	while (blocks--)
	{
		int count = ADLER_MOD_BLOCK_SIZE;
		while (count--)
		{
			b = *input++;
			s1 += b;
			s2 += s1;
		}

		s1 %= ADLER_MOD_BASE;
		s2 %= ADLER_MOD_BASE;
	}

	while (len--)
	{
		b = *input++;
		s1 += b;
		s2 += s1;
	}

	return ((s2 % ADLER_MOD_BASE) << 16) | (s1 % ADLER_MOD_BASE);
}

bool SVNLineDiff::IsCharWhiteSpace(TCHAR c)
{
	if ((c == ' ')||(c == '\t'))
		return true;
	return false;
}