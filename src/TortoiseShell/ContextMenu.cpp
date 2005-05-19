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
#include "stdafx.h"
#include "ShellExt.h"
#include "ItemIDList.h"
#include "PreserveChdir.h"
#include "UnicodeStrings.h"
#include "SVNProperties.h"
#include "SVNStatus.h"

#define GetPIDLFolder(pida) (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[0])
#define GetPIDLItem(pida, i) (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[i+1])

int g_shellidlist=RegisterClipboardFormat(CFSTR_SHELLIDLIST);

STDMETHODIMP CShellExt::Initialize(LPCITEMIDLIST pIDFolder,
                                   LPDATAOBJECT pDataObj,
                                   HKEY /* hRegKey */)
{
	files_.clear();
	folder_.erase();	
	isOnlyOneItemSelected = false;
	isInSVN = false;
	isConflicted = false;
	isFolder = false;
	isFolderInSVN = false;
	isNormal = false;
	isIgnored = false;
	isInVersionedFolder = false;
	isAdded = false;
	isLocked = false;
	isPatchFile = false;
	stdstring statuspath;
	svn_wc_status_kind fetchedstatus = svn_wc_status_unversioned;
	// get selected files/folders
	if (pDataObj)
	{
		STGMEDIUM medium;
		FORMATETC fmte = {(CLIPFORMAT)g_shellidlist,
			(DVTARGETDEVICE FAR *)NULL, 
			DVASPECT_CONTENT, 
			-1, 
			TYMED_HGLOBAL};
		HRESULT hres = pDataObj->GetData(&fmte, &medium);

		if (SUCCEEDED(hres) && medium.hGlobal)
		{
			if (m_State == DropHandler)
			{
				FORMATETC etc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
				STGMEDIUM stg = { TYMED_HGLOBAL };
				if ( FAILED( pDataObj->GetData ( &etc, &stg )))
					return E_INVALIDARG;


				HDROP drop = (HDROP)GlobalLock(stg.hGlobal);
				if ( NULL == drop )
				{
					ReleaseStgMedium ( &medium );
					return E_INVALIDARG;
				}

				int count = DragQueryFile(drop, (UINT)-1, NULL, 0);
				for (int i = 0; i < count; i++)
				{
					// find the path length in chars
					UINT len = DragQueryFile(drop, i, NULL, 0);
					if (len == 0)
						continue;
					TCHAR * szFileName = new TCHAR[len+1];
					if (0 == DragQueryFile(drop, i, szFileName, len+1))
					{
						delete szFileName;
						continue;
					}
					stdstring str = stdstring(szFileName);
					delete szFileName;
					if (str.empty() == false)
					{
						files_.push_back(str);
						if (i == 0)
						{
							//get the Subversion status of the item
							svn_wc_status_kind status = svn_wc_status_unversioned;
							CTSVNPath askedpath;
							askedpath.SetFromWin(str.c_str());
							if ((g_ShellCache.IsSimpleContext())&&(askedpath.IsDirectory()))
							{
								if (askedpath.HasAdminDir())
									status = svn_wc_status_normal;
							}
							else
							{
								try
								{
									SVNStatus stat;
									stat.GetStatus(CTSVNPath(str.c_str()), false, true, true);
									if (stat.status)
									{
										statuspath = str;
										status = SVNStatus::GetMoreImportant(stat.status->text_status, stat.status->prop_status);
										fetchedstatus = status;
										if ((stat.status->entry)&&(stat.status->entry->lock_token))
											isLocked = (stat.status->entry->lock_token[0] != 0);
									}
								}
								catch ( ... )
								{
									ATLTRACE2(_T("Exception in SVNStatus::GetAllStatus()\n"));
								}
							}
							if ((status != svn_wc_status_unversioned)&&(status != svn_wc_status_ignored)&&(status != svn_wc_status_none))
								isInSVN = true;
							if (status == svn_wc_status_ignored)
								isIgnored = true;
							if (status == svn_wc_status_normal)
								isNormal = true;
							if (status == svn_wc_status_conflicted)
								isConflicted = true;
							if (status == svn_wc_status_added)
								isAdded = true;
						}
					}
				} // for (int i = 0; i < count; i++)
				GlobalUnlock ( drop );
			} // if (m_State == DropHandler)
			else
			{
				//Enumerate PIDLs which the user has selected
				CIDA* cida = (CIDA*)medium.hGlobal;
				ItemIDList parent( GetPIDLFolder (cida));

				int count = cida->cidl;
				BOOL statfetched = FALSE;
				stdstring sAdm = _T("\\");
				sAdm += _T(SVN_WC_ADM_DIR_NAME);
				for (int i = 0; i < count; ++i)
				{
					ItemIDList child (GetPIDLItem (cida, i), &parent);
					stdstring str = child.toString();
					if (str.empty() == false)
					{
						//check if our menu is requested for a subversion admin directory
						if ((str.length() > sAdm.length())&&(str.compare(str.length()-sAdm.length(), sAdm.length(), sAdm)==0))
							continue;

						files_.push_back(str);
						CTSVNPath strpath;
						strpath.SetFromWin(str.c_str());
						isPatchFile |= (strpath.GetFileExtension().CompareNoCase(_T(".diff"))==0);
						isPatchFile |= (strpath.GetFileExtension().CompareNoCase(_T(".patch"))==0);

						if (!statfetched)
						{
							//get the Subversion status of the item
							svn_wc_status_kind status = svn_wc_status_unversioned;
							if ((g_ShellCache.IsSimpleContext())&&(strpath.IsDirectory()))
							{
								if (strpath.HasAdminDir())
									status = svn_wc_status_normal;
							}
							else
							{
								try
								{
									SVNStatus stat;
									stat.GetStatus(CTSVNPath(strpath), false, true, true);
									if (stat.status)
									{
										statuspath = str;
										status = SVNStatus::GetMoreImportant(stat.status->text_status, stat.status->prop_status);
										fetchedstatus = status;
										if ((stat.status->entry)&&(stat.status->entry->lock_token))
											isLocked = (stat.status->entry->lock_token[0] != 0);
									}	
									statfetched = TRUE;
								}
								catch ( ... )
								{
									ATLTRACE2(_T("Exception in SVNStatus::GetAllStatus()\n"));
								}
							}
							if ((status != svn_wc_status_unversioned)&&(status != svn_wc_status_ignored)&&(status != svn_wc_status_none))
								isInSVN = true;
							if (status == svn_wc_status_ignored)
							{
								isIgnored = true;
								// the item is ignored. Get the svn:ignored properties so we can (maybe) later
								// offer a 'remove from ignored list' entry
								SVNProperties props(strpath.GetContainingDirectory());
								ignoredprops.empty();
								for (int p=0; p<props.GetCount(); ++p)
								{
									if (props.GetItemName(p).compare(stdstring(_T("svn:ignore")))==0)
									{
										stdstring st = props.GetItemValue(p);
										ignoredprops = MultibyteToWide((char *)st.c_str());
										break;
									}
								}
							}
							if (status == svn_wc_status_normal)
								isNormal = true;
							if (status == svn_wc_status_conflicted)
								isConflicted = true;
							if (status == svn_wc_status_added)
								isAdded = true;
						}
					}
				} // for (int i = 0; i < count; ++i)
				ItemIDList child (GetPIDLItem (cida, 0), &parent);
				if (g_ShellCache.HasSVNAdminDir(child.toString().c_str(), FALSE))
					isInVersionedFolder = true;
			}


			if (medium.pUnkForRelease)
			{
				IUnknown* relInterface = (IUnknown*)medium.pUnkForRelease;
				relInterface->Release();
			}
		}
	}

	// get folder background
	if (pIDFolder)
	{
		ItemIDList list(pIDFolder);
		folder_ = list.toString();
		svn_wc_status_kind status = svn_wc_status_unversioned;
		if (folder_.compare(statuspath)!=0)
		{
			CTSVNPath askedpath;
			askedpath.SetFromWin(folder_.c_str());
			if ((g_ShellCache.IsSimpleContext())&&(askedpath.IsDirectory()))
			{
				if (askedpath.HasAdminDir())
					status = svn_wc_status_normal;
			}
			else
			{
				try
				{
					SVNStatus stat;
					stat.GetStatus(CTSVNPath(folder_.c_str()), false, true, true);
					if (stat.status)
					{
						status = SVNStatus::GetMoreImportant(stat.status->text_status, stat.status->prop_status);
						if ((stat.status->entry)&&(stat.status->entry->lock_token))
							isLocked = (stat.status->entry->lock_token[0] != 0);
					}
				}
				catch ( ... )
				{
					ATLTRACE2(_T("Exception in SVNStatus::GetAllStatus()\n"));
				}
			}
		}
		else
		{
			status = fetchedstatus;
		}
		if ((status != svn_wc_status_unversioned)&&(status != svn_wc_status_ignored))
		{
			isFolderInSVN = true;
			isInSVN = true;
		}
		if (status == svn_wc_status_ignored)
			isIgnored = true;
		isFolder = true;
	}
	if ((files_.size() == 1)&&(m_State != DropHandler))
	{
		isOnlyOneItemSelected = true;
		if (PathIsDirectory(files_.front().c_str()))
		{
			folder_ = files_.front();
			svn_wc_status_kind status = svn_wc_status_unversioned;
			if (folder_.compare(statuspath)!=0)
			{
				CTSVNPath askedpath;
				askedpath.SetFromWin(folder_.c_str());
				if (g_ShellCache.IsSimpleContext())
				{
					if (askedpath.HasAdminDir())
						status = svn_wc_status_normal;
				}
				try
				{
					SVNStatus stat;
					stat.GetStatus(CTSVNPath(folder_.c_str()), false, true, true);
					if (stat.status)
					{
						status = SVNStatus::GetMoreImportant(stat.status->text_status, stat.status->prop_status);
						if ((stat.status->entry)&&(stat.status->entry->lock_token))
							isLocked = (stat.status->entry->lock_token[0] != 0);
					}
				}
				catch ( ... )
				{
					ATLTRACE2(_T("Exception in SVNStatus::GetAllStatus()\n"));
				}
			}
			else
			{
				status = fetchedstatus;
			}
			if ((status != svn_wc_status_unversioned)&&(status != svn_wc_status_ignored))
				isFolderInSVN = true;
			if (status == svn_wc_status_ignored)
				isIgnored = true;
			isFolder = true;
			if (status == svn_wc_status_added)
				isAdded = true;
		}
	}
		
	return NOERROR;
}

