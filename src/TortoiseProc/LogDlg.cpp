// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Stefan Kueng

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
#include "cursor.h"
#include "UnicodeUtils.h"
#include "MergeDlg.h"
#include "LogDlg.h"
#include ".\logdlg.h"

// CLogDlg dialog

#define SHORTLOGMESSAGEWIDTH 500

IMPLEMENT_DYNAMIC(CLogDlg, CResizableDialog)
CLogDlg::CLogDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CLogDlg::IDD, pParent)
{
	m_pFindDialog = NULL;
	m_bCancelled = FALSE;
	m_bShowedAll = FALSE;
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pNotifyWindow = NULL;
}

CLogDlg::~CLogDlg()
{
	for (int i=0; i<m_templist.GetCount(); i++)
	{
		DeleteFile(m_templist.GetAt(i));
	}
	m_templist.RemoveAll();
}

void CLogDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LOGLIST, m_LogList);
	DDX_Control(pDX, IDC_LOGMSG, m_LogMsgCtrl);
}

const UINT CLogDlg::m_FindDialogMessage = RegisterWindowMessage(FINDMSGSTRING);

BEGIN_MESSAGE_MAP(CLogDlg, CResizableDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_NOTIFY(NM_CLICK, IDC_LOGLIST, OnNMClickLoglist)
	ON_NOTIFY(NM_RCLICK, IDC_LOGLIST, OnNMRclickLoglist)
	ON_NOTIFY(LVN_KEYDOWN, IDC_LOGLIST, OnLvnKeydownLoglist)
	ON_NOTIFY(NM_RCLICK, IDC_LOGMSG, OnNMRclickLogmsg)
	ON_REGISTERED_MESSAGE(m_FindDialogMessage, OnFindDialogMessage) 
	ON_BN_CLICKED(IDC_GETALL, OnBnClickedGetall)
	ON_NOTIFY(NM_DBLCLK, IDC_LOGMSG, OnNMDblclkLogmsg)
	ON_NOTIFY(NM_DBLCLK, IDC_LOGLIST, OnNMDblclkLoglist)
END_MESSAGE_MAP()



void CLogDlg::SetParams(CString path, long startrev /* = 0 */, long endrev /* = -1 */, BOOL hasWC)
{
	m_path = path;
	m_startrev = startrev;
	m_endrev = endrev;
	m_hasWC = hasWC;
}

void CLogDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CResizableDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CLogDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CLogDlg::OnInitDialog()
{
	CResizableDialog::OnInitDialog();
	// Set the icon for this dialog.  The framework does this automatically
	// when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	LOGFONT LogFont;
	LogFont.lfHeight         = -MulDiv((DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\LogFontSize"), 8), GetDeviceCaps(this->GetDC()->m_hDC, LOGPIXELSY), 72);
	LogFont.lfWidth          = 0;
	LogFont.lfEscapement     = 0;
	LogFont.lfOrientation    = 0;
	LogFont.lfWeight         = 400;
	LogFont.lfItalic         = 0;
	LogFont.lfUnderline      = 0;
	LogFont.lfStrikeOut      = 0;
	LogFont.lfCharSet        = DEFAULT_CHARSET;
	LogFont.lfOutPrecision   = OUT_DEFAULT_PRECIS;
	LogFont.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
	LogFont.lfQuality        = DRAFT_QUALITY;
	LogFont.lfPitchAndFamily = FF_DONTCARE | FIXED_PITCH;
	_tcscpy(LogFont.lfFaceName, (LPCTSTR)(CString)CRegString(_T("Software\\TortoiseSVN\\LogFontName"), _T("Courier New")));
	m_logFont.CreateFontIndirect(&LogFont);
	GetDlgItem(IDC_MSGVIEW)->SetFont(&m_logFont);

	m_LogList.SetExtendedStyle ( LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER );

	m_LogList.DeleteAllItems();
	int c = ((CHeaderCtrl*)(m_LogList.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_LogList.DeleteColumn(c--);
	CString temp;
	temp.LoadString(IDS_LOG_REVISION);
	m_LogList.InsertColumn(0, temp);
	temp.LoadString(IDS_LOG_AUTHOR);
	m_LogList.InsertColumn(1, temp);
	temp.LoadString(IDS_LOG_DATE);
	m_LogList.InsertColumn(2, temp);
	temp.LoadString(IDS_LOG_MESSAGE);
	m_LogList.InsertColumn(3, temp);
	m_arLogMessages.SetSize(0,100);
	m_arRevs.SetSize(0,100);

	m_LogList.SetRedraw(false);
	m_LogMsgCtrl.InsertColumn(0, _T("Message"));
	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(m_LogList.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		m_LogList.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}

	//first start a thread to obtain the log messages without
	//blocking the dialog
	DWORD dwThreadId;
	if ((m_hThread = CreateThread(NULL, 0, &LogThread, this, 0, &dwThreadId))==0)
	{
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);

	m_logcounter = 0;

	CString sTitle;
	GetWindowText(sTitle);
	SetWindowText(sTitle + _T(" - ") + m_path.Mid(m_path.ReverseFind('\\')+1));

	AddAnchor(IDC_LOGLIST, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_MSGVIEW, TOP_LEFT, MIDDLE_RIGHT);
	AddAnchor(IDC_LOGMSG, MIDDLE_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_GETALL, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	this->hWnd = this->m_hWnd;
	CenterWindow(CWnd::FromHandle(hWndExplorer));
	GetDlgItem(IDC_LOGLIST)->SetFocus();
	return FALSE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CLogDlg::FillLogMessageCtrl(CString msg, CString paths)
{
	GetDlgItem(IDC_MSGVIEW)->SetWindowText(msg);
	m_LogMsgCtrl.SetExtendedStyle ( LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER );
	m_LogMsgCtrl.DeleteAllItems();
	m_LogMsgCtrl.SetRedraw(FALSE);
	int line = 0;
	CString temp = _T("");
	int curpos = 0;
	int curposold = 0;
	CString linestr;
	while (paths.Find('\r', curpos)>=0)
	{
		curposold = curpos;
		curpos = paths.Find('\r', curposold);
		linestr = paths.Mid(curposold, curpos-curposold);
		linestr.Trim(_T("\n\r"));
		m_LogMsgCtrl.InsertItem(line++, linestr);
		curpos++;
	} // while (msg.Find('\n', curpos)>=0) 
	linestr = paths.Mid(curpos);
	linestr.Trim(_T("\n\r"));
	m_LogMsgCtrl.InsertItem(line++, linestr);

	m_LogMsgCtrl.SetColumnWidth(0,LVSCW_AUTOSIZE_USEHEADER);
	m_LogMsgCtrl.SetRedraw();
}

void CLogDlg::OnBnClickedGetall()
{
	m_LogList.DeleteAllItems();
	m_arLogMessages.RemoveAll();
	m_arLogPaths.RemoveAll();
	m_arRevs.RemoveAll();
	m_logcounter = 0;

	m_endrev = 1;
	m_bCancelled = FALSE;
	DWORD dwThreadId;
	if ((m_hThread = CreateThread(NULL, 0, &LogThread, this, 0, &dwThreadId))==0)
	{
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);
	m_bShowedAll = TRUE;
	GetDlgItem(IDC_GETALL)->ShowWindow(SW_HIDE);
}

BOOL CLogDlg::Cancel()
{
	return m_bCancelled;
}

void CLogDlg::OnCancel()
{
	CString temp, temp2;
	GetDlgItem(IDOK)->GetWindowText(temp);
	temp2.LoadString(IDS_MSGBOX_CANCEL);
	if (temp.Compare(temp2)==0)
	{
		m_bCancelled = TRUE;
		return;
	}
	__super::OnCancel();
}

BOOL CLogDlg::Log(LONG rev, CString author, CString date, CString message, CString& cpaths)
{
	int line = 0;
	CString temp;
	m_logcounter += 1;
	int count = m_LogList.GetItemCount();
	temp.Format(_T("%d"),rev);
	m_LogList.InsertItem(count, temp);
	m_LogList.SetItemText(count, 1, author);
	m_LogList.SetItemText(count, 2, date);

	// Add as many characters from the log message to the list control
	// so it has a fixed width. If the log message is longer than
	// this predefined fixed with, add "..." as an indication.
	CString sShortMessage = message;
	// Remove newlines 'cause those are not shown nicely in the listcontrol
	sShortMessage.Replace('\n', ' ');
	sShortMessage.Replace(_T("\r"), _T(""));
	if (m_LogList.GetStringWidth(sShortMessage) > SHORTLOGMESSAGEWIDTH)
	{
		// Make an initial guess on how many chars fit into the fixed width
		int nPix = m_LogList.GetStringWidth(sShortMessage);
		int nAvgCharWidth = nPix / sShortMessage.GetLength();
		sShortMessage = sShortMessage.Left(SHORTLOGMESSAGEWIDTH / nAvgCharWidth + 5);
		sShortMessage += _T("...");
	} // if (m_LogList.GetStringWidth(sShortMessage) > SHORTLOGMESSAGEWIDTH) 
	while (m_LogList.GetStringWidth(sShortMessage) > SHORTLOGMESSAGEWIDTH)
	{
		sShortMessage = sShortMessage.Left(sShortMessage.GetLength()-4);
		sShortMessage += _T("...");
	} // while (m_LogList.GetStringWidth(sShortMessage) > SHORTLOGMESSAGEWIDTH) 
	m_LogList.SetItemText(count, 3, sShortMessage);

	//split multiline logentries and concatenate them
	//again but this time with \r\n as line separators
	//so that the edit control recognizes them
	try
	{
		temp = "";
		if (message.GetLength()>0)
		{
			message.Replace(_T("\n\r"), _T("\n"));
			message.Replace(_T("\r\n"), _T("\n"));
			message.Replace(_T("\n"), _T("\r\n"));
			int pos = 0;
			if (message.Right(2).Compare(_T("\r\n"))==0)
				message = message.Left(message.GetLength()-2);
			while (message.Find('\n', pos)>=0)
			{
				line++;
				pos = message.Find('\n', pos);
				pos = pos + 1;		// add "\r"
			}
		} // if (message.GetLength()>0)
		m_arLogMessages.Add(message);
		m_arLogPaths.Add(cpaths);
		m_arRevs.Add(rev);
	}
	catch (CException * e)
	{
		::MessageBox(NULL, _T("not enough memory!"), _T("TortoiseSVN"), MB_ICONERROR);
		e->Delete();
		m_bCancelled = TRUE;
	}
	m_LogList.SetRedraw();
	return TRUE;
}

//this is the thread function which calls the subversion function
DWORD WINAPI LogThread(LPVOID pVoid)
{
	CLogDlg*	pDlg;
	pDlg = (CLogDlg*)pVoid;
	CString temp;
	temp.LoadString(IDS_MSGBOX_CANCEL);
	pDlg->GetDlgItem(IDOK)->SetWindowText(temp);
	if (pDlg->m_endrev < (-5))
	{
		SVNStatus status;
		long r = status.GetStatus(pDlg->m_path, TRUE);
		if (r != (-2))
		{
			pDlg->m_endrev = r + pDlg->m_endrev;
		} // if (r != (-2))
		if (pDlg->m_endrev <= 0)
		{
			pDlg->m_endrev = 1;
			pDlg->m_bShowedAll = TRUE;
		}
	} // if (pDlg->m_endrev < (-5))
	else
	{
		pDlg->m_bShowedAll = TRUE;
	}
	//disable the "Get All" button while we're receiving
	//log messages.
	pDlg->GetDlgItem(IDC_GETALL)->EnableWindow(FALSE);

	if (!pDlg->ReceiveLog(pDlg->m_path, pDlg->m_startrev, pDlg->m_endrev, true))
	{
		CMessageBox::Show(pDlg->m_hWnd, pDlg->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
	}

	pDlg->m_LogList.SetRedraw(false);
	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(pDlg->m_LogList.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		pDlg->m_LogList.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}
	pDlg->m_LogList.SetRedraw(true);

	temp.LoadString(IDS_MSGBOX_OK);
	pDlg->GetDlgItem(IDOK)->SetWindowText(temp);
	if (!pDlg->m_bShowedAll)
		pDlg->GetDlgItem(IDC_GETALL)->EnableWindow(TRUE);
	pDlg->m_bCancelled = TRUE;
	return 0;
}

void CLogDlg::OnNMClickLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	int selIndex = m_LogList.GetSelectionMark();
	if (selIndex >= 0)
	{
		//m_sLogMsgCtrl = m_arLogMessages.GetAt(selIndex);
		FillLogMessageCtrl(m_arLogMessages.GetAt(selIndex), m_arLogPaths.GetAt(selIndex));
		this->m_nSearchIndex = selIndex;
		UpdateData(FALSE);
	}
	else
	{
		//m_sLogMsgCtrl = _T("");
		FillLogMessageCtrl(_T(""), _T(""));
		UpdateData(FALSE);
	}
	*pResult = 0;
}

void CLogDlg::OnLvnKeydownLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
	int selIndex = m_LogList.GetSelectionMark();
	if (selIndex >= 0)
	{
		//check which key was pressed
		//this is necessary because we get here BEFORE
		//the selection actually changes, so we have to
		//adjust the selected index
		if (pLVKeyDow->wVKey == VK_UP)
		{
			if (selIndex > 0)
				selIndex--;
		}
		if (pLVKeyDow->wVKey == VK_DOWN)
		{
			selIndex++;		
			if (selIndex >= m_LogList.GetItemCount())
				selIndex = m_LogList.GetItemCount()-1;
		}
		//m_sLogMsgCtrl = m_arLogMessages.GetAt(selIndex);
		this->m_nSearchIndex = selIndex;
		FillLogMessageCtrl(m_arLogMessages.GetAt(selIndex), m_arLogPaths.GetAt(selIndex));
		UpdateData(FALSE);
	}
	else
	{
		if (m_arLogMessages.GetCount()>0)
			FillLogMessageCtrl(m_arLogMessages.GetAt(0), m_arLogPaths.GetAt(0));
		UpdateData(FALSE);
	}
	*pResult = 0;
}

void CLogDlg::OnNMRclickLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	//check if an entry is selected
	*pResult = 0;
	int selIndex = m_LogList.GetSelectionMark();
	this->m_nSearchIndex = selIndex;

	if (selIndex >= 0)
	{
		//entry is selected, now show the popup menu
		CMenu popup;
		POINT point;
		GetCursorPos(&point);
		if (popup.CreatePopupMenu())
		{
			CString temp;
			if (m_LogList.GetSelectedCount() == 1)
			{
				if (!PathIsDirectory(m_path))
				{
					temp.LoadString(IDS_LOG_POPUP_COMPARE);
					if (m_hasWC)
					{
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COMPARE, temp);
						popup.SetDefaultItem(ID_COMPARE, FALSE);
					}
					temp.LoadString(IDS_LOG_POPUP_GNUDIFF);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_GNUDIFF1, temp);
					popup.AppendMenu(MF_SEPARATOR, NULL);
					temp.LoadString(IDS_LOG_POPUP_SAVE);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_SAVEAS, temp);
				} // if (!PathIsDirectory(m_path))
				else
				{
					temp.LoadString(IDS_LOG_POPUP_GNUDIFF);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_GNUDIFF1, temp);
					popup.SetDefaultItem(ID_GNUDIFF1, FALSE);
					popup.AppendMenu(MF_SEPARATOR, NULL);
					temp.LoadString(IDS_LOG_BROWSEREPO);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_REPOBROWSE, temp);
					temp.LoadString(IDS_LOG_POPUP_COPY);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COPY, temp);
				}
				temp.LoadString(IDS_LOG_POPUP_UPDATE);
				if (m_hasWC)
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_UPDATE, temp);
				temp.LoadString(IDS_LOG_POPUP_REVERTREV);
				if (m_hasWC)
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_REVERTREV, temp);
			}
			else if (m_LogList.GetSelectedCount() == 2)
			{
				if (!PathIsDirectory(m_path))
				{
					temp.LoadString(IDS_LOG_POPUP_COMPARETWO);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COMPARETWO, temp);
				}
				temp.LoadString(IDS_LOG_POPUP_GNUDIFF);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_GNUDIFF2, temp);
			}
			popup.AppendMenu(MF_SEPARATOR, NULL);
			temp.LoadString(IDS_LOG_POPUP_FIND);
			popup.AppendMenu(MF_STRING | MF_ENABLED, ID_FINDENTRY, temp);

			int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
			GetDlgItem(IDOK)->EnableWindow(FALSE);
			this->m_app = &theApp;
			theApp.DoWaitCursor(1);
			switch (cmd)
			{
			case ID_GNUDIFF1:
				{
					int selIndex = m_LogList.GetSelectionMark();
					long rev = m_arRevs.GetAt(selIndex);
					this->m_bCancelled = FALSE;
					CString tempfile = CUtils::GetTempFile();
					tempfile += _T(".diff");
					m_templist.Add(tempfile);
					if (!Diff(m_path, rev-1, m_path, rev, TRUE, FALSE, TRUE, _T(""), tempfile))
					{
						CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						break;		//exit
					} // if (!Diff(m_path, rev-1, m_path, rev, TRUE, FALSE, TRUE, _T(""), tempfile))
					else
					{
						CUtils::StartDiffViewer(tempfile);
					}
				}
				break;
			case ID_GNUDIFF2:
				{
					POSITION pos = m_LogList.GetFirstSelectedItemPosition();
					long rev1 = m_arRevs.GetAt(m_LogList.GetNextSelectedItem(pos));
					long rev2 = m_arRevs.GetAt(m_LogList.GetNextSelectedItem(pos));
					this->m_bCancelled = FALSE;
					CString tempfile = CUtils::GetTempFile();
					tempfile += _T(".diff");
					m_templist.Add(tempfile);
					if (!Diff(m_path, rev2, m_path, rev1, TRUE, FALSE, TRUE, _T(""), tempfile))
					{
						CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						break;		//exit
					} // if (!Diff(m_path, rev2, m_path, rev1, TRUE, FALSE, TRUE, _T(""), tempfile))
					else
					{
						CUtils::StartDiffViewer(tempfile);
					}
				}
				break;
			case ID_REVERTREV:
				{
					int selIndex = m_LogList.GetSelectionMark();
					long rev = m_arRevs.GetAt(selIndex);
					if (CMessageBox::Show(this->m_hWnd, IDS_LOG_REVERT_CONFIRM, IDS_APPNAME, MB_YESNO | MB_ICONQUESTION) == IDYES)
					{
						SVNStatus status;
						status.GetStatus(m_path);
						if ((status.status == NULL)&&(status.status->entry == NULL))
						{
							CString temp;
							temp.Format(IDS_ERR_NOURLOFFILE, status.GetLastErrorMsg());
							CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
							TRACE(_T("could not retrieve the URL of the folder!\n"));
							break;		//exit
						} // if ((rev == (-2))||(status.status->entry == NULL))
						else
						{
							CString url = CUnicodeUtils::GetUnicode(status.status->entry->url);
							CSVNProgressDlg dlg;
							dlg.SetParams(Enum_Merge, false, m_path, url, url, rev);		//use the message as the second url
							dlg.m_nRevisionEnd = rev-1;
							dlg.DoModal();
						}
					} // if (CMessageBox::Show(this->m_hWnd, IDS_LOG_REVERT_CONFIRM, IDS_APPNAME, MB_ICONQUESTION) == IDOK) 
				}
				break;
			case ID_COPY:
				{
					int selIndex = m_LogList.GetSelectionMark();
					long rev = m_arRevs.GetAt(selIndex);
					CCopyDlg dlg;
					SVNStatus status;
					status.GetStatus(m_path);
					if ((status.status == NULL)&&(status.status->entry == NULL))
					{
						CString temp;
						temp.Format(IDS_ERR_NOURLOFFILE, status.GetLastErrorMsg());
						CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
						TRACE(_T("could not retrieve the URL of the folder!\n"));
						break;		//exit
					} // if (status.status->entry == NULL) 
					else
					{
						CString url = CUnicodeUtils::GetUnicode(status.status->entry->url);
						dlg.m_URL = url;
						dlg.m_path = m_path;
						if (dlg.DoModal() == IDOK)
						{
							SVN svn;
							if (!svn.Copy(url, dlg.m_URL, rev))
							{
								CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							}
							else
							{
								CMessageBox::Show(this->m_hWnd, IDS_LOG_COPY_SUCCESS, IDS_APPNAME, MB_ICONINFORMATION);
							}
						} // if (dlg.DoModal() == IDOK) 
					}
				} 
				break;
			case ID_COMPARE:
				{
					//user clicked on the menu item "compare with working copy"
					//now first get the revision which is selected
					int selIndex = m_LogList.GetSelectionMark();
					long rev = m_arRevs.GetAt(selIndex);
					CString tempfile = CUtils::GetTempFile();
					m_templist.Add(tempfile);
					SVN svn;
					if (!svn.Cat(m_path, rev, tempfile))
					{
						CMessageBox::Show(NULL, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						GetDlgItem(IDOK)->EnableWindow(TRUE);
						break;
					} // if (!svn.Cat(m_path, rev, tempfile))
					else
					{
						CString revname, wcname;
						CString ext = CUtils::GetFileExtFromPath(m_path);
						revname.Format(_T("%s Revision %ld"), CUtils::GetFileNameFromPath(m_path), rev);
						wcname.Format(IDS_DIFF_WCNAME, CUtils::GetFileNameFromPath(m_path));
						CUtils::StartDiffViewer(tempfile, m_path, FALSE, revname, wcname, ext);
					}
				}
				break;
			case ID_COMPARETWO:
				{
					//user clicked on the menu item "compare revisions"
					//now first get the revisions which are selected
					POSITION pos = m_LogList.GetFirstSelectedItemPosition();
					long rev1 = m_arRevs.GetAt(m_LogList.GetNextSelectedItem(pos));
					long rev2 = m_arRevs.GetAt(m_LogList.GetNextSelectedItem(pos));

					StartDiff(m_path, rev1, m_path, rev2);
				}
				break;
			case ID_SAVEAS:
				{
					//now first get the revision which is selected
					int selIndex = m_LogList.GetSelectionMark();
					long rev = m_arRevs.GetAt(selIndex);

					OPENFILENAME ofn;		// common dialog box structure
					TCHAR szFile[MAX_PATH];  // buffer for file name
					ZeroMemory(szFile, sizeof(szFile));
					if (m_hasWC)
						_tcscpy(szFile, m_path);
					// Initialize OPENFILENAME
					ZeroMemory(&ofn, sizeof(OPENFILENAME));
					//ofn.lStructSize = sizeof(OPENFILENAME);
					ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;		//to stay compatible with NT4
					ofn.hwndOwner = this->m_hWnd;
					ofn.lpstrFile = szFile;
					ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
					CString temp;
					temp.LoadString(IDS_LOG_POPUP_SAVE);
					//ofn.lpstrTitle = "Save revision to...\0";
					if (temp.IsEmpty())
						ofn.lpstrTitle = NULL;
					else
						ofn.lpstrTitle = temp;
					ofn.Flags = OFN_OVERWRITEPROMPT;

					CString sFilter;
					sFilter.LoadString(IDS_COMMONFILEFILTER);
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
					CString tempfile;
					if (GetSaveFileName(&ofn)==TRUE)
					{
						tempfile = CString(ofn.lpstrFile);
						SVN svn;
						if (!svn.Cat(m_path, rev, tempfile))
						{
							delete [] pszFilters;
							CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							GetDlgItem(IDOK)->EnableWindow(TRUE);
							break;
						} // if (!svn.Cat(m_path, rev, tempfile)) 
					} // if (GetSaveFileName(&ofn)==TRUE)
					delete [] pszFilters;
				}
				break;
			case ID_UPDATE:
				{
					//now first get the revision which is selected
					int selIndex = m_LogList.GetSelectionMark();
					long rev = m_arRevs.GetAt(selIndex);

					SVN svn;
					if (!svn.Update(m_path, rev, TRUE))
					{
						CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						GetDlgItem(IDOK)->EnableWindow(TRUE);
						break;
					}
				}
				break;
			case ID_FINDENTRY:
				{
					m_nSearchIndex = m_LogList.GetSelectionMark();
					if (m_pFindDialog)
					{
						break;
					}
					else
					{
						m_pFindDialog = new CFindReplaceDialog();
						m_pFindDialog->Create(TRUE, NULL, NULL, FR_HIDEUPDOWN | FR_HIDEWHOLEWORD, this);									
					}
				}
				break;
			case ID_REPOBROWSE:
				{
					int selIndex = m_LogList.GetSelectionMark();
					long rev = m_arRevs.GetAt(selIndex);
					CString url = m_path;
					if (m_hasWC)
					{
						SVNStatus status;
						if (status.GetStatus(m_path) != -2)
						{
							if (status.status->entry)
								url = status.status->entry->url;
						} // if (status.GetStatus(path)!=-2)
						else
						{
							CMessageBox::Show(this->m_hWnd, status.GetLastErrorMsg(), _T("TortoiseSVN"), MB_ICONERROR);
							GetDlgItem(IDOK)->EnableWindow(TRUE);
							break;
						}
					} // if (m_hasWC)

					CRepositoryBrowser dlg(url);
					dlg.m_nRevision = rev;
					dlg.m_bStandAlone = TRUE;
					dlg.DoModal();
				}
				break;
			default:
				break;
			} // switch (cmd)
			theApp.DoWaitCursor(-1);
			GetDlgItem(IDOK)->EnableWindow(TRUE);
		} // if (popup.CreatePopupMenu())
	} // if (selIndex >= 0)
}

