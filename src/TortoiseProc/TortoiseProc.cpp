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
#include "CheckForUpdatesDlg.h"
#include "RevisionGraphDlg.h"
#include "BrowseFolder.h"

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


CTortoiseProcApp::CTortoiseProcApp()
{
	EnableHtmlHelp();
	apr_initialize();
	SYS_IMAGE_LIST();
}

CTortoiseProcApp::~CTortoiseProcApp()
{
	apr_terminate();
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
	//set the resource dll for the required language
	CRegDWORD loc = CRegDWORD(_T("Software\\TortoiseSVN\\LanguageID"), 1033);
	long langId = loc;
	CString langDll;
	char procpath[MAX_PATH] = {0};
	GetModuleFileNameA(NULL, procpath, MAX_PATH);
	CStringA langpath = procpath;
	langpath = langpath.Left(langpath.ReverseFind('\\'));
	langpath = langpath.Left(langpath.ReverseFind('\\')+1);
	langpath += "Languages";
	bindtextdomain("subversion", (LPCSTR)langpath);
	SetThreadLocale(1033); 
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
			SetThreadLocale(langId);
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
		if (comVal.Compare(_T("test"))==0)
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
					STARTUPINFO startup;
					PROCESS_INFORMATION process;
					memset(&startup, 0, sizeof(startup));
					startup.cb = sizeof(startup);
					memset(&process, 0, sizeof(process));
					TCHAR com[MAX_PATH+100];
					GetModuleFileName(NULL, com, MAX_PATH);
					_tcscat(com, _T(" /command:updatecheck"));
					CreateProcess(NULL, com, NULL, NULL, FALSE, 0, 0, 0, &startup, &process);
				}
			}
		}

		//#region crash
		if (comVal.Compare(_T("crash"))==0)
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
				return FALSE;
			}
		}

		//#region about
		if (comVal.Compare(_T("about"))==0)
		{
			CAboutDlg dlg;
			m_pMainWnd = &dlg;
			dlg.DoModal();
		}
		//#endregion
		//#region rtfm
		if (comVal.Compare(_T("rtfm"))==0)
		{
			CMessageBox::Show(EXPLORERHWND, IDS_PROC_RTFM, IDS_APPNAME, MB_ICONINFORMATION);
			TCHAR path[MAX_PATH];
			SHGetFolderPath(EXPLORERHWND, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, path);
			ShellExecute(0,_T("explore"),path,NULL,NULL,SW_SHOWNORMAL);
		}
		//#endregion
		//#region log
		if (comVal.Compare(_T("log"))==0)
		{
			//the log command line looks like this:
			//command:log path:<path_to_file_or_directory_to_show_the_log_messages> [revstart:<startrevision>] [revend:<endrevision>]
			CString path = CUtils::GetLongPathname(parser.GetVal(_T("path")));
			CString val = parser.GetVal(_T("revstart"));
			long revstart = _tstol(val);
			val = parser.GetVal(_T("revend"));
			long revend = _tstol(val);
			if (revstart == 0)
			{
				revstart = SVNRev::REV_HEAD;
			}
			if (revend == 0)
			{
				CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100);
				revend = reg;
				revend = -revend;
			}
			CLogDlg dlg;
			m_pMainWnd = &dlg;
			dlg.SetParams(path, revstart, revend, !parser.HasKey(_T("nostrict")));
			dlg.DoModal();			
		}
		//#endregion
		//#region checkout
		if (comVal.Compare(_T("checkout"))==0)
		{
			//
			// Get the directory supplied in the command line. If there isn't
			// one then we should use the current working directory instead.
			//
			CString strPath = CUtils::GetLongPathname(parser.GetVal(_T("path")));
			if (strPath.IsEmpty())
			{
				DWORD dwBufSize = ::GetCurrentDirectory(0, NULL);
				LPTSTR tszPath = strPath.GetBufferSetLength(dwBufSize);
				::GetCurrentDirectory(dwBufSize, tszPath);
				strPath.ReleaseBuffer();
			}

			//
			// Create a checkout dialog and display it. If the user clicks OK,
			// we should create an SVN progress dialog, set it as the main
			// window and then display it.
			//
			CCheckoutDlg dlg;
			dlg.m_strCheckoutDirectory = strPath;
			if (dlg.DoModal() == IDOK)
			{
				TRACE(_T("url = %s\n"), (LPCTSTR)dlg.m_URL);
				TRACE(_T("checkout dir = %s \n"), (LPCTSTR)dlg.m_strCheckoutDirectory);

				strPath = dlg.m_strCheckoutDirectory;

				CSVNProgressDlg progDlg(PWND);
				progDlg.m_bCloseOnEnd = parser.HasKey(_T("closeonend"));
				m_pMainWnd = &progDlg;
				progDlg.SetParams(CSVNProgressDlg::Checkout, dlg.m_bNonRecursive ? ProgOptNonRecursive : ProgOptRecursive, strPath, dlg.m_URL, _T(""), dlg.Revision);
				progDlg.DoModal();
			}
		}
		//#endregion
		//#region import
		if (comVal.Compare(_T("import"))==0)
		{
			CImportDlg dlg;
			CString path = CUtils::GetLongPathname(parser.GetVal(_T("path")));
			dlg.m_path = path;
			if (dlg.DoModal() == IDOK)
			{
				TRACE(_T("url = %s\n"), (LPCTSTR)dlg.m_url);
				CSVNProgressDlg progDlg(PWND);
				progDlg.m_bCloseOnEnd = parser.HasKey(_T("closeonend"));
				m_pMainWnd = &progDlg;
				//construct the module name out of the path
				CString modname;
				progDlg.SetParams(CSVNProgressDlg::Import, ProgOptPathIsTempFile, path, dlg.m_url, dlg.m_message);
				progDlg.DoModal();
			}
		}
		//#endregion
		//#region update
		if (comVal.Compare(_T("update"))==0)
		{
			SVNRev rev = SVNRev(_T("HEAD"));
			CString path = CUtils::GetLongPathname(parser.GetVal(_T("path")));
			BOOL bUseTempfile = !parser.HasKey(_T("notempfile"));
			TRACE(_T("tempfile = %s\n"), (LPCTSTR)path);
			int options = ProgOptRecursive;
			if (parser.HasKey(_T("rev")))
			{
				CUpdateDlg dlg;
				if (dlg.DoModal() == IDOK)
				{
					rev = dlg.Revision;
					if (dlg.m_bNonRecursive)
					{
						options = ProgOptNonRecursive;
					}
					if (CRegDWORD(_T("Software\\TortoiseSVN\\updatetorev"), (DWORD)-1)==(DWORD)-1)
						if (SVNStatus::GetAllStatusRecursive(path)>svn_wc_status_normal)
							if (CMessageBox::ShowCheck(EXPLORERHWND, IDS_WARN_UPDATETOREV_WITHMODS, IDS_APPNAME, MB_OKCANCEL | MB_ICONWARNING, _T("updatetorev"), IDS_MSGBOX_DONOTSHOWAGAIN)!=IDOK)
								return FALSE;
				}
				else 
					return FALSE;
			}
			CSVNProgressDlg progDlg(PWND);
			progDlg.m_bCloseOnEnd = parser.HasKey(_T("closeonend"));
			m_pMainWnd = &progDlg;
			progDlg.SetParams(CSVNProgressDlg::Update, options | (bUseTempfile ? ProgOptPathIsTempFile : ProgOptPathIsTarget), path, _T(""), _T(""), rev);
			progDlg.DoModal();
		}
		//#endregion
		//#region commit
		if (comVal.Compare(_T("commit"))==0)
		{
			CString path = CUtils::GetLongPathname(parser.GetVal(_T("path")));
			if (parser.HasKey(_T("notempfile")))
			{
				path = CUtils::WritePathsToTempFile(path);
			} // if (parser.HasKey(_T("notempfile"))) 
			CLogPromptDlg dlg;
			if (parser.HasKey(_T("logmsg")))
			{
				dlg.m_sLogMessage = parser.GetVal(_T("logmsg"));
			}
			dlg.m_sPath = path;
			if (dlg.DoModal() == IDOK)
			{
				TRACE(_T("tempfile = %s\n"), (LPCTSTR)path);
				CSVNProgressDlg progDlg(PWND);
				progDlg.m_bCloseOnEnd = parser.HasKey(_T("closeonend"));
				m_pMainWnd = &progDlg;
				progDlg.SetParams(CSVNProgressDlg::Commit, ProgOptPathIsTempFile, path, _T(""), dlg.m_sLogMessage, !dlg.m_bRecursive);
				progDlg.DoModal();
			} // if (dlg.DoModal() == IDOK)
		}
		//#endregion
		//#region add
		if (comVal.Compare(_T("add"))==0)
		{
			CString path = CUtils::GetLongPathname(parser.GetVal(_T("path")));
			if (parser.HasKey(_T("notempfile")))
			{
				path = CUtils::WritePathsToTempFile(path);
			} // if (parser.HasKey(_T("notempfile"))) 
			CAddDlg dlg;
			dlg.m_sPath = path;
			if (dlg.DoModal() == IDOK)
			{
				TRACE(_T("tempfile = %s\n"), (LPCTSTR)path);
				CSVNProgressDlg progDlg(PWND);
				progDlg.m_bCloseOnEnd = parser.HasKey(_T("closeonend"));
				m_pMainWnd = &progDlg;
				progDlg.SetParams(CSVNProgressDlg::Add, ProgOptPathIsTempFile, path);
				progDlg.DoModal();
			} // if (dlg.DoModal() == IDOK) // if (dlg.DoModal() == IDOK) 
			else
			{
				DeleteFile(path);
			}
		}
		//#endregion
		//#region revert
		if (comVal.Compare(_T("revert"))==0)
		{
			CString path = CUtils::GetLongPathname(parser.GetVal(_T("path")));
			if (parser.HasKey(_T("notempfile")))
			{
				path = CUtils::WritePathsToTempFile(path);
			} // if (parser.HasKey(_T("notempfile"))) 
			CRevertDlg dlg;
			dlg.m_sPath = path;
			if (dlg.DoModal() == IDOK)
			{
				CSVNProgressDlg progDlg(PWND);
				progDlg.m_bCloseOnEnd = parser.HasKey(_T("closeonend"));
				m_pMainWnd = &progDlg;
				int options = ProgOptPathIsTempFile | (dlg.m_bRecursive ? ProgOptRecursive : ProgOptNonRecursive);
				progDlg.SetParams(CSVNProgressDlg::Revert, options, path);
				progDlg.DoModal();
			}
		}
		//#endregion
		//#region cleanup
		if (comVal.Compare(_T("cleanup"))==0)
		{
			CString path = CUtils::GetLongPathname(parser.GetVal(_T("path")));
			CProgressDlg progress;
			progress.SetTitle(IDS_PROC_CLEANUP);
			progress.ShowModeless(PWND);
			SVN svn;
			if (!svn.CleanUp(CTSVNPath(path)))
			{
				progress.Stop();
				CString errorMessage;
				errorMessage.Format(IDS_ERR_CLEANUP, (LPCTSTR)svn.GetLastErrorMessage());
				CMessageBox::Show(EXPLORERHWND, errorMessage, _T("TortoiseSVN"), MB_ICONERROR);
			}
			else
			{
				progress.Stop();
				CMessageBox::Show(EXPLORERHWND, IDS_PROC_CLEANUPFINISHED, IDS_APPNAME, MB_OK | MB_ICONINFORMATION);
			}
		}
		//#endregion
		//#region resolve
		if (comVal.Compare(_T("resolve"))==0)
		{
			UINT ret = IDYES;
			if (!parser.HasKey(_T("noquestion")))
				ret = CMessageBox::Show(EXPLORERHWND, IDS_PROC_RESOLVE, IDS_APPNAME, MB_ICONQUESTION | MB_YESNO);
			if (ret==IDYES)
			{
				CString path = CUtils::GetLongPathname(parser.GetVal(_T("path")));
				CSVNProgressDlg progDlg(PWND);
				progDlg.m_bCloseOnEnd = parser.HasKey(_T("closeonend"));
				m_pMainWnd = &progDlg;
				progDlg.SetParams(CSVNProgressDlg::Resolve, ProgOptPathIsTarget, path);
				progDlg.DoModal();
			}
		}
		//#endregion
		//#region repocreate
		if (comVal.Compare(_T("repocreate"))==0)
		{
			CString path = CUtils::GetLongPathname(parser.GetVal(_T("path")));
			path.Replace('/', '\\');

			CRepoCreateDlg dlg;
			if (dlg.DoModal() == IDOK)
			{
				if ((dlg.RepoType.Compare(_T("bdb"))==0)&&(GetDriveType(path.Left(path.Find('\\')+1))==DRIVE_REMOTE))
				{
					if (CMessageBox::Show(EXPLORERHWND, IDS_PROC_REPOCREATESHAREWARN, IDS_APPNAME, MB_ICONINFORMATION | MB_YESNO)==IDYES)
					{
						if (!SVN::CreateRepository(path, dlg.RepoType))
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
					if (!SVN::CreateRepository(path, dlg.RepoType))
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
		if (comVal.Compare(_T("switch"))==0)
		{
			CSwitchDlg dlg;
			CString path = CUtils::GetLongPathname(parser.GetVal(_T("path")));
			dlg.m_path = path;

			if (dlg.DoModal() == IDOK)
			{
				CSVNProgressDlg progDlg(PWND);
				progDlg.m_bCloseOnEnd = parser.HasKey(_T("closeonend"));
				m_pMainWnd = &progDlg;
				progDlg.SetParams(CSVNProgressDlg::Switch, ProgOptPathIsTarget, path, dlg.m_URL, _T(""), dlg.Revision);
				progDlg.DoModal();
			}
		}
		//#endregion
		//#region export
		if (comVal.Compare(_T("export"))==0)
		{
			TCHAR saveto[MAX_PATH];
			CString path = CUtils::GetLongPathname(parser.GetVal(_T("path")));
			if (SVNStatus::GetAllStatus(path) == svn_wc_status_unversioned)
			{
				CCheckoutDlg dlg;
				dlg.m_strCheckoutDirectory = path;
				dlg.IsExport = TRUE;
				if (dlg.DoModal() == IDOK)
				{
					path = dlg.m_strCheckoutDirectory;

					CSVNProgressDlg progDlg(PWND);
					progDlg.m_bCloseOnEnd = parser.HasKey(_T("closeonend"));
					m_pMainWnd = &progDlg;
					progDlg.SetParams(CSVNProgressDlg::Export, ProgOptPathIsTarget, path, dlg.m_URL, _T(""), dlg.Revision);
					progDlg.DoModal();
				}
			}
			else
			{
				CBrowseFolder folderBrowser;
				CString strTemp;
				strTemp.LoadString(IDS_PROC_EXPORT_1);
				folderBrowser.SetInfo(strTemp);
				folderBrowser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
				strTemp.LoadString(IDS_PROC_EXPORT_2);
				folderBrowser.SetCheckBoxText(strTemp);
				CRegDWORD regExtended = CRegDWORD(_T("Software\\TortoiseSVN\\ExportExtended"), FALSE);
				CBrowseFolder::m_bCheck = regExtended;
				if (folderBrowser.Show(EXPLORERHWND, saveto)==CBrowseFolder::OK)
				{
					CString saveplace = CString(saveto);
					saveplace += path.Right(path.GetLength() - path.ReverseFind('\\'));
					TRACE(_T("export %s to %s\n"), (LPCTSTR)path, (LPCTSTR)saveto);
					CProgressDlg progDlg;
					CString windowTitle;
					windowTitle.LoadString(IDS_PROC_EXPORT_3);
					progDlg.SetTitle(windowTitle);
					progDlg.SetLine(1, windowTitle);
					progDlg.SetShowProgressBar(true);
					progDlg.ShowModeless(CWnd::FromHandle(EXPLORERHWND));
					progDlg.SetAnimation(IDR_ANIMATION);
					SVN svn;
					if (!svn.Export(path, saveplace, SVNRev::REV_WC, TRUE, &progDlg, folderBrowser.m_bCheck))
					{
						progDlg.Stop();
						CMessageBox::Show(EXPLORERHWND, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_OK | MB_ICONERROR);
					}
					else
					{
						progDlg.Stop();
						CString strMessage;
						strMessage.Format(IDS_PROC_EXPORT_4, (LPCTSTR)path, (LPCTSTR)saveplace);
						CMessageBox::Show(EXPLORERHWND, strMessage, _T("TortoiseSVN"), MB_OK | MB_ICONINFORMATION);
					}
					regExtended = CBrowseFolder::m_bCheck;
				}
			}
		}
		//#endregion
		//#region merge
		if (comVal.Compare(_T("merge"))==0)
		{
			BOOL repeat = FALSE;
			CMergeDlg dlg;
			CString path = CUtils::GetLongPathname(parser.GetVal(_T("path")));
			dlg.m_wcPath = path;
			do 
			{	
				if (dlg.DoModal() == IDOK)
				{
					CSVNProgressDlg progDlg(PWND);
					progDlg.m_bCloseOnEnd = parser.HasKey(_T("closeonend"));
					//m_pMainWnd = &progDlg;
					progDlg.SetParams(CSVNProgressDlg::Merge, dlg.m_bDryRun ? (ProgOptDryRun | ProgOptPathIsTarget) : ProgOptPathIsTarget, path, dlg.m_URLFrom, dlg.m_URLTo, dlg.StartRev);		//use the message as the second url
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
		if (comVal.Compare(_T("copy"))==0)
		{
			CCopyDlg dlg;
			CString path = CUtils::GetLongPathname(parser.GetVal(_T("path")));
			dlg.m_path = path;
			if (dlg.DoModal() == IDOK)
			{
				m_pMainWnd = NULL;
				TRACE(_T("copy %s to %s\n"), (LPCTSTR)path, (LPCTSTR)dlg.m_URL);
				CSVNProgressDlg progDlg(PWND);
				progDlg.m_bCloseOnEnd = parser.HasKey(_T("closeonend"));
				progDlg.SetParams(CSVNProgressDlg::Copy, ProgOptPathIsTarget, path, dlg.m_URL, dlg.m_sLogMessage, (dlg.m_bDirectCopy ? SVNRev::REV_HEAD : SVNRev::REV_WC));
				progDlg.DoModal();
			}
		}
		//#endregion
		//#region settings
		if (comVal.Compare(_T("settings"))==0)
		{
			CSettings dlg(IDS_PROC_SETTINGS_TITLE);
			if (dlg.DoModal()==IDOK)
			{
				dlg.SaveData();
			}
		}
		//#endregion
		//#region remove
		if (comVal.Compare(_T("remove"))==0)
		{
			CString path = CUtils::GetLongPathname(parser.GetVal(_T("path")));
			if (parser.HasKey(_T("notempfile")))
			{
				path = CUtils::WritePathsToTempFile(path);
			} // if (parser.HasKey(_T("notempfile"))) 
			TRACE(_T("tempfile = %s\n"), (LPCTSTR)path);
			try
			{
				// open the temp file
				CStdioFile file(path, CFile::typeBinary | CFile::modeRead); 
				CString strLine = _T(""); // initialise the variable which holds each line's contents
				BOOL bForce = FALSE;
				SVN svn;
				while (file.ReadString(strLine))
				{
					TRACE(_T("remove file %s\n"), (LPCTSTR)strLine);
					// even though SVN::Remove takes a list of paths to delete at once
					// we delete each item individually so we can prompt the user
					// if something goes wrong or unversioned/modified items are
					// to be deleted
					CTSVNPath path(strLine);
					CTSVNPathList pathList(path);
					if (!svn.Remove(pathList, bForce))
					{
						if ((svn.Err->apr_err == SVN_ERR_UNVERSIONED_RESOURCE) ||
							(svn.Err->apr_err == SVN_ERR_CLIENT_MODIFIED))
						{
							svn.ReleasePool();
							CString msg, yes, no, yestoall;
							msg.Format(IDS_PROC_REMOVEFORCE, svn.GetLastErrorMessage());
							yes.LoadString(IDS_MSGBOX_YES);
							no.LoadString(IDS_MSGBOX_NO);
							yestoall.LoadString(IDS_PROC_YESTOALL);
							UINT ret = CMessageBox::Show(EXPLORERHWND, msg, _T("TortoiseSVN"), 2, IDI_ERROR, yes, no, yestoall);
							if (ret == 3)
								bForce = TRUE;
							if ((ret == 1)||(ret==3))
								if (!svn.Remove(pathList, TRUE))
								{
									CMessageBox::Show(EXPLORERHWND, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								}
						}
						else
							CMessageBox::Show(EXPLORERHWND, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					}
				}
				file.Close();
				CFile::Remove(path);
			}
			catch (CFileException* )
			{
				TRACE(_T("CFileException in remove!\n"));
			}
		}
		//#endregion
		//#region rename
		if (comVal.Compare(_T("rename"))==0)
		{
			CString path = CUtils::GetLongPathname(parser.GetVal(_T("path")));
			CRenameDlg dlg;
			CString filename = path.Right(path.GetLength() - path.ReverseFind('\\') - 1);
			CString filepath = path.Left(path.ReverseFind('\\') + 1);
			::SetCurrentDirectory(filepath);
			dlg.m_name = filename;
			if (dlg.DoModal() == IDOK)
			{
				TRACE(_T("rename file %s to %s\n"), (LPCTSTR)path, (LPCTSTR)dlg.m_name);
				if (PathIsRelative(dlg.m_name) && !PathIsURL(dlg.m_name))
					filepath =  filepath + dlg.m_name;
				else
					filepath = dlg.m_name;
				if (path.CompareNoCase(filepath)==0)
				{
					//rename to the same file!
					CMessageBox::Show(EXPLORERHWND, IDS_PROC_CASERENAME, IDS_APPNAME, MB_ICONERROR);
				}
				else
				{
					SVN svn;
					if (!svn.Move(CTSVNPath(path), CTSVNPath(filepath), TRUE))
					{
						TRACE(_T("%s\n"), (LPCTSTR)svn.GetLastErrorMessage());
						CMessageBox::Show(EXPLORERHWND, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					}
				}
			}
		}
		//#endregion
		//#region diff
		if (comVal.Compare(_T("diff"))==0)
		{
			CString path = CUtils::GetLongPathname(parser.GetVal(_T("path")));
			CString path2 = CUtils::GetLongPathname(parser.GetVal(_T("path2")));
			BOOL bDelete = FALSE;
			if (path2.IsEmpty())
			{
				if (PathIsDirectory(path))
				{
					path = CUtils::WritePathsToTempFile(path);
					CChangedDlg dlg;
					dlg.m_path = path;
					dlg.DoModal();
					DeleteFile(path);
				}
				else
				{
					CString name = CUtils::GetFileNameFromPath(path);
					CString ext = CUtils::GetFileExtFromPath(path);
					path2 = SVN::GetPristinePath(path);
					if ((!CRegDWORD(_T("Software\\TortoiseSVN\\DontConvertBase"), TRUE))&&(SVN::GetTranslatedFile(path, path)))
					{
						bDelete = TRUE;
					}
					CString n1, n2;
					n1.Format(IDS_DIFF_WCNAME, (LPCTSTR)name);
					n2.Format(IDS_DIFF_BASENAME, (LPCTSTR)name);
					CUtils::StartDiffViewer(path2, path, TRUE, n2, n1, ext);
				}
			} // if (path2.IsEmpty())
			else
				CUtils::StartDiffViewer(path2, path, TRUE);
			if (bDelete)
				DeleteFile(path);
		}
		//#endregion
		//#region dropcopyadd
		if (comVal.Compare(_T("dropcopyadd"))==0)
		{
			CString path = parser.GetVal(_T("path"));
			CString droppath = parser.GetVal(_T("droptarget"));

			CStringArray sarray;
			CStdioFile file(path, CFile::typeBinary | CFile::modeRead);
			CString strLine = _T("");
			while (file.ReadString(strLine))
			{
				if (strLine.CompareNoCase(droppath) != 0)
				{
					//copy the file to the new location
					CString name = strLine.Right(strLine.GetLength() - strLine.ReverseFind('\\') - 1);
					if (PathFileExists(droppath+_T("\\")+name))
					{
						CString strMessage;
						strMessage.Format(IDS_PROC_OVERWRITE_CONFIRM, (LPCTSTR)(droppath+_T("\\")+name));
						int ret = CMessageBox::Show(EXPLORERHWND, strMessage, _T("TortoiseSVN"), MB_YESNOCANCEL | MB_ICONQUESTION);
						if (ret == IDYES)
						{
							if (!CopyFile(strLine, droppath+_T("\\")+name, TRUE))
							{
								//the copy operation failed! Get out of here!
								file.Close();
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
							file.Close();
							return FALSE;		//cancel the whole operation
						}
					}
					else if (!CopyFile(strLine, droppath+_T("\\")+name, FALSE))
					{
						//the copy operation failed! Get out of here!
						file.Close();
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
					sarray.Add(droppath+_T("\\")+name);		//add the new filepath
				}
			}
			file.Close();
			CStdioFile newfile(path, CFile::typeBinary | CFile::modeCreate | CFile::modeWrite);
			for (int i=0; i<sarray.GetSize(); i++)
			{
				newfile.WriteString(sarray.GetAt(i)+_T("\n"));
			}
			newfile.Close();
			//now add all the newly copied files to the working copy
			TRACE(_T("tempfile = %s\n"), (LPCTSTR)path);
			CSVNProgressDlg progDlg(PWND);
			progDlg.m_bCloseOnEnd = parser.HasKey(_T("closeonend"));
			m_pMainWnd = &progDlg;
			progDlg.SetParams(CSVNProgressDlg::Add, ProgOptPathIsTempFile, path);
			progDlg.DoModal();
		}
		//#endregion
		//#region dropmove
		if (comVal.Compare(_T("dropmove"))==0)
		{
			CString path = parser.GetVal(_T("path"));
			CString droppath = parser.GetVal(_T("droptarget"));
			SVN svn;
			unsigned long totalcount = 0;
			unsigned long count = 0;
			CStdioFile file(path, CFile::typeBinary | CFile::modeRead);
			CString strLine = _T("");
			while (file.ReadString(strLine))
				totalcount++;
			file.SeekToBegin();
			CProgressDlg progress;
			if (progress.IsValid())
			{
				progress.SetTitle(IDS_PROC_MOVING);
				progress.SetAnimation(IDR_MOVEANI);
				progress.ShowModeless(CWnd::FromHandle(EXPLORERHWND));
			}
			while (file.ReadString(strLine))
			{
				CString name = strLine.Right(strLine.GetLength() - strLine.ReverseFind('\\') - 1);
				CString temp;
				temp.Format(IDS_PROC_MOVINGPROG, (LPCTSTR)strLine);
				CString temp2;
				temp2.Format(IDS_PROC_CPYMVPROG2, (LPCTSTR)(droppath+_T("\\")+name));
				if (strLine.CompareNoCase(droppath+_T("\\")+name)==0)
				{
					progress.Stop();
					CRenameDlg dlg;
					dlg.m_windowtitle.Format(IDS_PROC_RENAME, (LPCTSTR)name);
					if (dlg.DoModal() != IDOK)
					{
						return FALSE;
					}
					name = dlg.m_name;
				} // if (strLine.CompareNoCase(droppath+_T("\\")+name)==0) 
				if (!svn.Move(CTSVNPath(strLine), CTSVNPath(droppath+_T("\\")+name), FALSE))
				{
					TRACE(_T("%s\n"), (LPCTSTR)svn.GetLastErrorMessage());
					CMessageBox::Show(EXPLORERHWND, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					return FALSE;		//get out of here
				} // if (!svn.Move(strLine, droppath+_T("\\")+name, FALSE)) 
				count++;
				if (progress.IsValid())
				{
					progress.SetLine(1, temp, true);
					progress.SetLine(2, temp2, true);
					progress.SetProgress(count, totalcount);
				} // if (progress.IsValid())
				if ((progress.IsValid())&&(progress.HasUserCancelled()))
				{
					CMessageBox::Show(EXPLORERHWND, IDS_SVN_USERCANCELLED, IDS_APPNAME, MB_ICONINFORMATION);
					return FALSE;
				}
			}
		}
		//#endregion
		//#region dropexport
		if (comVal.Compare(_T("dropexport"))==0)
		{
			CString path = parser.GetVal(_T("path"));
			if (parser.HasKey(_T("notempfile")))
			{
				path = CUtils::WritePathsToTempFile(path);
			}
			CString droppath = parser.GetVal(_T("droptarget"));
			SVN svn;
			CStdioFile file(path, CFile::typeBinary | CFile::modeRead);
			CString strLine = _T("");
			CProgressDlg progDlg;
			while (file.ReadString(strLine))
			{
				CString strTitle;
				strTitle.LoadString(IDS_PROC_EXPORT_3);
				progDlg.SetTitle(strTitle);
				progDlg.SetLine(1, strTitle);
				progDlg.SetShowProgressBar(true);
				progDlg.ShowModeless(CWnd::FromHandle(EXPLORERHWND));
				progDlg.SetAnimation(IDR_ANIMATION);
				CString dropper = droppath + strLine.Right(strLine.GetLength() - strLine.ReverseFind('\\'));
				if (!svn.Export(strLine, dropper, SVNRev::REV_WC, TRUE, &progDlg, parser.HasKey(_T("extended"))))
				{
					progDlg.Stop();
					CMessageBox::Show(EXPLORERHWND, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_OK | MB_ICONERROR);
				}
			}
			progDlg.Stop();
		}
		//#endregion
		//#region dropcopy
		if (comVal.Compare(_T("dropcopy"))==0)
		{
			CString path = parser.GetVal(_T("path"));
			CString sDroppath = parser.GetVal(_T("droptarget"));
			SVN svn;
			unsigned long totalcount = 0;
			unsigned long count = 0;
			CStdioFile file(path, CFile::typeBinary | CFile::modeRead);
			CString strLine = _T("");
			while (file.ReadString(strLine))
				totalcount++;
			file.SeekToBegin();
			CProgressDlg progress;
			if (progress.IsValid())
			{
				progress.SetTitle(IDS_PROC_COPYING);
				progress.SetAnimation(IDR_MOVEANI);
				progress.ShowModeless(CWnd::FromHandle(EXPLORERHWND));
			}
			while (file.ReadString(strLine))
			{
				CTSVNPath linePath(strLine);

				CTSVNPath fullDropPath(sDroppath);
				fullDropPath.AppendString(_T("\\")+linePath.GetFileOrDirectoryName());
				CString temp;
				temp.Format(IDS_PROC_COPYINGPROG, (LPCTSTR)strLine);
				CString temp2;
				temp2.Format(IDS_PROC_CPYMVPROG2, fullDropPath.GetWinPath());
				// Check for a drop-on-to-ourselves
				if (linePath.IsEquivalentTo(fullDropPath))
				{
					// Offer a rename
					progress.Stop();
					CRenameDlg dlg;
					dlg.m_windowtitle.Format(IDS_PROC_RENAME, (LPCTSTR)linePath.GetFileOrDirectoryName());
					if (dlg.DoModal() != IDOK)
					{
						return FALSE;
					}
					// Rebuild the destination path, with the new name
					fullDropPath.SetFromUnknown(sDroppath);
					fullDropPath.AppendString(_T("\\")+dlg.m_name);
				} 
				if (!svn.Copy(linePath, fullDropPath, SVNRev::REV_WC, _T("")))
				{
					TRACE(_T("%s\n"), (LPCTSTR)svn.GetLastErrorMessage());
					CMessageBox::Show(EXPLORERHWND, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					return FALSE;		//get out of here
				} // if (!svn.Move(strLine, droppath+_T("\\")+name, FALSE)) 
				count++;
				if (progress.IsValid())
				{
					progress.SetLine(1, temp, true);
					progress.SetLine(2, temp2, true);
					progress.SetProgress(count, totalcount);
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
		if (comVal.Compare(_T("conflicteditor"))==0)
		{
			CString theirs;
			CString mine;
			CString base;
			CString merge = CUtils::GetLongPathname(parser.GetVal(_T("path")));
			CString path = merge.Left(merge.ReverseFind('\\'));
			path = path + _T("\\");

			//we have the conflicted file (%merged)
			//now look for the other required files
			SVNStatus status;
			status.GetStatus(merge);
			if ((status.status)&&(status.status->entry))
			{
				if (status.status->entry->conflict_new)
				{
					theirs = CUnicodeUtils::GetUnicode(status.status->entry->conflict_new);
					theirs = path + theirs;
				}
				if (status.status->entry->conflict_old)
				{
					base = CUnicodeUtils::GetUnicode(status.status->entry->conflict_old);
					base = path + base;
				}
				if (status.status->entry->conflict_wrk)
				{
					mine = CUnicodeUtils::GetUnicode(status.status->entry->conflict_wrk);
					mine = path + mine;
				}
			}
			CUtils::StartExtMerge(base, theirs, mine, merge);
		} 
		//#endregion
		//#region relocate
		if (comVal.Compare(_T("relocate"))==0)
		{
			CString path = CUtils::GetLongPathname(parser.GetVal(_T("path")));
			SVN svn;
			CRelocateDlg dlg;
			dlg.m_sFromUrl = svn.GetURLFromPath(CTSVNPath(path));
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
					if (!s.Relocate(path, dlg.m_sFromUrl, dlg.m_sToUrl, TRUE))
					{
						if (progress.IsValid())
							progress.Stop();
						TRACE(_T("%s\n"), (LPCTSTR)s.GetLastErrorMessage());
						CMessageBox::Show((EXPLORERHWND), s.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					}
					else
					{
						if (progress.IsValid())
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
		if (comVal.Compare(_T("help"))==0)
		{
			ShellExecute(EXPLORERHWND, _T("open"), m_pszHelpFilePath, NULL, NULL, SW_SHOWNORMAL);
		}
		//#endregion
		//#region repostatus
		if (comVal.Compare(_T("repostatus"))==0)
		{
			CString path = CUtils::GetLongPathname(parser.GetVal(_T("path")));
			path = CUtils::WritePathsToTempFile(path);
			CChangedDlg dlg;
			dlg.m_path = path;
			dlg.DoModal();
			DeleteFile(path);
		} // if (comVal.Compare(_T("repostatus"))==0)
		//#endregion 
		//#region repobrowser
		if (comVal.Compare(_T("repobrowser"))==0)
		{
			CString path = CUtils::GetLongPathname(parser.GetVal(_T("path")));
			CString url;
			BOOL bFile = FALSE;
			SVN svn;
			url = svn.GetURLFromPath(CTSVNPath(path));
			if (path.Left(8).CompareNoCase(_T("file:///"))==0)
			{
				path = path.Mid(8);
			}
			bFile = !PathIsDirectory(path);

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
			} // if (dlg.m_strUrl.IsEmpty())

			CString val = parser.GetVal(_T("rev"));
			long rev_val = _tstol(val);
			SVNRev rev(SVNRev::REV_HEAD);
			if (rev_val != 0)
				rev = SVNRev(rev_val);
			CRepositoryBrowser dlg(SVNUrl(url, rev), bFile);
			dlg.m_ProjectProperties.ReadProps(path);
			dlg.DoModal();
		}
		//#endregion 
		//#region ignore
		if (comVal.Compare(_T("ignore"))==0)
		{
			CString path = CUtils::GetLongPathname(parser.GetVal(_T("path")));
			if (parser.HasKey(_T("notempfile")))
			{
				path = CUtils::WritePathsToTempFile(path);
			} // if (parser.HasKey(_T("notempfile"))) 
			SVN svn;
			try
			{
				CStdioFile file(path, CFile::typeBinary | CFile::modeRead);
				CString strLine = _T("");
				CString filelist;
				BOOL err = FALSE;
				while (file.ReadString(strLine))
				{
					//strLine = _T("F:\\Development\\DirSync\\DirSync.cpp");
					CString name = strLine.Right(strLine.GetLength() - strLine.ReverseFind('\\') - 1);
					name = name.Trim(_T("\n\r"));
					filelist += name + _T("\n");
					if (parser.HasKey(_T("onlymask")))
					{
						name = _T("*")+name.Mid(name.ReverseFind('.'));
					}
					CString parentfolder = strLine.Left(strLine.ReverseFind('\\'));
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
			catch (CFileException* pE)
			{
				TRACE(_T("CFileException in Resolve!\n"));
				pE->Delete();
			}
		}
		//#endregion
		//#region blame
		if (comVal.Compare(_T("blame"))==0)
		{
			CString path = CUtils::GetLongPathname(parser.GetVal(_T("path")));
			CBlameDlg dlg;
			if (dlg.DoModal() == IDOK)
			{
				CBlame blame;
				CString tempfile;
				CString logfile;
				tempfile = blame.BlameToTempFile(path, dlg.StartRev, dlg.EndRev, logfile, TRUE);
				if (!tempfile.IsEmpty())
				{
					if (logfile.IsEmpty())
					{
						//open the default text editor for the result file
						CUtils::StartTextViewer(tempfile);
					}
					else
					{
						TCHAR tblame[MAX_PATH];
						GetModuleFileName(NULL, tblame, MAX_PATH);
						CString viewer = tblame;
						viewer.Replace(_T("TortoiseProc.exe"), _T("TortoiseBlame.exe"));
						viewer += _T(" \"") + tempfile + _T("\"");
						viewer += _T(" \"") + logfile + _T("\"");
						viewer += _T(" \"") + CUtils::GetFileNameFromPath(path) + _T("\"");
						STARTUPINFO startup;
						PROCESS_INFORMATION process;
						memset(&startup, 0, sizeof(startup));
						startup.cb = sizeof(startup);
						memset(&process, 0, sizeof(process));

						if (CreateProcess(NULL, const_cast<TCHAR*>((LPCTSTR)viewer), NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0)
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
							CString temp;
							temp.Format(IDS_ERR_EXTDIFFSTART, lpMsgBuf);
							CMessageBox::Show(NULL, temp, _T("TortoiseSVN"), MB_OK | MB_ICONINFORMATION);
							LocalFree( lpMsgBuf );
							return FALSE;
						} 
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
		if (comVal.Compare(_T("cat"))==0)
		{
			CString path = CUtils::GetLongPathname(parser.GetVal(_T("path")));
			CString savepath = CUtils::GetLongPathname(parser.GetVal(_T("savepath")));
			CString revision = parser.GetVal(_T("revision"));
			LONG rev = _ttol(revision);
			if (rev==0)
				rev = SVNRev::REV_HEAD;
			SVN svn;
			if (!svn.Cat(CTSVNPath(path), rev, CTSVNPath(savepath)))
			{
				::MessageBox(NULL, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
				DeleteFile(savepath);
			} 
		}
		//#endregion
		//#region createpatch
		if (comVal.Compare(_T("createpatch"))==0)
		{
			CString path = CUtils::GetLongPathname(parser.GetVal(_T("path")));
			CreatePatch(path);
		}
		//#endregion
		//#region updatecheck
		if (comVal.Compare(_T("updatecheck"))==0)
		{
			CCheckForUpdatesDlg dlg;
			if (parser.HasKey(_T("visible")))
				dlg.m_bShowInfo = TRUE;
			dlg.DoModal();
		}
		//#endregion
		//#region revisiongraph
		if (comVal.Compare(_T("revisiongraph"))==0)
		{
			CString path = CUtils::GetLongPathname(parser.GetVal(_T("path")));
			CRevisionGraphDlg dlg;
			dlg.m_sPath = path;
			dlg.DoModal();
		} 
		//#endregion

		if (TSVNMutex)
			::CloseHandle(TSVNMutex);
	} 

	// Look for temporary files left around by TortoiseSVN and
	// remove them. But only delete 'old' files because the some
	// apps might still be needing the recent ones.
	{
		TCHAR path[MAX_PATH];
		DWORD len = ::GetTempPath (MAX_PATH, path);
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
				else
					::CloseHandle(hFile);
			}
		}			
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

BOOL CTortoiseProcApp::CreatePatch(CString path, CString savepath /*= _T("")*/)
{
	OPENFILENAME ofn;		// common dialog box structure
	CString temp;

	if (savepath.IsEmpty())
	{
		TCHAR szFile[MAX_PATH];  // buffer for file name
		ZeroMemory(szFile, sizeof(szFile));
		// Initialize OPENFILENAME
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		//ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;		//to stay compatible with NT4
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
		savepath = ofn.lpstrFile;
	}

	bool bToClipboard = _tcsstr(savepath, PATCH_TO_CLIPBOARD_PSEUDO_FILENAME) != NULL;

	CProgressDlg progDlg;
	temp.LoadString(IDS_PROC_SAVEPATCHTO);
	progDlg.SetLine(1, temp, true);
	CString strProgressDest(savepath);
	if(bToClipboard)
	{
		strProgressDest.LoadString(IDS_CLIPBOARD_PROGRESS_DEST);
	}
	progDlg.SetLine(2, strProgressDest, true);
	progDlg.SetTitle(IDS_PROC_PATCHTITLE);
	progDlg.SetShowProgressBar(false);
	progDlg.ShowModeless(CWnd::FromHandle(EXPLORERHWND));
	//progDlg.SetAnimation(IDR_ANIMATION);

	CString strTempPatchFile;
	if (bToClipboard)
		strTempPatchFile = CUtils::GetTempFile();
	else
		strTempPatchFile = savepath;

	path = path.TrimRight('\\');
	CString sDir;
	if (!PathIsDirectory(path))
	{
		SetCurrentDirectory(path.Left(path.ReverseFind('\\')));
		sDir = path.Mid(path.ReverseFind('\\')+1);
	}
	else
		SetCurrentDirectory(path);

	SVN svn;
	if (!svn.Diff(sDir, SVNRev::REV_BASE, sDir, SVNRev::REV_WC, TRUE, FALSE, FALSE, _T(""), strTempPatchFile))
	{
		progDlg.Stop();
		::MessageBox((EXPLORERHWND), svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		return FALSE;
	}

	if(bToClipboard)
	{
		// The user actually asked for the patch to be written to the clipboard

		CStringA sClipdata;

		FILE* inFile = _tfopen(strTempPatchFile, _T("rb"));
		if(inFile)
		{
			char chunkBuffer[16384];
			while(!feof(inFile))
			{
				size_t readLength = fread(chunkBuffer, 1, sizeof(chunkBuffer), inFile);

				sClipdata += CStringA(chunkBuffer, (int)readLength);
			}
			fclose(inFile);

			CUtils::WriteAsciiStringToClipboard(sClipdata);

		}
	}

	progDlg.Stop();
	if (bToClipboard)
		DeleteFile(strTempPatchFile);
	return TRUE;
}