void CShellExt::InsertSVNMenu(BOOL ownerdrawn, BOOL istop, HMENU menu, UINT pos, UINT_PTR id, UINT stringid, UINT icon, UINT idCmdFirst, SVNCommands com)
{
	TCHAR menutextbuffer[255];
	ZeroMemory(menutextbuffer, sizeof(menutextbuffer));
	MAKESTRING(stringid);

	if (istop)
	{
		//menu entry for the top context menu, so append an "SVN " before
		//the menu text to indicate where the entry comes from
		_tcscpy(menutextbuffer, _T("SVN "));
	}
	_tcscat(menutextbuffer, stringtablebuffer);
	if (ownerdrawn==1) 
	{
		InsertMenu(menu, pos, MF_BYPOSITION | MF_STRING | MF_OWNERDRAW, id, menutextbuffer);
	}
	else if (ownerdrawn==0)
	{
		InsertMenu(menu, pos, MF_BYPOSITION | MF_STRING , id, menutextbuffer);
		HBITMAP bmp = IconToBitmap(icon, (COLORREF)GetSysColor(COLOR_MENU)); 
		SetMenuItemBitmaps(menu, pos, MF_BYPOSITION, bmp, bmp);
	}
	else
	{
		InsertMenu(menu, pos, MF_BYPOSITION | MF_STRING , id, menutextbuffer);
	}

	// We store the relative and absolute diameter
	// (drawitem callback uses absolute, others relative)
	myIDMap[id - idCmdFirst] = com;
	myIDMap[id] = com;
}

HBITMAP CShellExt::IconToBitmap(UINT uIcon, COLORREF transparentColor)
{
	HICON hIcon = (HICON)LoadImage(g_hResInst, MAKEINTRESOURCE(uIcon), IMAGE_ICON, 10, 10, LR_DEFAULTCOLOR);
	if (!hIcon)
		return NULL;

	RECT     rect;

	rect.right = ::GetSystemMetrics(SM_CXMENUCHECK);
	rect.bottom = ::GetSystemMetrics(SM_CYMENUCHECK);

	rect.left =
		rect.top  = 0;

	HWND desktop    = ::GetDesktopWindow();
	if (desktop == NULL)
		return NULL;

	HDC  screen_dev = ::GetDC(desktop);
	if (screen_dev == NULL)
		return NULL;

	// Create a compatible DC
	HDC dst_hdc = ::CreateCompatibleDC(screen_dev);
	if (dst_hdc == NULL)
	{
		::ReleaseDC(desktop, screen_dev); 
		return NULL;
	}

	// Create a new bitmap of icon size
	HBITMAP bmp = ::CreateCompatibleBitmap(screen_dev, rect.right, rect.bottom);
	if (bmp == NULL)
	{
		::DeleteDC(dst_hdc);
		::ReleaseDC(desktop, screen_dev); 
		return NULL;
	}

	// Select it into the compatible DC
	HBITMAP old_dst_bmp = (HBITMAP)::SelectObject(dst_hdc, bmp);
	if (old_dst_bmp == NULL)
		return NULL;

	// Fill the background of the compatible DC with the given colour
	HBRUSH brush = ::CreateSolidBrush(transparentColor);
	if (brush == NULL)
	{
		::DeleteDC(dst_hdc);
		::ReleaseDC(desktop, screen_dev); 
		return NULL;
	}
	::FillRect(dst_hdc, &rect, brush);
	::DeleteObject(brush);

	// Draw the icon into the compatible DC
	::DrawIconEx(dst_hdc, 0, 0, hIcon, rect.right, rect.bottom, 0, NULL, DI_NORMAL);

	// Restore settings
	::SelectObject(dst_hdc, old_dst_bmp);
	::DeleteDC(dst_hdc);
	::ReleaseDC(desktop, screen_dev); 
	DestroyIcon(hIcon);
	return bmp;
}


stdstring CShellExt::WriteFileListToTempFile()
{
	//write all selected files and paths to a temporary file
	//for TortoiseProc.exe to read out again.
	DWORD pathlength = GetTempPath(0, NULL);
	TCHAR * path = new TCHAR[pathlength+1];
	TCHAR * tempFile = new TCHAR[pathlength + 100];
	GetTempPath (pathlength+1, path);
	GetTempFileName (path, _T("svn"), 0, tempFile);
	stdstring retFilePath = stdstring(tempFile);
	
	HANDLE file = ::CreateFile (tempFile,
								GENERIC_WRITE, 
								FILE_SHARE_READ, 
								0, 
								CREATE_ALWAYS, 
								FILE_ATTRIBUTE_TEMPORARY,
								0);

	delete path;
	delete tempFile;
	if (file == INVALID_HANDLE_VALUE)
		return stdstring();
		
	DWORD written = 0;
	if (files_.empty())
	{
		::WriteFile (file, folder_.c_str(), folder_.size()*sizeof(TCHAR), &written, 0);
		::WriteFile (file, _T("\n"), 2, &written, 0);
	}

	for (std::vector<stdstring>::iterator I = files_.begin(); I != files_.end(); ++I)
	{
		::WriteFile (file, I->c_str(), I->size()*sizeof(TCHAR), &written, 0);
		::WriteFile (file, _T("\n"), 2, &written, 0);
	}
	::CloseHandle(file);
	return retFilePath;
}

STDMETHODIMP CShellExt::QueryContextMenu(HMENU hMenu,
                                         UINT indexMenu,
                                         UINT idCmdFirst,
                                         UINT /*idCmdLast*/,
                                         UINT uFlags)
{
	//first check if our drophandler is called
	//and then (if true) provide the context menu for the
	//drop handler
	if (m_State == DropHandler)
	{
		LoadLangDll();
		PreserveChdir preserveChdir;

		if ((uFlags & CMF_DEFAULTONLY)!=0)
			return NOERROR;					//we don't change the default action

		if ((files_.size() == 0)&&(folder_.size() == 0))
			return NOERROR;

		if (((uFlags & 0x000f)!=CMF_NORMAL)&&(!(uFlags & CMF_EXPLORE))&&(!(uFlags & CMF_VERBSONLY)))
			return NOERROR;

		//the drophandler only has four commands, but not all are visible at the same time:
		//if the source file(s) are under version control then those files can be moved
		//to the new location or they can be moved with a rename, 
		//if they are unversioned then they can be added to the working copy
		//if they are versioned, they also can be exported to an unversioned location
		UINT idCmd = idCmdFirst;

		if ((isInSVN)&&(isFolderInSVN)&&(!isAdded))
			InsertSVNMenu(FALSE, FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPMOVEMENU, 0, idCmdFirst, DropMove);
		if ((isInSVN)&&(isFolderInSVN)&&(!isAdded))
			InsertSVNMenu(FALSE, FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPCOPYMENU, 0, idCmdFirst, DropCopy);
		if ((isInSVN)&&(isFolderInSVN)&&(!isAdded))
			InsertSVNMenu(FALSE, FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPCOPYADDMENU, 0, idCmdFirst, DropCopyAdd);
		if (isInSVN)
			InsertSVNMenu(FALSE, FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPEXPORTMENU, 0, idCmdFirst, DropExport);
		if (isInSVN)
			InsertSVNMenu(FALSE, FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPEXPORTEXTENDEDMENU, 0, idCmdFirst, DropExportExtended);
		if (idCmd != idCmdFirst)
			InsertMenu(hMenu, indexMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL); 

		return ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, 0, (USHORT)(idCmd - idCmdFirst)));
	}

	PreserveChdir preserveChdir;

	if ((uFlags & CMF_DEFAULTONLY)!=0)
		return NOERROR;					//we don't change the default action

	if ((uFlags & CMF_VERBSONLY)!=0)
		return NOERROR;					//we don't show the menu on shortcuts

	if ((files_.size() == 0)&&(folder_.size() == 0))
		return NOERROR;

	if (((uFlags & 0x000f)!=CMF_NORMAL)&&(!(uFlags & CMF_EXPLORE))&&(!(uFlags & CMF_VERBSONLY)))
		return NOERROR;
	//check if our menu is requested for the start menu
	TCHAR buf[MAX_PATH];	//MAX_PATH ok, since SHGetSpecialFolderPath doesn't return the required buffer length!
	if (SHGetSpecialFolderPath(NULL, buf, CSIDL_STARTMENU, FALSE))
	{
		if (_tcscmp(buf, folder_.c_str())==0)
			return NOERROR;
	}
	//check if our menu is requested for the trash bin folder
	if (SHGetSpecialFolderPath(NULL, buf, CSIDL_BITBUCKET, FALSE))
	{
		if (_tcscmp(buf, folder_.c_str())==0)
			return NOERROR;
	}
	//check if our menu is requested for the virtual folder containing icons for the Control Panel applications
	if (SHGetSpecialFolderPath(NULL, buf, CSIDL_CONTROLS, FALSE))
	{
		if (_tcscmp(buf, folder_.c_str())==0)
			return NOERROR;
	}
	//check if our menu is requested for the cookies folder
	if (SHGetSpecialFolderPath(NULL, buf, CSIDL_COOKIES, FALSE))
	{
		if (_tcscmp(buf, folder_.c_str())==0)
			return NOERROR;
	}
	//check if our menu is requested for the internet folder
	if (SHGetSpecialFolderPath(NULL, buf, CSIDL_INTERNET, FALSE))
	{
		if (_tcscmp(buf, folder_.c_str())==0)
			return NOERROR;
	}
	//check if our menu is requested for the printer folder
	if (SHGetSpecialFolderPath(NULL, buf, CSIDL_PRINTERS, FALSE))
	{
		if (_tcscmp(buf, folder_.c_str())==0)
			return NOERROR;
	}
	
	//check if our menu is requested for a subversion admin directory
	stdstring sAdm = _T("\\");
	sAdm += _T(SVN_WC_ADM_DIR_NAME);
	if ((folder_.length() > sAdm.length())&&(folder_.compare(folder_.length()-sAdm.length(), sAdm.length(), sAdm)==0))
		return NOERROR;

	LoadLangDll();
	//bool extended = ((uFlags & CMF_EXTENDEDVERBS)!=0);		//true if shift was pressed for the context menu
	UINT idCmd = idCmdFirst;

	//create the submenu
	HMENU subMenu = CreateMenu();
	int indexSubMenu = 0;
	int lastSeparator = 0;

	DWORD topmenu = g_ShellCache.GetMenuLayout();