void CLogDlg::OnNMDblclkLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	int selIndex = pNMLV->iItem;

	if (selIndex >= 0)
	{
		CString temp;
		GetDlgItem(IDOK)->EnableWindow(FALSE);
		this->m_app = &theApp;
		theApp.DoWaitCursor(1);
		if (!PathIsDirectory(m_path))
		{
			long rev = m_arRevs.GetAt(selIndex);
			CString tempfile = CUtils::GetTempFile();
			m_templist.Add(tempfile);

			SVN svn;
			if (!svn.Cat(m_path, rev, tempfile))
			{
				CMessageBox::Show(NULL, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
				GetDlgItem(IDOK)->EnableWindow(TRUE);
			} // if (!svn.Cat(m_path, rev, tempfile))
			else
			{
				CString revname, wcname;
				CString ext = CUtils::GetFileExtFromPath(m_path);
				revname.Format(_T("%s Revision %ld"), CUtils::GetFileNameFromPath(m_path), rev);
				wcname.Format(IDS_DIFF_WCNAME, CUtils::GetFileNameFromPath(m_path));
				CUtils::StartDiffViewer(tempfile, m_path, FALSE, revname, wcname, ext);
			}
		} // if (!PathIsDirectory(m_path))
		else
		{
			long rev = m_arRevs.GetAt(selIndex);
			this->m_bCancelled = FALSE;
			CString tempfile = CUtils::GetTempFile();
			tempfile += _T(".diff");
			m_templist.Add(tempfile);
			if (!Diff(m_path, rev-1, m_path, rev, TRUE, FALSE, TRUE, _T(""), tempfile))
			{
				CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			} // if (!Diff(m_path, rev-1, m_path, rev, TRUE, FALSE, TRUE, _T(""), tempfile))
			else
			{
				CUtils::StartDiffViewer(tempfile);
			}
		}
		theApp.DoWaitCursor(-1);
		GetDlgItem(IDOK)->EnableWindow(TRUE);
	} // if (selIndex >= 0)
}

