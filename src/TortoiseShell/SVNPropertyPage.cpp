// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2010 - TortoiseSVN

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
#include "stdafx.h"

#include "ShellExt.h"
#include "svnpropertypage.h"
#include "UnicodeUtils.h"
#include "PathUtils.h"
#include "SVNStatus.h"
#include "auto_buffer.h"
#include "CreateProcessHelper.h"

#define MAX_STRING_LENGTH		4096			//should be big enough

// Nonmember function prototypes
BOOL CALLBACK PageProc (HWND, UINT, WPARAM, LPARAM);
UINT CALLBACK PropPageCallbackProc ( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp );

// CShellExt member functions (needed for IShellPropSheetExt)
STDMETHODIMP CShellExt::AddPages (LPFNADDPROPSHEETPAGE lpfnAddPage,
                                  LPARAM lParam)
{
	for (std::vector<tstring>::iterator I = files_.begin(); I != files_.end(); ++I)
	{
		SVNStatus svn;
		if (svn.GetStatus(CTSVNPath(I->c_str())) == (-2))
			return S_OK;			// file/directory not under version control

		if (svn.status->entry == NULL)
			return S_OK;
	}

	if (files_.size() == 0)
		return S_OK;

	LoadLangDll();
    PROPSHEETPAGE psp;
	SecureZeroMemory(&psp, sizeof(PROPSHEETPAGE));
	HPROPSHEETPAGE hPage;
	CSVNPropertyPage *sheetpage = NULL;
	try
	{
		sheetpage = new (std::nothrow) CSVNPropertyPage(files_);
	}
	catch (std::bad_alloc)
	{
		sheetpage = NULL;
	}
	if (sheetpage == NULL)
		return E_OUTOFMEMORY;

    psp.dwSize = sizeof (psp);
    psp.dwFlags = PSP_USEREFPARENT | PSP_USETITLE | PSP_USEICONID | PSP_USECALLBACK;	
	psp.hInstance = g_hResInst;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE);
    psp.pszIcon = MAKEINTRESOURCE(IDI_APPSMALL);
    psp.pszTitle = _T("Subversion");
    psp.pfnDlgProc = (DLGPROC) PageProc;
    psp.lParam = (LPARAM) sheetpage;
    psp.pfnCallback = PropPageCallbackProc;
    psp.pcRefParent = (UINT*)&g_cRefThisDll;

    hPage = CreatePropertySheetPage (&psp);

	if (hPage != NULL)
	{
        if (!lpfnAddPage (hPage, lParam))
        {
            delete sheetpage;
            DestroyPropertySheetPage (hPage);
        }
	}

    return S_OK;
}



STDMETHODIMP CShellExt::ReplacePage (UINT /*uPageID*/, LPFNADDPROPSHEETPAGE /*lpfnReplaceWith*/, LPARAM /*lParam*/)
{
    return E_FAIL;
}

/////////////////////////////////////////////////////////////////////////////
// Dialog procedures and other callback functions

BOOL CALLBACK PageProc (HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    CSVNPropertyPage * sheetpage;

    if (uMessage == WM_INITDIALOG)
    {
        sheetpage = (CSVNPropertyPage*) ((LPPROPSHEETPAGE) lParam)->lParam;
        SetWindowLongPtr (hwnd, GWLP_USERDATA, (LONG_PTR) sheetpage);
        sheetpage->SetHwnd(hwnd);
    }
    else
    {
        sheetpage = (CSVNPropertyPage*) GetWindowLongPtr (hwnd, GWLP_USERDATA);
    }

    if (sheetpage != 0L)
        return sheetpage->PageProc(hwnd, uMessage, wParam, lParam);
    else
        return FALSE;
}

UINT CALLBACK PropPageCallbackProc ( HWND /*hwnd*/, UINT uMsg, LPPROPSHEETPAGE ppsp )
{
    // Delete the page before closing.
    if (PSPCB_RELEASE == uMsg)
    {
        CSVNPropertyPage* sheetpage = (CSVNPropertyPage*) ppsp->lParam;
		delete sheetpage;
    }
    return 1;
}

// *********************** CSVNPropertyPage *************************

CSVNPropertyPage::CSVNPropertyPage(const std::vector<tstring> &newFilenames)
	:filenames(newFilenames)
{
}

CSVNPropertyPage::~CSVNPropertyPage(void)
{
}

void CSVNPropertyPage::SetHwnd(HWND newHwnd)
{
    m_hwnd = newHwnd;
}

