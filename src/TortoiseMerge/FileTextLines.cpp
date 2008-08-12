// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2007-2008 - TortoiseSVN

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
#include "Resource.h"
#include "UnicodeUtils.h"
#include ".\filetextlines.h"


CFileTextLines::CFileTextLines(void)
{
}

CFileTextLines::~CFileTextLines(void)
{
}

CFileTextLines::UnicodeType CFileTextLines::CheckUnicodeType(LPVOID pBuffer, int cb)
{
	if (cb < 2)
		return CFileTextLines::ASCII;
	UINT16 * pVal = (UINT16 *)pBuffer;
	UINT8 * pVal2 = (UINT8 *)(pVal+1);
	// scan the whole buffer for a 0x0000 sequence
	// if found, we assume a binary file
	for (int i=0; i<(cb-2); i=i+2)
	{
		if (0x0000 == *pVal++)
			return CFileTextLines::BINARY;
	}
	pVal = (UINT16 *)pBuffer;
	if (*pVal == 0xFEFF)
		return CFileTextLines::UNICODE_LE;
	if (cb < 3)
		return ASCII;
	if (*pVal == 0xBBEF)
	{
		if (*pVal2 == 0xBF)
			return CFileTextLines::UTF8BOM;
	}
	// check for illegal UTF8 chars
	pVal2 = (UINT8 *)pBuffer;
	for (int i=0; i<cb; ++i)
	{
		if ((*pVal2 == 0xC0)||(*pVal2 == 0xC1)||(*pVal2 >= 0xF5))
			return CFileTextLines::ASCII;
		pVal2++;
	}
	pVal2 = (UINT8 *)pBuffer;
	for (int i=0; i<(cb-3); ++i)
	{
		if ((*pVal2 & 0xE0)==0xC0)
		{
			pVal2++;i++;
			if ((*pVal2 & 0xC0)!=0x80)
				return CFileTextLines::ASCII;
		}
		if ((*pVal2 & 0xF0)==0xE0)
		{
			pVal2++;i++;
			if ((*pVal2 & 0xC0)!=0x80)
				return CFileTextLines::ASCII;
			pVal2++;i++;
			if ((*pVal2 & 0xC0)!=0x80)
				return CFileTextLines::ASCII;
		}
		if ((*pVal2 & 0xF8)==0xF0)
		{
			pVal2++;i++;
			if ((*pVal2 & 0xC0)!=0x80)
				return CFileTextLines::ASCII;
			pVal2++;i++;
			if ((*pVal2 & 0xC0)!=0x80)
				return CFileTextLines::ASCII;
			pVal2++;i++;
			if ((*pVal2 & 0xC0)!=0x80)
				return CFileTextLines::ASCII;
		}
		pVal2++;
	}
	return CFileTextLines::UTF8;
}


EOL CFileTextLines::CheckLineEndings(LPVOID pBuffer, int cb)
{
	EOL retval = EOL_AUTOLINE;
	char * buf = (char *)pBuffer;
	for (int i=0; i<cb; i++)
	{
		//now search the buffer for line endings
		if (buf[i] == 0x0a)
		{
			if ((i+1)<cb)
			{
				if (buf[i+1] == 0)
				{
					//UNICODE
					if ((i+2)<cb)
					{
						if (buf[i+2] == 0x0d)
						{
							retval = EOL_LFCR;
							break;
						}
						else
						{
							retval = EOL_LF;
							break;
						}
					}
				}
				else if (buf[i+1] == 0x0d)
				{
					retval = EOL_LFCR;
					break;
				}
			}
			retval = EOL_LF;
			break;
		}
		else if (buf[i] == 0x0d)
		{
			if ((i+1)<cb)
			{
				if (buf[i+1] == 0)
				{
					//UNICODE
					if ((i+2)<cb)
					{
						if (buf[i+2] == 0x0a)
						{
							retval = EOL_CRLF;
							break;
						}
						else
						{
							retval = EOL_CR;
							break;
						}
					}
				}
				else if (buf[i+1] == 0x0a)
				{
					retval = EOL_CRLF;
					break;
				}
			}
			retval = EOL_CR;
			break;
		}
	} 
	return retval;	
}