LRESULT CLogDlg::OnFindDialogMessage(WPARAM wParam, LPARAM lParam)
{
    ASSERT(m_pFindDialog != NULL);

    if (m_pFindDialog->IsTerminating())
    {
	    // invalidate the handle identifying the dialog box.
        m_pFindDialog = NULL;
        return 0;
    }

    if(m_pFindDialog->FindNext())
    {
        //read data from dialog
        CString FindText = m_pFindDialog->GetFindString();
        bool bMatchCase = (m_pFindDialog->MatchCase() == TRUE);
		bool bFound = FALSE;

		for (int i=this->m_nSearchIndex; i<m_LogList.GetItemCount(); i++)
		{
			if (bMatchCase)
			{
				if (m_arLogMessages.GetAt(i).Find(FindText) >= 0)
				{
					bFound = TRUE;
					break;
				} // if (m_arLogMessages.GetAt(i).Find(FindText) >= 0) 
			} // if (bMatchCase) 
			else
			{
				CString msg = m_arLogMessages.GetAt(i);
				msg = msg.MakeLower();
				CString find = FindText.MakeLower();
				if (msg.Find(FindText) >= 0)
				{
					bFound = TRUE;
					break;
				} // if (msg.Find(FindText) >= 0) 
			} 
		} // for (int i=this->m_nSearchIndex; i<m_LogList.GetItemCount(); i++) 
		if (bFound)
		{
			this->m_nSearchIndex = (i+1);
			m_LogList.EnsureVisible(i, FALSE);
			m_LogList.SetItemState(m_LogList.GetSelectionMark(), 0, LVIS_SELECTED);
			m_LogList.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
			m_LogList.SetSelectionMark(i);
			FillLogMessageCtrl(m_arLogMessages.GetAt(i), m_arLogPaths.GetAt(i));
			UpdateData(FALSE);
		}
    } // if(m_pFindDialog->FindNext()) 

    return 0;
}