//#region menu
#define HMENU(x) ((topmenu & (x)) ? hMenu : subMenu)
#define INDEXMENU(x) ((topmenu & (x)) ? indexMenu++ : indexSubMenu++)
#define ISTOP(x) (topmenu &(x))
	//---- separator before
	InsertMenu(hMenu, indexMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL); idCmd++;

	BOOL ownerdrawn = CRegStdWORD(_T("Software\\TortoiseSVN\\OwnerdrawnMenus"), TRUE);
	//now fill in the entries 
	if ((!isInSVN)&&(isFolder))
		InsertSVNMenu(ownerdrawn, ISTOP(MENUCHECKOUT), HMENU(MENUCHECKOUT), INDEXMENU(MENUCHECKOUT), idCmd++, IDS_MENUCHECKOUT, IDI_CHECKOUT, idCmdFirst, Checkout);

	if ((isInSVN)&&(!isAdded))
		InsertSVNMenu(ownerdrawn, ISTOP(MENUUPDATE), HMENU(MENUUPDATE), INDEXMENU(MENUUPDATE), idCmd++, IDS_MENUUPDATE, IDI_UPDATE, idCmdFirst, Update);

	if (isInSVN)
		InsertSVNMenu(ownerdrawn, ISTOP(MENUCOMMIT), HMENU(MENUCOMMIT), INDEXMENU(MENUCOMMIT), idCmd++, IDS_MENUCOMMIT, IDI_COMMIT, idCmdFirst, Commit);
	
	//---- separator 
	if ((idCmd != (UINT)(lastSeparator + 1)) && (indexSubMenu != 0))
	{
		InsertMenu(subMenu, indexSubMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL); 
		lastSeparator = idCmd++;
	}

	if ((isInSVN)&&(!isNormal)&&(isOnlyOneItemSelected)&&(!isFolder)&&(!isConflicted))
		InsertSVNMenu(ownerdrawn, ISTOP(MENUDIFF), HMENU(MENUDIFF),INDEXMENU(MENUDIFF), idCmd++, IDS_MENUDIFF, IDI_DIFF, idCmdFirst, Diff);

	if (files_.size() == 2)	//compares the two selected files
		InsertSVNMenu(ownerdrawn, ISTOP(MENUDIFF), HMENU(MENUDIFF), INDEXMENU(MENUDIFF), idCmd++, IDS_MENUDIFF, IDI_DIFF, idCmdFirst, Diff);

	if (((isInSVN)&&(isOnlyOneItemSelected)&&(!isAdded))||((isFolder)&&(isFolderInSVN)&&(!isAdded)))
		InsertSVNMenu(ownerdrawn, ISTOP(MENULOG), HMENU(MENULOG), INDEXMENU(MENULOG), idCmd++, IDS_MENULOG, IDI_LOG, idCmdFirst, Log);

	if ((isOnlyOneItemSelected)||(isFolder))
		InsertSVNMenu(ownerdrawn, ISTOP(MENUREPOBROWSE), HMENU(MENUREPOBROWSE), INDEXMENU(MENUREPOBROWSE), idCmd++, IDS_MENUREPOBROWSE, IDI_REPOBROWSE, idCmdFirst, RepoBrowse);
	if (((isInSVN)&&(isOnlyOneItemSelected))||((isFolder)&&(isFolderInSVN)))
		InsertSVNMenu(ownerdrawn, ISTOP(MENUSHOWCHANGED), HMENU(MENUSHOWCHANGED), INDEXMENU(MENUSHOWCHANGED), idCmd++, IDS_MENUSHOWCHANGED, IDI_SHOWCHANGED, idCmdFirst, ShowChanged);
	if (((isInSVN)&&(isOnlyOneItemSelected)&&(!isAdded))||((isFolder)&&(isFolderInSVN)&&(!isAdded)))
		InsertSVNMenu(ownerdrawn, ISTOP(MENUREVISIONGRAPH), HMENU(MENUREVISIONGRAPH), INDEXMENU(MENUREVISIONGRAPH), idCmd++, IDS_MENUREVISIONGRAPH, IDI_REVISIONGRAPH, idCmdFirst, RevisionGraph);

	//---- separator 
	if ((idCmd != (UINT)(lastSeparator + 1)) && (indexSubMenu != 0))
	{
		InsertMenu(subMenu, indexSubMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL); 
		lastSeparator = idCmd++;
	}

	if ((isInSVN)&&(isConflicted)&&(isOnlyOneItemSelected))
	{
		if(!isFolder)
			InsertSVNMenu(ownerdrawn, ISTOP(MENUCONFLICTEDITOR), HMENU(MENUCONFLICTEDITOR), INDEXMENU(MENUCONFLICTEDITOR), idCmd++, IDS_MENUCONFLICT, IDI_CONFLICT, idCmdFirst, ConflictEditor);
		InsertSVNMenu(ownerdrawn, ISTOP(MENURESOLVE), HMENU(MENURESOLVE), INDEXMENU(MENURESOLVE), idCmd++, IDS_MENURESOLVE, IDI_RESOLVE, idCmdFirst, Resolve);
	}
	if ((isInSVN)&&(!isAdded))
		InsertSVNMenu(ownerdrawn, ISTOP(MENUUPDATEEXT), HMENU(MENUUPDATEEXT), INDEXMENU(MENUUPDATEEXT), idCmd++, IDS_MENUUPDATEEXT, IDI_UPDATE, idCmdFirst, UpdateExt);
	if ((isInSVN)&&(isOnlyOneItemSelected)&&(!isAdded))
		InsertSVNMenu(ownerdrawn, ISTOP(MENURENAME), HMENU(MENURENAME), INDEXMENU(MENURENAME), idCmd++, IDS_MENURENAME, IDI_RENAME, idCmdFirst, Rename);
	if ((isInSVN)&&(!isAdded))
		InsertSVNMenu(ownerdrawn, ISTOP(MENUREMOVE), HMENU(MENUREMOVE), INDEXMENU(MENUREMOVE), idCmd++, IDS_MENUREMOVE, IDI_DELETE, idCmdFirst, Remove);
	if (((isInSVN)&&(!isNormal))||((isFolder)&&(isFolderInSVN)))
		InsertSVNMenu(ownerdrawn, ISTOP(MENUREVERT), HMENU(MENUREVERT), INDEXMENU(MENUREVERT), idCmd++, IDS_MENUREVERT, IDI_REVERT, idCmdFirst, Revert);
	if ((isInSVN)&&(isFolder)&&(isFolderInSVN))
		InsertSVNMenu(ownerdrawn, ISTOP(MENUCLEANUP), HMENU(MENUCLEANUP), INDEXMENU(MENUCLEANUP), idCmd++, IDS_MENUCLEANUP, IDI_CLEANUP, idCmdFirst, Cleanup);
	if ((isInSVN)&&(!isFolder)&&(!isLocked))
		InsertSVNMenu(ownerdrawn, ISTOP(MENULOCK), HMENU(MENULOCK), INDEXMENU(MENULOCK), idCmd++, IDS_MENU_LOCK, IDI_LOCK, idCmdFirst, Lock);
	if ((isInSVN)&&(!isFolder)&&(isLocked)&&(!(uFlags & CMF_EXTENDEDVERBS)))
		InsertSVNMenu(ownerdrawn, ISTOP(MENUUNLOCK), HMENU(MENUUNLOCK), INDEXMENU(MENUUNLOCK), idCmd++, IDS_MENU_UNLOCK, IDI_UNLOCK, idCmdFirst, Unlock);
	if ((isInSVN)&&(!isFolder)&&(isLocked)&&(uFlags & CMF_EXTENDEDVERBS))
		InsertSVNMenu(ownerdrawn, ISTOP(MENUUNLOCK), HMENU(MENUUNLOCK), INDEXMENU(MENUUNLOCK), idCmd++, IDS_MENU_UNLOCKFORCE, IDI_UNLOCK, idCmdFirst, UnlockForce);

	//---- separator 
	if ((idCmd != (UINT)(lastSeparator + 1)) && (indexSubMenu != 0))
	{
		InsertMenu(subMenu, indexSubMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL); 
		lastSeparator = idCmd++;
	}

	if ((isInSVN)&&(((isOnlyOneItemSelected)&&(!isAdded))||((isFolder)&&(isFolderInSVN))))
		InsertSVNMenu(ownerdrawn, ISTOP(MENUCOPY), HMENU(MENUCOPY), INDEXMENU(MENUCOPY), idCmd++, IDS_MENUBRANCH, IDI_COPY, idCmdFirst, Copy);
	if ((isInSVN)&&(((isOnlyOneItemSelected)&&(!isAdded))||((isFolder)&&(isFolderInSVN))))
		InsertSVNMenu(ownerdrawn, ISTOP(MENUSWITCH), HMENU(MENUSWITCH), INDEXMENU(MENUSWITCH), idCmd++, IDS_MENUSWITCH, IDI_SWITCH, idCmdFirst, Switch);
	if ((isInSVN)&&(((isOnlyOneItemSelected)&&(!isAdded))||((isFolder)&&(isFolderInSVN))))
		InsertSVNMenu(ownerdrawn, ISTOP(MENUMERGE), HMENU(MENUMERGE), INDEXMENU(MENUMERGE), idCmd++, IDS_MENUMERGE, IDI_MERGE, idCmdFirst, Merge);
	if (isFolder)
		InsertSVNMenu(ownerdrawn, ISTOP(MENUEXPORT), HMENU(MENUEXPORT), INDEXMENU(MENUEXPORT), idCmd++, IDS_MENUEXPORT, IDI_EXPORT, idCmdFirst, Export);
	if ((isInSVN)&&(isFolder)&&(isFolderInSVN))
		InsertSVNMenu(ownerdrawn, ISTOP(MENURELOCATE), HMENU(MENURELOCATE), INDEXMENU(MENURELOCATE), idCmd++, IDS_MENURELOCATE, IDI_RELOCATE, idCmdFirst, Relocate);

	//---- separator 
	if ((idCmd != (UINT)(lastSeparator + 1)) && (indexSubMenu != 0))
	{
		InsertMenu(subMenu, indexSubMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL); 
		lastSeparator = idCmd++;
	}

	if ((!isInSVN)&&(isFolder)&&(!isFolderInSVN))
		InsertSVNMenu(ownerdrawn, ISTOP(MENUCREATEREPOS), HMENU(MENUCREATEREPOS), INDEXMENU(MENUCREATEREPOS), idCmd++, IDS_MENUCREATEREPOS, IDI_CREATEREPOS, idCmdFirst, CreateRepos);
	if ((!isInSVN && isInVersionedFolder)||(isInSVN && isFolder)||(isIgnored))
		InsertSVNMenu(ownerdrawn, ISTOP(MENUADD), HMENU(MENUADD), INDEXMENU(MENUADD), idCmd++, IDS_MENUADD, IDI_ADD, idCmdFirst, Add);
	if ((!isInSVN)&&(isFolder))
		InsertSVNMenu(ownerdrawn, ISTOP(MENUIMPORT), HMENU(MENUIMPORT), INDEXMENU(MENUIMPORT), idCmd++, IDS_MENUIMPORT, IDI_IMPORT, idCmdFirst, Import);
	if ((isInSVN)&&(!isFolder)&&(!isAdded)&&(isOnlyOneItemSelected))
		InsertSVNMenu(ownerdrawn, ISTOP(MENUBLAME), HMENU(MENUBLAME), INDEXMENU(MENUBLAME), idCmd++, IDS_MENUBLAME, IDI_BLAME, idCmdFirst, Blame);
	if (((!isInSVN)&&(!isIgnored)&&(isInVersionedFolder))||(isOnlyOneItemSelected && isIgnored && (ignoredprops.size() > 0)))
	{
		HMENU ignoresubmenu = NULL;
		int indexignoresub = 0;
		bool bShowIgnoreMenu = false;
		TCHAR maskbuf[MAX_PATH];		// MAX_PATH is ok, since this only holds a filename
		TCHAR ignorepath[MAX_PATH];		// MAX_PATH is ok, since this only holds a filename
		std::vector<stdstring>::iterator I = files_.begin();
		if (_tcsrchr(I->c_str(), '\\'))
			_tcscpy(ignorepath, _tcsrchr(I->c_str(), '\\')+1);
		else
			_tcscpy(ignorepath, I->c_str());
		if (isIgnored)
		{
			// check if the item name is ignored or the mask
			size_t p = ignoredprops.find(ignorepath);
			if ((p!=-1) &&
				((ignoredprops.compare(ignorepath)==0) || (ignoredprops.find('\n', p)==p+_tcslen(ignorepath)+1) || (ignoredprops.rfind('\n', p)==p-1)))
			{
				ignoresubmenu = CreateMenu();
				InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, ignorepath);
				myIDMap[idCmd - idCmdFirst] = UnIgnore;
				myIDMap[idCmd++] = UnIgnore;
				bShowIgnoreMenu = true;
			}
			_tcscpy(maskbuf, _T("*"));
			if (_tcsrchr(ignorepath, '.'))
			{
				_tcscat(maskbuf, _tcsrchr(ignorepath, '.'));
				p = ignoredprops.find(maskbuf);
				if ((p!=-1) &&
					((ignoredprops.compare(maskbuf)==0) || (ignoredprops.find('\n', p)==p+_tcslen(maskbuf)+1) || (ignoredprops.rfind('\n', p)==p-1)))
				{
					if (ignoresubmenu==NULL)
						ignoresubmenu = CreateMenu();

					InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, maskbuf);
					myIDMap[idCmd - idCmdFirst] = UnIgnoreCaseSensitive;
					myIDMap[idCmd++] = UnIgnoreCaseSensitive;
					bShowIgnoreMenu = true;
				}
			}
		}
		else
		{
			bShowIgnoreMenu = true;
			ignoresubmenu = CreateMenu();
			if (isOnlyOneItemSelected)
			{
				InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, ignorepath);
				myIDMap[idCmd - idCmdFirst] = Ignore;
				myIDMap[idCmd++] = Ignore;

				_tcscpy(maskbuf, _T("*"));
				if (_tcsrchr(ignorepath, '.'))
				{
					_tcscat(maskbuf, _tcsrchr(ignorepath, '.'));
					InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, maskbuf);
					myIDMap[idCmd - idCmdFirst] = IgnoreCaseSensitive;
					myIDMap[idCmd++] = IgnoreCaseSensitive;
				}
			}
			else
			{
				MAKESTRING(IDS_MENUIGNOREMULTIPLE);
				_stprintf(ignorepath, stringtablebuffer, files_.size());
				InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, ignorepath);
				myIDMap[idCmd - idCmdFirst] = Ignore;
				myIDMap[idCmd++] = Ignore;
			}
		}

		if (bShowIgnoreMenu)
		{
			MENUITEMINFO menuiteminfo;
			ZeroMemory(&menuiteminfo, sizeof(menuiteminfo));
			menuiteminfo.cbSize = sizeof(menuiteminfo);
			OSVERSIONINFOEX inf;
			ZeroMemory(&inf, sizeof(OSVERSIONINFOEX));
			inf.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
			GetVersionEx((OSVERSIONINFO *)&inf);
			WORD fullver = MAKEWORD(inf.dwMinorVersion, inf.dwMajorVersion);
			if ((ownerdrawn==1)&&(fullver >= 0x0501))
				menuiteminfo.fMask = MIIM_FTYPE | MIIM_ID | MIIM_SUBMENU;
			else if (ownerdrawn == 0)
				menuiteminfo.fMask = MIIM_STRING | MIIM_ID | MIIM_SUBMENU | MIIM_CHECKMARKS | MIIM_DATA;
			else
				menuiteminfo.fMask = MIIM_STRING | MIIM_ID | MIIM_SUBMENU| MIIM_DATA;
			menuiteminfo.fType = MFT_OWNERDRAW;
			HBITMAP bmp = IconToBitmap(IDI_IGNORE, (COLORREF)GetSysColor(COLOR_MENU));
			menuiteminfo.hbmpChecked = bmp;
			menuiteminfo.hbmpUnchecked = bmp;
			menuiteminfo.hSubMenu = ignoresubmenu;
			menuiteminfo.wID = idCmd;
			ZeroMemory(stringtablebuffer, sizeof(stringtablebuffer));
			if (isIgnored)
				GetMenuTextFromResource(UnIgnore);
			else
				GetMenuTextFromResource(IgnoreSub);
			menuiteminfo.dwTypeData = stringtablebuffer;
			menuiteminfo.cch = (UINT)min(_tcslen(menuiteminfo.dwTypeData), UINT_MAX);
			InsertMenuItem(HMENU(MENUIGNORE), INDEXMENU(MENUIGNORE), TRUE, &menuiteminfo);
			if (isIgnored)
			{
				myIDMap[idCmd - idCmdFirst] = UnIgnoreSub;
				myIDMap[idCmd++] = UnIgnoreSub;
			}
			else
			{
				myIDMap[idCmd - idCmdFirst] = IgnoreSub;
				myIDMap[idCmd++] = IgnoreSub;
			}
		}
	}

	//---- separator 
	if ((idCmd != (UINT)(lastSeparator + 1)) && (indexSubMenu != 0))
	{
		InsertMenu(subMenu, indexSubMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL); 
		lastSeparator = idCmd++;
	} // if ((idCmd != (lastSeparator + 1)) && (indexSubMenu != 0))

	if ((isInSVN)&&(!isAdded)&&((isOnlyOneItemSelected)||((isFolder)&&(isFolderInSVN))))
		InsertSVNMenu(ownerdrawn, ISTOP(MENUCREATEPATCH), HMENU(MENUCREATEPATCH), INDEXMENU(MENUCREATEPATCH), idCmd++, IDS_MENUCREATEPATCH, IDI_CREATEPATCH, idCmdFirst, CreatePatch);
	if (((isInSVN)&&(!isAdded)&&(isFolder)&&(isFolderInSVN))||(isOnlyOneItemSelected && isPatchFile))
		InsertSVNMenu(ownerdrawn, ISTOP(MENUAPPLYPATCH), HMENU(MENUAPPLYPATCH), INDEXMENU(MENUAPPLYPATCH), idCmd++, IDS_MENUAPPLYPATCH, IDI_PATCH, idCmdFirst, ApplyPatch);

	//---- separator 
	if ((idCmd != (UINT)(lastSeparator + 1)) && (indexSubMenu != 0))
	{
		InsertMenu(subMenu, indexSubMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL); 
		lastSeparator = idCmd++;
	}
	InsertSVNMenu(ownerdrawn, FALSE, subMenu, indexSubMenu++, idCmd++, IDS_MENUHELP, IDI_HELP, idCmdFirst, Help);
	InsertSVNMenu(ownerdrawn, FALSE, subMenu, indexSubMenu++, idCmd++, IDS_MENUSETTINGS, IDI_SETTINGS, idCmdFirst, Settings);
	InsertSVNMenu(ownerdrawn, FALSE, subMenu, indexSubMenu++, idCmd++, IDS_MENUABOUT, IDI_ABOUT, idCmdFirst, About);


	//add submenu to main context menu
	//don't use InsertMenu because this will lead to multiple menu entries in the explorer file menu.
	//see http://support.microsoft.com/default.aspx?scid=kb;en-us;214477 for details of that.
	MENUITEMINFO menuiteminfo;
	ZeroMemory(&menuiteminfo, sizeof(menuiteminfo));
	menuiteminfo.cbSize = sizeof(menuiteminfo);
	OSVERSIONINFOEX inf;
	ZeroMemory(&inf, sizeof(OSVERSIONINFOEX));
	inf.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	GetVersionEx((OSVERSIONINFO *)&inf);
	WORD fullver = MAKEWORD(inf.dwMinorVersion, inf.dwMajorVersion);
	if ((ownerdrawn==1)&&(fullver >= 0x0501))
		menuiteminfo.fMask = MIIM_FTYPE | MIIM_ID | MIIM_SUBMENU;
	else if (ownerdrawn == 0)
		menuiteminfo.fMask = MIIM_STRING | MIIM_ID | MIIM_SUBMENU | MIIM_CHECKMARKS | MIIM_DATA;
	else
		menuiteminfo.fMask = MIIM_STRING | MIIM_ID | MIIM_SUBMENU| MIIM_DATA;
	menuiteminfo.fType = MFT_OWNERDRAW;
 	menuiteminfo.dwTypeData = _T("TortoiseSVN\0\0");
	menuiteminfo.cch = (UINT)min(_tcslen(menuiteminfo.dwTypeData), UINT_MAX);
	HBITMAP bmp = IconToBitmap(IDI_MENU, (COLORREF)GetSysColor(COLOR_MENU));
	menuiteminfo.hbmpChecked = bmp;
	menuiteminfo.hbmpUnchecked = bmp;
	menuiteminfo.hSubMenu = subMenu;
	menuiteminfo.wID = idCmd;
	InsertMenuItem(hMenu, indexMenu++, TRUE, &menuiteminfo);
	myIDMap[idCmd - idCmdFirst] = SubMenu;
	myIDMap[idCmd++] = SubMenu;

	//separator after
	InsertMenu(hMenu, indexMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL); idCmd++;

	//return number of menu items added
	return ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, 0, (USHORT)(idCmd - idCmdFirst)));
