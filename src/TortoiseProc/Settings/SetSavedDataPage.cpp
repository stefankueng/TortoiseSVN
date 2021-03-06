// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2015, 2021 - TortoiseSVN

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
#include "registry.h"
#include "PathUtils.h"
#include "AppUtils.h"
#include "DirFileEnum.h"
#include "SetSavedDataPage.h"
#include "StringUtils.h"
#include "SettingsClearAuth.h"

IMPLEMENT_DYNAMIC(CSetSavedDataPage, ISettingsPropPage)

CSetSavedDataPage::CSetSavedDataPage()
    : ISettingsPropPage(CSetSavedDataPage::IDD)
    , m_maxLines(0)
{
    m_regMaxLines = CRegDWORD(L"Software\\TortoiseSVN\\MaxLinesInLogfile", 4000);
    m_maxLines    = m_regMaxLines;
}

CSetSavedDataPage::~CSetSavedDataPage()
{
}

void CSetSavedDataPage::DoDataExchange(CDataExchange* pDX)
{
    ISettingsPropPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_URLHISTCLEAR, m_btnUrlHistClear);
    DDX_Control(pDX, IDC_LOGHISTCLEAR, m_btnLogHistClear);
    DDX_Control(pDX, IDC_RESIZABLEHISTCLEAR, m_btnResizableHistClear);
    DDX_Control(pDX, IDC_AUTHHISTCLEAR, m_btnAuthHistClear);
    DDX_Control(pDX, IDC_AUTHHISTCLEARSELECT, m_btnAuthHistClearSelect);
    DDX_Control(pDX, IDC_REPOLOGCLEAR, m_btnRepoLogClear);
    DDX_Text(pDX, IDC_MAXLINES, m_maxLines);
    DDX_Control(pDX, IDC_ACTIONLOGSHOW, m_btnActionLogShow);
    DDX_Control(pDX, IDC_ACTIONLOGCLEAR, m_btnActionLogClear);
    DDX_Control(pDX, IDC_HOOKCLEAR, m_btnHookClear);
}

