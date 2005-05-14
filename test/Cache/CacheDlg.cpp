// CacheDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Cache.h"
#include "DirFileEnum.h"
#include <WinInet.h>
#include ".\cachedlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// The name of the named-pipe for the cache
#define TSVN_CACHE_PIPE_NAME _T("\\\\.\\pipe\\TSVNCache")
#define TSVN_CACHE_COMMANDPIPE_NAME _T("\\\\.\\pipe\\TSVNCacheCommand")


// A structure passed as a request from the shell (or other client) to the external cache
struct TSVNCacheRequest
{
	DWORD flags;
	WCHAR path[MAX_PATH+1];
};

// The structure returned as a response
struct TSVNCacheResponse
{
	svn_wc_status2_t m_status;
	svn_wc_entry_t m_entry;
	svn_node_kind_t m_kind;
	char m_url[INTERNET_MAX_URL_LENGTH+1];
	char m_owner[255];		///< owner of the lock
	char m_author[255];
	bool m_readonly;		///< whether the file is write protected or not
};

struct TSVNCacheCommand
{
	BYTE command;		///< the command to execute
	WCHAR path[MAX_PATH+1];		///< path to do the command for
};

#define		TSVNCACHECOMMAND_END		0		///< ends the thread handling the pipe communication
#define		TSVNCACHECOMMAND_CRAWL		1		///< start crawling the specified path for changes


/// Set this flag if you already know whether or not the item is a folder
#define TSVNCACHE_FLAGS_FOLDERISKNOWN		0x01
/// Set this flag if the item is a folder
#define TSVNCACHE_FLAGS_ISFOLDER			0x02
/// Set this flag if you want recursive folder status (safely ignored for file paths)
#define TSVNCACHE_FLAGS_RECUSIVE_STATUS		0x04
/// Set this flag if notifications to the shell are not allowed
#define TSVNCACHE_FLAGS_NONOTIFICATIONS		0x08



CCacheDlg::CCacheDlg(CWnd* pParent /*=NULL*/)
: CDialog(CCacheDlg::IDD, pParent)
, m_sRootPath(_T(""))
, m_hPipe(INVALID_HANDLE_VALUE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CCacheDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_ROOTPATH, m_sRootPath);
}