//#endregion
}


// This is called when you invoke a command on the menu:
STDMETHODIMP CShellExt::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
	PreserveChdir preserveChdir;
	HRESULT hr = E_INVALIDARG;
	if (lpcmi == NULL)
		return hr;

	std::string command;
	std::string parent;
	std::string file;

	if (!HIWORD(lpcmi->lpVerb))
	{
		if ((files_.size() > 0)||(folder_.size() > 0))
		{
			UINT idCmd = LOWORD(lpcmi->lpVerb);

			// See if we have a handler interface for this id
			if (myIDMap.find(idCmd) != myIDMap.end())
			{
				STARTUPINFO startup;
				PROCESS_INFORMATION process;
				memset(&startup, 0, sizeof(startup));
				startup.cb = sizeof(startup);
				memset(&process, 0, sizeof(process));
				CRegStdString tortoiseProcPath(_T("Software\\TortoiseSVN\\ProcPath"), _T("TortoiseProc.exe"), false, HKEY_LOCAL_MACHINE);
				CRegStdString tortoiseMergePath(_T("Software\\TortoiseSVN\\TMergePath"), _T("TortoiseMerge.exe"), false, HKEY_LOCAL_MACHINE);

				//TortoiseProc expects a command line of the form:
				//"/command:<commandname> /path:<path> /revstart:<revisionstart> /revend:<revisionend>
				//path is either a path to a single file/directory for commands which only act on single files (log, checkout, ...)
				//or a path to a temporary file which contains a list of filepaths
				stdstring svnCmd = _T(" /command:");
				stdstring tempfile;
				switch (myIDMap[idCmd])
				{
					//#region case
					case Checkout:
						svnCmd += _T("checkout /path:\"");
						svnCmd += folder_;
						svnCmd += _T("\"");
						break;
					case Update:
						tempfile = WriteFileListToTempFile();
						svnCmd += _T("update /path:\"");
						svnCmd += tempfile;
						svnCmd += _T("\"");
						break;
					case UpdateExt:
						tempfile = WriteFileListToTempFile();
						svnCmd += _T("update /path:\"");
						svnCmd += tempfile;
						svnCmd += _T("\" /rev");
						break;
					case Commit:
						tempfile = WriteFileListToTempFile();
						svnCmd += _T("commit /path:\"");
						svnCmd += tempfile;
						svnCmd += _T("\"");
						break;
					case Add:
						tempfile = WriteFileListToTempFile();
						svnCmd += _T("add /path:\"");
						svnCmd += tempfile;
						svnCmd += _T("\"");
						break;
					case Ignore:
						tempfile = WriteFileListToTempFile();
						svnCmd += _T("ignore /path:\"");
						svnCmd += tempfile;
						svnCmd += _T("\"");
						break;
					case IgnoreCaseSensitive:
						tempfile = WriteFileListToTempFile();
						svnCmd += _T("ignore /path:\"");
						svnCmd += tempfile;
						svnCmd += _T("\"");
						svnCmd += _T(" /onlymask");
						break;
					case UnIgnore:
						tempfile = WriteFileListToTempFile();
						svnCmd += _T("unignore /path:\"");
						svnCmd += tempfile;
						svnCmd += _T("\"");
						break;
					case UnIgnoreCaseSensitive:
						tempfile = WriteFileListToTempFile();
						svnCmd += _T("unignore /path:\"");
						svnCmd += tempfile;
						svnCmd += _T("\"");
						svnCmd += _T(" /onlymask");
						break;
					case Revert:
						tempfile = WriteFileListToTempFile();
						svnCmd += _T("revert /path:\"");
						svnCmd += tempfile;
						svnCmd += _T("\"");
						break;
					case Cleanup:
						svnCmd += _T("cleanup /path:\"");
						svnCmd += folder_;
						svnCmd += _T("\"");
						break;
					case Resolve:
						svnCmd += _T("resolve /path:\"");
						if (files_.size() > 0)
							svnCmd += files_.front();
						else
							svnCmd += folder_;
						svnCmd += _T("\"");
						break;
					case Switch:
						svnCmd += _T("switch /path:\"");
						if (files_.size() > 0)
							svnCmd += files_.front();
						else
							svnCmd += folder_;
						svnCmd += _T("\"");
						break;
					case Import:
						svnCmd += _T("import /path:\"");
						svnCmd += folder_;
						svnCmd += _T("\"");
						break;
					case Export:
						svnCmd += _T("export /path:\"");
						svnCmd += folder_;
						svnCmd += _T("\"");
						break;
					case About:
						svnCmd += _T("about");
						break;
					case CreateRepos:
						svnCmd += _T("repocreate /path:\"");
						svnCmd += folder_;
						svnCmd += _T("\"");
						break;
					case Merge:
						svnCmd += _T("merge /path:\"");
						if (files_.size() > 0)
							svnCmd += files_.front();
						else
							svnCmd += folder_;
						svnCmd += _T("\"");
						break;
					case Copy:
						svnCmd += _T("copy /path:\"");
						if (files_.size() > 0)
							svnCmd += files_.front();
						else
							svnCmd += folder_;
						svnCmd += _T("\"");
						break;
					case Settings:
						svnCmd += _T("settings");
						break;
					case Help:
						svnCmd += _T("help");
						break;
					case Rename:
						svnCmd += _T("rename /path:\"");
						if (files_.size() > 0)
							svnCmd += files_.front();
						else
							svnCmd += folder_;
						svnCmd += _T("\"");
						break;
					case Remove:
						tempfile = WriteFileListToTempFile();
						svnCmd += _T("remove /path:\"");
						svnCmd += tempfile;
						svnCmd += _T("\"");
						break;
					case Diff:
						svnCmd += _T("diff /path:\"");
						if (files_.size() == 1)
							svnCmd += files_.front();
						else if (files_.size() == 2)
						{
							std::vector<stdstring>::iterator I = files_.begin();
							svnCmd += *I;
							I++;
							svnCmd += _T("\" /path2:\"");
							svnCmd += *I;
						}
						else
							svnCmd += folder_;
						svnCmd += _T("\"");
						break;
					case DropCopyAdd:
						tempfile = WriteFileListToTempFile();
						svnCmd += _T("dropcopyadd /path:\"");
						svnCmd += tempfile;
						svnCmd += _T("\"");
						svnCmd += _T(" /droptarget:\"");
						svnCmd += folder_;
						svnCmd += _T("\"";)
						break;
					case DropCopy:
						tempfile = WriteFileListToTempFile();
						svnCmd += _T("dropcopy /path:\"");
						svnCmd += tempfile;
						svnCmd += _T("\"");
						svnCmd += _T(" /droptarget:\"");
						svnCmd += folder_;
						svnCmd += _T("\"";)
						break;
					case DropMove:
						tempfile = WriteFileListToTempFile();
						svnCmd += _T("dropmove /path:\"");
						svnCmd += tempfile;
						svnCmd += _T("\"");
						svnCmd += _T(" /droptarget:\"");
						svnCmd += folder_;
						svnCmd += _T("\"");
						break;
					case DropExport:
						tempfile = WriteFileListToTempFile();
						svnCmd += _T("dropexport /path:\"");
						svnCmd += tempfile;
						svnCmd += _T("\"");
						svnCmd += _T(" /droptarget:\"");
						svnCmd += folder_;
						svnCmd += _T("\"");
						break;
					case DropExportExtended:
						tempfile = WriteFileListToTempFile();
						svnCmd += _T("dropexport /path:\"");
						svnCmd += tempfile;
						svnCmd += _T("\"");
						svnCmd += _T(" /droptarget:\"");
						svnCmd += folder_;
						svnCmd += _T("\"");
						svnCmd += _T(" /extended");
						break;
					case Log:
						svnCmd += _T("log /path:\"");
						if (files_.size() > 0)
							svnCmd += files_.front();
						else
							svnCmd += folder_;
						svnCmd += _T("\"");
						break;
					case ConflictEditor:
						svnCmd += _T("conflicteditor /path:\"");
						if (files_.size() > 0)
							svnCmd += files_.front();
						else
							svnCmd += folder_;
						svnCmd += _T("\"");
						break;
					case Relocate:
						svnCmd += _T("relocate /path:\"");
						if (files_.size() > 0)
							svnCmd += files_.front();
						else
							svnCmd += folder_;
						svnCmd += _T("\"");
						break;
					case ShowChanged:
						svnCmd += _T("repostatus /path:\"");
						if (files_.size() > 0)
							svnCmd += files_.front();
						else
							svnCmd += folder_;
						svnCmd += _T("\"");
						break;
					case RepoBrowse:
						svnCmd += _T("repobrowser /path:\"");
						if (files_.size() > 0)
							svnCmd += files_.front();
						else
							svnCmd += folder_;
						svnCmd += _T("\"");
						break;
					case Blame:
						svnCmd += _T("blame /path:\"");
						if (files_.size() > 0)
							svnCmd += files_.front();
						else
							svnCmd += folder_;
						svnCmd += _T("\"");
						break;
					case CreatePatch:
						svnCmd += _T("createpatch /path:\"");
						if (files_.size() > 0)
							svnCmd += files_.front();
						else
							svnCmd += folder_;
						svnCmd += _T("\"");
						break;
					case ApplyPatch:
						if (isPatchFile)
						{
							svnCmd = _T(" /diff:\"");
							if (files_.size() > 0)
								svnCmd += files_.front();
							else
								svnCmd += folder_;
							svnCmd += _T("\"");
						}
						else
						{
							svnCmd = _T(" /patchpath:\"");
							if (files_.size() > 0)
								svnCmd += files_.front();
							else
								svnCmd += folder_;
							svnCmd += _T("\"");
						}
						myIDMap.clear();
						if (CreateProcess(tortoiseMergePath, const_cast<TCHAR*>(svnCmd.c_str()), NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0)
						{
							LPVOID lpMsgBuf;
							FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
								FORMAT_MESSAGE_FROM_SYSTEM | 
								FORMAT_MESSAGE_IGNORE_INSERTS,
								NULL,
								GetLastError(),
								MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
								(LPTSTR) &lpMsgBuf,
								0,
								NULL 
								);
							MessageBox( NULL, (LPCTSTR)lpMsgBuf, _T("Error"), MB_OK | MB_ICONINFORMATION );
							LocalFree( lpMsgBuf );
						} // if (CreateProcess(tortoiseMergePath, const_cast<TCHAR*>(svnCmd.c_str()), NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0) 
						return NOERROR;
						break;
					case RevisionGraph:
						svnCmd += _T("revisiongraph /path:\"");
						if (files_.size() > 0)
							svnCmd += files_.front();
						else
							svnCmd += folder_;
						svnCmd += _T("\"");
						break;
					case Lock:
						tempfile = WriteFileListToTempFile();
						svnCmd += _T("lock /path:\"");
						svnCmd += tempfile;
						svnCmd += _T("\"");
						break;
					case Unlock:
						tempfile = WriteFileListToTempFile();
						svnCmd += _T("unlock /path:\"");
						svnCmd += tempfile;
						svnCmd += _T("\"");
						break;
					case UnlockForce:
						tempfile = WriteFileListToTempFile();
						svnCmd += _T("unlock /path:\"");
						svnCmd += tempfile;
						svnCmd += _T("\" /force");
						break;
					default:
						break;
					//#endregion
				} // switch (myIDMap[idCmd])
				svnCmd += _T(" /hwnd:");
				TCHAR buf[30];
				_stprintf(buf, _T("%d"), lpcmi->hwnd);
				svnCmd += buf;
				myIDMap.clear();
				if (CreateProcess(tortoiseProcPath, const_cast<TCHAR*>(svnCmd.c_str()), NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0)
				{
					LPVOID lpMsgBuf;
					FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
									FORMAT_MESSAGE_FROM_SYSTEM | 
									FORMAT_MESSAGE_IGNORE_INSERTS,
									NULL,
									GetLastError(),
									MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
									(LPTSTR) &lpMsgBuf,
									0,
									NULL 
									);
					MessageBox( NULL, (LPCTSTR)lpMsgBuf, _T("Error"), MB_OK | MB_ICONINFORMATION );
					LocalFree( lpMsgBuf );
				} // if (CreateProcess(tortoiseProcPath, const_cast<TCHAR*>(svnCmd.c_str()), NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0) 
				hr = NOERROR;
			} // if (myIDMap.find(idCmd) != myIDMap.end()) 
		} // if ((files_.size() > 0)||(folder_.size() > 0)) 
	} // if (!HIWORD(lpcmi->lpVerb)) 
	return hr;

}

