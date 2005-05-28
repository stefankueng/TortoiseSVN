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
#include "TortoiseProc.h"
#include "messagebox.h"
#include "SysImageList.h"
#include "CrashReport.h"
#include "SVNProperties.h"
#include "Blame.h"
#include "DirFileEnum.h"
#include "CmdLineParser.h"
#include "AboutDlg.h"
#include "LogDlg.h"
#include "SVNProgressDlg.h"
#include "ImportDlg.h"
#include "CheckoutDlg.h"
#include "UpdateDlg.h"
#include "LogPromptDlg.h"
#include "AddDlg.h"
#include "RevertDlg.h"
#include "RepoCreateDlg.h"
#include "RenameDlg.h"
#include "SwitchDlg.h"
#include "MergeDlg.h"
#include "CopyDlg.h"
#include "Settings.h"
#include "RelocateDlg.h"
#include "URLDlg.h"
#include "ChangedDlg.h"
#include "RepositoryBrowser.h"
#include "BlameDlg.h"
#include "LockDlg.h"
#include "CheckForUpdatesDlg.h"
#include "RevisionGraphDlg.h"
#include "FileDiffDlg.h"
#include "InputDlg.h"
#include "BrowseFolder.h"
#include "SVNStatus.h"
#include "SVNInfo.h"
#include "Utils.h"
#include "SoundUtils.h"
#include "libintl.h"

#include "..\version.h"

#ifndef UNICODE
#define COMPILE_MULTIMON_STUBS
#include "multimon.h"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define PWND (hWndExplorer ? CWnd::FromHandle(hWndExplorer) : NULL)
#define EXPLORERHWND (IsWindow(hWndExplorer) ? hWndExplorer : NULL)

// This is a fake filename which we use to fill-in the create-patch file-open dialog
#define PATCH_TO_CLIPBOARD_PSEUDO_FILENAME		_T(".TSVNPatchToClipboard")

BEGIN_MESSAGE_MAP(CTortoiseProcApp, CWinApp)
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////

typedef enum
{
	cmdTest,
	cmdCrash,
	cmdAbout,
	cmdRTFM,
	cmdLog,
	cmdCheckout,
	cmdImport,
	cmdUpdate,
	cmdCommit,
	cmdAdd,
	cmdRevert,
	cmdCleanup,
	cmdResolve,
	cmdRepoCreate,
	cmdSwitch,
	cmdExport,
	cmdMerge,
	cmdCopy,
	cmdSettings,
	cmdRemove,
	cmdRename,
	cmdDiff,
	cmdDropCopyAdd,
	cmdDropMove,
	cmdDropExport,
	cmdDropCopy,
	cmdConflictEditor,
	cmdRelocate,
	cmdHelp,
	cmdRepoStatus,
	cmdRepoBrowser,
	cmdIgnore,
	cmdUnIgnore,
	cmdBlame,
	cmdCat,
	cmdCreatePatch,
	cmdUpdateCheck,
	cmdRevisionGraph,
	cmdLock,
	cmdUnlock
} TSVNCommand;

static const struct CommandInfo
{
	TSVNCommand command;
	LPCTSTR pCommandName;
	bool bPathIsTempFileByDefault;
} commandInfo[] = 
{
	//                                          PathIsTempFile?
	{	cmdTest,			_T("test"),				false	},
	{	cmdCrash,			_T("crash"),			false	},
	{	cmdAbout,			_T("about"),			false	},
	{	cmdRTFM,			_T("rtfm"),				false	},
	{	cmdLog,				_T("log"),				false	},
	{	cmdCheckout,		_T("checkout"),			false	},
	{	cmdImport,			_T("import"),			false	},
	{	cmdUpdate,			_T("update"),			true	},
	{	cmdCommit,			_T("commit"),			true	},
	{	cmdAdd,				_T("add"),				true	},
	{	cmdRevert,			_T("revert"),			true	},
	{	cmdCleanup,			_T("cleanup"),			false	},
	{	cmdResolve,			_T("resolve"),			false	},
	{	cmdRepoCreate,		_T("repocreate"),		false	},
	{	cmdSwitch,			_T("switch"),			false	},
	{	cmdExport,			_T("export"),			false	},
	{	cmdMerge,			_T("merge"),			false	},
	{	cmdCopy,			_T("copy"),				false	},
	{	cmdSettings,		_T("settings"),			false	},
	{	cmdRemove,			_T("remove"),			true	},
	{	cmdRename,			_T("rename"),			false	},
	{	cmdDiff,			_T("diff"),				false	},
	{	cmdDropCopyAdd,		_T("dropcopyadd"),		true	},
	{	cmdDropMove,		_T("dropmove"),			true	},
	{	cmdDropExport,		_T("dropexport"),		true	},
	{	cmdDropCopy,		_T("dropcopy"),			true	},
	{	cmdConflictEditor,	_T("conflicteditor"),	false	},
	{	cmdRelocate,		_T("relocate"),			false	},
	{	cmdHelp,			_T("help"),				false	},
	{	cmdRepoStatus,		_T("repostatus"),		false	},
	{	cmdRepoBrowser,		_T("repobrowser"),		false	},
	{	cmdIgnore,			_T("ignore"),			true	},
	{	cmdUnIgnore,		_T("unignore"),			true	},
	{	cmdBlame,			_T("blame"),			false	},
	{	cmdCat,				_T("cat"),				false	},
	{	cmdCreatePatch,		_T("createpatch"),		false	},
	{	cmdUpdateCheck,		_T("updatecheck"),		false	},
	{	cmdRevisionGraph,	_T("revisiongraph"),	false	},
	{	cmdLock,			_T("lock"),				true	},
	{	cmdUnlock,			_T("unlock"),			true	},
};

//////////////////////////////////////////////////////////////////////////



CTortoiseProcApp::CTortoiseProcApp()
{
	EnableHtmlHelp();
	int argc = 0;
	const char* const * argv = NULL;
	apr_app_initialize(&argc, &argv, NULL);
	SYS_IMAGE_LIST();
}

CTortoiseProcApp::~CTortoiseProcApp()
{
	apr_terminate();
	// seems that apr_initialize() is called every time the dll is loaded,
	// but since the dll isn't forcibly unloaded the apr_terminate() has
	// a count of > 1 and therefore doesn't clean up allocated memory.
	// So clean up the memory by force here.
	apr_pool_terminate();
	SYS_IMAGE_LIST().Cleanup();
}

// The one and only CTortoiseProcApp object
CTortoiseProcApp theApp;
HWND hWndExplorer;

CCrashReport crasher("crashreports@tortoisesvn.tigris.org", "Crash Report for TortoiseSVN : " STRPRODUCTVER);// crash

// CTortoiseProcApp initialization