BOOL CFileTextLines::Load(const CString& sFilePath, int lengthHint /* = 0*/)
{
	m_LineEndings = EOL_AUTOLINE;
	m_UnicodeType = CFileTextLines::AUTOTYPE;
	RemoveAll();
	m_endings.clear();
	if(lengthHint != 0)
	{
		Reserve(lengthHint);
	}
	
	if (PathIsDirectory(sFilePath))
	{
		m_sErrorString.Format(IDS_ERR_FILE_NOTAFILE, (LPCTSTR)sFilePath);
		return FALSE;
	}
	
	if (!PathFileExists(sFilePath))
	{
		//file does not exist, so just return SUCCESS
		return TRUE;
	}

	HANDLE hFile = CreateFile(sFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		SetErrorString();
		return FALSE;
	}

	LARGE_INTEGER fsize;
	if (!GetFileSizeEx(hFile, &fsize))
	{
		SetErrorString();
		CloseHandle(hFile);
		return false;
	}
	if (fsize.HighPart)
	{
		// file is way too big for us
		CloseHandle(hFile);
		m_sErrorString.LoadString(IDS_ERR_FILE_TOOBIG);
		return FALSE;
	}

	LPVOID pFileBuf = new BYTE[fsize.LowPart];
	DWORD dwReadBytes = 0;
	if (!ReadFile(hFile, pFileBuf, fsize.LowPart, &dwReadBytes, NULL))
	{
		SetErrorString();
		CloseHandle(hFile);
		return FALSE;
	}
	if (m_UnicodeType == CFileTextLines::AUTOTYPE)
	{
		m_UnicodeType = this->CheckUnicodeType(pFileBuf, min(10000, dwReadBytes));
	}
	if (m_LineEndings == EOL_AUTOLINE)
	{
		m_LineEndings = CheckLineEndings(pFileBuf, min(10000, dwReadBytes));
	}
	CloseHandle(hFile);

	if (m_UnicodeType == CFileTextLines::BINARY)
	{
		m_sErrorString.Format(IDS_ERR_FILE_BINARY, (LPCTSTR)sFilePath);
		delete [] pFileBuf;
		return FALSE;
	}

	// we may have to convert the file content
	if ((m_UnicodeType == UTF8)||(m_UnicodeType == UTF8BOM))
	{
		int ret = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)pFileBuf, dwReadBytes, NULL, 0);
		wchar_t * pWideBuf = new wchar_t[ret];
		int ret2 = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)pFileBuf, dwReadBytes, pWideBuf, ret);
		if (ret2 == ret)
		{
			delete [] pFileBuf;
			pFileBuf = pWideBuf;
			dwReadBytes = ret2;
		}
	}
	else if (m_UnicodeType == ASCII)
	{
		int ret = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPCSTR)pFileBuf, dwReadBytes, NULL, 0);
		wchar_t * pWideBuf = new wchar_t[ret];
		int ret2 = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPCSTR)pFileBuf, dwReadBytes, pWideBuf, ret);
		if (ret2 == ret)
		{
			delete [] pFileBuf;
			pFileBuf = pWideBuf;
			dwReadBytes = ret2;
		}
	}
	// fill in the lines into the array
	wchar_t * pTextBuf = (wchar_t *)pFileBuf;
	wchar_t * pLineStart = (wchar_t *)pFileBuf;
	if (m_UnicodeType == UNICODE_LE) 
	{
		// UTF16 have two bytes per char
		dwReadBytes/=2; 
	}
	if ((m_UnicodeType == UTF8BOM)||(m_UnicodeType == UNICODE_LE))
	{
		// ignore the BOM
		++pTextBuf; 
		++pLineStart;
		--dwReadBytes; 
	}

	for (DWORD i = 0; i<dwReadBytes; ++i)
	{
		if (*pTextBuf == '\r')
		{
			if ((i + 1) < dwReadBytes)
			{
				if (*(pTextBuf+1) == '\n')
				{
					// crlf line ending
					CString line(pLineStart, pTextBuf-pLineStart);
					Add(line, EOL_CRLF);
					pLineStart = pTextBuf+2;
					++pTextBuf;
					++i;
				}
				else
				{
					// cr line ending
					CString line(pLineStart, pTextBuf-pLineStart);
					Add(line, EOL_CR);
					pLineStart =pTextBuf+1;
				}
			}
		}
		else if (*pTextBuf == '\n')
		{
			// lf line ending
			CString line(pLineStart, pTextBuf-pLineStart);
			Add(line, EOL_LF);
			pLineStart =pTextBuf+1;
		}
		++pTextBuf;
	}
	if (pLineStart < pTextBuf)
	{
		CString line(pLineStart, pTextBuf-pLineStart);
		Add(line, EOL_NOENDING);
		m_bReturnAtEnd = false;		
	}
	else
		m_bReturnAtEnd = true;

	delete [] pFileBuf;


	return TRUE;
}

