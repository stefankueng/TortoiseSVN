// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003 - Tim Kemp and Stefan Kueng

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
#include <uxtheme.h>
#include <tmschema.h>

class CIconStatic : public CStatic
{
public:
	CIconStatic();

public:
	bool SetIcon(UINT nIconID);
	bool SetText(CString strText);
	bool Init(UINT nIconID);
	virtual ~CIconStatic();

protected:
	CStatic m_wndPicture;
	UINT	m_nIconID;
	CString m_strText;
	CBitmap m_MemBMP;

	afx_msg void OnSysColorChange();

	DECLARE_MESSAGE_MAP()

private:
	typedef HTHEME(__stdcall *PFNOPENTHEMEDATA)(HWND hwnd, LPCWSTR pszClassList);
	typedef BOOL(__stdcall *PFNISAPPTHEMED)();
	typedef HRESULT (__stdcall *PFNDRAWTHEMETEXT)(HTHEME hTheme, HDC hdc, int iPartId, 
		int iStateId, LPCWSTR pszText, int iCharCount, DWORD dwTextFlags, 
		DWORD dwTextFlags2, const RECT *pRect);
	typedef HRESULT(__stdcall *PFNCLOSETHEMEDATA)(HTHEME hTheme);

};

/////////////////////////////////////////////////////////////////////////////