BOOL CSetSavedDataPage::OnInitDialog()
{
    ISettingsPropPage::OnInitDialog();

    // find out how many log messages and URLs we've stored
    int          nLogHistWC    = 0;
    INT_PTR      nLogHistMsg   = 0;
    int          nUrlHistWC    = 0;
    INT_PTR      nUrlHistItems = 0;
    int          nLogHistRepo  = 0;
    CRegistryKey regLogHist(L"Software\\TortoiseSVN\\History");
    CStringList  logHistList;
    regLogHist.getSubKeys(logHistList);
    for (POSITION pos = logHistList.GetHeadPosition(); pos != nullptr;)
    {
        CString sHistName = logHistList.GetNext(pos);
        if (sHistName.Left(6).CompareNoCase(L"commit") == 0)
        {
            nLogHistWC++;
            CRegistryKey regLogHistWc(L"Software\\TortoiseSVN\\History\\" + sHistName);
            CStringList  logHistListWc;
            regLogHistWc.getValues(logHistListWc);
            nLogHistMsg += logHistListWc.GetCount();
        }
        else
        {
            // repoURLs
            CStringList  urlHistListMain;
            CStringList  urlHistListMainValues;
            CRegistryKey regUrlHistList(L"Software\\TortoiseSVN\\History\\repoURLS");
            regUrlHistList.getSubKeys(urlHistListMain);
            regUrlHistList.getValues(urlHistListMainValues);
            nUrlHistItems += urlHistListMainValues.GetCount();
            for (POSITION urlPos = urlHistListMain.GetHeadPosition(); urlPos != nullptr;)
            {
                CString sWcUid = urlHistListMain.GetNext(urlPos);
                nUrlHistWC++;
                CStringList  urlHistListWc;
                CRegistryKey regUrlHistListWc(L"Software\\TortoiseSVN\\History\\repoURLS\\" + sWcUid);
                regUrlHistListWc.getValues(urlHistListWc);
                nUrlHistItems += urlHistListWc.GetCount();
            }
        }
    }

    // find out how many dialog sizes / positions we've stored
    INT_PTR      nResizableDialogs = 0;
    CRegistryKey regResizable(L"Software\\TortoiseSVN\\TortoiseProc\\ResizableState");
    CStringList  resizableList;
    regResizable.getValues(resizableList);
    nResizableDialogs += resizableList.GetCount();

    // find out how many auth data we've stored
    int nSimple   = 0;
    int nSSL      = 0;
    int nUsername = 0;

    CString sFile;
    bool    bIsDir = false;

    PWSTR pszPath = nullptr;
    if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, nullptr, &pszPath) == S_OK)
    {
        CString path = pszPath;
        CoTaskMemFree(pszPath);

        path += L"\\Subversion\\auth\\";

        CString      sSimple   = path + L"svn.simple";
        CString      sSSL      = path + L"svn.ssl.server";
        CString      sUsername = path + L"svn.username";
        CDirFileEnum simpleEnum(sSimple);
        while (simpleEnum.NextFile(sFile, &bIsDir))
            nSimple++;
        CDirFileEnum sslEnum(sSSL);
        while (sslEnum.NextFile(sFile, &bIsDir))
            nSSL++;
        CDirFileEnum userEnum(sUsername);
        while (userEnum.NextFile(sFile, &bIsDir))
            nUsername++;
    }

    CRegistryKey regCerts(L"Software\\TortoiseSVN\\CAPIAuthz");
    CStringList  certList;
    regCerts.getValues(certList);
    int nCapi = static_cast<int>(certList.GetCount());

    CDirFileEnum logenum(CPathUtils::GetAppDataDirectory() + L"logcache");
    while (logenum.NextFile(sFile, &bIsDir))
        nLogHistRepo++;
    // the "Repositories.dat" is not a cache file
    nLogHistRepo--;

    BOOL bActionLog = PathFileExists(CPathUtils::GetLocalAppDataDirectory() + L"logfile.txt");

    INT_PTR      nHooks = 0;
    CRegistryKey regHooks(L"Software\\TortoiseSVN\\approvedhooks");
    CStringList  hooksList;
    regHooks.getValues(hooksList);
    nHooks += hooksList.GetCount();

    DialogEnableWindow(&m_btnLogHistClear, nLogHistMsg || nLogHistWC);
    DialogEnableWindow(&m_btnUrlHistClear, nUrlHistItems || nUrlHistWC);
    DialogEnableWindow(&m_btnResizableHistClear, nResizableDialogs > 0);
    DialogEnableWindow(&m_btnAuthHistClear, nSimple || nSSL || nUsername || nCapi);
    DialogEnableWindow(&m_btnAuthHistClearSelect, nSimple || nSSL || nUsername || nCapi);
    DialogEnableWindow(&m_btnRepoLogClear, nLogHistRepo >= 0);
    DialogEnableWindow(&m_btnActionLogClear, bActionLog);
    DialogEnableWindow(&m_btnActionLogShow, bActionLog);
    DialogEnableWindow(&m_btnHookClear, nHooks > 0);

    EnableToolTips();

    CString sTT;
    sTT.FormatMessage(IDS_SETTINGS_SAVEDDATA_LOGHIST_TT, nLogHistMsg, nLogHistWC);
    m_tooltips.AddTool(IDC_LOGHISTORY, sTT);
    m_tooltips.AddTool(IDC_LOGHISTCLEAR, sTT);
    sTT.FormatMessage(IDS_SETTINGS_SAVEDDATA_URLHIST_TT, nUrlHistItems, nUrlHistWC);
    m_tooltips.AddTool(IDC_URLHISTORY, sTT);
    m_tooltips.AddTool(IDC_URLHISTCLEAR, sTT);
    sTT.Format(IDS_SETTINGS_SAVEDDATA_RESIZABLE_TT, nResizableDialogs);
    m_tooltips.AddTool(IDC_RESIZABLEHISTORY, sTT);
    m_tooltips.AddTool(IDC_RESIZABLEHISTCLEAR, sTT);
    sTT.FormatMessage(IDS_SETTINGS_SAVEDDATA_AUTH_TT, nSimple, nSSL + nCapi, nUsername);
    m_tooltips.AddTool(IDC_AUTHHISTORY, sTT);
    m_tooltips.AddTool(IDC_AUTHHISTCLEAR, sTT);
    sTT.Format(IDS_SETTINGS_SAVEDDATA_REPOLOGHIST_TT, nLogHistRepo);
    m_tooltips.AddTool(IDC_REPOLOG, sTT);
    m_tooltips.AddTool(IDC_REPOLOGCLEAR, sTT);
    sTT.LoadString(IDS_SETTINGS_SHOWACTIONLOG_TT);
    m_tooltips.AddTool(IDC_ACTIONLOGSHOW, sTT);
    sTT.LoadString(IDS_SETTINGS_MAXACTIONLOGLINES_TT);
    m_tooltips.AddTool(IDC_MAXLINES, sTT);
    sTT.LoadString(IDS_SETTINGS_CLEARACTIONLOG_TT);
    m_tooltips.AddTool(IDC_ACTIONLOGCLEAR, sTT);
    sTT.Format(IDS_SETTINGS_CLEARHOOKS_TT, nHooks);
    m_tooltips.AddTool(IDC_HOOKCLEAR, sTT);

    return TRUE;
}

