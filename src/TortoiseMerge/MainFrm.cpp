// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "TortoiseMerge.h"
#include "OpenDlg.h"
#include "Patch.h"
#include "ProgressDlg.h"
#include "Settings.h"

#include "MainFrm.h"
#include ".\mainfrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CNewFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CNewFrameWnd)
	ON_WM_CREATE()
	// Global help commands
	ON_COMMAND(ID_HELP_FINDER, CNewFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_HELP, CNewFrameWnd::OnHelp)
	ON_COMMAND(ID_CONTEXT_HELP, CNewFrameWnd::OnContextHelp)
	ON_COMMAND(ID_DEFAULT_HELP, CNewFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_VIEW_WHITESPACES, OnViewWhitespaces)
	ON_WM_SIZE()
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_AS, OnUpdateFileSaveAs)
	ON_COMMAND(ID_VIEW_ONEWAYDIFF, OnViewOnewaydiff)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ONEWAYDIFF, OnUpdateViewOnewaydiff)
	ON_UPDATE_COMMAND_UI(ID_VIEW_WHITESPACES, OnUpdateViewWhitespaces)
	ON_COMMAND(ID_VIEW_OPTIONS, OnViewOptions)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_LEFTVIEW,
	ID_INDICATOR_RIGHTVIEW,
	ID_INDICATOR_BOTTOMVIEW,
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL
};


// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	m_bInitSplitter = FALSE;
	m_bOneWay = (0 != ((DWORD)CRegDWORD(_T("Software\\TortoiseMerge\\OnePane"))));
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CNewFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT | TBSTYLE_WRAPABLE| CBRS_SIZE_DYNAMIC) ||
		 !m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}
	m_wndToolBar.SetWindowText(_T("Main"));

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	} 

	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY);

	if (!m_wndLocatorBar.Create(this, IDD_DIFFLOCATOR, 
		CBRS_ALIGN_LEFT, AFX_IDW_DIALOGBAR))
	{
		TRACE0("Failed to create dialogbar\n");
		return -1;		// fail to create
	}
	//if (!m_wndReBar.Create(this) ||
	//	!m_wndReBar.AddBar(&m_wndToolBar))
	//{
	//	TRACE0("Failed to create rebar\n");
	//	return -1;      // fail to create
	//}
	m_wndLocatorBar.m_pMainFrm = this;
	m_DefaultNewMenu.LoadToolBar(IDR_MAINFRAME);
	m_DefaultNewMenu.SetXpBlendig();
	m_DefaultNewMenu.SetSelectDisableMode(FALSE);
	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	CNewMenu::SetMenuDrawMode(CNewMenu::STYLE_XP);
	if( !CNewFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	return TRUE;
}


// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CNewFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CNewFrameWnd::Dump(dc);
}

#endif //_DEBUG


// CMainFrame message handlers


BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
	CRect cr; 
	GetClientRect( &cr);


	// split into three panes:
	//        -------------
	//        |     |     |
	//        |     |     |
	//        |------------
	//        |           |
	//        |           |
	//        |------------

	// create a splitter with 2 rows, 1 column 
	if (!m_wndSplitter.CreateStatic(this, 2, 1))
	{ 
		TRACE0("Failed to CreateStaticSplitter\n"); 
		return FALSE; 
	}

	// add the second splitter pane - which is a nested splitter with 2 columns 
	if (!m_wndSplitter2.CreateStatic( 
		&m_wndSplitter, // our parent window is the first splitter 
		1, 2, // the new splitter is 1 row, 2 columns
		WS_CHILD | WS_VISIBLE | WS_BORDER, // style, WS_BORDER is needed 
		m_wndSplitter.IdFromRowCol(0, 0) 
		// new splitter is in the first column, 2nd column of first splitter 
		)) 
	{ 
		TRACE0("Failed to create nested splitter\n"); 
		return FALSE; 
	}
	// add the first splitter pane - the default view in row 0 
	if (!m_wndSplitter.CreateView(1, 0, 
		RUNTIME_CLASS(CBottomView), CSize(cr.Width(), cr.Height()), pContext)) 
	{ 
		TRACE0("Failed to create first pane\n"); 
		return FALSE; 
	}
	m_pwndBottomView = (CBottomView *)m_wndSplitter.GetPane(1,0);
	m_pwndBottomView->m_pwndLocator = &m_wndLocatorBar;
	m_pwndBottomView->m_pwndStatusBar = &m_wndStatusBar;
	m_pwndBottomView->m_pMainFrame = this;

	// now create the two views inside the nested splitter 

	if (!m_wndSplitter2.CreateView(0, 0, 
		RUNTIME_CLASS(CLeftView), CSize(cr.Width()/2, cr.Height()/2), pContext)) 
	{ 
		TRACE0("Failed to create second pane\n"); 
		return FALSE; 
	}
	m_pwndLeftView = (CLeftView *)m_wndSplitter2.GetPane(0,0);
	m_pwndLeftView->m_pwndLocator = &m_wndLocatorBar;
	m_pwndLeftView->m_pwndStatusBar = &m_wndStatusBar;
	m_pwndLeftView->m_pMainFrame = this;

	if (!m_wndSplitter2.CreateView(0, 1, 
		RUNTIME_CLASS(CRightView), CSize(cr.Width()/2, cr.Height()/2), pContext)) 
	{ 
		TRACE0("Failed to create third pane\n"); 
		return FALSE; 
	}
	m_pwndRightView = (CRightView *)m_wndSplitter2.GetPane(0,1);
	m_pwndRightView->m_pwndLocator = &m_wndLocatorBar;
	m_pwndRightView->m_pwndStatusBar = &m_wndStatusBar;
	m_pwndRightView->m_pMainFrame = this;
	m_bInitSplitter = TRUE;

	m_dlgFilePatches.Create(IDD_FILEPATCHES, this);
	return TRUE;
}

BOOL CMainFrame::PatchFile(CString sFilePath, CString sVersion)
{
	//first, do a "dry run" of patching...
	if (!m_Patch.PatchFile(sFilePath))
	{
		//patching not successful, so retreive the
		//base file from version control and try
		//again...
		CString sTemp;
		CProgressDlg progDlg;
		sTemp.Format(IDS_GETVERSIONOFFILE, sVersion);
		progDlg.SetLine(1, sTemp, true);
		progDlg.SetLine(2, sFilePath, true);
		sTemp.LoadString(IDS_GETVERSIONOFFILETITLE);
		progDlg.SetTitle(sTemp);
		progDlg.SetShowProgressBar(false);
		progDlg.SetAnimation(IDR_DOWNLOAD);
		progDlg.SetTime(FALSE);
		progDlg.ShowModeless(this);
		CString sBaseFile = m_TempFiles.GetTempFilePath();
		if (!CUtils::GetVersionedFile(sFilePath, sVersion, sBaseFile, &progDlg, this->m_hWnd))
		{
			progDlg.Stop();
			CString sErrMsg;
			sErrMsg.Format(IDS_ERR_MAINFRAME_FILEVERSIONNOTFOUND, sVersion, sFilePath);
			MessageBox(sErrMsg, NULL, MB_ICONERROR);
			return FALSE;
		} // if (!CUtils::GetVersionedFile(sFilePath, sVersion, sBaseFile, this->m_hWnd)) 
		progDlg.Stop();
		CString sTempFile = m_TempFiles.GetTempFilePath();
		if (!m_Patch.PatchFile(sFilePath, sTempFile, sBaseFile))
		{
			MessageBox(m_Patch.GetErrorMessage(), NULL, MB_ICONERROR);
			return FALSE;
		} // if (!m_Patch.PatchFile(sFilePath, sTempFile, sBaseFile)) 
		this->m_Data.m_sBaseFile = sBaseFile;
		this->m_Data.m_sTheirFile = sTempFile;
		this->m_Data.m_sYourFile = sFilePath;
		TRACE(_T("comparing %s and %s\nagainst the base file %s\n"), sTempFile, sFilePath, sBaseFile);
	} // if (!m_Patch.PatchFile(sFilePath)) 
	else
	{
		//"dry run" was successful, so save the patched file somewhere...
		CString sTempFile = m_TempFiles.GetTempFilePath();
		if (!m_Patch.PatchFile(sFilePath, sTempFile))
		{
			MessageBox(m_Patch.GetErrorMessage(), NULL, MB_ICONERROR);
			return FALSE;
		}
		this->m_Data.m_sBaseFile = sFilePath;
		this->m_Data.m_sTheirFile = sTempFile;
		this->m_Data.m_sYourFile.Empty();
		TRACE(_T("comparing %s\nwith the patched result %s\n"), sFilePath, sTempFile);
	}
	LoadViews();
	return TRUE;
}