// This is for the status bar and things like that:
STDMETHODIMP CShellExt::GetCommandString(UINT_PTR idCmd,
                                         UINT uFlags,
                                         UINT FAR * /*reserved*/,
                                         LPSTR pszName,
                                         UINT cchMax)
{   
	//do we know the id?
	if (myIDMap.find(idCmd) == myIDMap.end())
	{
		return E_INVALIDARG;		//no, we don't
	}

	LoadLangDll();
	HRESULT hr = E_INVALIDARG;
	switch (myIDMap[idCmd])
	{
		case Checkout:
			MAKESTRING(IDS_MENUDESCCHECKOUT);
			break;
		case Update:
			MAKESTRING(IDS_MENUDESCUPDATE);
			break;
		case UpdateExt:
			MAKESTRING(IDS_MENUDESCUPDATEEXT);
			break;
		case Commit:
			MAKESTRING(IDS_MENUDESCCOMMIT);
			break;
		case Add:
			MAKESTRING(IDS_MENUDESCADD);
			break;
		case Revert:
			MAKESTRING(IDS_MENUDESCREVERT);
			break;
		case Cleanup:
			MAKESTRING(IDS_MENUDESCCLEANUP);
			break;
		case Resolve:
			MAKESTRING(IDS_MENUDESCRESOLVE);
			break;
		case Switch:
			MAKESTRING(IDS_MENUDESCSWITCH);
			break;
		case Import:
			MAKESTRING(IDS_MENUDESCIMPORT);
			break;
		case Export:
			MAKESTRING(IDS_MENUDESCEXPORT);
			break;
		case About:
			MAKESTRING(IDS_MENUDESCABOUT);
			break;
		case Help:
			MAKESTRING(IDS_MENUDESCHELP);
			break;
		case CreateRepos:
			MAKESTRING(IDS_MENUDESCCREATEREPOS);
			break;
		case Copy:
			MAKESTRING(IDS_MENUDESCCOPY);
			break;
		case Merge:
			MAKESTRING(IDS_MENUDESCMERGE);
			break;
		case Settings:
			MAKESTRING(IDS_MENUDESCSETTINGS);
			break;
		case Rename:
			MAKESTRING(IDS_MENUDESCRENAME);
			break;
		case Remove:
			MAKESTRING(IDS_MENUDESCREMOVE);
			break;
		case Diff:
			MAKESTRING(IDS_MENUDESCDIFF);
			break;
		case Log:
			MAKESTRING(IDS_MENUDESCLOG);
			break;
		case ConflictEditor:
			MAKESTRING(IDS_MENUDESCCONFLICT);
			break;
		case Relocate:
			MAKESTRING(IDS_MENUDESCRELOCATE);
			break;
		case ShowChanged:
			MAKESTRING(IDS_MENUDESCSHOWCHANGED);
			break;
		case RepoBrowse:
			MAKESTRING(IDS_MENUDESCREPOBROWSE);
			break;
		case UnIgnore:
			MAKESTRING(IDS_MENUDESCUNIGNORE);
			break;
		case UnIgnoreCaseSensitive:
			MAKESTRING(IDS_MENUDESCUNIGNORE);
			break;
		case Ignore:
			MAKESTRING(IDS_MENUDESCIGNORE);
			break;
		case IgnoreCaseSensitive:
			MAKESTRING(IDS_MENUDESCIGNORE);
			break;
		case Blame:
			MAKESTRING(IDS_MENUDESCBLAME);
			break;
		case ApplyPatch:
			MAKESTRING(IDS_MENUDESCAPPLYPATCH);
			break;
		case CreatePatch:
			MAKESTRING(IDS_MENUDESCCREATEPATCH);
			break;
		case RevisionGraph:
			MAKESTRING(IDS_MENUDESCREVISIONGRAPH);
			break;
		case Lock:
			MAKESTRING(IDS_MENUDESC_LOCK);
			break;
		case Unlock:
			MAKESTRING(IDS_MENUDESC_UNLOCK);
			break;
		case UnlockForce:
			MAKESTRING(IDS_MENUDESC_UNLOCKFORCE);
			break;
		default:
			MAKESTRING(IDS_MENUDESCDEFAULT);
			break;
	} // switch (myIDMap[idCmd])
	const TCHAR * desc = stringtablebuffer;
	switch(uFlags)
	{
#ifdef UNICODE
	case GCS_HELPTEXTA:
		{
			std::string help = WideToMultibyte(desc);
			lstrcpynA(pszName, help.c_str(), cchMax);
			hr = S_OK;
			break; 
		}
	case GCS_HELPTEXTW: 
		{
			wide_string help = desc;
			lstrcpynW((LPWSTR)pszName, help.c_str(), cchMax); 
			hr = S_OK;
			break; 
		}
#else
	case GCS_HELPTEXTA:
		{
			std::string help = desc;
			lstrcpynA(pszName, help.c_str(), cchMax);
			hr = S_OK;
			break; 
		}
	case GCS_HELPTEXTW: 
		{
			wide_string help = MultibyteToWide(desc);
			lstrcpynW((LPWSTR)pszName, help.c_str(), cchMax); 
			hr = S_OK;
			break; 
		}
#endif
	}
	return hr;
}