void CLogDlg::OnOK()
{
	CString temp;
	CString buttontext;
	GetDlgItem(IDOK)->GetWindowText(buttontext);
	temp.LoadString(IDS_MSGBOX_CANCEL);
	if (temp.Compare(buttontext) != 0)
		__super::OnOK();
	m_bCancelled = TRUE;
	if (m_pNotifyWindow)
	{
		int selIndex = m_LogList.GetSelectionMark();
		if (selIndex >= 0)
		{	
			m_pNotifyWindow->SendMessage(WM_REVSELECTED, 0, m_arRevs.GetAt(selIndex));
		}
	}
}

void CLogDlg::OnNMRclickLogmsg(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
	int selIndex = m_LogMsgCtrl.GetSelectionMark();
	if (selIndex < 0)
		return;
	int s = m_LogList.GetSelectionMark();
	if (s < 0)
		return;
	long rev = m_arRevs.GetAt(s);
	//entry is selected, now show the popup menu
	CMenu popup;
	POINT point;
	GetCursorPos(&point);
	if (popup.CreatePopupMenu())
	{
		CString temp;
		if (m_LogMsgCtrl.GetSelectedCount() == 1)
		{
			temp = m_LogMsgCtrl.GetItemText(selIndex, 0);
			temp = temp.Left(temp.Find(' '));
			CString t, tt;
			t.LoadString(IDS_SVNACTION_ADD);
			tt.LoadString(IDS_SVNACTION_DELETE);
			if ((rev > 1)&&(temp.Compare(t)!=0)&&(temp.Compare(tt)!=0))
			{
				temp.LoadString(IDS_LOG_POPUP_DIFF);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_DIFF, temp);
			}
			int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
			switch (cmd)
			{
			case ID_DIFF:
				{
					DoDiffFromLog(selIndex,temp,rev);
				}
				break;
			default:
				break;
			} // switch (cmd)
		} // if (m_LogMsgCtrl.GetSelectedCount() == 1)
	} // if (popup.CreatePopupMenu())
}

