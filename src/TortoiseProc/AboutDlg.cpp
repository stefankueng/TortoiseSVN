// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2014 - TortoiseSVN

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
#include "TortoiseProc.h"
#include "AboutDlg.h"
#include "svn_version.h"
#include "../version.h"
#include "../../ext/serf/serf.h"
#include "../../ext/sqlite/sqlite3.h"
#include "AppUtils.h"

IMPLEMENT_DYNAMIC(CAboutDlg, CStandAloneDialog)
CAboutDlg::CAboutDlg(CWnd* pParent /*=NULL*/)
    : CStandAloneDialog(CAboutDlg::IDD, pParent)
{
}

CAboutDlg::~CAboutDlg()
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_WEBLINK, m_cWebLink);
    DDX_Control(pDX, IDC_SUPPORTLINK, m_cSupportLink);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CStandAloneDialog)
    ON_WM_TIMER()
    ON_WM_MOUSEMOVE()
    ON_BN_CLICKED(IDC_UPDATE, OnBnClickedUpdate)
    ON_WM_CLOSE()
END_MESSAGE_MAP()

BOOL CAboutDlg::OnInitDialog()
{
    CStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    ExtendFrameIntoClientArea(IDC_VERSIONBOX);
    m_aeroControls.SubclassControl(this, IDC_CREDITS1);
    m_aeroControls.SubclassControl(this, IDC_CREDITS2);
    m_aeroControls.SubclassControl(this, IDOK);
    m_aeroControls.SubclassControl(this, IDC_UPDATE);

    // set the version string
    CString temp;
    const svn_version_t * svnver = svn_client_version();

    CString svnvertag = CString(svnver->tag);
    if (svnvertag.IsEmpty())
    {
        svnvertag = L"-release";
    }
    CString sIPV6;

#if APR_HAVE_IPV6
    sIPV6 = L"\r\nipv6 enabled";
#endif

    temp.Format(IDS_ABOUTVERSION, TSVN_VERMAJOR, TSVN_VERMINOR, TSVN_VERMICRO, TSVN_VERBUILD, _T(TSVN_PLATFORM), _T(TSVN_VERDATE), (LPCWSTR)sIPV6,
        svnver->major, svnver->minor, svnver->patch, (LPCWSTR)svnvertag,
        APR_MAJOR_VERSION, APR_MINOR_VERSION, APR_PATCH_VERSION,
        APU_MAJOR_VERSION, APU_MINOR_VERSION, APU_PATCH_VERSION,
        SERF_MAJOR_VERSION, SERF_MINOR_VERSION, SERF_PATCH_VERSION,
        _T(OPENSSL_VERSION_TEXT),
        _T(ZLIB_VERSION),
        _T(SQLITE_VERSION));
    SetDlgItemText(IDC_VERSIONABOUT, temp);
    this->SetWindowText(L"TortoiseSVN");

    CPictureHolder tmpPic;
    tmpPic.CreateFromBitmap(IDB_LOGOFLIPPED);
    m_renderSrc.Create32BitFromPicture(&tmpPic,468,64);
    m_renderDest.Create32BitFromPicture(&tmpPic,468,64);

    m_waterEffect.Create(468,64);
    SetTimer(ID_EFFECTTIMER, 40, NULL);
    SetTimer(ID_DROPTIMER, 1500, NULL);

    m_cWebLink.SetURL(L"http://tortoisesvn.net");
    m_cSupportLink.SetURL(L"http://tortoisesvn.net/donate.html");

    CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    GetDlgItem(IDOK)->SetFocus();
    return FALSE;
}

void CAboutDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == ID_EFFECTTIMER)
    {
        m_waterEffect.Render((DWORD*)m_renderSrc.GetDIBits(), (DWORD*)m_renderDest.GetDIBits());
        CClientDC dc(this);
        CPoint ptOrigin(15,20);
        m_renderDest.Draw(&dc,ptOrigin);
    }
    if (nIDEvent == ID_DROPTIMER)
    {
        CRect r;
        r.left = 15;
        r.top = 20;
        r.right = r.left + m_renderSrc.GetWidth();
        r.bottom = r.top + m_renderSrc.GetHeight();
        m_waterEffect.Blob(random(r.left,r.right), random(r.top, r.bottom), 5, 800, m_waterEffect.m_iHpage);
    }
    CStandAloneDialog::OnTimer(nIDEvent);
}

void CAboutDlg::OnMouseMove(UINT nFlags, CPoint point)
{
    CRect r;
    r.left = 15;
    r.top = 20;
    r.right = r.left + m_renderSrc.GetWidth();
    r.bottom = r.top + m_renderSrc.GetHeight();

    if(r.PtInRect(point) == TRUE)
    {
        // dibs are drawn upside down...
        point.y -= 20;
        point.y = 64-point.y;

        if (nFlags & MK_LBUTTON)
            m_waterEffect.Blob(point.x -15,point.y,10,1600,m_waterEffect.m_iHpage);
        else
            m_waterEffect.Blob(point.x -15,point.y,5,50,m_waterEffect.m_iHpage);

    }


    CStandAloneDialog::OnMouseMove(nFlags, point);
}

void CAboutDlg::OnBnClickedUpdate()
{
    TCHAR com[MAX_PATH + 100] = { 0 };
    GetModuleFileName(NULL, com, MAX_PATH);

    CCreateProcessHelper::CreateProcessDetached(com, L" /command:updatecheck /visible");
}

void CAboutDlg::OnClose()
{
    KillTimer(ID_EFFECTTIMER);
    KillTimer(ID_DROPTIMER);

    CStandAloneDialog::OnClose();
}