void CMainFrame::OnFileOpen()
{
	COpenDlg dlg;
	if (dlg.DoModal()!=IDOK)
	{
		return;
	}
	m_dlgFilePatches.ShowWindow(SW_HIDE);
	TRACE(_T("got the files:\n   %s\n   %s\n   %s\n   %s\n   %s\n"), dlg.m_sBaseFile, dlg.m_sTheirFile, dlg.m_sYourFile, 
		dlg.m_sUnifiedDiffFile, dlg.m_sPatchDirectory);
	this->m_Data.m_sBaseFile = dlg.m_sBaseFile;
	this->m_Data.m_sTheirFile = dlg.m_sTheirFile;
	this->m_Data.m_sYourFile = dlg.m_sYourFile;
	this->m_Data.m_sDiffFile = dlg.m_sUnifiedDiffFile;
	this->m_Data.m_sPatchPath = dlg.m_sPatchDirectory;
	g_crasher.AddFile((LPCSTR)(LPCTSTR)m_Data.m_sBaseFile, (LPCSTR)(LPCTSTR)_T("Basefile"));
	g_crasher.AddFile((LPCSTR)(LPCTSTR)m_Data.m_sTheirFile, (LPCSTR)(LPCTSTR)_T("Theirfile"));
	g_crasher.AddFile((LPCSTR)(LPCTSTR)m_Data.m_sYourFile, (LPCSTR)(LPCTSTR)_T("Yourfile"));
	g_crasher.AddFile((LPCSTR)(LPCTSTR)m_Data.m_sDiffFile, (LPCSTR)(LPCTSTR)_T("Difffile"));
	
	if (m_Data.m_sBaseFile.IsEmpty() && !m_Data.m_sTheirFile.IsEmpty() && !m_Data.m_sYourFile.IsEmpty())
	{
		// a diff between two files means "Yours" against "Base", not "Theirs" against "Yours"
		m_Data.m_sBaseFile = m_Data.m_sTheirFile;
		m_Data.m_sTheirFile.Empty();
	} // if (m_Data.m_sBaseFile.IsEmpty() && !m_Data.m_sTheirFile.IsEmpty() && !m_Data.m_sYourFile.IsEmpty()) 
	if (!m_Data.m_sBaseFile.IsEmpty() && !m_Data.m_sTheirFile.IsEmpty() && m_Data.m_sYourFile.IsEmpty())
	{
		// a diff between two files means "Yours" against "Base", not "Theirs" against "Base"
		m_Data.m_sYourFile = m_Data.m_sTheirFile;
		m_Data.m_sTheirFile.Empty();
	} // if (m_Data.m_sBaseFile.IsEmpty() && !m_Data.m_sTheirFile.IsEmpty() && !m_Data.m_sYourFile.IsEmpty()) 

	LoadViews();
}