void CFileTextLines::StripWhiteSpace(CString& sLine,DWORD dwIgnoreWhitespaces, bool blame)
{
	if (blame)
	{
		if (sLine.GetLength() > 66)
			sLine = sLine.Mid(66);
	}
	switch (dwIgnoreWhitespaces)
	{
	case 0:
		// Compare whitespaces
		// do nothing
		break;
	case 1: 
		// Ignore all whitespaces
		sLine.TrimLeft(_T(" \t"));
		sLine.TrimRight(_T(" \t"));
		break;
	case 2:
		// Ignore leading whitespace
		sLine.TrimLeft(_T(" \t"));
		break;
	case 3:
		// Ignore ending whitespace
		sLine.TrimRight(_T(" \t"));
		break;
	}
}

void CFileTextLines::StripAsciiWhiteSpace(CStringA& sLine,DWORD dwIgnoreWhitespaces, bool blame)
{
	if (blame)
	{
		if (sLine.GetLength() > 66)
			sLine = sLine.Mid(66);
	}
	switch (dwIgnoreWhitespaces)
	{
	case 0: // Compare whitespaces
		// do nothing
		break;
	case 1:
		// Ignore all whitespaces
		StripAsciiWhiteSpace(sLine);
		break;
	case 2:
		// Ignore leading whitespace
		sLine.TrimLeft(" \t");
		break;
	case 3:
		// Ignore leading whitespace
		sLine.TrimRight(" \t");
		break;
	}
}

//
// Fast in-place removal of spaces and tabs from CStringA line
//
void CFileTextLines::StripAsciiWhiteSpace(CStringA& sLine)
{
	int outputLen = 0;
	char* pWriteChr = sLine.GetBuffer(sLine.GetLength());
	const char* pReadChr = pWriteChr;
	while(*pReadChr)
	{
		if(*pReadChr != ' ' && *pReadChr != '\t')
		{
			*pWriteChr++ = *pReadChr;
			outputLen++;
		}
		++pReadChr;
	}
	*pWriteChr = '\0';
	sLine.ReleaseBuffer(outputLen);
}