BEGIN_MESSAGE_MAP(CCacheDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// CCacheDlg message handlers

BOOL CCacheDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CCacheDlg::OnPaint() 
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
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CCacheDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CCacheDlg::OnBnClickedOk()
{
	UpdateData();
	AfxBeginThread(TestThreadEntry, this);
}
UINT CCacheDlg::TestThreadEntry(LPVOID pVoid)
{
	return ((CCacheDlg*)pVoid)->TestThread();
}

//this is the thread function which calls the subversion function
UINT CCacheDlg::TestThread()
{
	CDirFileEnum direnum(m_sRootPath);
	m_filelist.RemoveAll();
	CString filepath;
	bool bIsDir = false;
	while (direnum.NextFile(filepath, &bIsDir))
		m_filelist.Add(filepath);

	CTime starttime = CTime::GetCurrentTime();
	GetDlgItem(IDC_STARTTIME)->SetWindowText(starttime.Format(_T("%H:%M:%S")));
	int filecounter = 0;

	DWORD startticks = GetTickCount();

	CString sNumber;
	srand(GetTickCount());
	for (int i=0; i < 100000; ++i)
	{
		CString filepath = m_filelist.GetAt(rand() % m_filelist.GetCount());
		GetDlgItem(IDC_FILEPATH)->SetWindowText(filepath);
		GetStatusFromRemoteCache(CTSVNPath(filepath), true);
		sNumber.Format(_T("%d"), i);
		GetDlgItem(IDC_DONE)->SetWindowText(sNumber);
		if ((GetTickCount()%10)==1)
			Sleep(2000);
		if ((rand()%10)==3)
			RemoveFromCache(filepath);
	}
	CTime endtime = CTime::GetCurrentTime();
	CString sEnd = endtime.Format(_T("%H:%M:%S"));

	DWORD endticks = GetTickCount();

	CString sEndText;
	sEndText.Format(_T("%s  - %d ms"), sEnd, endticks-startticks);

	GetDlgItem(IDC_ENDTIME)->SetWindowText(sEndText);

	return 0;
}


bool CCacheDlg::EnsurePipeOpen()
{
	if(m_hPipe != INVALID_HANDLE_VALUE)
	{
		return true;
	}

	m_hPipe = CreateFile(
		_T("\\\\.\\pipe\\TSVNCache"),   // pipe name
		GENERIC_READ |					// read and write access
		GENERIC_WRITE,
		0,								// no sharing
		NULL,							// default security attributes
		OPEN_EXISTING,				// opens existing pipe
		FILE_FLAG_OVERLAPPED,			// default attributes
		NULL);							// no template file

	if (m_hPipe == INVALID_HANDLE_VALUE && GetLastError() == ERROR_PIPE_BUSY)
	{
		// TSVNCache is running but is busy connecting a different client.
		// Do not give up immediately but wait for a few milliseconds until
		// the server has created the next pipe instance
		if (WaitNamedPipe(_T("\\\\.\\pipe\\TSVNCache"), 50))
		{
			m_hPipe = CreateFile(
				_T("\\\\.\\pipe\\TSVNCache"),   // pipe name
				GENERIC_READ |					// read and write access
				GENERIC_WRITE,
				0,								// no sharing
				NULL,							// default security attributes
				OPEN_EXISTING,				// opens existing pipe
				FILE_FLAG_OVERLAPPED,			// default attributes
				NULL);							// no template file
		}
	}


	if (m_hPipe != INVALID_HANDLE_VALUE)
	{
		// The pipe connected; change to message-read mode.
		DWORD dwMode;

		dwMode = PIPE_READMODE_MESSAGE;
		if(!SetNamedPipeHandleState(
			m_hPipe,    // pipe handle
			&dwMode,  // new pipe mode
			NULL,     // don't set maximum bytes
			NULL))    // don't set maximum time
		{
			ATLTRACE("SetNamedPipeHandleState failed");
			CloseHandle(m_hPipe);
			m_hPipe = INVALID_HANDLE_VALUE;
			return false;
		}
		// create an unnamed (=local) manual reset event for use in the overlapped structure
		m_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (m_hEvent)
			return true;
		ATLTRACE("CreateEvent failed");
		ClosePipe();
		return false;
	}

	return false;
}


void CCacheDlg::ClosePipe()
{
	if(m_hPipe != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hPipe);
		CloseHandle(m_hEvent);
		m_hPipe = INVALID_HANDLE_VALUE;
		m_hEvent = INVALID_HANDLE_VALUE;
	}
}

