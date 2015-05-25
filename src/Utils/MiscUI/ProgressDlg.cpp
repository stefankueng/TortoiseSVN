// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2008-2012, 2014-2015 - TortoiseSVN

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
#include "ProgressDlg.h"

CProgressDlg::CProgressDlg()
    : m_pIDlg(NULL)
    , m_isVisible(false)
    , m_dwDlgFlags(PROGDLG_NORMAL)
    , m_hWndProgDlg(NULL)
    , m_hWndParent(NULL)
    , m_hWndFocus(NULL)
{
    EnsureValid();
}

CProgressDlg::~CProgressDlg()
{
    if (IsValid())
    {
        if (m_isVisible)            //still visible, so stop first before destroying
            m_pIDlg->StopProgressDialog();

        m_pIDlg = nullptr;
        m_hWndProgDlg = NULL;
    }
}

bool CProgressDlg::EnsureValid()
{
    if(IsValid())
        return true;

    HRESULT hr = m_pIDlg.CoCreateInstance (CLSID_ProgressDialog, NULL, CLSCTX_INPROC_SERVER);
    return (SUCCEEDED(hr));
}

void CProgressDlg::SetTitle(LPCTSTR szTitle)
{
    USES_CONVERSION;
    if (IsValid())
    {
        m_pIDlg->SetTitle(T2COLE(szTitle));
    }
}
void CProgressDlg::SetTitle ( UINT idTitle)
{
    SetTitle(CString(MAKEINTRESOURCE(idTitle)));
}

void CProgressDlg::SetLine(DWORD dwLine, LPCTSTR szText, bool bCompactPath /* = false */)
{
    USES_CONVERSION;
    if (IsValid())
    {
        m_pIDlg->SetLine(dwLine, T2COLE(szText), bCompactPath, NULL);
    }
}

#ifdef _MFC_VER
void CProgressDlg::SetCancelMsg ( UINT idMessage )
{
    SetCancelMsg(CString(MAKEINTRESOURCE(idMessage)));
}
#endif // _MFC_VER

void CProgressDlg::SetCancelMsg(LPCTSTR szMessage)
{
    USES_CONVERSION;
    if (IsValid())
    {
        m_pIDlg->SetCancelMsg(T2COLE(szMessage), NULL);
    }
}

void CProgressDlg::SetAnimation(HINSTANCE hinst, UINT uRsrcID)
{
    if (IsValid())
    {
        m_pIDlg->SetAnimation(hinst, uRsrcID);
    }
}
#ifdef _MFC_VER
void CProgressDlg::SetAnimation(UINT uRsrcID)
{
    if (IsValid())
    {
        m_pIDlg->SetAnimation(AfxGetResourceHandle(), uRsrcID);
    }
}
#endif
void CProgressDlg::SetTime(bool bTime /* = true */)
{
    m_dwDlgFlags &= ~(PROGDLG_NOTIME | PROGDLG_AUTOTIME);

    if (bTime)
        m_dwDlgFlags |= PROGDLG_AUTOTIME;
    else
        m_dwDlgFlags |= PROGDLG_NOTIME;
}

void CProgressDlg::SetShowProgressBar(bool bShow /* = true */)
{
    if (bShow)
        m_dwDlgFlags &= ~PROGDLG_NOPROGRESSBAR;
    else
        m_dwDlgFlags |= PROGDLG_NOPROGRESSBAR;
}
#ifdef _MFC_VER
HRESULT CProgressDlg::ShowModal (CWnd* pwndParent, BOOL immediately /* = true */)
{
  EnsureValid();
  return ShowModal(pwndParent->GetSafeHwnd(), immediately);
}

HRESULT CProgressDlg::ShowModeless(CWnd* pwndParent, BOOL immediately)
{
    EnsureValid();
    return ShowModeless(pwndParent->GetSafeHwnd(), immediately);
}

void CProgressDlg::FormatPathLine ( DWORD dwLine, UINT idFormatText, ...)
{
    va_list args;
    va_start(args, idFormatText);

    CString sText;
    sText.FormatMessageV(CString(MAKEINTRESOURCE(idFormatText)), &args);
    SetLine(dwLine, sText, true);

    va_end(args);
}

void CProgressDlg::FormatNonPathLine(DWORD dwLine, UINT idFormatText, ...)
{
    va_list args;
    va_start(args, idFormatText);

    CString sText;
    sText.FormatMessageV(CString(MAKEINTRESOURCE(idFormatText)), &args);
    SetLine(dwLine, sText, false);

    va_end(args);
}


#endif
HRESULT CProgressDlg::ShowModal(HWND hWndParent, BOOL immediately /* = true */)
{
    EnsureValid();
    m_hWndProgDlg = NULL;
    if (!IsValid())
        return E_FAIL;
    m_hWndParent = hWndParent;
    auto winId = GetWindowThreadProcessId(m_hWndParent, 0);
    auto threadId = GetCurrentThreadId();
    if (winId != threadId)
        AttachThreadInput(winId, threadId, TRUE);
    m_hWndFocus = GetFocus();
    if (winId != threadId)
        AttachThreadInput(winId, threadId, FALSE);
    HRESULT hr = m_pIDlg->StartProgressDialog(hWndParent, NULL, m_dwDlgFlags | PROGDLG_MODAL, NULL);
    if(FAILED(hr))
        return hr;

    ATL::CComPtr<IOleWindow> pOleWindow;
    HRESULT hr2 = m_pIDlg.QueryInterface(&pOleWindow);
    if(SUCCEEDED(hr2))
    {
        hr2 = pOleWindow->GetWindow(&m_hWndProgDlg);
        if(SUCCEEDED(hr2))
        {
            if(immediately)
                ShowWindow(m_hWndProgDlg, SW_SHOW);
        }
    }

    m_isVisible = true;
    return hr;
}

