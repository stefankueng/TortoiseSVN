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
#include "MergeDlg.h"
#include "RepositoryBrowser.h"
#include "Balloon.h"
#include "BrowseFolder.h"
#include "MessageBox.h"
#include "registry.h"
#include "Utils.h"
#include ".\mergedlg.h"

IMPLEMENT_DYNAMIC(CMergeDlg, CStandAloneDialog)
CMergeDlg::CMergeDlg(CWnd* pParent /*=NULL*/)
	: CStandAloneDialog(CMergeDlg::IDD, pParent)
	, m_URLFrom(_T(""))
	, m_URLTo(_T(""))
	, StartRev(0)
	, EndRev(_T("HEAD"))
	, m_bUseFromURL(TRUE)
	, m_bDryRun(FALSE)
{
	m_pLogDlg = NULL;
	m_pLogDlg2 = NULL;
	bRepeating = FALSE;
}

CMergeDlg::~CMergeDlg()
{
	if (m_pLogDlg)
		delete m_pLogDlg;
	if (m_pLogDlg2)
		delete m_pLogDlg2;
}

void CMergeDlg::DoDataExchange(CDataExchange* pDX)
{
	CStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_URLCOMBO, m_URLCombo);
	DDX_Text(pDX, IDC_REVISION_START, m_sStartRev);
	DDX_Text(pDX, IDC_REVISION_END, m_sEndRev);
	DDX_Control(pDX, IDC_URLCOMBO2, m_URLCombo2);
	DDX_Check(pDX, IDC_USEFROMURL, m_bUseFromURL);
}


BEGIN_MESSAGE_MAP(CMergeDlg, CStandAloneDialog)
	ON_REGISTERED_MESSAGE(WM_REVSELECTED, OnRevSelected)
	ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
	ON_BN_CLICKED(IDC_BROWSE2, OnBnClickedBrowse2)
	ON_BN_CLICKED(IDC_REVISION_HEAD, OnBnClickedRevisionHead)
	ON_BN_CLICKED(IDC_REVISION_HEAD1, OnBnClickedRevisionHead1)
	ON_BN_CLICKED(IDC_REVISION_N, OnBnClickedRevisionN)
	ON_BN_CLICKED(IDC_REVISION_N1, OnBnClickedRevisionN1)
	ON_BN_CLICKED(IDC_FINDBRANCHSTART, OnBnClickedFindbranchstart)
	ON_BN_CLICKED(IDC_FINDBRANCHEND, OnBnClickedFindbranchend)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_BN_CLICKED(IDC_USEFROMURL, OnBnClickedUsefromurl)
	ON_BN_CLICKED(IDC_WCLOG, OnBnClickedWCLog)
	ON_BN_CLICKED(IDC_DRYRUNBUTTON, OnBnClickedDryrunbutton)
	ON_BN_CLICKED(IDC_DIFFBUTTON, OnBnClickedDiffbutton)
	ON_CBN_EDITCHANGE(IDC_URLCOMBO, OnCbnEditchangeUrlcombo)
END_MESSAGE_MAP()


// CMergeDlg message handlers