void CLogDlg::OnNMDblclkLogmsg(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
	int selIndex = m_LogMsgCtrl.GetSelectionMark();
	if (selIndex < 0)
		return;
	int s = m_LogList.GetSelectionMark();
	if (s < 0)
		return;
	long rev = m_arRevs.GetAt(s);
	CString temp = m_LogMsgCtrl.GetItemText(selIndex, 0);
	temp = temp.Left(temp.Find(' '));
	CString t, tt;
	t.LoadString(IDS_SVNACTION_ADD);
	tt.LoadString(IDS_SVNACTION_DELETE);
	if ((rev > 1)&&(temp.Compare(t)!=0)&&(temp.Compare(tt)!=0))
	{
		DoDiffFromLog(selIndex,temp,rev);
	}
}

void CLogDlg::DoDiffFromLog(int selIndex, CString temp, long rev)
{
	GetDlgItem(IDOK)->EnableWindow(FALSE);
	this->m_app = &theApp;
	theApp.DoWaitCursor(1);
	//get the filename
	CString filepath;
	if (SVN::PathIsURL(m_path))
	{
		filepath = m_path;
	}
	else
	{
		SVNStatus status;
		if ((status.GetStatus(m_path) == (-2))||(status.status->entry == NULL))
		{
			theApp.DoWaitCursor(-1);
			CString temp;
			temp.Format(IDS_ERR_NOURLOFFILE, status.GetLastErrorMsg());
			CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
			TRACE(_T("could not retrieve the URL of the file!\n"));
			GetDlgItem(IDOK)->EnableWindow(TRUE);
			theApp.DoWaitCursor(-11);
			return;		//exit
		} // if ((rev == (-2))||(status.status->entry == NULL))
		filepath = CString(status.status->entry->url);
	}
	temp = m_LogMsgCtrl.GetItemText(selIndex, 0);
	m_bCancelled = FALSE;
	filepath = GetRepositoryRoot(filepath, rev);
	temp = temp.Mid(temp.Find(' '));
	temp = temp.Trim();
	filepath += temp;
	StartDiff(filepath, rev, filepath, rev-1);
	theApp.DoWaitCursor(-1);
	GetDlgItem(IDOK)->EnableWindow(TRUE);
}