BOOL CMainFrame::LoadViews()
{
	if (!this->m_Data.Load())
	{
		::MessageBox(NULL, m_Data.GetError(), _T("TortoiseMerge"), MB_ICONERROR);
		return FALSE;
	} // if (!this->m_Data.Load())
	BOOL bGoFirstDiff = (0 != ((DWORD)CRegDWORD(_T("Software\\TortoiseMerge\\FirstDiffOnLoad"))));
	if (m_Data.m_sBaseFile.IsEmpty())
	{
		if (!m_Data.m_sYourFile.IsEmpty() && !m_Data.m_sTheirFile.IsEmpty())
		{
			m_Data.m_sBaseFile = m_Data.m_sTheirFile;
			m_Data.m_sTheirFile.Empty();
		} // if (!m_Data.m_sYourFile.IsEmpty() && !m_Data.m_sTheirFile.IsEmpty()) 
		else if ((!m_Data.m_sDiffFile.IsEmpty())&&(!m_Patch.OpenUnifiedDiffFile(m_Data.m_sDiffFile)))
		{
			MessageBox(m_Patch.GetErrorMessage(), NULL, MB_ICONERROR);
			return FALSE;
		} // if (!m_Patch.OpenUnifiedDiffFile(m_Data.m_sDiffFile)) 
		if (m_Patch.GetNumberOfFiles() > 0)
		{
			m_dlgFilePatches.Init(&m_Patch, this, m_Data.m_sPatchPath);
			m_dlgFilePatches.ShowWindow(SW_SHOW);
			m_pwndLeftView->m_sWindowName = _T("");
			m_pwndRightView->m_sWindowName = _T("");
			m_pwndBottomView->m_sWindowName = _T("");
			m_pwndLeftView->DocumentUpdated();
			m_pwndRightView->DocumentUpdated();
			m_pwndBottomView->DocumentUpdated();
			m_wndLocatorBar.DocumentUpdated();
			if (!m_wndSplitter.IsRowHidden(1))
				m_wndSplitter.HideRow(1);
			UpdateLayout();
			if (bGoFirstDiff)
				m_pwndLeftView->GoToFirstDifference();
		} // if (m_Patch.GetNumberOfFiles() > 0) 
	} // if (m_Data.m_sBaseFile.IsEmpty()) 
	if (!m_Data.m_sBaseFile.IsEmpty() && m_Data.m_sYourFile.IsEmpty() && !m_Data.m_sTheirFile.IsEmpty())
	{
		m_Data.m_sYourFile = m_Data.m_sTheirFile;
		m_Data.m_sTheirFile.Empty();
	}
	if (!m_Data.m_sBaseFile.IsEmpty() && !m_Data.m_sYourFile.IsEmpty() && m_Data.m_sTheirFile.IsEmpty())
	{
		//diff between YOUR and BASE
		if (m_bOneWay)
		{
			if (!m_wndSplitter2.IsColumnHidden(1))
				m_wndSplitter2.HideColumn(1);
			m_pwndLeftView->m_arDiffLines = &m_Data.m_arDiffYourBaseBoth;
			m_pwndLeftView->m_arLineStates = &m_Data.m_arStateYourBaseBoth;
			m_pwndLeftView->m_sWindowName = (m_Data.m_sYourFile.Mid(m_Data.m_sYourFile.ReverseFind('\\')+1))+ _T(" - ") +
											(m_Data.m_sBaseFile.Mid(m_Data.m_sBaseFile.ReverseFind('\\')+1));
			m_pwndRightView->m_arDiffLines = &m_Data.m_arDiffYourBaseRight;
			m_pwndRightView->m_arLineStates = &m_Data.m_arStateYourBaseRight;
			m_pwndRightView->m_sWindowName = m_Data.m_sYourFile.Mid(m_Data.m_sYourFile.ReverseFind('\\')+1);
			m_pwndLeftView->DocumentUpdated();
			m_pwndRightView->DocumentUpdated();
			m_pwndBottomView->DocumentUpdated();
			m_wndLocatorBar.DocumentUpdated();
			if (!m_wndSplitter.IsRowHidden(1))
				m_wndSplitter.HideRow(1);
			if (bGoFirstDiff)
				m_pwndLeftView->GoToFirstDifference();
		} // if (m_bOneWay)
		else
		{
			if (m_wndSplitter2.IsColumnHidden(1))
				m_wndSplitter2.ShowColumn();
			m_pwndLeftView->m_arDiffLines = &m_Data.m_arDiffYourBaseLeft;
			m_pwndLeftView->m_arLineStates = &m_Data.m_arStateYourBaseLeft;
			m_pwndLeftView->m_sWindowName = m_Data.m_sBaseFile.Mid(m_Data.m_sBaseFile.ReverseFind('\\')+1);
			m_pwndRightView->m_arDiffLines = &m_Data.m_arDiffYourBaseRight;
			m_pwndRightView->m_arLineStates = &m_Data.m_arStateYourBaseRight;
			m_pwndRightView->m_sWindowName = m_Data.m_sYourFile.Mid(m_Data.m_sYourFile.ReverseFind('\\')+1);
			m_pwndLeftView->DocumentUpdated();
			m_pwndRightView->DocumentUpdated();
			m_pwndBottomView->DocumentUpdated();
			m_wndLocatorBar.DocumentUpdated();
			if (!m_wndSplitter.IsRowHidden(1))
				m_wndSplitter.HideRow(1);
			if (bGoFirstDiff)
				m_pwndLeftView->GoToFirstDifference();
		}
		UpdateLayout();
	} // if (!m_Data.m_sBaseFile.IsEmpty() && !m_Data.m_sYourFile.IsEmpty() && m_Data.m_sTheirFile.IsEmpty())
	else if (!m_Data.m_sBaseFile.IsEmpty() && !m_Data.m_sYourFile.IsEmpty() && !m_Data.m_sTheirFile.IsEmpty())
	{
		//diff between THEIR, YOUR and BASE
		m_pwndLeftView->m_arDiffLines = &m_Data.m_arDiffTheirBaseBoth;
		m_pwndLeftView->m_arLineStates = &m_Data.m_arStateTheirBaseBoth;
		m_pwndLeftView->m_sWindowName = _T("Theirs - ")+(m_Data.m_sTheirFile.Mid(m_Data.m_sTheirFile.ReverseFind('\\')+1));
		m_pwndRightView->m_arDiffLines = &m_Data.m_arDiffYourBaseBoth;
		m_pwndRightView->m_arLineStates = &m_Data.m_arStateYourBaseBoth;
		m_pwndRightView->m_sWindowName = _T("Yours - ")+(m_Data.m_sYourFile.Mid(m_Data.m_sYourFile.ReverseFind('\\')+1));
		m_pwndBottomView->m_arDiffLines = &m_Data.m_arDiff3;
		m_pwndBottomView->m_arLineStates = &m_Data.m_arStateDiff3;
		m_pwndBottomView->m_sWindowName = _T("Merged - ")+(m_Data.m_sMergedFile.Mid(m_Data.m_sMergedFile.ReverseFind('\\')+1));
		m_pwndLeftView->DocumentUpdated();
		m_pwndRightView->DocumentUpdated();
		m_pwndBottomView->DocumentUpdated();
		m_wndLocatorBar.DocumentUpdated();
		if (m_wndSplitter.IsRowHidden(1))
			m_wndSplitter.ShowRow();
		if (bGoFirstDiff)
			m_pwndLeftView->GoToFirstDifference();
		UpdateLayout();
	}
	return TRUE;
}

