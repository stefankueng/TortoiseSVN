// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2004 - Stefan Kueng

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
#pragma once
#include "afxcoll.h"

/**
 * \ingroup TortoiseMerge
 *
 * Represents an array of text lines which are read from a file.
 */
class CFileTextLines : public CStringArray
{
public:
	CFileTextLines(void);
	~CFileTextLines(void);

	enum LineEndings
	{
		AUTOLINE,
		LF,
		CRLF,
		LFCR,
		CR,
	};

	/**
	 * Loads the text file and adds each line to the array
	 * \param sFilePath the path to the file
	 */
	BOOL		Load(CString sFilePath);
	/**
	 * Saves the whole array of text lines to a file, preserving
	 * the line endings detected at Load()
	 * \param sFilePath the path to save the file to
	 */
	BOOL		Save(CString sFilePath, BOOL bIgnoreWhitespaces = FALSE, BOOL bIgnoreLineendings = FALSE);
	/**
	 * Returns an error string of the last failed operation
	 */
	CString		GetErrorString() {return m_sErrorString;}
	/**
	 * Copies the settings of a file like the line ending styles
	 * to another CFileTextLines object.
	 */
	void		CopySettings(CFileTextLines * pFileToCopySettingsTo);

	BOOL		m_bUnicode;
	CFileTextLines::LineEndings m_LineEndings;
protected:
	/**
	 * Checks if a buffer is an UNICODE text 
	 * \param pBuffer pointer to the buffer containing text
	 * \param cd size of the text buffer in bytes
	 */
	BOOL		IsTextUnicode(LPVOID pBuffer, int cb);
	/**
	 * Checks the line endings in a text buffer
	 * \param pBuffer pointer to the buffer containing text
	 * \param cd size of the text buffer in bytes
	 */
	CFileTextLines::LineEndings CheckLineEndings(LPVOID pBuffer, int cb); 

	void		GetLastError();

	CString		m_sErrorString;

};
