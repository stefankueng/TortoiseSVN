// TortoiseSVN - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005 - Stefan Kueng

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
#include "stdafx.h"
#include "FileDropEdit.h"
#include ".\filedropedit.h"


// CFileDropEdit

IMPLEMENT_DYNAMIC(CFileDropEdit, CEdit)
CFileDropEdit::CFileDropEdit() : m_pDropTarget(NULL)
{
}

CFileDropEdit::~CFileDropEdit()
{
	if (m_pDropTarget)
		delete m_pDropTarget;
}


BEGIN_MESSAGE_MAP(CFileDropEdit, CEdit)
END_MESSAGE_MAP()



// CFileDropEdit message handlers

void CFileDropEdit::PreSubclassWindow()
{
	m_pDropTarget = new CFileDropTarget(m_hWnd);
	RegisterDragDrop(m_hWnd,m_pDropTarget);
	// create the supported formats:
	FORMATETC ftetc={0}; 
	ftetc.cfFormat = CF_TEXT; 
	ftetc.dwAspect = DVASPECT_CONTENT; 
	ftetc.lindex = -1; 
	ftetc.tymed = TYMED_HGLOBAL; 
	m_pDropTarget->AddSuportedFormat(ftetc); 
	ftetc.cfFormat=CF_HDROP; 
	m_pDropTarget->AddSuportedFormat(ftetc);

	CEdit::PreSubclassWindow();
}