bool CCacheDlg::GetStatusFromRemoteCache(const CTSVNPath& Path, bool bRecursive)
{
	if(!EnsurePipeOpen())
	{
		STARTUPINFO startup;
		PROCESS_INFORMATION process;
		memset(&startup, 0, sizeof(startup));
		startup.cb = sizeof(startup);
		memset(&process, 0, sizeof(process));

		CString sCachePath = _T("TSVNCache.exe");
		if (CreateProcess(sCachePath.GetBuffer(sCachePath.GetLength()+1), _T(""), NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0)
		{
			// It's not appropriate to do a message box here, because there may be hundreds of calls
			sCachePath.ReleaseBuffer();
			ATLTRACE("Failed to start cache\n");
			return false;
		}
		sCachePath.ReleaseBuffer();

		// Wait for the cache to open
		long endTime = (long)GetTickCount()+1000;
		while(!EnsurePipeOpen())
		{
			if(((long)GetTickCount() - endTime) > 0)
			{
				return false;
			}
		}
	}


	DWORD nBytesRead;
	TSVNCacheRequest request;
	request.flags = TSVNCACHE_FLAGS_NONOTIFICATIONS;
	if(bRecursive)
	{
		request.flags |= TSVNCACHE_FLAGS_RECUSIVE_STATUS;
	}
	wcsncpy(request.path, Path.GetWinPath(), MAX_PATH);
	ZeroMemory(&m_Overlapped, sizeof(OVERLAPPED));
	m_Overlapped.hEvent = m_hEvent;
	// Do the transaction in overlapped mode.
	// That way, if anything happens which might block this call
	// we still can get out of it. We NEVER MUST BLOCK THE SHELL!
	// A blocked shell is a very bad user impression, because users
	// who don't know why it's blocked might find the only solution
	// to such a problem is a reboot and therefore they might loose
	// valuable data.
	// Sure, it would be better to have no situations where the shell
	// even can get blocked, but the timeout of 5 seconds is long enough
	// so that users still recognize that something might be wrong and
	// report back to us so we can investigate further.

	TSVNCacheResponse ReturnedStatus;
	BOOL fSuccess = TransactNamedPipe(m_hPipe,
		&request, sizeof(request),
		&ReturnedStatus, sizeof(ReturnedStatus),
		&nBytesRead, &m_Overlapped);

	if (!fSuccess)
	{
		if (GetLastError()!=ERROR_IO_PENDING)
		{
			ClosePipe();
			return false;
		}

		// TransactNamedPipe is working in an overlapped operation.
		// Wait for it to finish
		DWORD dwWait = WaitForSingleObject(m_hEvent, INFINITE);
		if (dwWait == WAIT_OBJECT_0)
		{
			fSuccess = GetOverlappedResult(m_hPipe, &m_Overlapped, &nBytesRead, FALSE);
			return TRUE;
		}
		else
			fSuccess = FALSE;
	}

	ClosePipe();
	return false;
}

void CCacheDlg::RemoveFromCache(const CString& path)
{
	// if we use the external cache, we tell the cache directly that something
	// has changed, without the detour via the shell.
	HANDLE hPipe = CreateFile( 
		TSVN_CACHE_COMMANDPIPE_NAME,	// pipe name 
		GENERIC_READ |					// read and write access 
		GENERIC_WRITE, 
		0,								// no sharing 
		NULL,							// default security attributes
		OPEN_EXISTING,					// opens existing pipe 
		FILE_FLAG_OVERLAPPED,			// default attributes 
		NULL);							// no template file 


	if (hPipe != INVALID_HANDLE_VALUE) 
	{
		// The pipe connected; change to message-read mode. 
		DWORD dwMode; 

		dwMode = PIPE_READMODE_MESSAGE; 
		if(SetNamedPipeHandleState( 
			hPipe,    // pipe handle 
			&dwMode,  // new pipe mode 
			NULL,     // don't set maximum bytes 
			NULL))    // don't set maximum time 
		{
			DWORD cbWritten; 
			TSVNCacheCommand cmd;
			cmd.command = TSVNCACHECOMMAND_CRAWL;
			wcsncpy(cmd.path, path, MAX_PATH);
			BOOL fSuccess = WriteFile( 
				hPipe,			// handle to pipe 
				&cmd,			// buffer to write from 
				sizeof(cmd),	// number of bytes to write 
				&cbWritten,		// number of bytes written 
				NULL);			// not overlapped I/O 

			if (! fSuccess || sizeof(cmd) != cbWritten)
			{
				DisconnectNamedPipe(hPipe); 
				CloseHandle(hPipe); 
				hPipe = INVALID_HANDLE_VALUE;
			}
			if (hPipe != INVALID_HANDLE_VALUE)
			{
				// now tell the cache we don't need it's command thread anymore
				DWORD cbWritten; 
				TSVNCacheCommand cmd;
				cmd.command = TSVNCACHECOMMAND_END;
				WriteFile( 
					hPipe,			// handle to pipe 
					&cmd,			// buffer to write from 
					sizeof(cmd),	// number of bytes to write 
					&cbWritten,		// number of bytes written 
					NULL);			// not overlapped I/O 
				DisconnectNamedPipe(hPipe); 
				CloseHandle(hPipe); 
				hPipe = INVALID_HANDLE_VALUE;
			}
		}
		else
		{
			ATLTRACE("SetNamedPipeHandleState failed"); 
			CloseHandle(hPipe);
		}
	}
}