HRESULT CProgressDlg::ShowModeless(HWND hWndParent, BOOL immediately)
{
    EnsureValid();
    m_hWndProgDlg = NULL;
    if (!IsValid())
        return E_FAIL;
    m_hWndParent = hWndParent;
    auto winId = GetWindowThreadProcessId(m_hWndParent, 0);
    auto threadId = GetCurrentThreadId();
    if (winId != threadId)
        AttachThreadInput(winId, threadId, TRUE);
    m_hWndFocus = GetFocus();
    if (winId != threadId)
        AttachThreadInput(winId, threadId, FALSE);
    HRESULT hr = m_pIDlg->StartProgressDialog(hWndParent, NULL, m_dwDlgFlags, NULL);
    if(FAILED(hr))
        return hr;

    ATL::CComPtr<IOleWindow> pOleWindow;
    HRESULT hr2 = m_pIDlg.QueryInterface(&pOleWindow);
    if(SUCCEEDED(hr2))
    {
        hr2 = pOleWindow->GetWindow(&m_hWndProgDlg);
        if(SUCCEEDED(hr2))
        {
            if (immediately)
                ShowWindow(m_hWndProgDlg, SW_SHOW);
        }
    }
    m_isVisible = true;
    return hr;
}

void CProgressDlg::SetProgress(DWORD dwProgress, DWORD dwMax)
{
    if (IsValid())
    {
        m_pIDlg->SetProgress(dwProgress, dwMax);
    }
}

void CProgressDlg::SetProgress64(ULONGLONG u64Progress, ULONGLONG u64ProgressMax)
{
    if (IsValid())
    {
        m_pIDlg->SetProgress64(u64Progress, u64ProgressMax);
    }
}

bool CProgressDlg::HasUserCancelled()
{
    if (!IsValid())
        return false;

    return (0 != m_pIDlg->HasUserCancelled());
}

void CProgressDlg::Stop()
{
    if ((m_isVisible)&&(IsValid()))
    {
        m_pIDlg->StopProgressDialog();
        // Sometimes the progress dialog sticks around after stopping it,
        // until the mouse pointer is moved over it or some other triggers.
        // We hide the window here immediately.
        if (m_hWndProgDlg)
        {
            // The progress dialog is handled on a separate thread, which means
            // even calling StopProgressDialog() will not stop it immediately.
            // Even destroying the progress window is not enough.
            // Which can cause problems with modality: if we stop the progress
            // dialog and immediately show e.g. a messagebox over the same
            // window the progress dialog has as its parent, then the messagebox
            // is modal first (deactivates the parent), but then a little bit
            // later the progress dialog finally closes properly, and by doing that
            // re-enables its parent window.
            // Which leads to the parent window being enabled even though
            // the modal messagebox is shown over the parent window.
            // This situation can even lead to the messagebox appearing *behind*
            // the parent window (race condition)
            //
            // So, to really ensure that the progress dialog is fully stopped
            // and destroyed, we have to attach to its UI thread and handle
            // all messages until there are no more messages: that's when
            // the progress dialog is really gone.
            AttachThreadInput(GetWindowThreadProcessId(m_hWndProgDlg, 0), GetCurrentThreadId(), TRUE);
            // StartProgressDialog creates a new thread to host the progress window.
            // When the window receives WM_DESTROY message StopProgressDialog() wrongly
            // attempts to re-enable the parent in the calling thread (our thread),
            // after the progress window is destroyed and the progress thread has died.
            // When the progress window dies, the system tries to assign a new foreground window.
            // It cannot assign to hwndParent because StartProgressDialog (w/PROGDLG_MODAL) disabled the parent window.
            // So the system hands the foreground activation to the next process that wants it in the
            // system foreground queue. Thus we lose our right to recapture the foreground window.
            // To fix this problem, we enable the parent window and set to focus to it here, after
            // we've attached to the window thread.
            ShowWindow(m_hWndProgDlg, SW_HIDE);
            EnableWindow(m_hWndParent, TRUE);
            if (m_hWndFocus)
                SetFocus(m_hWndFocus);
            else
                SetFocus(m_hWndParent);
            auto start = GetTickCount64();
            while (::IsWindow(m_hWndProgDlg) && ((GetTickCount64() - start) < 3000))
            {
                MSG msg = { 0 };
                while (PeekMessage(&msg, m_hWndProgDlg, 0, 0, PM_REMOVE))
                {
                }
            }
        }
        m_isVisible = false;
        m_pIDlg = nullptr;
        m_hWndProgDlg = NULL;
    }
}

void CProgressDlg::ResetTimer()
{
    if (IsValid())
    {
        m_pIDlg->Timer(PDTIMER_RESET, NULL);
    }
}