BEGIN_MESSAGE_MAP(CSetSavedDataPage, ISettingsPropPage)
    ON_BN_CLICKED(IDC_URLHISTCLEAR, &CSetSavedDataPage::OnBnClickedUrlhistclear)
    ON_BN_CLICKED(IDC_LOGHISTCLEAR, &CSetSavedDataPage::OnBnClickedLoghistclear)
    ON_BN_CLICKED(IDC_RESIZABLEHISTCLEAR, &CSetSavedDataPage::OnBnClickedResizablehistclear)
    ON_BN_CLICKED(IDC_AUTHHISTCLEAR, &CSetSavedDataPage::OnBnClickedAuthhistclear)
    ON_BN_CLICKED(IDC_REPOLOGCLEAR, &CSetSavedDataPage::OnBnClickedRepologclear)
    ON_BN_CLICKED(IDC_ACTIONLOGSHOW, &CSetSavedDataPage::OnBnClickedActionlogshow)
    ON_BN_CLICKED(IDC_ACTIONLOGCLEAR, &CSetSavedDataPage::OnBnClickedActionlogclear)
    ON_EN_CHANGE(IDC_MAXLINES, OnModified)
    ON_BN_CLICKED(IDC_HOOKCLEAR, &CSetSavedDataPage::OnBnClickedHookclear)
    ON_BN_CLICKED(IDC_AUTHHISTCLEARSELECT, &CSetSavedDataPage::OnBnClickedAuthhistclearselect)
END_MESSAGE_MAP()

void CSetSavedDataPage::OnBnClickedUrlhistclear()
{
    CRegistryKey reg(L"Software\\TortoiseSVN\\History\\repoURLS");
    reg.removeKey();
    DialogEnableWindow(&m_btnUrlHistClear, false);
    m_tooltips.DelTool(GetDlgItem(IDC_URLHISTCLEAR));
    m_tooltips.DelTool(GetDlgItem(IDC_URLHISTORY));
}

void CSetSavedDataPage::OnBnClickedLoghistclear()
{
    CRegistryKey reg(L"Software\\TortoiseSVN\\History");
    CStringList  histList;
    reg.getSubKeys(histList);
    for (POSITION pos = histList.GetHeadPosition(); pos != nullptr;)
    {
        CString sHist = histList.GetNext(pos);
        if (sHist.Left(6).CompareNoCase(L"commit") == 0)
        {
            CRegistryKey regKey(L"Software\\TortoiseSVN\\History\\" + sHist);
            regKey.removeKey();
        }
    }

    DialogEnableWindow(&m_btnLogHistClear, false);
    m_tooltips.DelTool(GetDlgItem(IDC_RESIZABLEHISTCLEAR));
    m_tooltips.DelTool(GetDlgItem(IDC_RESIZABLEHISTORY));
}

void CSetSavedDataPage::OnBnClickedResizablehistclear()
{
    CRegistryKey reg(L"Software\\TortoiseSVN\\TortoiseProc\\ResizableState");
    reg.removeKey();
    DialogEnableWindow(&m_btnResizableHistClear, false);
    m_tooltips.DelTool(GetDlgItem(IDC_RESIZABLEHISTCLEAR));
    m_tooltips.DelTool(GetDlgItem(IDC_RESIZABLEHISTORY));
}

