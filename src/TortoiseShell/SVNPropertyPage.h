// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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
#include "ShellUpdater.h"

#define ListView_GetItemTextEx(hwndLV, i, iSubItem_, __buf) \
{ \
  int nLen = 1024;\
  int nRes;\
  LV_ITEM _ms_lvi;\
  _ms_lvi.iSubItem = iSubItem_;\
  do\
  {\
	nLen += 2;\
	_ms_lvi.cchTextMax = nLen;\
    if (__buf)\
		delete[] __buf;\
	__buf = new TCHAR[nLen];\
	_ms_lvi.pszText = __buf;\
    nRes  = (int)::SendMessage((hwndLV), LVM_GETITEMTEXT, (WPARAM)(i), (LPARAM)(LV_ITEM *)&_ms_lvi);\
  } while (nRes == nLen-1);\
}
#define GetDlgItemTextEx(hwndDlg, _id, __buf) \
{\
	int nLen = 1024;\
	int nRes;\
	do\
	{\
		nLen *= 2;\
		if (__buf)\
			delete [] __buf;\
		__buf = new TCHAR[nLen];\
		nRes = GetDlgItemText(hwndDlg, _id, __buf, nLen);\
	} while (nRes == nLen-1);\
}

/**
 * \ingroup TortoiseShell
 * Displays and updates all controls on the property page. The property
 * page itself is shown by explorer.
 *
 * \par requirements
 * win95 or later
 * winNT4 or later
 *
 * \version 1.0
 * first version
 *
 * \date 10-11-2002
 *
 * \author kueng
 *
 * \par license
 * This code is absolutely free to use and modify. The code is provided "as is" with
 * no expressed or implied warranty. The author accepts no liability if it causes
 * any damage to your computer, causes your pet to fall ill, increases baldness
 * or makes your car start emitting strange noises when you start it up.
 * This code has no bugs, just undocumented features!
 * 
 */
class CSVNPropertyPage
{
public:
	CSVNPropertyPage(const std::vector<stdstring> &filenames);
	virtual ~CSVNPropertyPage();

	/**
	 * Sets the window handle.
	 * \param hwnd the handle.
	 */
	virtual void SetHwnd(HWND hwnd);
	/**
	 * Callback function which receives the window messages of the
	 * property page. See the Win32 API for PropertySheets for details.
	 */
	virtual BOOL PageProc(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

	//virtual const stdstring &GetFilename() const { return filename; }
	HWND hwndTT;

protected:
	/**
	 * Initializes the property page.
	 */
	virtual void InitWorkfileView();
	void Time64ToTimeString(__time64_t time, TCHAR * buf, size_t buflen);

	bool SaveProperties();
	
	//static void Unescape(LPTSTR psz);
	static void Unescape(char * psz);

	struct listproperty
	{
		stdstring name;
		stdstring value;
		int		  count;
	};
	HWND m_hwnd;
	std::vector<stdstring> filenames;
	std::map<stdstring, stdstring> propmap;
	TCHAR stringtablebuffer[255];
};