STDMETHODIMP CShellExt::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   LRESULT res;
   return HandleMenuMsg2(uMsg, wParam, lParam, &res);
}

STDMETHODIMP CShellExt::HandleMenuMsg2(UINT uMsg, WPARAM /*wParam*/, LPARAM lParam, LRESULT *pResult)
{
//a great tutorial on owner drawn menus in shell extension can be found
//here: http://www.codeproject.com/shell/shellextguide7.asp

	LRESULT res;
	if (pResult == NULL)
		pResult = &res;
	*pResult = FALSE;

	LoadLangDll();
	switch (uMsg)
	{
		case WM_MEASUREITEM:
		{
			MEASUREITEMSTRUCT* lpmis = (MEASUREITEMSTRUCT*)lParam;
			if (lpmis==NULL)
				break;
			*pResult = TRUE;
			lpmis->itemWidth = 0;
			lpmis->itemHeight = 0;
			POINT size;
			//get the information about the shell dc, font, ...
			NONCLIENTMETRICS ncm;
			ncm.cbSize = sizeof(NONCLIENTMETRICS);
			if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0)==0)
				break;
			HDC hDC = CreateCompatibleDC(NULL);
			if (hDC == NULL)
				break;
			HFONT hFont = CreateFontIndirect(&ncm.lfMenuFont);
			if (hFont == NULL)
				break;
			HFONT oldFont = (HFONT)SelectObject(hDC, hFont);
			GetMenuTextFromResource(myIDMap[lpmis->itemID]);
			GetTextExtentPoint32(hDC, stringtablebuffer, _tcslen(stringtablebuffer), (SIZE*)&size);
			LPtoDP(hDC, &size, 1);
			//now release the font and dc
			SelectObject(hDC, oldFont);
			DeleteObject(hFont);
			DeleteDC(hDC);

			lpmis->itemWidth = size.x + size.y + 6;						//width of string + height of string (~ width of icon) + space between
			lpmis->itemHeight = max(size.y + 4, ncm.iMenuHeight);		//two pixels on both sides
		}
		break;
		case WM_DRAWITEM:
		{
			LPCTSTR resource;
			TCHAR *szItem;
			DRAWITEMSTRUCT* lpdis = (DRAWITEMSTRUCT*)lParam;
			if ((lpdis==NULL)||(lpdis->CtlType != ODT_MENU))
				return S_OK;		//not for a menu
			resource = GetMenuTextFromResource(myIDMap[lpdis->itemID]);
			if (resource == NULL)
				return S_OK;
			szItem = stringtablebuffer;
			if (lpdis->itemAction & (ODA_DRAWENTIRE|ODA_SELECT))
			{
				int ix, iy;
				RECT rt, rtTemp;
				SIZE size;
				//choose text colors
				if (lpdis->itemState & ODS_SELECTED)
				{
					COLORREF crText;
					if (lpdis->itemState & ODS_GRAYED)
						crText = SetTextColor(lpdis->hDC, GetSysColor(COLOR_GRAYTEXT)); //RGB(128, 128, 128));
					else
						crText = SetTextColor(lpdis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
					SetBkColor(lpdis->hDC, GetSysColor(COLOR_HIGHLIGHT));
				}
				CopyRect(&rtTemp, &(lpdis->rcItem));

				ix = lpdis->rcItem.left + space;
				SetRect(&rt, ix, rtTemp.top, ix + 16, rtTemp.bottom);

				HICON hIcon = (HICON)LoadImage(GetModuleHandle(_T("TortoiseSVN")), resource, IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);

				if (hIcon == NULL)
					return S_OK;
				//convert the icon into a bitmap
				ICONINFO ii;
				if (GetIconInfo(hIcon, &ii)==FALSE)
					return S_OK;
				HBITMAP hbmItem = ii.hbmColor; 
				if (hbmItem)
				{
					BITMAP bm;
					GetObject(hbmItem, sizeof(bm), &bm);

					int tempY = lpdis->rcItem.top + ((lpdis->rcItem.bottom - lpdis->rcItem.top) - bm.bmHeight) / 2;
					SetRect(&rt, ix, tempY, ix + 16, tempY + 16);

					ExtTextOut(lpdis->hDC, 0, 0, ETO_CLIPPED|ETO_OPAQUE, &rtTemp, NULL, 0, (LPINT)NULL);
					rtTemp.left = rt.right;

					HDC hDCTemp = CreateCompatibleDC(lpdis->hDC);
					SelectObject(hDCTemp, hbmItem);
					DrawIconEx(lpdis->hDC, rt.left , rt.top, hIcon, bm.bmWidth, bm.bmHeight,
						0, NULL, DI_NORMAL);

					DeleteDC(hDCTemp);
					DeleteObject(hbmItem);
				} // if (hbmItem)
				ix = rt.right + space;

				//free memory
				DeleteObject(ii.hbmColor);
				DeleteObject(ii.hbmMask);
				DestroyIcon(hIcon);

				//draw menu text
				GetTextExtentPoint32(lpdis->hDC, szItem, lstrlen(szItem), &size);
				iy = ((lpdis->rcItem.bottom - lpdis->rcItem.top) - size.cy) / 2;
				iy = lpdis->rcItem.top + (iy>=0 ? iy : 0);
				SetRect(&rt, ix , iy, lpdis->rcItem.right - 4, lpdis->rcItem.bottom);
				ExtTextOut(lpdis->hDC, ix, iy, ETO_CLIPPED|ETO_OPAQUE, &rtTemp, NULL, 0, (LPINT)NULL);
				if (lpdis->itemState & ODS_GRAYED)
				{        
					SetBkMode(lpdis->hDC, TRANSPARENT);
					OffsetRect( &rt, 1, 1 );
					SetTextColor( lpdis->hDC, RGB(255,255,255) );
					DrawText( lpdis->hDC, szItem, lstrlen(szItem), &rt, DT_LEFT|DT_EXPANDTABS );
					OffsetRect( &rt, -1, -1 );
					SetTextColor( lpdis->hDC, RGB(128,128,128) );
					DrawText( lpdis->hDC, szItem, lstrlen(szItem), &rt, DT_LEFT|DT_EXPANDTABS );         
				}
				else
					DrawText( lpdis->hDC, szItem, lstrlen(szItem), &rt, DT_LEFT|DT_EXPANDTABS );
			}
			*pResult = TRUE;
		}
		break;
		default:
			return NOERROR;
	}

	return NOERROR;
}

