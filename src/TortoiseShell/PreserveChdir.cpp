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

#include "PreserveChdir.h"
#include <string.h>
#include <tchar.h>

PreserveChdir::PreserveChdir()
{
	GetCurrentDirectory(MAX_PATH, originalCurrentDirectory);
}

PreserveChdir::~PreserveChdir()
{
	TCHAR currentDirectory[MAX_PATH + 1];

	// _tchdir is an expensive function - don't call it unless we really have to
	GetCurrentDirectory(MAX_PATH, currentDirectory);
	if(_tcscmp(currentDirectory, originalCurrentDirectory) != 0)
	{
		SetCurrentDirectory(originalCurrentDirectory);
	}
}