void CTortoiseProcApp::CrashProgram()
{
	int * a;
	a = NULL;
	*a = 7;
}
BOOL CTortoiseProcApp::InitInstance()
{
	CheckUpgrade();
	//set the resource dll for the required language
	CRegDWORD loc = CRegDWORD(_T("Software\\TortoiseSVN\\LanguageID"), 1033);
	long langId = loc;
	CString langDll;
	CStringA langpath = CStringA(CUtils::GetAppParentDirectory());
	langpath += "Languages";
	bindtextdomain("subversion", (LPCSTR)langpath);
	bind_textdomain_codeset("subversion", "UTF-8");
	HINSTANCE hInst = NULL;
	do
	{
		langDll.Format(_T("..\\Languages\\TortoiseProc%d.dll"), langId);
		
		hInst = LoadLibrary(langDll);

		CString sVer = _T(STRPRODUCTVER);
		sVer = sVer.Left(sVer.ReverseFind(','));
		CString sFileVer = CUtils::GetVersionFromFile(langDll);
		int commaIndex = sFileVer.ReverseFind(',');
		if (commaIndex==-1 || sFileVer.Left(commaIndex).Compare(sVer)!=0)
		{
			FreeLibrary(hInst);
			hInst = NULL;
		}
		if (hInst != NULL)
		{
			AfxSetResourceHandle(hInst);
		}
		else
		{
			DWORD lid = SUBLANGID(langId);
			lid--;
			if (lid > 0)
			{
				langId = MAKELANGID(PRIMARYLANGID(langId), lid);
			}
			else
				langId = 0;
		}
	} while ((hInst == NULL) && (langId != 0));
	TCHAR buf[6];
	langId = loc;
	CString sHelppath;
	sHelppath = this->m_pszHelpFilePath;
	sHelppath = sHelppath.MakeLower();
	sHelppath.Replace(_T("tortoiseproc.chm"), _T("TortoiseSVN_en.chm"));
	free((void*)m_pszHelpFilePath);
	m_pszHelpFilePath=_tcsdup(sHelppath);
	do
	{
		GetLocaleInfo(MAKELCID(langId, SORT_DEFAULT), LOCALE_SISO639LANGNAME, buf, sizeof(buf));
		CString sLang = _T("_");
		sLang += buf;
		sHelppath.Replace(_T("_en"), sLang);
		if (PathFileExists(sHelppath))
		{
			free((void*)m_pszHelpFilePath);
			m_pszHelpFilePath=_tcsdup(sHelppath);
		} // if (PathFileExists(sHelppath))

		DWORD lid = SUBLANGID(langId);
		lid--;
		if (lid > 0)
		{
			langId = MAKELANGID(PRIMARYLANGID(langId), lid);
		}
		else
			langId = 0;
	} while (langId);
	
	// InitCommonControls() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	
    INITCOMMONCONTROLSEX used = {
        sizeof(INITCOMMONCONTROLSEX),
			ICC_ANIMATE_CLASS | ICC_BAR_CLASSES | ICC_COOL_CLASSES | ICC_DATE_CLASSES |
			ICC_HOTKEY_CLASS | ICC_INTERNET_CLASSES | ICC_LISTVIEW_CLASSES |
			ICC_NATIVEFNTCTL_CLASS | ICC_PAGESCROLLER_CLASS | ICC_PROGRESS_CLASS |
			ICC_TAB_CLASSES | ICC_TREEVIEW_CLASSES | ICC_UPDOWN_CLASS |
			ICC_USEREX_CLASSES | ICC_WIN95_CLASSES
    };
    InitCommonControlsEx(&used);
	AfxOleInit();
	AfxInitRichEdit2();
	CWinApp::InitInstance();
	SetRegistryKey(_T("TortoiseSVN"));

	CCmdLineParser parser(AfxGetApp()->m_lpCmdLine);

	if (CRegDWORD(_T("Software\\TortoiseSVN\\Debug"), FALSE)==TRUE)
		AfxMessageBox(AfxGetApp()->m_lpCmdLine, MB_OK | MB_ICONINFORMATION);

	if (!parser.HasKey(_T("command")))
	{
		CAboutDlg dlg;
		m_pMainWnd = &dlg;
		dlg.DoModal();
		return FALSE;
	}

	CString comVal = parser.GetVal(_T("command"));
	if (comVal.IsEmpty())
	{
		CMessageBox::Show(NULL, IDS_ERR_NOCOMMANDVALUE, IDS_APPNAME, MB_ICONERROR);
		return FALSE;
	}
	else
	{

		// Look up the command
		TSVNCommand command = cmdAbout;		// Something harmless as a default
		for(int nCommand = 0; nCommand < (sizeof(commandInfo)/sizeof(commandInfo[0])); nCommand++)
		{
			if(comVal.Compare(commandInfo[nCommand].pCommandName) == 0)
			{
				// We've found the command
				command = commandInfo[nCommand].command;
				// If this fires, you've let the enum get out of sync with the commandInfo array
				ASSERT((int)command == nCommand);
				break;
			}
		}

		bool bPathIsTempfile = false;
		if(commandInfo[command].bPathIsTempFileByDefault)
		{
			// The path argument is probably a temporary file containing path names, unless
			// the notempfile argument is present
			bPathIsTempfile = !parser.HasKey(_T("notempfile"));
		}

		CString sPathArgument = CUtils::GetLongPathname(parser.GetVal(_T("path")));
		CTSVNPath cmdLinePath(sPathArgument);

		CTSVNPathList pathList;
		if(bPathIsTempfile)
		{
			if (pathList.LoadFromTemporaryFile(cmdLinePath)==false)
				return FALSE;		// no path specified!
			if(cmdLinePath.GetFileExtension().CompareNoCase(_T(".tmp")) == 0)
			{
				// We can delete the temporary path file, now that we've loaded it
				::DeleteFile(cmdLinePath.GetWinPath());
			}
			// This was a path to a temporary file - it's got no meaning now, and
			// anybody who uses it again is in for a problem...
			cmdLinePath.Reset();
		}
		else
		{
			pathList.LoadFromAsteriskSeparatedString(sPathArgument);
		}
		
		if (command == cmdTest)
		{
			CMessageBox::Show(NULL, _T("Test command successfully executed"), _T("Info"), MB_OK);
			return FALSE;
		} // if (comVal.Compare(_T("test"))==0)

		CString sVal = parser.GetVal(_T("hwnd"));
		hWndExplorer = (HWND)_ttoi64(sVal);

		while (GetParent(hWndExplorer)!=NULL)
			hWndExplorer = GetParent(hWndExplorer);
		if (!IsWindow(hWndExplorer))
		{
			hWndExplorer = NULL;
		}
// check for newer versions
		if (CRegDWORD(_T("Software\\TortoiseSVN\\CheckNewer"), TRUE) != FALSE)
		{
			time_t now;
			struct tm *ptm;

			time(&now);
			ptm = localtime(&now);
			int week = ptm->tm_yday / 7;

			CRegDWORD oldweek = CRegDWORD(_T("Software\\TortoiseSVN\\CheckNewerWeek"), (DWORD)-1);
			if (((DWORD)oldweek) == -1)
				oldweek = week;
			else
			{
				if ((DWORD)week != oldweek)
				{
					oldweek = week;

					TCHAR com[MAX_PATH+100];
					GetModuleFileName(NULL, com, MAX_PATH);
					_tcscat(com, _T(" /command:updatecheck"));

//BUGBUG - Should this really have an error message string resource ID?
					CUtils::LaunchApplication(com, 0, false);
				}
			}
		}

		//#region crash
		if (command == cmdCrash)
		{
			CMessageBox::Show(NULL, _T("You are testing the crashhandler.\n<ct=0x0000FF>Do NOT send the crashreport!!!!</ct>"), _T("TortoiseSVN"), MB_ICONINFORMATION);
			CrashProgram();
			CMessageBox::Show(NULL, IDS_ERR_NOCOMMAND, IDS_APPNAME, MB_ICONERROR);
			return FALSE;
		}
		//#endregion
		HANDLE TSVNMutex = ::CreateMutex(NULL, FALSE, _T("TortoiseProc.exe"));	
		{
			CString err = SVN::CheckConfigFile();
			if (!err.IsEmpty())
			{
				CMessageBox::Show(EXPLORERHWND, err, _T("TortoiseSVN"), MB_ICONERROR);
				// Normally, we give-up and exit at this point, but there is a trap here
				// in that the user might need to use the settings dialog to edit the config file.
				if(command != cmdSettings)
				{
					return FALSE;
				}
			}
		}

		//#region about
		if (command == cmdAbout)
		{
			CAboutDlg dlg;
			m_pMainWnd = &dlg;
			dlg.DoModal();
		}
		//#endregion
		//#region rtfm
		if (command == cmdRTFM)
		{
			CMessageBox::Show(EXPLORERHWND, IDS_PROC_RTFM, IDS_APPNAME, MB_ICONINFORMATION);
			TCHAR path[MAX_PATH];
			SHGetFolderPath(EXPLORERHWND, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, path);
			ShellExecute(0,_T("explore"),path,NULL,NULL,SW_SHOWNORMAL);
		}
		//#endregion
		//#region log
		if (command == cmdLog)
		{
			//the log command line looks like this:
			//command:log path:<path_to_file_or_directory_to_show_the_log_messages> [revstart:<startrevision>] [revend:<endrevision>]
			CString val = parser.GetVal(_T("revstart"));
			long revstart = _tstol(val);
			val = parser.GetVal(_T("revend"));
			long revend = _tstol(val);
			val = parser.GetVal(_T("limit"));
			int limit = _tstoi(val);
			if (revstart == 0)
			{
				revstart = SVNRev::REV_HEAD;
			}
			if (revend == 0)
			{
				revend = 1;
			}
			if (limit == 0)
			{
				CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100);
				limit = (int)(LONG)reg;
			}
			CLogDlg dlg;
			m_pMainWnd = &dlg;
			dlg.SetParams(cmdLinePath, revstart, revend, limit, parser.HasKey(_T("strict")));
			val = parser.GetVal(_T("propspath"));
			if (!val.IsEmpty())
				dlg.SetProjectPropertiesPath(CTSVNPath(val));
			dlg.DoModal();			
		}
		//#endregion
		//#region checkout
		if (command == cmdCheckout)
		{
			//
			// Get the directory supplied in the command line. If there isn't
			// one then we should use the current working directory instead.
			CTSVNPath checkoutDirectory;
			if (cmdLinePath.IsEmpty())
			{
				DWORD len = ::GetCurrentDirectory(0, NULL);
				TCHAR * tszPath = new TCHAR[len];
				::GetCurrentDirectory(len, tszPath);
				checkoutDirectory.SetFromWin(tszPath, true);
				delete tszPath;
			}
			else
			{
				checkoutDirectory = cmdLinePath;
			}

			//
			// Create a checkout dialog and display it. If the user clicks OK,
			// we should create an SVN progress dialog, set it as the main
			// window and then display it.
			//
			CCheckoutDlg dlg;
			dlg.m_strCheckoutDirectory = cmdLinePath.GetDirectory().GetWinPathString();
			dlg.m_URL = parser.GetVal(_T("url"));
			if (dlg.m_URL.Left(5).Compare(_T("tsvn:"))==0)
			{
				dlg.m_URL = dlg.m_URL.Mid(5);
			}
			if (dlg.DoModal() == IDOK)
			{
				TRACE(_T("url = %s\n"), (LPCTSTR)dlg.m_URL);
				TRACE(_T("checkout dir = %s \n"), (LPCTSTR)dlg.m_strCheckoutDirectory);

				checkoutDirectory.SetFromWin(dlg.m_strCheckoutDirectory, true);

				CSVNProgressDlg progDlg;
				progDlg.m_dwCloseOnEnd = parser.GetLongVal(_T("closeonend"));
				m_pMainWnd = &progDlg;
				int opts = dlg.m_bNonRecursive ? ProgOptNonRecursive : ProgOptRecursive;
				if (dlg.m_bNoExternals)
					opts |= ProgOptIgnoreExternals;
				progDlg.SetParams(CSVNProgressDlg::Checkout, opts, CTSVNPathList(checkoutDirectory), dlg.m_URL, _T(""), dlg.Revision);
				progDlg.DoModal();
			}
		}
		//#endregion
		//#region import
		if (command == cmdImport)
		{
			CImportDlg dlg;
			dlg.m_path = cmdLinePath;
			if (dlg.DoModal() == IDOK)
			{
				TRACE(_T("url = %s\n"), (LPCTSTR)dlg.m_url);
				CSVNProgressDlg progDlg;
				progDlg.m_dwCloseOnEnd = parser.GetLongVal(_T("closeonend"));
				m_pMainWnd = &progDlg;
				progDlg.SetParams(CSVNProgressDlg::Import, 0, pathList, dlg.m_url, dlg.m_sMessage);
				progDlg.DoModal();
			}
		}
		//#endregion
		//#region update
		if (command == cmdUpdate)
		{
			SVNRev rev = SVNRev(_T("HEAD"));
			int options = ProgOptRecursive;
			if (parser.HasKey(_T("rev")))
			{
				CUpdateDlg dlg;
				if (pathList.GetCount()>0)
					dlg.m_wcPath = pathList[0];
				if (dlg.DoModal() == IDOK)
				{
					rev = dlg.Revision;
					if (dlg.m_bNonRecursive)
					{
						options = ProgOptNonRecursive;
					}
					if (dlg.m_bNoExternals)
					{
						options |= ProgOptIgnoreExternals;
					}
					if (CRegDWORD(_T("Software\\TortoiseSVN\\updatetorev"), (DWORD)-1)==(DWORD)-1)
						if (SVNStatus::GetAllStatusRecursive(cmdLinePath)>svn_wc_status_normal)
							if (CMessageBox::ShowCheck(EXPLORERHWND, IDS_WARN_UPDATETOREV_WITHMODS, IDS_APPNAME, MB_OKCANCEL | MB_ICONWARNING, _T("updatetorev"), IDS_MSGBOX_DONOTSHOWAGAIN)!=IDOK)
								return FALSE;
				}
				else 
					return FALSE;
			}
			CSVNProgressDlg progDlg;
			progDlg.m_dwCloseOnEnd = parser.GetLongVal(_T("closeonend"));
			m_pMainWnd = &progDlg;
			progDlg.SetParams(CSVNProgressDlg::Update, options, pathList, _T(""), _T(""), rev);
			progDlg.DoModal();
		}
		//#endregion
		//#region commit
		if (command == cmdCommit)
		{
			CLogPromptDlg dlg;
			if (parser.HasKey(_T("logmsg")))
			{
				dlg.m_sLogMessage = parser.GetVal(_T("logmsg"));
			}
			dlg.m_pathList = pathList;
			if (dlg.DoModal() == IDOK)
			{
				CSVNProgressDlg progDlg;
				progDlg.m_dwCloseOnEnd = parser.GetLongVal(_T("closeonend"));
				m_pMainWnd = &progDlg;
				progDlg.SetParams(CSVNProgressDlg::Commit, dlg.m_bKeepLocks ? ProgOptKeeplocks : 0, dlg.m_pathList, _T(""), dlg.m_sLogMessage, !dlg.m_bRecursive);
				progDlg.DoModal();
			}
		}
		//#endregion
		//#region add
		if (command == cmdAdd)
		{
			CAddDlg dlg;
			dlg.m_pathList = pathList;
			if (dlg.DoModal() == IDOK)
			{
				CSVNProgressDlg progDlg;
				progDlg.m_dwCloseOnEnd = parser.GetLongVal(_T("closeonend"));
				m_pMainWnd = &progDlg;
				progDlg.SetParams(CSVNProgressDlg::Add, 0, dlg.m_pathList);
				progDlg.DoModal();
			} // if (dlg.DoModal() == IDOK) // if (dlg.DoModal() == IDOK) 
		}
		//#endregion
		//#region revert
		if (command == cmdRevert)
		{
			CRevertDlg dlg;
			dlg.m_pathList = pathList;
			if (dlg.DoModal() == IDOK)
			{
				CSVNProgressDlg progDlg;
				progDlg.m_dwCloseOnEnd = parser.GetLongVal(_T("closeonend"));
				m_pMainWnd = &progDlg;
				int options = (dlg.m_bRecursive ? ProgOptRecursive : ProgOptNonRecursive);
				progDlg.SetParams(CSVNProgressDlg::Revert, options, dlg.m_pathList);
				progDlg.DoModal();
			}
		}
		//#endregion
		//#region cleanup
		if (command == cmdCleanup)
		{
			CProgressDlg progress;
			progress.SetTitle(IDS_PROC_CLEANUP);
			progress.ShowModeless(PWND);
			SVN svn;
			if (!svn.CleanUp(cmdLinePath))
			{
				progress.Stop();
				CString errorMessage;
				errorMessage.Format(IDS_ERR_CLEANUP, (LPCTSTR)svn.GetLastErrorMessage());
				CMessageBox::Show(EXPLORERHWND, errorMessage, _T("TortoiseSVN"), MB_ICONERROR);
			}
			else
			{
				// after the cleanup has finished, crawl the path downwards and send a change
				// notification for every directory to the shell. This will update the
				// overlays in the left treeview of the explorer.
				CDirFileEnum crawler(cmdLinePath.GetWinPathString());
				CString sAdminDir = _T("\\");
				sAdminDir += _T(SVN_WC_ADM_DIR_NAME);
				sAdminDir += _T("\\");
				CString sPath;
				bool bDir = false;
				while (crawler.NextFile(sPath, &bDir)) 
				{
					if ((bDir)&&(sPath.Find(sAdminDir)<0))
					{
						SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH | SHCNF_FLUSHNOWAIT, (LPCTSTR)sPath, NULL);
					}
				}
				SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH | SHCNF_FLUSHNOWAIT, cmdLinePath.GetWinPath(), NULL);
				
				progress.Stop();
				CMessageBox::Show(EXPLORERHWND, IDS_PROC_CLEANUPFINISHED, IDS_APPNAME, MB_OK | MB_ICONINFORMATION);
			}
		}
		//#endregion
		//#region resolve
		if (command == cmdResolve)
		{
			UINT ret = IDYES;
			if (!parser.HasKey(_T("noquestion")))
				ret = CMessageBox::Show(EXPLORERHWND, IDS_PROC_RESOLVE, IDS_APPNAME, MB_ICONQUESTION | MB_YESNO);
			if (ret==IDYES)
			{
				CSVNProgressDlg progDlg(PWND);
				progDlg.m_dwCloseOnEnd = parser.GetLongVal(_T("closeonend"));
				m_pMainWnd = &progDlg;
				progDlg.SetParams(CSVNProgressDlg::Resolve, 0, pathList);
				progDlg.DoModal();
			}
		}
		//#endregion
		//#region repocreate
		if (command == cmdRepoCreate)
		{
			CRepoCreateDlg dlg;
			if (dlg.DoModal() == IDOK)
			{
				if ((dlg.RepoType.Compare(_T("bdb"))==0)&&(GetDriveType(cmdLinePath.GetRootPathString())==DRIVE_REMOTE))
				{
					if (CMessageBox::Show(EXPLORERHWND, IDS_PROC_REPOCREATESHAREWARN, IDS_APPNAME, MB_ICONINFORMATION | MB_YESNO)==IDYES)
					{
						if (!SVN::CreateRepository(cmdLinePath.GetWinPathString(), dlg.RepoType))
						{
							CMessageBox::Show(EXPLORERHWND, IDS_PROC_REPOCREATEERR, IDS_APPNAME, MB_ICONERROR);
						}
						else
						{
							CMessageBox::Show(EXPLORERHWND, IDS_PROC_REPOCREATEFINISHED, IDS_APPNAME, MB_OK | MB_ICONINFORMATION);
						}
					} 
				}
				else
				{
					if (!SVN::CreateRepository(cmdLinePath.GetWinPathString(), dlg.RepoType))
					{
						CMessageBox::Show(EXPLORERHWND, IDS_PROC_REPOCREATEERR, IDS_APPNAME, MB_ICONERROR);
					}
					else
					{
						CMessageBox::Show(EXPLORERHWND, IDS_PROC_REPOCREATEFINISHED, IDS_APPNAME, MB_OK | MB_ICONINFORMATION);
					}
				}
			}
		} // if (comVal.Compare(_T("repocreate"))==0) 
		//#endregion
		//#region switch
		if (command == cmdSwitch)
		{
			CSwitchDlg dlg;
			dlg.m_path = cmdLinePath.GetWinPathString();

			if (dlg.DoModal() == IDOK)
			{
				CSVNProgressDlg progDlg;
				progDlg.m_dwCloseOnEnd = parser.GetLongVal(_T("closeonend"));
				m_pMainWnd = &progDlg;
				progDlg.SetParams(CSVNProgressDlg::Switch, 0, pathList, dlg.m_URL, _T(""), dlg.Revision);
				progDlg.DoModal();
			}
		}
		//#endregion
		//#region export
		if (command == cmdExport)
		{
			TCHAR saveto[MAX_PATH];
			if (SVNStatus::GetAllStatus(cmdLinePath) == svn_wc_status_unversioned)
			{
				CCheckoutDlg dlg;
				dlg.m_strCheckoutDirectory = cmdLinePath.GetWinPathString();
				dlg.IsExport = TRUE;
				if (dlg.DoModal() == IDOK)
				{
					CTSVNPath checkoutPath(dlg.m_strCheckoutDirectory);

					CSVNProgressDlg progDlg;
					progDlg.m_dwCloseOnEnd = parser.GetLongVal(_T("closeonend"));
					m_pMainWnd = &progDlg;
					progDlg.SetParams(CSVNProgressDlg::Export, dlg.m_bNoExternals ? ProgOptIgnoreExternals : 0, CTSVNPathList(checkoutPath), dlg.m_URL, _T(""), dlg.Revision);
					progDlg.DoModal();
				}
			}
			else
			{
				CBrowseFolder folderBrowser;
				CString strTemp;
				strTemp.LoadString(IDS_PROC_EXPORT_1);
				folderBrowser.SetInfo(strTemp);
				folderBrowser.m_style = BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS | BIF_VALIDATE;
				strTemp.LoadString(IDS_PROC_EXPORT_2);
				folderBrowser.SetCheckBoxText(strTemp);
				strTemp.LoadString(IDS_PROC_OMMITEXTERNALS);
				folderBrowser.SetCheckBoxText2(strTemp);
				CRegDWORD regExtended = CRegDWORD(_T("Software\\TortoiseSVN\\ExportExtended"), FALSE);
				CBrowseFolder::m_bCheck = regExtended;
				if (folderBrowser.Show(EXPLORERHWND, saveto)==CBrowseFolder::OK)
				{
					CString saveplace = CString(saveto);
					saveplace += _T("\\") + cmdLinePath.GetFileOrDirectoryName();
					TRACE(_T("export %s to %s\n"), (LPCTSTR)cmdLinePath.GetUIPathString(), (LPCTSTR)saveto);
					CProgressDlg progDlg;
					progDlg.SetTitle(IDS_PROC_EXPORT_3);
					progDlg.SetShowProgressBar(true);
					progDlg.SetAnimation(IDR_ANIMATION);
					progDlg.ShowModeless(CWnd::FromHandle(EXPLORERHWND));
					progDlg.FormatNonPathLine(1, IDS_PROC_EXPORT_3);
					SVN svn;
					if (!svn.Export(cmdLinePath, CTSVNPath(saveplace), SVNRev::REV_WC ,SVNRev::REV_WC, TRUE, folderBrowser.m_bCheck2, &progDlg, folderBrowser.m_bCheck))
					{
						progDlg.Stop();
						CMessageBox::Show(EXPLORERHWND, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_OK | MB_ICONERROR);
					}
					else
					{
						progDlg.Stop();
						CString strMessage;
						strMessage.Format(IDS_PROC_EXPORT_4, (LPCTSTR)cmdLinePath.GetUIPathString(), (LPCTSTR)saveplace);
						CMessageBox::Show(EXPLORERHWND, strMessage, _T("TortoiseSVN"), MB_OK | MB_ICONINFORMATION);
					}
					regExtended = CBrowseFolder::m_bCheck;
				}
			}
		}
		//#endregion
		//#region merge
		if (command == cmdMerge)
		{
			BOOL repeat = FALSE;
			CMergeDlg dlg;
			dlg.m_wcPath = cmdLinePath;
			do 
			{	
				if (dlg.DoModal() == IDOK)
				{
					CSVNProgressDlg progDlg;
					progDlg.m_dwCloseOnEnd = parser.GetLongVal(_T("closeonend"));
					//m_pMainWnd = &progDlg;
					progDlg.SetParams(CSVNProgressDlg::Merge, dlg.m_bDryRun ? ProgOptDryRun : 0, pathList, dlg.m_URLFrom, dlg.m_URLTo, dlg.StartRev);		//use the message as the second url
					progDlg.m_RevisionEnd = dlg.EndRev;
					progDlg.DoModal();
					repeat = dlg.m_bDryRun;
					dlg.bRepeating = TRUE;
				}
				else
					repeat = FALSE;
			} while(repeat);
		}
		//#endregion
		//#region copy
		if (command == cmdCopy)
		{
			CCopyDlg dlg;
			dlg.m_path = cmdLinePath;
			if (dlg.DoModal() == IDOK)
			{
				m_pMainWnd = NULL;
				TRACE(_T("copy %s to %s\n"), (LPCTSTR)cmdLinePath.GetWinPathString(), (LPCTSTR)dlg.m_URL);
				CSVNProgressDlg progDlg;
				progDlg.m_dwCloseOnEnd = parser.GetLongVal(_T("closeonend"));
				progDlg.SetParams(CSVNProgressDlg::Copy, 0, pathList, dlg.m_URL, dlg.m_sLogMessage, dlg.m_CopyRev);
				progDlg.DoModal();
			}
		}
		//#endregion
		//#region settings
		if (command == cmdSettings)
		{
			CSettings dlg(IDS_PROC_SETTINGS_TITLE);
			dlg.SetTreeViewMode(TRUE, TRUE, TRUE);
			dlg.SetTreeWidth(180);

			if (dlg.DoModal()==IDOK)
			{
				dlg.SaveData();
			}
		}
		//#endregion
		//#region remove
		if (command == cmdRemove)
		{
			BOOL bForce = FALSE;
			SVN svn;
			for(int nPath = 0; nPath < pathList.GetCount(); nPath++)
			{
				TRACE(_T("remove file %s\n"), (LPCTSTR)pathList[nPath].GetUIPathString());
				// even though SVN::Remove takes a list of paths to delete at once
				// we delete each item individually so we can prompt the user
				// if something goes wrong or unversioned/modified items are
				// to be deleted
				CTSVNPathList removePathList(pathList[nPath]);
				if (!svn.Remove(removePathList, bForce))
				{
					if ((svn.Err->apr_err == SVN_ERR_UNVERSIONED_RESOURCE) ||
						(svn.Err->apr_err == SVN_ERR_CLIENT_MODIFIED))
					{
						CString msg, yes, no, yestoall;
						msg.Format(IDS_PROC_REMOVEFORCE, svn.GetLastErrorMessage());
						yes.LoadString(IDS_MSGBOX_YES);
						no.LoadString(IDS_MSGBOX_NO);
						yestoall.LoadString(IDS_PROC_YESTOALL);
						UINT ret = CMessageBox::Show(EXPLORERHWND, msg, _T("TortoiseSVN"), 2, IDI_ERROR, yes, no, yestoall);
						if (ret == 3)
							bForce = TRUE;
						if ((ret == 1)||(ret==3))
							if (!svn.Remove(removePathList, TRUE))
							{
								CMessageBox::Show(EXPLORERHWND, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							}
					}
					else
						CMessageBox::Show(EXPLORERHWND, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
				}
			}
		}
		//#endregion
		//#region rename
		if (command == cmdRename)
		{
			CRenameDlg dlg;
			CString filename = cmdLinePath.GetFileOrDirectoryName();
			CString basePath = cmdLinePath.GetContainingDirectory().GetWinPathString();
			::SetCurrentDirectory(basePath);
			dlg.m_name = filename;
			if (dlg.DoModal() == IDOK)
			{
				TRACE(_T("rename file %s to %s\n"), (LPCTSTR)cmdLinePath.GetWinPathString(), (LPCTSTR)dlg.m_name);
				CTSVNPath destinationPath(basePath);
				if (PathIsRelative(dlg.m_name) && !PathIsURL(dlg.m_name))
					destinationPath.AppendPathString(dlg.m_name);
				else
					destinationPath.SetFromWin(dlg.m_name);
				if (cmdLinePath.IsEquivalentTo(destinationPath))
				{
					//rename to the same file!
					CMessageBox::Show(EXPLORERHWND, IDS_PROC_CASERENAME, IDS_APPNAME, MB_ICONERROR);
				}
				else
				{
					if ((cmdLinePath.IsDirectory())||(pathList.GetCount() > 1))
					{
						CSVNProgressDlg progDlg;
						progDlg.m_dwCloseOnEnd = parser.GetLongVal(_T("closeonend"));
						progDlg.SetParams(CSVNProgressDlg::Rename, 0, pathList, destinationPath.GetWinPathString(), CString(), SVNRev::REV_WC);
						progDlg.DoModal();
					}
					else
					{
						SVN svn;
						if (!svn.Move(cmdLinePath, destinationPath, TRUE))
						{
							TRACE(_T("%s\n"), (LPCTSTR)svn.GetLastErrorMessage());
							CMessageBox::Show(EXPLORERHWND, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						}
					}
				}
			}
		}
		//#endregion
		//#region diff
		if (command == cmdDiff)
		{
			CString path2 = CUtils::GetLongPathname(parser.GetVal(_T("path2")));
			if (path2.IsEmpty())
			{
				if (cmdLinePath.IsDirectory())
				{
					CChangedDlg dlg;
					dlg.m_pathList = CTSVNPathList(cmdLinePath);
					dlg.DoModal();
				}
				else
				{
					CTSVNPath temporaryFile;
					SVN::DiffFileAgainstBase(cmdLinePath, temporaryFile);
				}
			} 
			else
				CUtils::StartExtDiff(CTSVNPath(path2), cmdLinePath);
		}
		//#endregion
		//#region dropcopyadd
		if (command == cmdDropCopyAdd)
		{
			CString droppath = parser.GetVal(_T("droptarget"));

			CTSVNPathList copiedFiles;
			for(int nPath = 0; nPath < pathList.GetCount(); nPath++)
			{
				if (!pathList[nPath].IsEquivalentTo(CTSVNPath(droppath)))
				{
					//copy the file to the new location
					CString name = pathList[nPath].GetFileOrDirectoryName();
					if (::PathFileExists(droppath+_T("\\")+name))
					{
						CString strMessage;
						strMessage.Format(IDS_PROC_OVERWRITE_CONFIRM, (LPCTSTR)(droppath+_T("\\")+name));
						int ret = CMessageBox::Show(EXPLORERHWND, strMessage, _T("TortoiseSVN"), MB_YESNOCANCEL | MB_ICONQUESTION);
						if (ret == IDYES)
						{
							if (!::CopyFile(pathList[nPath].GetWinPath(), droppath+_T("\\")+name, TRUE))
							{
								//the copy operation failed! Get out of here!
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
								CString strMessage;
								strMessage.Format(IDS_ERR_COPYFILES, lpMsgBuf);
								CMessageBox::Show(EXPLORERHWND, strMessage, _T("TortoiseSVN"), MB_OK | MB_ICONINFORMATION);
								LocalFree( lpMsgBuf );
								return FALSE;
							}
						}
						if (ret == IDCANCEL)
						{
							return FALSE;		//cancel the whole operation
						}
					}
					else if (!CopyFile(pathList[nPath].GetWinPath(), droppath+_T("\\")+name, FALSE))
					{
						//the copy operation failed! Get out of here!
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
						CString strMessage;
						strMessage.Format(IDS_ERR_COPYFILES, lpMsgBuf);
						CMessageBox::Show(EXPLORERHWND, strMessage, _T("TortoiseSVN"), MB_OK | MB_ICONINFORMATION);
						LocalFree( lpMsgBuf );
						return FALSE;
					}
					copiedFiles.AddPath(CTSVNPath(droppath+_T("\\")+name));		//add the new filepath
				}
			}
			//now add all the newly copied files to the working copy
			CSVNProgressDlg progDlg;
			progDlg.m_dwCloseOnEnd = parser.GetLongVal(_T("closeonend"));
			m_pMainWnd = &progDlg;
			progDlg.SetParams(CSVNProgressDlg::Add, 0, copiedFiles);
			progDlg.DoModal();
		}
		//#endregion
		//#region dropmove
		if (command == cmdDropMove)
		{
			CString droppath = parser.GetVal(_T("droptarget"));
			SVN svn;
			unsigned long count = 0;
			CProgressDlg progress;
			if (progress.IsValid())
			{
				progress.SetTitle(IDS_PROC_MOVING);
				progress.SetAnimation(IDR_MOVEANI);
				progress.ShowModeless(CWnd::FromHandle(EXPLORERHWND));
			}
			for(int nPath = 0; nPath < pathList.GetCount(); nPath++)
			{
				CTSVNPath destPath(droppath+_T("\\")+pathList[nPath].GetFileOrDirectoryName());
				if (pathList[nPath].IsEquivalentTo(destPath))
				{
					CString name = pathList[nPath].GetFileOrDirectoryName();
					progress.Stop();
					CRenameDlg dlg;
					dlg.m_windowtitle.Format(IDS_PROC_RENAME, (LPCTSTR)name);
					if (dlg.DoModal() != IDOK)
					{
						return FALSE;
					}
					destPath.SetFromWin(droppath+_T("\\")+name);
				} // if (strLine.CompareNoCase(droppath+_T("\\")+name)==0) 
				if (!svn.Move(pathList[nPath], destPath, FALSE))
				{
					if (pathList[nPath].IsDirectory()||(SVNStatus::GetAllStatus(pathList[nPath]) > svn_wc_status_normal))
					{
						// file/folder seems to have local modifications. Ask the user if
						// a force is requested.
						CString temp;
						temp.Format(IDS_PROC_FORCEMOVE, pathList[nPath].GetWinPathString());
						if (CMessageBox::Show(EXPLORERHWND, temp, _T("TortoiseSVN"), MB_YESNO)==IDYES)
						{
							if (!svn.Move(pathList[nPath], destPath, TRUE))
							{
								CMessageBox::Show(EXPLORERHWND, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								return FALSE;		//get out of here
							}
						}
					}
					else
					{
						TRACE(_T("%s\n"), (LPCTSTR)svn.GetLastErrorMessage());
						CMessageBox::Show(EXPLORERHWND, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						return FALSE;		//get out of here
					}
				} 
				count++;
				if (progress.IsValid())
				{
					progress.FormatPathLine(1, IDS_PROC_MOVINGPROG, pathList[nPath].GetWinPath());
					progress.FormatPathLine(2, IDS_PROC_CPYMVPROG2, destPath.GetWinPath());
					progress.SetProgress(count, pathList.GetCount());
				}
				if ((progress.IsValid())&&(progress.HasUserCancelled()))
				{
					CMessageBox::Show(EXPLORERHWND, IDS_SVN_USERCANCELLED, IDS_APPNAME, MB_ICONINFORMATION);
					return FALSE;
				}
			}
		}
		//#endregion
		//#region dropexport
		if (command == cmdDropExport)
		{
			CString droppath = parser.GetVal(_T("droptarget"));
			SVN svn;
			CProgressDlg progDlg;
			for(int nPath = 0; nPath < pathList.GetCount(); nPath++)
			{
				progDlg.SetTitle(IDS_PROC_EXPORT_3);
				progDlg.SetShowProgressBar(true);
				progDlg.SetAnimation(IDR_ANIMATION);
				progDlg.ShowModeless(CWnd::FromHandle(EXPLORERHWND));
				progDlg.FormatNonPathLine(1, IDS_PROC_EXPORT_3);
				CString dropper = droppath + _T("\\") + pathList[nPath].GetFileOrDirectoryName();
				if (!svn.Export(pathList[nPath], CTSVNPath(dropper), SVNRev::REV_WC ,SVNRev::REV_WC, TRUE, FALSE, &progDlg, parser.HasKey(_T("extended"))))
				{
					progDlg.Stop();
					CMessageBox::Show(EXPLORERHWND, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_OK | MB_ICONERROR);
				}
			}
			progDlg.Stop();
		}
		//#endregion
		//#region dropcopy
		if (command == cmdDropCopy)
		{
			CString sDroppath = parser.GetVal(_T("droptarget"));
			SVN svn;
			unsigned long count = 0;
			CProgressDlg progress;
			if (progress.IsValid())
			{
				progress.SetTitle(IDS_PROC_COPYING);
				progress.SetAnimation(IDR_MOVEANI);
				progress.ShowModeless(CWnd::FromHandle(EXPLORERHWND));
			}
			for(int nPath = 0; nPath < pathList.GetCount(); nPath++)
			{
				const CTSVNPath& sourcePath = pathList[nPath];

				CTSVNPath fullDropPath(sDroppath);
				fullDropPath.AppendPathString(sourcePath.GetFileOrDirectoryName());
				// Check for a drop-on-to-ourselves
				if (sourcePath.IsEquivalentTo(fullDropPath))
				{
					// Offer a rename
					progress.Stop();
					CRenameDlg dlg;
					dlg.m_windowtitle.Format(IDS_PROC_RENAME, (LPCTSTR)sourcePath.GetFileOrDirectoryName());
					if (dlg.DoModal() != IDOK)
					{
						return FALSE;
					}
					// Rebuild the destination path, with the new name
					fullDropPath.SetFromUnknown(sDroppath);
					fullDropPath.AppendPathString(dlg.m_name);
				} 
				if (!svn.Copy(sourcePath, fullDropPath, SVNRev::REV_WC, _T("")))
				{
					TRACE(_T("%s\n"), (LPCTSTR)svn.GetLastErrorMessage());
					CMessageBox::Show(EXPLORERHWND, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					return FALSE;		//get out of here
				} // if (!svn.Move(strLine, droppath+_T("\\")+name, FALSE)) 
				count++;
				if (progress.IsValid())
				{
					progress.FormatPathLine(1, IDS_PROC_COPYINGPROG, sourcePath.GetWinPath());
					progress.FormatPathLine(2, IDS_PROC_CPYMVPROG2, fullDropPath.GetWinPath());
					progress.SetProgress(count, pathList.GetCount());
				} // if (progress.IsValid())
				if ((progress.IsValid())&&(progress.HasUserCancelled()))
				{
					CMessageBox::Show(EXPLORERHWND, IDS_SVN_USERCANCELLED, IDS_APPNAME, MB_ICONINFORMATION);
					return FALSE;
				}
			}
		}
		//#endregion
		//#region conflicteditor
		if (command == cmdConflictEditor)
		{
			SVN::StartConflictEditor(cmdLinePath);
		} 
		//#endregion
		//#region relocate
		if (command == cmdRelocate)
		{
			SVN svn;
			CRelocateDlg dlg;
			dlg.m_sFromUrl = svn.GetURLFromPath(cmdLinePath);
			dlg.m_sToUrl = dlg.m_sFromUrl;

			if (dlg.DoModal() == IDOK)
			{
				TRACE(_T("relocate from %s to %s\n"), (LPCTSTR)dlg.m_sFromUrl, (LPCTSTR)dlg.m_sToUrl);
				if (CMessageBox::Show((EXPLORERHWND), IDS_WARN_RELOCATEREALLY, IDS_APPNAME, MB_YESNO | MB_ICONWARNING)==IDYES)
				{
					SVN s;

					CProgressDlg progress;
					if (progress.IsValid())
					{
						progress.SetTitle(IDS_PROC_RELOCATING);
						progress.ShowModeless(PWND);
					}
					if (!s.Relocate(cmdLinePath, CTSVNPath(dlg.m_sFromUrl), CTSVNPath(dlg.m_sToUrl), TRUE))
					{
						progress.Stop();
						TRACE(_T("%s\n"), (LPCTSTR)s.GetLastErrorMessage());
						CMessageBox::Show((EXPLORERHWND), s.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					}
					else
					{
						progress.Stop();
						CString strMessage;
						strMessage.Format(IDS_PROC_RELOCATEFINISHED, (LPCTSTR)dlg.m_sToUrl);
						CMessageBox::Show((EXPLORERHWND), strMessage, _T("TortoiseSVN"), MB_ICONINFORMATION);
					}
				}
			}
		} // if (comVal.Compare(_T("relocate"))==0)
		//#endregion
		//#region help
		if (command == cmdHelp)
		{
			ShellExecute(EXPLORERHWND, _T("open"), m_pszHelpFilePath, NULL, NULL, SW_SHOWNORMAL);
		}
		//#endregion
		//#region repostatus
		if (command == cmdRepoStatus)
		{
			CChangedDlg dlg;
			dlg.m_pathList = pathList;
			dlg.DoModal();
		} // if (comVal.Compare(_T("repostatus"))==0)
		//#endregion 
		//#region repobrowser
		if (command == cmdRepoBrowser)
		{
			CString url;
			BOOL bFile = FALSE;
			SVN svn;
			if (!cmdLinePath.IsEmpty())
			{
				if (svn.IsRepository(cmdLinePath.GetWinPathString()))
				{
					// The path points to a local repository.
					// Add 'file:///' so the repository browser recognizes
					// it as an URL to the local repository.
					url = _T("file:///")+cmdLinePath.GetWinPathString();
				}
				else
					url = svn.GetURLFromPath(cmdLinePath);
			}
			if (cmdLinePath.GetUIPathString().Left(8).CompareNoCase(_T("file:///"))==0)
			{
				cmdLinePath.SetFromUnknown(cmdLinePath.GetUIPathString().Mid(8));
			}
			bFile = PathFileExists(cmdLinePath.GetWinPath()) ? !cmdLinePath.IsDirectory() : FALSE;

			if (url.IsEmpty())
			{
				CURLDlg urldlg;
				if (urldlg.DoModal() != IDOK)
				{
					if (TSVNMutex)
						::CloseHandle(TSVNMutex);
					return FALSE;
				}
				url = urldlg.m_url;
			}

			CString val = parser.GetVal(_T("rev"));
			SVNRev rev(val);
			CRepositoryBrowser dlg(SVNUrl(url, rev), bFile);
			dlg.m_ProjectProperties.ReadProps(cmdLinePath);
			dlg.m_path = cmdLinePath;
			dlg.DoModal();
		}
		//#endregion 
		//#region ignore
		if (command == cmdIgnore)
		{

			CString filelist;
			BOOL err = FALSE;
			for(int nPath = 0; nPath < pathList.GetCount(); nPath++)
			{
				//strLine = _T("F:\\Development\\DirSync\\DirSync.cpp");
				CString name = pathList[nPath].GetFileOrDirectoryName();
				filelist += name + _T("\n");
				if (parser.HasKey(_T("onlymask")))
				{
					name = _T("*")+pathList[nPath].GetFileExtension();
				}
				CTSVNPath parentfolder = pathList[nPath].GetContainingDirectory();
				SVNProperties props(parentfolder);
				CStringA value;
				for (int i=0; i<props.GetCount(); i++)
				{
					CString propname(props.GetItemName(i).c_str());
					if (propname.CompareNoCase(_T("svn:ignore"))==0)
					{
						stdstring stemp;
						stdstring tmp = props.GetItemValue(i);
						//treat values as normal text even if they're not
						value = (char *)tmp.c_str();
					}
				}
				if (value.IsEmpty())
					value = name;
				else
				{
					value = value.Trim("\n\r");
					value += "\n";
					value += name;
					value.Remove('\r');
				}
				if (!props.Add(_T("svn:ignore"), value))
				{
					CString temp;
					temp.Format(IDS_ERR_FAILEDIGNOREPROPERTY, name);
					CMessageBox::Show(EXPLORERHWND, temp, _T("TortoiseSVN"), MB_ICONERROR);
					err = TRUE;
					break;
				}
			}
			if (err == FALSE)
			{
				CString temp;
				temp.Format(IDS_PROC_IGNORESUCCESS, filelist);
				CMessageBox::Show(EXPLORERHWND, temp, _T("TortoiseSVN"), MB_ICONINFORMATION);
			}
		}
		//#endregion
		//#region unignore
		if (command == cmdUnIgnore)
		{
			CString filelist;
			BOOL err = FALSE;
			for(int nPath = 0; nPath < pathList.GetCount(); nPath++)
			{
				CString name = pathList[nPath].GetFileOrDirectoryName();
				filelist = name;
				if (parser.HasKey(_T("onlymask")))
				{
					name = _T("*")+pathList[nPath].GetFileExtension();
				}
				CTSVNPath parentfolder = pathList[nPath].GetContainingDirectory();
				SVNProperties props(parentfolder);
				CStringA value;
				for (int i=0; i<props.GetCount(); i++)
				{
					CString propname(props.GetItemName(i).c_str());
					if (propname.CompareNoCase(_T("svn:ignore"))==0)
					{
						stdstring stemp;
						stdstring tmp = props.GetItemValue(i);
						//treat values as normal text even if they're not
						value = (char *)tmp.c_str();
					}
				}
				value.Replace(CUnicodeUtils::GetUTF8(name), "");
				value = value.Trim("\n\r");
				value += "\n";
				value.Remove('\r');
				value.Replace("\n\n", "\n");
				if (!props.Add(_T("svn:ignore"), value))
				{
					CString temp;
					temp.Format(IDS_ERR_FAILEDUNIGNOREPROPERTY, name);
					CMessageBox::Show(EXPLORERHWND, temp, _T("TortoiseSVN"), MB_ICONERROR);
					err = TRUE;
					break;
				}
			}
			if (err == FALSE)
			{
				CString temp;
				temp.Format(IDS_PROC_UNIGNORESUCCESS, filelist);
				CMessageBox::Show(EXPLORERHWND, temp, _T("TortoiseSVN"), MB_ICONINFORMATION);
			}
		}
		//#endregion
		//#region blame
		if (command == cmdBlame)
		{
			CBlameDlg dlg;
			if (dlg.DoModal() == IDOK)
			{
				CBlame blame;
				CString tempfile;
				CString logfile;
				tempfile = blame.BlameToTempFile(cmdLinePath, dlg.StartRev, dlg.EndRev, logfile, TRUE);
				if (!tempfile.IsEmpty())
				{
					if (logfile.IsEmpty())
					{
						//open the default text editor for the result file
						CUtils::StartTextViewer(tempfile);
					}
					else
					{
						CUtils::LaunchTortoiseBlame(tempfile, logfile, cmdLinePath.GetFileOrDirectoryName());
					}
				} // if (!tempfile.IsEmpty()) 
				else
				{
					CMessageBox::Show(EXPLORERHWND, blame.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
				}
			} // if (dlg.DoModal() == IDOK) 
		} // if (comVal.Compare(_T("blame"))==0) 
		//#endregion 
		//#region cat
		if (command == cmdCat)
		{
			CString savepath = CUtils::GetLongPathname(parser.GetVal(_T("savepath")));
			CString revision = parser.GetVal(_T("revision"));
			CString pegrevision = parser.GetVal(_T("pegrevision"));
			LONG rev = _ttol(revision);
			if (rev==0)
				rev = SVNRev::REV_HEAD;
			LONG pegrev = _ttol(pegrevision);
			if (pegrev == 0)
				pegrev = SVNRev::REV_HEAD;
			SVN svn;
			if (!svn.Cat(cmdLinePath, pegrev, rev, CTSVNPath(savepath)))
			{
				::MessageBox(NULL, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
				::DeleteFile(savepath);
			} 
		}
		//#endregion
		//#region createpatch
		if (command == cmdCreatePatch)
		{
			CString savepath = CUtils::GetLongPathname(parser.GetVal(_T("savepath")));
			CreatePatch(cmdLinePath, CTSVNPath(savepath));
		}
		//#endregion
		//#region updatecheck
		if (command == cmdUpdateCheck)
		{
			CCheckForUpdatesDlg dlg;
			if (parser.HasKey(_T("visible")))
				dlg.m_bShowInfo = TRUE;
			dlg.DoModal();
		}
		//#endregion
		//#region revisiongraph
		if (command == cmdRevisionGraph)
		{
			CRevisionGraphDlg dlg;
			dlg.m_sPath = cmdLinePath.GetWinPathString();
			dlg.DoModal();
		} 
		//#endregion
		//#region lock
		if (command == cmdLock)
		{
			CLockDlg lockDlg;
			lockDlg.m_pathList = pathList;
			if (lockDlg.DoModal()==IDOK)
			{
				CSVNProgressDlg progDlg;
				progDlg.SetParams(CSVNProgressDlg::Lock, lockDlg.m_bStealLocks ? ProgOptLockForce : 0, pathList, CString(), lockDlg.m_sLockMessage);
				progDlg.DoModal();
			}
		}
		//#endregion
		//#region unlock
		if (command == cmdUnlock)
		{
			CSVNProgressDlg progDlg;
			progDlg.SetParams(CSVNProgressDlg::Unlock, parser.HasKey(_T("force")) ? ProgOptLockForce : 0, pathList);
			progDlg.DoModal();
		} 
		//#endregion

		if (TSVNMutex)
			::CloseHandle(TSVNMutex);
	} 

	// Look for temporary files left around by TortoiseSVN and
	// remove them. But only delete 'old' files because some
	// apps might still be needing the recent ones.
	{
		DWORD len = ::GetTempPath(0, NULL);
		TCHAR * path = new TCHAR[len + 100];
		len = ::GetTempPath (len+100, path);
		if (len != 0)
		{
			CSimpleFileFind finder = CSimpleFileFind(path, _T("svn*.*"));
			FILETIME systime_;
			::GetSystemTimeAsFileTime(&systime_);
			__int64 systime = (((_int64)systime_.dwHighDateTime)<<32) | ((__int64)systime_.dwLowDateTime);
			while (finder.FindNextFileNoDirectories())
			{
				CString filepath = finder.GetFilePath();
				HANDLE hFile = ::CreateFile(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
				if (hFile != INVALID_HANDLE_VALUE)
				{
					FILETIME createtime_;
					if (::GetFileTime(hFile, &createtime_, NULL, NULL))
					{
						::CloseHandle(hFile);
						__int64 createtime = (((_int64)createtime_.dwHighDateTime)<<32) | ((__int64)createtime_.dwLowDateTime);
						if ((createtime + 864000000000) < systime)		//only delete files older than a day
						{
							::DeleteFile(filepath);
						}
					}
					else
						::CloseHandle(hFile);
				}
			}
		}	
		delete path;		
	}


	// Since the dialog has been closed, return FALSE so that we exit the
	// application, rather than start the application's message pump.
	return FALSE;
}


UINT_PTR CALLBACK 
CTortoiseProcApp::CreatePatchFileOpenHook(HWND hDlg, UINT uiMsg, WPARAM wParam, LPARAM /*lParam*/)
{
	if(uiMsg ==	WM_COMMAND && LOWORD(wParam) == IDC_PATCH_TO_CLIPBOARD)
	{
		HWND hFileDialog = GetParent(hDlg);
		
		CString strFilename = CUtils::GetTempFile() + PATCH_TO_CLIPBOARD_PSEUDO_FILENAME;

		CommDlg_OpenSave_SetControlText(hFileDialog, edt1, (LPCTSTR)strFilename);   

		PostMessage(hFileDialog, WM_COMMAND, MAKEWPARAM(IDOK, BM_CLICK), (LPARAM)(GetDlgItem(hDlg, IDOK)));
	}
	return 0;
}

BOOL CTortoiseProcApp::CreatePatch(const CTSVNPath& path, const CTSVNPath& cmdLineSavePath)
{
	OPENFILENAME ofn;		// common dialog box structure
	CString temp;
	CTSVNPath savePath;

	if (cmdLineSavePath.IsEmpty())
	{
		TCHAR szFile[MAX_PATH];  // buffer for file name
		ZeroMemory(szFile, sizeof(szFile));
		// Initialize OPENFILENAME
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		//ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;		//to stay compatible with NT4
		ofn.hwndOwner = (EXPLORERHWND);
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
		temp.LoadString(IDS_REPOBROWSE_SAVEAS);
		CUtils::RemoveAccelerators(temp);
		if (temp.IsEmpty())
			ofn.lpstrTitle = NULL;
		else
			ofn.lpstrTitle = temp;
		ofn.Flags = OFN_OVERWRITEPROMPT | OFN_ENABLETEMPLATE | OFN_EXPLORER | OFN_ENABLEHOOK;

		ofn.hInstance = AfxGetResourceHandle();
		ofn.lpTemplateName = MAKEINTRESOURCE(IDD_PATCH_FILE_OPEN_CUSTOM);
		ofn.lpfnHook = CreatePatchFileOpenHook;

		CString sFilter;
		sFilter.LoadString(IDS_PATCHFILEFILTER);
		TCHAR * pszFilters = new TCHAR[sFilter.GetLength()+4];
		_tcscpy (pszFilters, sFilter);
		// Replace '|' delimeters with '\0's
		TCHAR *ptr = pszFilters + _tcslen(pszFilters);  //set ptr at the NULL
		while (ptr != pszFilters)
		{
			if (*ptr == '|')
				*ptr = '\0';
			ptr--;
		} // while (ptr != pszFilters) 
		ofn.lpstrFilter = pszFilters;
		ofn.nFilterIndex = 1;
		// Display the Open dialog box. 
		if (GetSaveFileName(&ofn)==FALSE)
		{
			delete [] pszFilters;
			return FALSE;
		} // if (GetSaveFileName(&ofn)==FALSE)
		delete [] pszFilters;
		savePath = CTSVNPath(ofn.lpstrFile);
		if (ofn.nFilterIndex == 1)
		{
			if (savePath.GetFileExtension().IsEmpty())
				savePath.AppendRawString(_T(".patch"));
		}
	}
	else
	{
		savePath = cmdLineSavePath;
	}

	// This is horrible and I should be ashamed of myself, but basically, the 
	// the file-open dialog writes ".TSVNPatchToClipboard" to its file field if the user clicks
	// the "Save To Clipboard" button.
	bool bToClipboard = _tcsstr(savePath.GetWinPath(), PATCH_TO_CLIPBOARD_PSEUDO_FILENAME) != NULL;

	CProgressDlg progDlg;
	progDlg.SetTitle(IDS_PROC_PATCHTITLE);
	progDlg.SetShowProgressBar(false);
	progDlg.ShowModeless(CWnd::FromHandle(EXPLORERHWND));
	progDlg.FormatNonPathLine(1, IDS_PROC_SAVEPATCHTO);
	if(bToClipboard)
	{
		progDlg.FormatNonPathLine(2, IDS_CLIPBOARD_PROGRESS_DEST);
	}
	else
	{
		progDlg.SetLine(2, savePath.GetUIPathString(), true);
	}
	//progDlg.SetAnimation(IDR_ANIMATION);

	CTSVNPath tempPatchFilePath;
	if (bToClipboard)
		tempPatchFilePath = CUtils::GetTempFilePath();
	else
		tempPatchFilePath = savePath;

	CTSVNPath sDir;
	if (!path.IsDirectory())
	{
		::SetCurrentDirectory(path.GetDirectory().GetWinPath());
		sDir.SetFromWin(path.GetFilename());
	}
	else
	{
		::SetCurrentDirectory(path.GetWinPath());
	}

	SVN svn;
	if (!svn.Diff(sDir, SVNRev::REV_BASE, sDir, SVNRev::REV_WC, TRUE, FALSE, FALSE, FALSE, _T(""), tempPatchFilePath))
	{
		progDlg.Stop();
		::MessageBox((EXPLORERHWND), svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		return FALSE;
	}

	if(bToClipboard)
	{
		// The user actually asked for the patch to be written to the clipboard
		CStringA sClipdata;
		FILE* inFile = _tfopen(tempPatchFilePath.GetWinPath(), _T("rb"));
		if(inFile)
		{
			char chunkBuffer[16384];
			while(!feof(inFile))
			{
				size_t readLength = fread(chunkBuffer, 1, sizeof(chunkBuffer), inFile);
				sClipdata.Append(chunkBuffer, (int)readLength);
			}
			fclose(inFile);

			CUtils::WriteAsciiStringToClipboard(sClipdata);

		}
	}

	progDlg.Stop();
	if (bToClipboard)
	{
		DeleteFile(tempPatchFilePath.GetWinPath());
	}
	return TRUE;
}

void CTortoiseProcApp::CheckUpgrade()
{
	CRegString regVersion = CRegString(_T("Software\\TortoiseSVN\\CurrentVersion"));
	CString sVersion = regVersion;
	if (sVersion.Compare(_T(STRPRODUCTVER))==0)
		return;
	// we're starting the first time with a new version!
	
	LONG lVersion = 0;
	int pos = sVersion.Find(',');
	if (pos > 0)
	{
		lVersion = (_ttol(sVersion.Left(pos))<<24);
		lVersion |= (_ttol(sVersion.Mid(pos+1))<<16);
		pos = sVersion.Find(',', pos+1);
		lVersion |= (_ttol(sVersion.Mid(pos+1))<<8);
	}
	
	CRegDWORD regval = CRegDWORD(_T("Software\\TortoiseSVN\\DontConvertBase"), 999);
	if ((DWORD)regval != 999)
	{
		// there's a leftover registry setting we have to convert and then delete it
		CRegDWORD newregval = CRegDWORD(_T("Software\\TortoiseSVN\\ConvertBase"));
		newregval = !regval;
		regval.removeValue();
	}

	if (lVersion <= 0x01010300)
	{
		CSoundUtils::RegisterTSVNSounds();
		// remove all saved dialog positions
		CRegString(_T("Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\")).removeKey();
		CRegDWORD(_T("Software\\TortoiseSVN\\RecursiveOverlay")).removeValue();
		// remove the external cache key
		CRegDWORD(_T("Software\\TortoiseSVN\\ExternalCache")).removeValue();
	}
	
	// set the current version so we don't come here again until the next update!
	regVersion = _T(STRPRODUCTVER);	
}