void CSetSavedDataPage::OnBnClickedHookclear()
{
    CRegistryKey reg(L"Software\\TortoiseSVN\\approvedhooks");
    reg.removeKey();
    DialogEnableWindow(&m_btnHookClear, false);
    m_tooltips.DelTool(GetDlgItem(IDC_HOOKCLEAR));
    m_tooltips.DelTool(GetDlgItem(IDC_HOOKS));
}

void CSetSavedDataPage::OnBnClickedAuthhistclear()
{
    CRegStdString auth = CRegStdString(L"Software\\TortoiseSVN\\Auth\\");
    auth.removeKey();

    PWSTR pszPath = nullptr;
    if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, nullptr, &pszPath) == S_OK)
    {
        CString path = pszPath;
        CoTaskMemFree(pszPath);
        path += L"\\Subversion\\auth\\";
        DeleteViaShell(path, IDS_SETTINGS_DELFILE);
    }
    SHDeleteKey(HKEY_CURRENT_USER, L"Software\\TortoiseSVN\\CAPIAuthz");
    DialogEnableWindow(&m_btnAuthHistClear, false);
    DialogEnableWindow(&m_btnAuthHistClearSelect, false);
    m_tooltips.DelTool(GetDlgItem(IDC_AUTHHISTCLEAR));
    m_tooltips.DelTool(GetDlgItem(IDC_AUTHHISTORY));
}

void CSetSavedDataPage::OnBnClickedRepologclear()
{
    CString path              = CPathUtils::GetAppDataDirectory() + L"logcache";
    wchar_t pathbuf[MAX_PATH] = {0};
    wcscpy_s(pathbuf, static_cast<LPCWSTR>(path));
    pathbuf[wcslen(pathbuf) + 1] = 0;

    DeleteViaShell(pathbuf, IDS_SETTINGS_DELCACHE);

    DialogEnableWindow(&m_btnRepoLogClear, false);
    m_tooltips.DelTool(GetDlgItem(IDC_REPOLOG));
    m_tooltips.DelTool(GetDlgItem(IDC_REPOLOGCLEAR));
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void CSetSavedDataPage::OnBnClickedActionlogshow()
{
    CString logfile = CPathUtils::GetLocalAppDataDirectory() + L"logfile.txt";
    CAppUtils::StartTextViewer(logfile);
}

void CSetSavedDataPage::OnBnClickedActionlogclear()
{
    CString logfile = CPathUtils::GetLocalAppDataDirectory() + L"logfile.txt";
    DeleteFile(logfile);
    DialogEnableWindow(&m_btnActionLogClear, false);
    DialogEnableWindow(&m_btnActionLogShow, false);
}

void CSetSavedDataPage::OnModified()
{
    SetModified();
}

BOOL CSetSavedDataPage::OnApply()
{
    Store(m_maxLines, m_regMaxLines);
    return ISettingsPropPage::OnApply();
}

void CSetSavedDataPage::DeleteViaShell(LPCWSTR path, UINT progressText) const
{
    CString p(path);
    p += L"||";
    int  len = p.GetLength();
    auto buf = std::make_unique<wchar_t[]>(len + 2LL);
    wcscpy_s(buf.get(), len + 2LL, p);
    CStringUtils::PipesToNulls(buf.get(), len);

    CString        progText(MAKEINTRESOURCE(progressText));
    SHFILEOPSTRUCT fileOp;
    fileOp.hwnd              = m_hWnd;
    fileOp.wFunc             = FO_DELETE;
    fileOp.pFrom             = buf.get();
    fileOp.pTo               = nullptr;
    fileOp.fFlags            = FOF_NO_CONNECTED_ELEMENTS | FOF_NOCONFIRMATION;
    fileOp.lpszProgressTitle = progText;
    SHFileOperation(&fileOp);
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void CSetSavedDataPage::OnBnClickedAuthhistclearselect()
{
    CSettingsClearAuth dlg;
    dlg.DoModal();
}