BOOL CLogDlg::StartDiff(CString path1, LONG rev1, CString path2, LONG rev2)
{
	CString tempfile1 = CUtils::GetTempFile();
	CString tempfile2 = CUtils::GetTempFile();
	m_templist.Add(tempfile1);
	m_templist.Add(tempfile2);
	CProgressDlg progDlg;
	if (progDlg.IsValid())
	{
		CString temp;
		temp.Format(IDS_PROGRESSGETFILE, path1);
		progDlg.SetLine(1, temp, true);
		temp.Format(IDS_PROGRESSREVISION, rev1);
		progDlg.SetLine(2, temp, false);
		temp.LoadString(IDS_PROGRESSWAIT);
		progDlg.SetTitle(temp);
		progDlg.SetLine(3, _T(""));
		progDlg.SetCancelMsg(_T(""));
		progDlg.ShowModeless(this);
	}
	m_bCancelled = FALSE;
	this->m_app = &theApp;
	theApp.DoWaitCursor(1);
	if (!Cat(path1, rev1, tempfile1))
	{
		theApp.DoWaitCursor(-1);
		CMessageBox::Show(NULL, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		return FALSE;
	} // if (!Cat(path1, rev1, tempfile1))
	if (progDlg.IsValid())
	{
		progDlg.SetProgress((DWORD)1,(DWORD)2);
		CString temp;
		temp.Format(IDS_PROGRESSGETFILE, path1);
		progDlg.SetLine(1, temp, true);
		temp.Format(IDS_PROGRESSREVISION, rev1);
		progDlg.SetLine(2, temp, false);
	}
	if (!Cat(path2, rev2, tempfile2))
	{
		theApp.DoWaitCursor(-1);
		CMessageBox::Show(NULL, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		return FALSE;
	}
	if (progDlg.IsValid())
	{
		progDlg.SetProgress((DWORD)2,(DWORD)2);
		progDlg.Stop();
	}
	theApp.DoWaitCursor(-1);
	CString revname1, revname2;
	CString ext = CUtils::GetFileExtFromPath(path1);
	revname1.Format(_T("%s Revision %ld"), CUtils::GetFileNameFromPath(path1), rev1);
	revname2.Format(_T("%s Revision %ld"), CUtils::GetFileNameFromPath(path2), rev2);
	return CUtils::StartDiffViewer(tempfile2, tempfile1, FALSE, revname2, revname1, ext);
}