BOOL CSVNPropertyPage::PageProc (HWND /*hwnd*/, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	switch (uMessage)
	{
	case WM_INITDIALOG:
		InitWorkfileView();
		return TRUE;
	case WM_NOTIFY:
		{
			LPNMHDR point = (LPNMHDR)lParam;
			int code = point->code;
			//
			// Respond to notifications.
			//    
			if (code == PSN_APPLY)
			{
			}
			SetWindowLongPtr (m_hwnd, DWLP_MSGRESULT, FALSE);
			return TRUE;        
		}
	case WM_DESTROY:
		return TRUE;
	case WM_COMMAND:
		switch (HIWORD(wParam))
		{
			case BN_CLICKED:
				if (LOWORD(wParam) == IDC_SHOWLOG)
				{
					tstring svnCmd = _T(" /command:");
					svnCmd += _T("log /path:\"");
					svnCmd += filenames.front().c_str();
					svnCmd += _T("\"");
					RunCommand(svnCmd);
				}
				if (LOWORD(wParam) == IDC_EDITPROPERTIES)
				{
					DWORD pathlength = GetTempPath(0, NULL);
					auto_buffer<TCHAR> path(pathlength+1);
					auto_buffer<TCHAR> tempFile(pathlength + 100);
					GetTempPath (pathlength+1, path);
					GetTempFileName (path, _T("svn"), 0, tempFile);
					tstring retFilePath = tstring(tempFile);

					HANDLE file = ::CreateFile (tempFile,
						GENERIC_WRITE, 
						FILE_SHARE_READ, 
						0, 
						CREATE_ALWAYS, 
						FILE_ATTRIBUTE_TEMPORARY,
						0);

					if (file != INVALID_HANDLE_VALUE)
					{
						DWORD written = 0;
						for (std::vector<tstring>::iterator I = filenames.begin(); I != filenames.end(); ++I)
						{
							::WriteFile (file, I->c_str(), (DWORD)I->size()*sizeof(TCHAR), &written, 0);
							::WriteFile (file, _T("\n"), 2, &written, 0);
						}
						::CloseHandle(file);

						tstring svnCmd = _T(" /command:");
						svnCmd += _T("properties /pathfile:\"");
						svnCmd += retFilePath.c_str();
						svnCmd += _T("\"");
						svnCmd += _T(" /deletepathfile");
						RunCommand(svnCmd);
					}
				}
				break;
		} // switch (HIWORD(wParam)) 
	} // switch (uMessage) 
	return FALSE;
}

