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
#include "StdAfx.h"
#include ".\tempfiles.h"

CTempFiles::CTempFiles(void)
{
}

CTempFiles::~CTempFiles(void)
{
	for (int i=0; i<m_arTempFileList.GetCount(); i++)
	{
		DeleteFile(m_arTempFileList.GetAt(i));
	}
}

CString CTempFiles::GetTempFilePath()
{
	TCHAR path[MAX_PATH];
	TCHAR tempF[MAX_PATH];
	GetTempPath (MAX_PATH, path);
	GetTempFileName (path, TEXT("tsm"), 0, tempF);
	CString tempfile = CString(tempF);
	m_arTempFileList.Add(tempfile);
	return tempfile;
}