LPCTSTR CShellExt::GetMenuTextFromResource(int id)
{
	TCHAR textbuf[255];
	LPCTSTR resource = NULL;
	DWORD layout = g_ShellCache.GetMenuLayout();
	space = 6;
#define SETSPACE(x) space = ((layout & (x)) ? 0 : 6)
#define PREPENDSVN(x) if (layout & (x)) {_tcscpy(textbuf, _T("SVN "));_tcscat(textbuf, stringtablebuffer);_tcscpy(stringtablebuffer, textbuf);}
	switch (id)
	{
		case SubMenu:
			MAKESTRING(IDS_MENUSUBMENU);
			resource = MAKEINTRESOURCE(IDI_MENU);
			space = 0;
			break;
		case Checkout:
			MAKESTRING(IDS_MENUCHECKOUT);
			resource = MAKEINTRESOURCE(IDI_CHECKOUT);
			SETSPACE(MENUCHECKOUT);
			PREPENDSVN(MENUCHECKOUT);
			break;
		case Update:
			MAKESTRING(IDS_MENUUPDATE);
			resource = MAKEINTRESOURCE(IDI_UPDATE);
			SETSPACE(MENUUPDATE);
			PREPENDSVN(MENUUPDATE);
			break;
		case UpdateExt:
			MAKESTRING(IDS_MENUUPDATEEXT);
			resource = MAKEINTRESOURCE(IDI_UPDATE);
			SETSPACE(MENUUPDATEEXT);
			PREPENDSVN(MENUUPDATEEXT);
			break;
		case Log:
			MAKESTRING(IDS_MENULOG);
			resource = MAKEINTRESOURCE(IDI_LOG);
			SETSPACE(MENULOG);
			PREPENDSVN(MENULOG);
			break;
		case Commit:
			MAKESTRING(IDS_MENUCOMMIT);
			resource = MAKEINTRESOURCE(IDI_COMMIT);
			SETSPACE(MENUCOMMIT);
			PREPENDSVN(MENUCOMMIT);
			break;
		case CreateRepos:
			MAKESTRING(IDS_MENUCREATEREPOS);
			resource = MAKEINTRESOURCE(IDI_CREATEREPOS);
			SETSPACE(MENUCREATEREPOS);
			PREPENDSVN(MENUCREATEREPOS);
			break;
		case Add:
			MAKESTRING(IDS_MENUADD);
			resource = MAKEINTRESOURCE(IDI_ADD);
			SETSPACE(MENUADD);
			PREPENDSVN(MENUADD);
			break;
		case Revert:
			MAKESTRING(IDS_MENUREVERT);
			resource = MAKEINTRESOURCE(IDI_REVERT);
			SETSPACE(MENUREVERT);
			PREPENDSVN(MENUREVERT);
			break;
		case Cleanup:
			MAKESTRING(IDS_MENUCLEANUP);
			resource = MAKEINTRESOURCE(IDI_CLEANUP);
			SETSPACE(MENUCLEANUP);
			PREPENDSVN(MENUCLEANUP);
			break;
		case Resolve:
			MAKESTRING(IDS_MENURESOLVE);
			resource = MAKEINTRESOURCE(IDI_RESOLVE);
			SETSPACE(MENURESOLVE);
			PREPENDSVN(MENURESOLVE);
			break;
		case Switch:
			MAKESTRING(IDS_MENUSWITCH);
			resource = MAKEINTRESOURCE(IDI_SWITCH);
			SETSPACE(MENUSWITCH);
			PREPENDSVN(MENUSWITCH);
			break;
		case Import:
			MAKESTRING(IDS_MENUIMPORT);
			resource = MAKEINTRESOURCE(IDI_IMPORT);
			SETSPACE(MENUIMPORT);
			PREPENDSVN(MENUIMPORT);
			break;
		case Export:
			MAKESTRING(IDS_MENUEXPORT);
			resource = MAKEINTRESOURCE(IDI_EXPORT);
			SETSPACE(MENUEXPORT);
			PREPENDSVN(MENUEXPORT);
			break;
		case Copy:
			MAKESTRING(IDS_MENUBRANCH);
			resource = MAKEINTRESOURCE(IDI_COPY);
			SETSPACE(MENUSWITCH);
			PREPENDSVN(MENUSWITCH);
			break;
		case Merge:
			MAKESTRING(IDS_MENUMERGE);
			resource = MAKEINTRESOURCE(IDI_MERGE);
			SETSPACE(MENUMERGE);
			PREPENDSVN(MENUMERGE);
			break;
		case Remove:
			MAKESTRING(IDS_MENUREMOVE);
			resource = MAKEINTRESOURCE(IDI_DELETE);
			SETSPACE(MENUREMOVE);
			PREPENDSVN(MENUREMOVE);
			break;
		case Rename:
			MAKESTRING(IDS_MENURENAME);
			resource = MAKEINTRESOURCE(IDI_RENAME);
			SETSPACE(MENURENAME);
			PREPENDSVN(MENURENAME);
			break;
		case Diff:
			MAKESTRING(IDS_MENUDIFF);
			resource = MAKEINTRESOURCE(IDI_DIFF);
			SETSPACE(MENUDIFF);
			PREPENDSVN(MENUDIFF);
			break;
		case ConflictEditor:
			MAKESTRING(IDS_MENUCONFLICT);
			resource = MAKEINTRESOURCE(IDI_CONFLICT);
			SETSPACE(MENUCONFLICTEDITOR);
			PREPENDSVN(MENUCONFLICTEDITOR);
			break;
		case Relocate:
			MAKESTRING(IDS_MENURELOCATE);
			resource = MAKEINTRESOURCE(IDI_RELOCATE);
			SETSPACE(MENURELOCATE);
			PREPENDSVN(MENURELOCATE);
			break;
		case Settings:
			MAKESTRING(IDS_MENUSETTINGS);
			resource = MAKEINTRESOURCE(IDI_SETTINGS);
			break;
		case About:
			MAKESTRING(IDS_MENUABOUT);
			resource = MAKEINTRESOURCE(IDI_ABOUT);
			break;
		case Help:
			MAKESTRING(IDS_MENUHELP);
			resource = MAKEINTRESOURCE(IDI_HELP);
			break;
		case ShowChanged:
			MAKESTRING(IDS_MENUSHOWCHANGED);
			resource = MAKEINTRESOURCE(IDI_SHOWCHANGED);
			SETSPACE(MENUSHOWCHANGED);
			PREPENDSVN(MENUSHOWCHANGED);
			break;
		case RepoBrowse:
			MAKESTRING(IDS_MENUREPOBROWSE);
			resource = MAKEINTRESOURCE(IDI_REPOBROWSE);
			SETSPACE(MENUREPOBROWSE);
			PREPENDSVN(MENUREPOBROWSE);
			break;
		case IgnoreCaseSensitive:
		case IgnoreSub:
		case Ignore:
			MAKESTRING(IDS_MENUIGNORE);
			resource = MAKEINTRESOURCE(IDI_IGNORE);
			SETSPACE(MENUIGNORE);
			PREPENDSVN(MENUIGNORE);
			break;
		case UnIgnoreSub:
		case UnIgnoreCaseSensitive:
		case UnIgnore:
			MAKESTRING(IDS_MENUUNIGNORE);
			resource = MAKEINTRESOURCE(IDI_IGNORE);
			SETSPACE(MENUIGNORE);
			PREPENDSVN(MENUIGNORE);
			break;
		case Blame:
			MAKESTRING(IDS_MENUBLAME);
			resource = MAKEINTRESOURCE(IDI_BLAME);
			SETSPACE(MENUBLAME);
			PREPENDSVN(MENUBLAME);
			break;
		case ApplyPatch:
			MAKESTRING(IDS_MENUAPPLYPATCH);
			resource = MAKEINTRESOURCE(IDI_PATCH);
			SETSPACE(MENUAPPLYPATCH);
			PREPENDSVN(MENUAPPLYPATCH);
			break;
		case CreatePatch:
			MAKESTRING(IDS_MENUCREATEPATCH);
			resource = MAKEINTRESOURCE(IDI_CREATEPATCH);
			SETSPACE(MENUCREATEPATCH);
			PREPENDSVN(MENUCREATEPATCH);
			break;
		case RevisionGraph:
			MAKESTRING(IDS_MENUREVISIONGRAPH);
			resource = MAKEINTRESOURCE(IDI_REVISIONGRAPH);
			SETSPACE(MENUREVISIONGRAPH);
			PREPENDSVN(MENUREVISIONGRAPH);
			break;
		case Lock:
			MAKESTRING(IDS_MENU_LOCK);
			resource = MAKEINTRESOURCE(IDI_LOCK);
			SETSPACE(MENULOCK);
			PREPENDSVN(MENULOCK);
			break;
		case Unlock:
			MAKESTRING(IDS_MENU_UNLOCK);
			resource = MAKEINTRESOURCE(IDI_UNLOCK);
			SETSPACE(MENUUNLOCK);
			PREPENDSVN(MENUUNLOCK);
			break;
		case UnlockForce:
			MAKESTRING(IDS_MENU_UNLOCKFORCE);
			resource = MAKEINTRESOURCE(IDI_UNLOCK);
			SETSPACE(MENUUNLOCK);
			PREPENDSVN(MENUUNLOCK);
			break;
		default:
			return NULL;
	}
	return resource;
}