BOOL CMergeDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();

	m_bFile = !PathIsDirectory(m_URLFrom);
	SVN svn;
	CString url = svn.GetURLFromPath(CTSVNPath(m_wcPath));
	CString sUUID = svn.GetUUIDFromPath(CTSVNPath(m_wcPath));
	if (url.IsEmpty())
	{
		CString temp;
		temp.Format(IDS_ERR_NOURLOFFILE, m_wcPath.GetWinPath());
		CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
		this->EndDialog(IDCANCEL);
		return TRUE;
	}
	else
	{
		if (!bRepeating)
		{
			m_URLFrom = url;
			m_URLTo = url;
		}
		GetDlgItem(IDC_WCURL)->SetWindowText(url);
		GetDlgItem(IDC_WCPATH)->SetWindowText(m_wcPath.GetWinPath());
	}

	m_URLCombo.SetURLHistory(TRUE);
	m_URLCombo.LoadHistory(_T("Software\\TortoiseSVN\\History\\repoURLS\\")+sUUID, _T("url"));
	if (!(DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\MergeWCURL"), FALSE))
		m_URLCombo.SetCurSel(0);
	if ((m_URLCombo.GetString().IsEmpty())||bRepeating)
		m_URLCombo.SetWindowText(m_URLFrom);
	m_URLCombo2.SetURLHistory(TRUE);
	m_URLCombo2.LoadHistory(_T("Software\\TortoiseSVN\\History\\repoURLS\\")+sUUID, _T("url"));
	m_URLCombo2.SetCurSel(0);
	if ((m_URLCombo2.GetString().IsEmpty())||bRepeating)
		m_URLCombo2.SetWindowText(m_URLTo);

	if (bRepeating)
	{
		if (StartRev.IsHead())
			CheckRadioButton(IDC_REVISION_HEAD1, IDC_REVISION_N1, IDC_REVISION_HEAD1);
		else
			CheckRadioButton(IDC_REVISION_HEAD1, IDC_REVISION_N1, IDC_REVISION_N1);
		if (EndRev.IsHead())
			CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
		else
			CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
		if (m_bUseFromURL)
			GetDlgItem(IDC_URLCOMBO2)->EnableWindow(FALSE);
		else
		{
			GetDlgItem(IDC_URLCOMBO2)->EnableWindow(TRUE);
			GetDlgItem(IDC_URLCOMBO2)->SetWindowText(m_URLTo);
		}
	}
	else
	{
		GetDlgItem(IDC_URLCOMBO2)->EnableWindow(FALSE);
		// set head revision as default revision
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
		CheckRadioButton(IDC_REVISION_HEAD1, IDC_REVISION_N1, IDC_REVISION_N1);
	}
	OnBnClickedUsefromurl();
	if ((m_pParentWnd==NULL)&&(hWndExplorer))
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CMergeDlg::CheckData()
{
	if (!UpdateData(TRUE))
		return FALSE;

	StartRev = SVNRev(m_sStartRev);
	EndRev = SVNRev(m_sEndRev);
	// if head revision, set revision as -1
	if (GetCheckedRadioButton(IDC_REVISION_HEAD1, IDC_REVISION_N1) == IDC_REVISION_HEAD1)
	{
		StartRev = SVNRev(_T("HEAD"));
	}
	if (!StartRev.IsValid())
	{
		CBalloon::ShowBalloon(this, CBalloon::GetCtrlCentre(this,IDC_REVISION_START), IDS_ERR_INVALIDREV, TRUE, IDI_EXCLAMATION);
		return FALSE;
	}

	// if head revision, set revision as -1
	if (GetCheckedRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N) == IDC_REVISION_HEAD)
	{
		EndRev = SVNRev(_T("HEAD"));
	}
	if (!EndRev.IsValid())
	{
		CBalloon::ShowBalloon(this, CBalloon::GetCtrlCentre(this,IDC_REVISION_END), IDS_ERR_INVALIDREV, TRUE, IDI_EXCLAMATION);
		return FALSE;
	}

	m_URLCombo.SaveHistory();
	m_URLFrom = m_URLCombo.GetString();
	if (!m_bUseFromURL)
	{
		m_URLCombo2.SaveHistory();
		m_URLTo = m_URLCombo2.GetString();
	}
	else
		m_URLTo = m_URLFrom;

	UpdateData(FALSE);
	return TRUE;
}

void CMergeDlg::OnOK()
{
	m_bDryRun = FALSE;
	delete m_pLogDlg;
	m_pLogDlg = NULL;
	delete m_pLogDlg2;
	m_pLogDlg2 = NULL;
	if (CheckData())
		CStandAloneDialog::OnOK();
	else
		return;
}

void CMergeDlg::OnBnClickedDryrunbutton()
{
	m_bDryRun = TRUE;
	delete m_pLogDlg;
	m_pLogDlg = NULL;
	delete m_pLogDlg2;
	m_pLogDlg2 = NULL;
	if (CheckData())
		CStandAloneDialog::OnOK();
	else
		return;
}

void CMergeDlg::OnBnClickedDiffbutton()
{
	if (!CheckData())
		return;
	AfxGetApp()->DoWaitCursor(1);
	// create a unified diff of the merge
	SVN svn;
	CTSVNPath tempfile = CUtils::GetTempFilePath(CTSVNPath(_T("test.diff")));
	if (m_bUseFromURL)
	{
		if (!svn.PegDiff(CTSVNPath(m_URLFrom), StartRev, StartRev, EndRev, TRUE, FALSE, FALSE, FALSE, CString(), tempfile))
		{
			CMessageBox::Show(m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			AfxGetApp()->DoWaitCursor(-1);
			return;
		}
	}
	else
	{
		if (!svn.Diff(CTSVNPath(m_URLFrom), StartRev, CTSVNPath(m_URLTo), EndRev, TRUE, FALSE, FALSE, FALSE, CString(), tempfile))
		{
			CMessageBox::Show(m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			AfxGetApp()->DoWaitCursor(-1);
			return;
		}
	}
	CUtils::StartUnifiedDiffViewer(tempfile);
	AfxGetApp()->DoWaitCursor(-1);
}

void CMergeDlg::OnBnClickedBrowse()
{
	CString strUrl;
	m_URLCombo.GetWindowText(strUrl);
	if (strUrl.Left(7) == _T("file://"))
	{
		CString strFile(strUrl);
		SVN::UrlToPath(strFile);

		SVN svn;
		if (svn.IsRepository(strFile))
		{
			// browse repository - show repository browser
			CRepositoryBrowser browser(strUrl, this, m_bFile);
			if (browser.DoModal() == IDOK)
			{
				m_URLCombo.SetWindowText(browser.GetPath());
				if (m_bUseFromURL)
					m_URLCombo2.SetWindowText(browser.GetPath());
			}
		}
		else
		{
			// browse local directories
			CBrowseFolder folderBrowser;
			folderBrowser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
			if (folderBrowser.Show(GetSafeHwnd(), strUrl) == CBrowseFolder::OK)
			{
				SVN::PathToUrl(strUrl);

				m_URLCombo.SetWindowText(strUrl);
				if (m_bUseFromURL)
					m_URLCombo2.SetWindowText(strUrl);
			}
		}
	}
	else if ((strUrl.Left(7) == _T("http://")
		||(strUrl.Left(8) == _T("https://"))
		||(strUrl.Left(6) == _T("svn://"))
		||(strUrl.Left(10) == _T("svn+ssh://"))) && strUrl.GetLength() > 6)
	{
		// browse repository - show repository browser
		CRepositoryBrowser browser(strUrl, this, m_bFile);
		if (browser.DoModal() == IDOK)
		{
			m_URLCombo.SetWindowText(browser.GetPath());
			if (m_bUseFromURL)
				m_URLCombo2.SetWindowText(browser.GetPath());
		}
	} 
	else
	{
		// browse local directories
		CBrowseFolder folderBrowser;
		folderBrowser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
		if (folderBrowser.Show(GetSafeHwnd(), strUrl) == CBrowseFolder::OK)
		{
			SVN::PathToUrl(strUrl);

			m_URLCombo.SetWindowText(strUrl);
			if (m_bUseFromURL)
				m_URLCombo2.SetWindowText(strUrl);
		}
	}
}

void CMergeDlg::OnBnClickedBrowse2()
{
	CString strUrl;
	m_URLCombo2.GetWindowText(strUrl);
	if (strUrl.Left(7) == _T("file://"))
	{
		CString strFile(strUrl);
		SVN::UrlToPath(strFile);

		SVN svn;
		if (svn.IsRepository(strFile))
		{
			// browse repository - show repository browser
			CRepositoryBrowser browser(strUrl, this, m_bFile);
			if (browser.DoModal() == IDOK)
			{
				m_URLCombo2.SetWindowText(browser.GetPath());
			}
		}
		else
		{
			// browse local directories
			CBrowseFolder folderBrowser;
			folderBrowser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
			if (folderBrowser.Show(GetSafeHwnd(), strUrl) == CBrowseFolder::OK)
			{
				SVN::PathToUrl(strUrl);

				m_URLCombo2.SetWindowText(strUrl);
			}
		}
	}
	else if ((strUrl.Left(7) == _T("http://")
		||(strUrl.Left(8) == _T("https://"))
		||(strUrl.Left(6) == _T("svn://"))
		||(strUrl.Left(10) == _T("svn+ssh://"))) && strUrl.GetLength() > 6)
	{
		// browse repository - show repository browser
		CRepositoryBrowser browser(strUrl, this, m_bFile);
		if (browser.DoModal() == IDOK)
		{
			m_URLCombo2.SetWindowText(browser.GetPath());
		}
	} 
	else
	{
		// browse local directories
		CBrowseFolder folderBrowser;
		folderBrowser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
		if (folderBrowser.Show(GetSafeHwnd(), strUrl) == CBrowseFolder::OK)
		{
			SVN::PathToUrl(strUrl);

			m_URLCombo2.SetWindowText(strUrl);
		}
	}
}

void CMergeDlg::OnBnClickedRevisionHead()
{
	GetDlgItem(IDC_REVISION_END)->EnableWindow(FALSE);
}

void CMergeDlg::OnBnClickedRevisionN()
{
	GetDlgItem(IDC_REVISION_END)->EnableWindow(TRUE);
}

void CMergeDlg::OnBnClickedRevisionHead1()
{
	GetDlgItem(IDC_REVISION_START)->EnableWindow(FALSE);
}

void CMergeDlg::OnBnClickedRevisionN1()
{
	GetDlgItem(IDC_REVISION_START)->EnableWindow(TRUE);
}

void CMergeDlg::OnBnClickedFindbranchstart()
{
	UpdateData(TRUE);
	if (::IsWindow(m_pLogDlg->GetSafeHwnd())&&(m_pLogDlg->IsWindowVisible()))
		return;
	CString url;
	m_URLCombo.GetWindowText(url);
	AfxGetApp()->DoWaitCursor(1);
	//now show the log dialog for the main trunk
	if (!url.IsEmpty())
	{
		delete m_pLogDlg;
		m_pLogDlg = new CLogDlg();
		CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100);
		int limit = (int)(LONG)reg;
		m_pLogDlg->SetParams(CTSVNPath(url), SVNRev::REV_HEAD, 1, limit, TRUE, FALSE);
		m_pLogDlg->Create(IDD_LOGMESSAGE, this);
		m_pLogDlg->ShowWindow(SW_SHOW);
		m_pLogDlg->m_wParam = (m_bUseFromURL ? (MERGE_REVSELECTSTARTEND | MERGE_REVSELECTMINUSONE) : MERGE_REVSELECTSTART);
		m_pLogDlg->m_pNotifyWindow = this;
	}
	AfxGetApp()->DoWaitCursor(-1);
}

void CMergeDlg::OnBnClickedFindbranchend()
{
	UpdateData(TRUE);
	if (::IsWindow(m_pLogDlg2->GetSafeHwnd())&&(m_pLogDlg2->IsWindowVisible()))
		return;
	CString url;
	if (m_bUseFromURL)
		m_URLCombo.GetWindowText(url);
	else
		m_URLCombo2.GetWindowText(url);
	AfxGetApp()->DoWaitCursor(1);
	//now show the log dialog for the main trunk
	if (!url.IsEmpty())
	{
		delete m_pLogDlg2;
		m_pLogDlg2 = new CLogDlg();
		CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100);
		int limit = (int)(LONG)reg;
		m_pLogDlg2->SetParams(CTSVNPath(url), SVNRev::REV_HEAD, 1, limit, TRUE, FALSE);
		m_pLogDlg2->Create(IDD_LOGMESSAGE, this);
		m_pLogDlg2->ShowWindow(SW_SHOW);
		m_pLogDlg2->m_wParam = (m_bUseFromURL ? (MERGE_REVSELECTSTARTEND | MERGE_REVSELECTMINUSONE) : MERGE_REVSELECTEND);
		m_pLogDlg2->m_pNotifyWindow = this;
	}
	AfxGetApp()->DoWaitCursor(-1);
}

LPARAM CMergeDlg::OnRevSelected(WPARAM wParam, LPARAM lParam)
{
	CString temp;

	if (wParam & MERGE_REVSELECTSTART)
	{
		if (wParam & MERGE_REVSELECTMINUSONE)
			lParam--;
		temp.Format(_T("%ld"), lParam);
		GetDlgItem(IDC_REVISION_START)->SetWindowText(temp);
		CheckRadioButton(IDC_REVISION_HEAD1, IDC_REVISION_N1, IDC_REVISION_N1);
		GetDlgItem(IDC_REVISION_START)->EnableWindow(TRUE);
	}
	if (wParam & MERGE_REVSELECTEND)
	{
		temp.Format(_T("%ld"), lParam);
		GetDlgItem(IDC_REVISION_END)->SetWindowText(temp);
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
		GetDlgItem(IDC_REVISION_END)->EnableWindow(TRUE);
	}
	return 0;
}

void CMergeDlg::OnBnClickedHelp()
{
	OnHelp();
}

void CMergeDlg::OnBnClickedUsefromurl()
{
	UpdateData();
	if (m_bUseFromURL)
	{
		CString str;
		m_URLCombo.GetWindowText(str);
		m_URLCombo2.SetWindowText(str);
		GetDlgItem(IDC_URLCOMBO2)->EnableWindow(FALSE);
		GetDlgItem(IDC_BROWSE2)->EnableWindow(FALSE);
		GetDlgItem(IDC_FINDBRANCHEND)->EnableWindow(FALSE);
	}
	else
	{
		GetDlgItem(IDC_URLCOMBO2)->EnableWindow(TRUE);
		GetDlgItem(IDC_BROWSE2)->EnableWindow(TRUE);
		GetDlgItem(IDC_FINDBRANCHEND)->EnableWindow(TRUE);
	}
	UpdateData(FALSE);
}

void CMergeDlg::OnBnClickedWCLog()
{
	UpdateData(TRUE);
	if ((m_pLogDlg)&&(m_pLogDlg->IsWindowVisible()))
		return;
	AfxGetApp()->DoWaitCursor(1);
	//now show the log dialog for working copy
	if (!m_wcPath.IsEmpty())
	{
		delete [] m_pLogDlg;
		m_pLogDlg = new CLogDlg();
		m_pLogDlg->SetParams(m_wcPath, SVNRev::REV_HEAD, 1, (int)(DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100), TRUE, FALSE);
		m_pLogDlg->Create(IDD_LOGMESSAGE, this);
		m_pLogDlg->ShowWindow(SW_SHOW);
	} // if (!url.IsEmpty()) 
	AfxGetApp()->DoWaitCursor(-1);
}

void CMergeDlg::OnCbnEditchangeUrlcombo()
{
	if (m_bUseFromURL)
	{
		CString str;
		m_URLCombo.GetWindowText(str);
		m_URLCombo2.SetWindowText(str);
	}
}