void CMainFrame::UpdateLayout()
{
	if (m_bInitSplitter)
	{
		CRect cr;
		GetWindowRect(&cr);
		m_wndSplitter.SetRowInfo(0, cr.Height()/2, 0);
		m_wndSplitter.SetRowInfo(1, cr.Height()/2, 0);
		m_wndSplitter.SetColumnInfo(0, cr.Width() / 2, 50);
		m_wndSplitter2.SetRowInfo(0, cr.Height()/2, 0);
		m_wndSplitter2.SetColumnInfo(0, cr.Width() / 2, 50);
		m_wndSplitter2.SetColumnInfo(1, cr.Width() / 2, 50);

		m_wndSplitter.RecalcLayout();
	} // if (m_bInitSplitter) 
}

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
    CNewFrameWnd::OnSize(nType, cx, cy);
    CRect cr;
    GetWindowRect(&cr);

    if (m_bInitSplitter && nType != SIZE_MINIMIZED)
    {
        m_wndSplitter.SetRowInfo(0, cy/2, 0);
        m_wndSplitter.SetRowInfo(1, cy/2, 0);
        m_wndSplitter.SetColumnInfo(0, cr.Width() / 2, 50);
        m_wndSplitter2.SetRowInfo(0, cy/2, 0);
        m_wndSplitter2.SetColumnInfo(0, cr.Width() / 2, 50);
        m_wndSplitter2.SetColumnInfo(1, cr.Width() / 2, 50);

        m_wndSplitter.RecalcLayout();
    } // if (m_bInitSplitter && nType != SIZE_MINIMIZED) 
}

