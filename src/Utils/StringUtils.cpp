// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2005 - Stefan Kueng

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
#include "stringutils.h"

#ifdef _MFC_VER
BOOL CStringUtils::WildCardMatch(const CString& wildcard, const CString& string)
{
	return _tcswildcmp(wildcard, string);
}
#endif // #ifdef _MFC_VER

int strwildcmp(const char *wild, const char *string)
{
	const char *cp = NULL;
	const char *mp = NULL;
	while ((*string) && (*wild != '*')) 
	{
		if ((*wild != *string) && (*wild != '?')) 
		{
			return 0; 
		}
		wild++; 
		string++; 
	} // while ((*string) && (*wild != '*')) 
	while (*string) 
	{
		if (*wild == '*') 
		{
			if (!*++wild) 
			{
				return 1; 
			} 
			mp = wild; 
			cp = string+1;
		} // if (*wild == '*') 
		else if ((*wild == *string) || (*wild == '?')) 
		{
			wild++;
			string++;
		} 
		else 
		{
			wild = mp;
			string = cp++;
		}
	} // while (*string)

	while (*wild == '*') 
	{
		wild++;
	}
	return !*wild;
}

int wcswildcmp(const wchar_t *wild, const wchar_t *string)
{
	const wchar_t *cp = NULL;
	const wchar_t *mp = NULL;
	while ((*string) && (*wild != '*')) 
	{
		if ((*wild != *string) && (*wild != '?')) 
		{
			return 0; 
		}
		wild++; 
		string++; 
	} // while ((*string) && (*wild != '*')) 
	while (*string) 
	{
		if (*wild == '*') 
		{
			if (!*++wild) 
			{
				return 1; 
			} 
			mp = wild; 
			cp = string+1;
		} // if (*wild == '*') 
		else if ((*wild == *string) || (*wild == '?')) 
		{
			wild++;
			string++;
		} 
		else 
		{
			wild = mp;
			string = cp++;
		}
	} // while (*string)

	while (*wild == '*') 
	{
		wild++;
	}
	return !*wild;
}

CString CStringUtils::LinesWrap(const CString& longstring, int limit /* = 80 */, bool bCompactPaths /* = true */)
{
	CString retString;
	CStringArray arWords;
	if (longstring.GetLength() < limit)
		return longstring;	// no wrapping needed.
	// now start breaking the string into words

	if (longstring.Find('\n')<0)
		return WordWrap(longstring, limit, bCompactPaths);

	int linepos = 0;
	int lineposold = 0;
	while ((linepos = longstring.Find('\n', linepos)) >= 0)
	{
		CString temp = longstring.Mid(lineposold, linepos-lineposold);
		if (temp.IsEmpty())
			break;
		lineposold = linepos;
		if ((linepos+1)<longstring.GetLength())
			linepos++;
		retString += WordWrap(temp, limit, bCompactPaths);
	}
	retString.Trim();
	retString.Replace(_T("\n\n"), _T("\n"));
	return retString;
}

CString CStringUtils::WordWrap(const CString& longstring, int limit /* = 80 */, bool bCompactPaths /* = true */)
{
	CString retString;
	if (longstring.GetLength() < limit)
		return longstring;	// no wrapping needed.
	CString temp = longstring;
	while (temp.GetLength() > limit)
	{
		int pos=0;
		int oldpos=0;
		while ((pos>=0)&&(temp.Find(' ', pos)<limit))
		{
			oldpos = pos;
			pos = temp.Find(' ', pos+1);
		}
		if (oldpos==0)
			oldpos = temp.Find(' ');
		if (pos<0)
		{
			retString += temp;
			temp.Empty();
		}
		else
		{
			CString longline = temp.Left(oldpos+1);
			if ((bCompactPaths)&&(longline.GetLength() < MAX_PATH))
			{
				if (((!PathIsFileSpec(longline))&&longline.Find(':')<3)||(PathIsURL(longline)))
				{
					TCHAR buf[MAX_PATH];
					PathCompactPathEx(buf, longline, limit+1, 0);
					longline = buf;
				}				
			}

			retString += longline;
			temp = temp.Mid(oldpos+1);
		}
		retString += _T("\n");
		pos = oldpos;
	}
	retString += temp;
	retString.Trim();
	return retString;
}

#if defined(_DEBUG)
// Some test cases for these classes
static class StringUtilsTest
{
public:
	StringUtilsTest()
	{
		CString longline = _T("this is a test of how a string can be splitted into several lines");
		CString splittedline = CStringUtils::WordWrap(longline, 10);
		splittedline = CStringUtils::LinesWrap(longline, 10);
		longline = _T("c:\\this_is_a_very_long\\path_on_windows and of course some other words added to make the line longer");
		splittedline = CStringUtils::WordWrap(longline, 10);
		splittedline = CStringUtils::LinesWrap(longline, 10);
		longline = _T("Forced failure in https://myserver.com/a_long_url_to_split PROPFIND error");
		splittedline = CStringUtils::WordWrap(longline, 20);
		splittedline = CStringUtils::LinesWrap(longline, 20);
		longline = _T("Forced\nfailure in https://myserver.com/a_long_url_to_split PROPFIND\nerror");
		splittedline = CStringUtils::WordWrap(longline);
		splittedline = CStringUtils::LinesWrap(longline);
		ATLTRACE(splittedline);
	}
} StringUtilsTest;

#endif