BOOL CFileTextLines::Save(const CString& sFilePath, bool bSaveAsUTF8, DWORD dwIgnoreWhitespaces /*=0*/, BOOL bIgnoreCase /*= FALSE*/, bool bBlame /*= false*/)
{
	try
	{
		CString destPath = sFilePath;
		// now make sure that the destination directory exists
		int ind = 0;
		while (destPath.Find('\\', ind)>=2)
		{
			if (!PathIsDirectory(destPath.Left(destPath.Find('\\', ind))))
			{
				if (!CreateDirectory(destPath.Left(destPath.Find('\\', ind)), NULL))
					return FALSE;
			}
			ind = destPath.Find('\\', ind)+1;
		}
		
		CStdioFile file;			// Hugely faster than CFile for big file writes - because it uses buffering
		if (!file.Open(sFilePath, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary))
		{
			m_sErrorString.Format(IDS_ERR_FILE_OPEN, (LPCTSTR)sFilePath);
			return FALSE;
		}
		if ((!bSaveAsUTF8)&&(m_UnicodeType == CFileTextLines::UNICODE_LE))
		{
			//first write the BOM
			UINT16 wBOM = 0xFEFF;
			file.Write(&wBOM, 2);
			for (int i=0; i<GetCount(); i++)
			{
				CString sLine = GetAt(i);
				EOL ending = GetLineEnding(i);
				StripWhiteSpace(sLine,dwIgnoreWhitespaces, bBlame);
				if (bIgnoreCase)
					sLine = sLine.MakeLower();
				file.Write((LPCTSTR)sLine, sLine.GetLength()*sizeof(TCHAR));
				if ((ending == EOL_AUTOLINE)||(ending == EOL_NOENDING))
					ending = m_LineEndings;
				switch (ending)
				{
				case EOL_CR:
					sLine = _T("\x0d");
					break;
				case EOL_CRLF:
				case EOL_AUTOLINE:
					sLine = _T("\x0d\x0a");
					break;
				case EOL_LF:
					sLine = _T("\x0a");
					break;
				case EOL_LFCR:
					sLine = _T("\x0a\x0d");
					break;
				}
				if ((m_bReturnAtEnd)||(i != GetCount()-1))
					file.Write((LPCTSTR)sLine, sLine.GetLength()*sizeof(TCHAR));
			}
		}
		else if ((!bSaveAsUTF8)&&((m_UnicodeType == CFileTextLines::ASCII)||(m_UnicodeType == CFileTextLines::AUTOTYPE)))
		{
			for (int i=0; i< GetCount(); i++)
			{
				// Copy CString to 8 bit without conversion
				CString sLineT = GetAt(i);
				CStringA sLine = CStringA(sLineT);
				EOL ending = GetLineEnding(i);

				StripAsciiWhiteSpace(sLine,dwIgnoreWhitespaces, bBlame);
				if (bIgnoreCase)
					sLine = sLine.MakeLower();
				if ((m_bReturnAtEnd)||(i != GetCount()-1))
				{
					if ((ending == EOL_AUTOLINE)||(ending == EOL_NOENDING))
						ending = m_LineEndings;
					switch (ending)
					{
					case EOL_CR:
						sLine += '\x0d';
						break;
					case EOL_CRLF:
					case EOL_AUTOLINE:
						sLine.Append("\x0d\x0a", 2);
						break;
					case EOL_LF:
						sLine += '\x0a';
						break;
					case EOL_LFCR:
						sLine.Append("\x0a\x0d", 2);
						break;
					}
				}
				file.Write((LPCSTR)sLine, sLine.GetLength());
			}
		}
		else if ((bSaveAsUTF8)||((m_UnicodeType == CFileTextLines::UTF8BOM)||(m_UnicodeType == CFileTextLines::UTF8)))
		{
			if (m_UnicodeType == CFileTextLines::UTF8BOM)
			{
				//first write the BOM
				UINT16 wBOM = 0xBBEF;
				file.Write(&wBOM, 2);
				UINT8 uBOM = 0xBF;
				file.Write(&uBOM, 1);
			}
			for (int i=0; i<GetCount(); i++)
			{
				CStringA sLine = CUnicodeUtils::GetUTF8(GetAt(i));
				EOL ending = GetLineEnding(i);
				StripAsciiWhiteSpace(sLine,dwIgnoreWhitespaces, bBlame);
				if (bIgnoreCase)
					sLine = sLine.MakeLower();

				if ((m_bReturnAtEnd)||(i != GetCount()-1))
				{
					if ((ending == EOL_AUTOLINE)||(ending == EOL_NOENDING))
						ending = m_LineEndings;
					switch (ending)
					{
					case EOL_CR:
						sLine += '\x0d';
						break;
					case EOL_CRLF:
					case EOL_AUTOLINE:
						sLine.Append("\x0d\x0a",2);
						break;
					case EOL_LF:
						sLine += '\x0a';
						break;
					case EOL_LFCR:
						sLine.Append("\x0a\x0d",2);
						break;
					}
				}
				file.Write((LPCSTR)sLine, sLine.GetLength());
			}
		}
		file.Close();
	}
	catch (CException * e)
	{
		e->GetErrorMessage(m_sErrorString.GetBuffer(4096), 4096);
		m_sErrorString.ReleaseBuffer();
		e->Delete();
		return FALSE;
	}
	return TRUE;
}

void CFileTextLines::SetErrorString()
{
		LPVOID lpMsgBuf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			::GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
			);
		m_sErrorString = (LPCTSTR)lpMsgBuf;
		LocalFree( lpMsgBuf );
}

void CFileTextLines::CopySettings(CFileTextLines * pFileToCopySettingsTo)
{
	if (pFileToCopySettingsTo)
	{
		pFileToCopySettingsTo->m_UnicodeType = m_UnicodeType;
		pFileToCopySettingsTo->m_LineEndings = m_LineEndings;
	}
}