void CMainFrame::OnViewWhitespaces()
{
	BOOL bViewWhitespaces = FALSE;
	if (m_pwndLeftView)
		bViewWhitespaces = m_pwndLeftView->m_bViewWhitespace;

	bViewWhitespaces = !bViewWhitespaces;
	if (m_pwndLeftView)
	{
		m_pwndLeftView->m_bViewWhitespace = bViewWhitespaces;
		m_pwndLeftView->Invalidate();
	} // if (m_pwndLeftView)
	if (m_pwndRightView)
	{
		m_pwndRightView->m_bViewWhitespace = bViewWhitespaces;
		m_pwndRightView->Invalidate();
	} // if (m_pwndRightView)
	if (m_pwndBottomView)
	{
		m_pwndBottomView->m_bViewWhitespace = bViewWhitespaces;
		m_pwndBottomView->Invalidate();
	} // if (m_pwndBottomView) 
}

void CMainFrame::OnUpdateViewWhitespaces(CCmdUI *pCmdUI)
{
	if (m_pwndLeftView)
		pCmdUI->SetCheck(m_pwndLeftView->m_bViewWhitespace);
}

void CMainFrame::OnViewOnewaydiff()
{
	m_bOneWay = !m_bOneWay;
	LoadViews();
}

BOOL CMainFrame::CheckResolved()
{
	//only in three way diffs can be conflicts!
	if (this->m_pwndBottomView->IsWindowVisible())
	{
		if (this->m_pwndBottomView->m_arLineStates)
		{
			for (int i=0; i<this->m_pwndBottomView->m_arLineStates->GetCount(); i++)
			{
				if (CDiffData::DIFFSTATE_CONFLICTED == (CDiffData::DiffStates)this->m_pwndBottomView->m_arLineStates->GetAt(i))
					return FALSE;
			} // for (int i=0; i<this->m_pwndBottomView->m_arLineStates->GetCount(); i++) 
		} // if (this->m_pwndBottomView->m_arLineStates) 
	} // if (this->m_pwndBottomView->IsWindowVisible())
	return TRUE;
}

void CMainFrame::SaveFile(CString sFilePath)
{
	CStringArray * arText = NULL;
	CDWordArray * arStates = NULL;
	CFileTextLines * pOriginFile = &m_Data.m_arBaseFile;
	if (m_pwndBottomView)
	{
		if (m_pwndBottomView->IsWindowVisible())
		{
			arText = m_pwndBottomView->m_arDiffLines;
			arStates = m_pwndBottomView->m_arLineStates;
		} // if (m_pwndBottomView->IsWindowVisible()) 
	} // if (m_pwndBottomView) 
	else if (m_pwndRightView)
	{
		if (m_pwndRightView->IsWindowVisible())
		{
			arText = m_pwndRightView->m_arDiffLines;
			arStates = m_pwndRightView->m_arLineStates;
		} // if (m_pwndRightView->IsWindowVisible()) 
	} 
	else
	{
		// nothing to save!
		return;
	}
	if ((arText)&&(arStates)&&(pOriginFile))
	{
		CFileTextLines file;
		pOriginFile->CopySettings(&file);
		for (int i=0; i<arText->GetCount(); i++)
		{
			//only copy non-removed lines
			CDiffData::DiffStates state = (CDiffData::DiffStates)arStates->GetAt(i);
			switch (state)
			{
			case CDiffData::DIFFSTATE_ADDED:
			case CDiffData::DIFFSTATE_CONFLICTADDED:
			case CDiffData::DIFFSTATE_CONFLICTED:
			case CDiffData::DIFFSTATE_CONFLICTEMPTY:
			case CDiffData::DIFFSTATE_EMPTY:
			case CDiffData::DIFFSTATE_IDENTICAL:
			case CDiffData::DIFFSTATE_IDENTICALADDED:
			case CDiffData::DIFFSTATE_NORMAL:
			case CDiffData::DIFFSTATE_THEIRSADDED:
			case CDiffData::DIFFSTATE_UNKNOWN:
			case CDiffData::DIFFSTATE_YOURSADDED:
				file.Add(arText->GetAt(i));
				break;
			case CDiffData::DIFFSTATE_IDENTICALREMOVED:
			case CDiffData::DIFFSTATE_REMOVED:
			case CDiffData::DIFFSTATE_THEIRSREMOVED:
			case CDiffData::DIFFSTATE_YOURSREMOVED:
				break;
			default:
				break;
			} // switch (state) 
		} // for (int i=0; i<arText->GetCount(); i++) 
		file.Save(sFilePath);
	} // if ((arText)&&(arStates)&&(pOriginFile)) 
}