void CSVNPropertyPage::Time64ToTimeString(__time64_t time, TCHAR * buf, size_t buflen)
{
	struct tm newtime;
	SYSTEMTIME systime;
	TCHAR timebuf[MAX_STRING_LENGTH];
	TCHAR datebuf[MAX_STRING_LENGTH];

	LCID locale = (WORD)CRegStdDWORD(_T("Software\\TortoiseSVN\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT));
	locale = MAKELCID(locale, SORT_DEFAULT);

	*buf = '\0';
	if (time)
	{
		_localtime64_s(&newtime, &time);

		systime.wDay = (WORD)newtime.tm_mday;
		systime.wDayOfWeek = (WORD)newtime.tm_wday;
		systime.wHour = (WORD)newtime.tm_hour;
		systime.wMilliseconds = 0;
		systime.wMinute = (WORD)newtime.tm_min;
		systime.wMonth = (WORD)newtime.tm_mon+1;
		systime.wSecond = (WORD)newtime.tm_sec;
		systime.wYear = (WORD)newtime.tm_year+1900;
		int ret = 0;
		if (CRegStdDWORD(_T("Software\\TortoiseSVN\\LogDateFormat")) == 1)
			ret = GetDateFormat(locale, DATE_SHORTDATE, &systime, NULL, datebuf, MAX_STRING_LENGTH);
		else
			ret = GetDateFormat(locale, DATE_LONGDATE, &systime, NULL, datebuf, MAX_STRING_LENGTH);
		if (ret == 0)
			datebuf[0] = '\0';
		ret = GetTimeFormat(locale, 0, &systime, NULL, timebuf, MAX_STRING_LENGTH);
		if (ret == 0)
			timebuf[0] = '\0';
		*buf = '\0';
		_tcsncat_s(buf, buflen, timebuf, MAX_STRING_LENGTH-1);
		_tcsncat_s(buf, buflen, _T(", "), MAX_STRING_LENGTH-1);
		_tcsncat_s(buf, buflen, datebuf, MAX_STRING_LENGTH-1);
	}
}

void CSVNPropertyPage::InitWorkfileView()
{
	SVNStatus svn;
	TCHAR tbuf[MAX_STRING_LENGTH];
	if (filenames.size() == 1)
	{
		if (svn.GetStatus(CTSVNPath(filenames.front().c_str()))>(-2))
		{
			TCHAR buf[MAX_STRING_LENGTH];
			__time64_t	time;
			if (svn.status->entry != NULL)
			{
				LoadLangDll();
				if (svn.status->text_status == svn_wc_status_added)
				{
					// disable the "show log" button for added files
					HWND showloghwnd = GetDlgItem(m_hwnd, IDC_SHOWLOG);
					if (GetFocus() == showloghwnd)
						::SendMessage(m_hwnd, WM_NEXTDLGCTL, 0, FALSE);
					::EnableWindow(showloghwnd, FALSE);
				}
				else
				{
					_stprintf_s(buf, MAX_STRING_LENGTH, _T("%d"), svn.status->entry->revision);
					SetDlgItemText(m_hwnd, IDC_REVISION, buf);
				}
				if (svn.status->entry->url)
				{
					size_t len = strlen(svn.status->entry->url);
					auto_buffer<char> unescapedurl(len+1);
					strcpy_s(unescapedurl, len+1, svn.status->entry->url);
					CPathUtils::Unescape(unescapedurl);
					SetDlgItemText(m_hwnd, IDC_REPOURL, UTF8ToWide(unescapedurl.get()).c_str());
					if (strcmp(unescapedurl, svn.status->entry->url))
					{
						ShowWindow(GetDlgItem(m_hwnd, IDC_ESCAPEDURLLABEL), SW_SHOW);
						ShowWindow(GetDlgItem(m_hwnd, IDC_REPOURLUNESCAPED), SW_SHOW);
						SetDlgItemText(m_hwnd, IDC_REPOURLUNESCAPED, UTF8ToWide(svn.status->entry->url).c_str());
					}
					else
					{
						ShowWindow(GetDlgItem(m_hwnd, IDC_ESCAPEDURLLABEL), SW_HIDE);
						ShowWindow(GetDlgItem(m_hwnd, IDC_REPOURLUNESCAPED), SW_HIDE);
					}
				}
				else
				{
					ShowWindow(GetDlgItem(m_hwnd, IDC_ESCAPEDURLLABEL), SW_HIDE);
					ShowWindow(GetDlgItem(m_hwnd, IDC_REPOURLUNESCAPED), SW_HIDE);
				}
				if (svn.status->text_status != svn_wc_status_added)
				{
					_stprintf_s(buf, MAX_STRING_LENGTH, _T("%d"), svn.status->entry->cmt_rev);
					SetDlgItemText(m_hwnd, IDC_CREVISION, buf);
					time = (__time64_t)svn.status->entry->cmt_date/1000000L;
					Time64ToTimeString(time, buf, MAX_STRING_LENGTH);
					SetDlgItemText(m_hwnd, IDC_CDATE, buf);
				}
				if (svn.status->entry->cmt_author)
					SetDlgItemText(m_hwnd, IDC_AUTHOR, UTF8ToWide(svn.status->entry->cmt_author).c_str());
				SVNStatus::GetStatusString(g_hResInst, svn.status->text_status, buf, sizeof(buf)/sizeof(TCHAR), (WORD)CRegStdDWORD(_T("Software\\TortoiseSVN\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)));
				SetDlgItemText(m_hwnd, IDC_TEXTSTATUS, buf);
				SVNStatus::GetStatusString(g_hResInst, svn.status->prop_status, buf, sizeof(buf)/sizeof(TCHAR), (WORD)CRegStdDWORD(_T("Software\\TortoiseSVN\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)));
				SetDlgItemText(m_hwnd, IDC_PROPSTATUS, buf);
				time = (__time64_t)svn.status->entry->text_time/1000000L;
				Time64ToTimeString(time, buf, MAX_STRING_LENGTH);
				SetDlgItemText(m_hwnd, IDC_TEXTDATE, buf);
				time = (__time64_t)svn.status->entry->prop_time/1000000L;
				Time64ToTimeString(time, buf, MAX_STRING_LENGTH);
				SetDlgItemText(m_hwnd, IDC_PROPDATE, buf);

				if (svn.status->entry->lock_owner)
					SetDlgItemText(m_hwnd, IDC_LOCKOWNER, UTF8ToWide(svn.status->entry->lock_owner).c_str());
				time = (__time64_t)svn.status->entry->lock_creation_date/1000000L;
				Time64ToTimeString(time, buf, MAX_STRING_LENGTH);
				SetDlgItemText(m_hwnd, IDC_LOCKDATE, buf);
	
				if (svn.status->entry->uuid)
					SetDlgItemText(m_hwnd, IDC_REPOUUID, UTF8ToWide(svn.status->entry->uuid).c_str());
				if (svn.status->entry->changelist)
					SetDlgItemText(m_hwnd, IDC_CHANGELIST, UTF8ToWide(svn.status->entry->changelist).c_str());
				SVNStatus::GetDepthString(g_hResInst, svn.status->entry->depth, buf, sizeof(buf)/sizeof(TCHAR), (WORD)CRegStdDWORD(_T("Software\\TortoiseSVN\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)));
				SetDlgItemText(m_hwnd, IDC_DEPTHEDIT, buf);

				if (svn.status->entry->checksum)
					SetDlgItemText(m_hwnd, IDC_CHECKSUM, UTF8ToWide(svn.status->entry->checksum).c_str());

				if (svn.status->locked)
					MAKESTRING(IDS_YES);
				else
					MAKESTRING(IDS_NO);
				SetDlgItemText(m_hwnd, IDC_LOCKED, stringtablebuffer);

				if (svn.status->copied)
					MAKESTRING(IDS_YES);
				else
					MAKESTRING(IDS_NO);
				SetDlgItemText(m_hwnd, IDC_COPIED, stringtablebuffer);

				if (svn.status->switched)
					MAKESTRING(IDS_YES);
				else
					MAKESTRING(IDS_NO);
				SetDlgItemText(m_hwnd, IDC_SWITCHED, stringtablebuffer);

				if (svn.status->file_external)
					MAKESTRING(IDS_YES);
				else
					MAKESTRING(IDS_NO);
				SetDlgItemText(m_hwnd, IDC_FILEEXTERNAL, stringtablebuffer);

				if (svn.status->tree_conflict)
					MAKESTRING(IDS_YES);
				else
					MAKESTRING(IDS_NO);
				SetDlgItemText(m_hwnd, IDC_TREECONFLICT, stringtablebuffer);
			}
		}
	}
	else if (filenames.size() != 0)
	{
		//deactivate the show log button
		HWND logwnd = GetDlgItem(m_hwnd, IDC_SHOWLOG);
		if (GetFocus() == logwnd)
			::SendMessage(m_hwnd, WM_NEXTDLGCTL, 0, FALSE);
		::EnableWindow(logwnd, FALSE);
		//get the handle of the list view
		if (svn.GetStatus(CTSVNPath(filenames.front().c_str()))>(-2))
		{
			if (svn.status->entry != NULL)
			{
				LoadLangDll();
				if (svn.status->entry->url)
				{
					CPathUtils::Unescape((char*)svn.status->entry->url);
					_tcsncpy_s(tbuf, MAX_STRING_LENGTH, UTF8ToWide(svn.status->entry->url).c_str(), 4095);
					TCHAR * ptr = _tcsrchr(tbuf, '/');
					if (ptr != 0)
					{
						*ptr = 0;
					}
					SetDlgItemText(m_hwnd, IDC_REPOURL, tbuf);
				}
				SetDlgItemText(m_hwnd, IDC_LOCKED, _T(""));
				SetDlgItemText(m_hwnd, IDC_COPIED, _T(""));
				SetDlgItemText(m_hwnd, IDC_SWITCHED, _T(""));
				SetDlgItemText(m_hwnd, IDC_FILEEXTERNAL, _T(""));
				SetDlgItemText(m_hwnd, IDC_TREECONFLICT, _T(""));

				SetDlgItemText(m_hwnd, IDC_DEPTHEDIT, _T(""));
				SetDlgItemText(m_hwnd, IDC_CHECKSUM, _T(""));
				SetDlgItemText(m_hwnd, IDC_REPOUUID, _T(""));

				ShowWindow(GetDlgItem(m_hwnd, IDC_ESCAPEDURLLABEL), SW_HIDE);
				ShowWindow(GetDlgItem(m_hwnd, IDC_REPOURLUNESCAPED), SW_HIDE);
			}
		}
	} 
}

void CSVNPropertyPage::RunCommand(const tstring& command)
{
	tstring tortoiseProcPath = GetAppDirectory() + _T("TortoiseProc.exe");
	CCreateProcessHelper::CreateProcessDetached(tortoiseProcPath.c_str(), const_cast<TCHAR*>(command.c_str()));
}