void CMainFrame::OnFileSave()
{
	if (this->m_Data.m_sMergedFile.IsEmpty())
		return OnFileSaveAs();
	if (CheckResolved())
	{
		if (((DWORD)CRegDWORD(_T("Software\\TortoiseMerge\\Backup"))) != 0)
		{
			DeleteFile(m_Data.m_sMergedFile + _T(".bak"));
			MoveFile(m_Data.m_sMergedFile, m_Data.m_sMergedFile + _T(".bak"));
		}
		SaveFile(this->m_Data.m_sMergedFile);
	}
	else
	{
		CString sTemp;
		sTemp.LoadString(IDS_ERR_MAINFRAME_FILEHASCONFLICTS);
		MessageBox(sTemp, 0, MB_ICONERROR);
	}
}

void CMainFrame::OnFileSaveAs()
{
	if (!CheckResolved())
	{
		CString sTemp;
		sTemp.LoadString(IDS_ERR_MAINFRAME_FILEHASCONFLICTS);
		MessageBox(sTemp, 0, MB_ICONERROR);
		return;
	}
	OPENFILENAME ofn;		// common dialog box structure
	TCHAR szFile[MAX_PATH];  // buffer for file name
	ZeroMemory(szFile, sizeof(szFile));
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	//ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;		//to stay compatible with NT4
	ofn.hwndOwner = this->m_hWnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
	CString temp;
	temp.LoadString(IDS_SAVEASTITLE);
	if (!temp.IsEmpty())
		ofn.lpstrTitle = temp;
	ofn.Flags = OFN_OVERWRITEPROMPT;

	// Display the Open dialog box. 
	CString sFile;
	if (GetSaveFileName(&ofn)==TRUE)
	{
		sFile = CString(ofn.lpstrFile);
		SaveFile(sFile);
	} // if (GetSaveFileName(&ofn)==TRUE)
}

void CMainFrame::OnUpdateFileSave(CCmdUI *pCmdUI)
{
	BOOL bEnable = FALSE;
	if (!this->m_Data.m_sMergedFile.IsEmpty())
	{
		if (m_pwndBottomView)
		{
			if (m_pwndBottomView->IsWindowVisible())
			{
				bEnable = TRUE;
			} // if (m_pwndBottomView->IsWindowVisible()) 
		} // if (m_pwndBottomView) 
	} // if (!this->m_Data.m_sMergedFile.IsEmpty())
	pCmdUI->Enable(bEnable);
}

void CMainFrame::OnUpdateFileSaveAs(CCmdUI *pCmdUI)
{
	BOOL bEnable = FALSE;
	if (m_pwndBottomView)
	{
		if ((m_pwndBottomView->IsWindowVisible())&&(m_pwndBottomView->m_arDiffLines))
		{
			bEnable = TRUE;
		} // if (m_pwndBottomView->IsWindowVisible()) 
	} // if (m_pwndBottomView) 
	else if (m_pwndRightView)
	{
		if ((m_pwndRightView->IsWindowVisible())&&(m_pwndRightView->m_arDiffLines))
		{
			bEnable = TRUE;
		} // if (m_pwndRightView->IsWindowVisible()) 
	} 
	pCmdUI->Enable(bEnable);
}


void CMainFrame::OnUpdateViewOnewaydiff(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bOneWay);
	BOOL bEnable = TRUE;
	if (m_pwndBottomView)
	{
		if (m_pwndBottomView->IsWindowVisible())
			bEnable = FALSE;
	} // if (m_pwndBottomView)
	pCmdUI->Enable(bEnable);
}

void CMainFrame::OnViewOptions()
{
	CString sTemp;
	sTemp.LoadString(IDS_SETTINGSTITLE);
	CSettings dlg(sTemp);
	dlg.DoModal();
	if (((m_pwndBottomView)&&(m_pwndBottomView->IsModified())) ||
		((m_pwndRightView)&&(m_pwndRightView->IsModified())))
	{
		CString sTemp;
		sTemp.LoadString(IDS_WARNMODIFIEDLOOSECHANGES);
		if (MessageBox(sTemp, 0, MB_YESNO | MB_ICONQUESTION)==IDNO)
		{
			return;
		}
	} // ified())))
	m_Data.LoadRegistry();
	LoadViews();
}

