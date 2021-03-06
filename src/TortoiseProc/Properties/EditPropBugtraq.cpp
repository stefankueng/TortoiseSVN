// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010-2011, 2013-2017, 2021 - TortoiseSVN

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
#include "EditPropBugtraq.h"
#include "UnicodeUtils.h"
#include "AppUtils.h"
#include "BugtraqRegexTestDlg.h"

// CEditPropBugtraq dialog

IMPLEMENT_DYNAMIC(CEditPropBugtraq, CResizableStandAloneDialog)

CEditPropBugtraq::CEditPropBugtraq(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CEditPropBugtraq::IDD, pParent)
    , EditPropBase()
    , m_bWarnIfNoIssue(FALSE)
{
}

CEditPropBugtraq::~CEditPropBugtraq()
{
}

void CEditPropBugtraq::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Check(pDX, IDC_PROPRECURSIVE, m_bRecursive);
    DDX_Check(pDX, IDC_BUGTRAQWARN, m_bWarnIfNoIssue);
    DDX_Text(pDX, IDC_BUGTRAQURL, m_sBugtraqUrl);
    DDX_Text(pDX, IDC_BUGTRAQMESSAGE, m_sBugtraqMessage);
    DDX_Text(pDX, IDC_BUGTRAQLABEL, m_sBugtraqLabel);
    DDX_Text(pDX, IDC_BUGTRAQLOGREGEX1, m_sBugtraqRegex1);
    DDX_Text(pDX, IDC_BUGTRAQLOGREGEX2, m_sBugtraqRegex2);
    DDX_Text(pDX, IDC_UUID32, m_sProviderUuid);
    DDX_Text(pDX, IDC_UUID64, m_sProviderUuid64);
    DDX_Text(pDX, IDC_PARAMS, m_sProviderParams);
    DDX_Control(pDX, IDC_BUGTRAQLOGREGEX1, m_bugtraqRegex1);
    DDX_Control(pDX, IDC_BUGTRAQLOGREGEX2, m_bugtraqRegex2);
}

BEGIN_MESSAGE_MAP(CEditPropBugtraq, CResizableStandAloneDialog)
    ON_BN_CLICKED(IDHELP, &CEditPropBugtraq::OnBnClickedHelp)
    ON_BN_CLICKED(IDC_TESTREGEX, &CEditPropBugtraq::OnBnClickedTestregex)
END_MESSAGE_MAP()

// CEditPropBugtraq message handlers

BOOL CEditPropBugtraq::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);
    BlockResize(DIALOG_BLOCKVERTICAL);

    ExtendFrameIntoClientArea(IDC_DWM);
    m_aeroControls.SubclassControl(this, IDC_PROPRECURSIVE);
    m_aeroControls.SubclassOkCancelHelp(this);

    CheckRadioButton(IDC_TOPRADIO, IDC_BOTTOMRADIO, IDC_BOTTOMRADIO);
    CheckRadioButton(IDC_TEXTRADIO, IDC_NUMERICRADIO, IDC_NUMERICRADIO);

    for (auto it = m_properties.begin(); it != m_properties.end(); ++it)
    {
        if (it->second.isInherited)
            continue;
        if (it->first.compare(BUGTRAQPROPNAME_URL) == 0)
        {
            m_sBugtraqUrl = CUnicodeUtils::StdGetUnicode(it->second.value).c_str();
        }
        else if (it->first.compare(BUGTRAQPROPNAME_MESSAGE) == 0)
        {
            m_sBugtraqMessage = CUnicodeUtils::StdGetUnicode(it->second.value).c_str();
        }
        else if (it->first.compare(BUGTRAQPROPNAME_LABEL) == 0)
        {
            m_sBugtraqLabel = CUnicodeUtils::StdGetUnicode(it->second.value).c_str();
        }
        else if (it->first.compare(BUGTRAQPROPNAME_PROVIDERUUID) == 0)
        {
            m_sProviderUuid = CUnicodeUtils::StdGetUnicode(it->second.value).c_str();
        }
        else if (it->first.compare(BUGTRAQPROPNAME_PROVIDERUUID64) == 0)
        {
            m_sProviderUuid64 = CUnicodeUtils::StdGetUnicode(it->second.value).c_str();
        }
        else if (it->first.compare(BUGTRAQPROPNAME_PROVIDERPARAMS) == 0)
        {
            m_sProviderParams = CUnicodeUtils::StdGetUnicode(it->second.value).c_str();
        }
        else if (it->first.compare(BUGTRAQPROPNAME_LOGREGEX) == 0)
        {
            CString sRegex = CUnicodeUtils::StdGetUnicode(it->second.value).c_str();
            int     nl     = sRegex.Find('\n');
            if (nl >= 0)
            {
                m_sBugtraqRegex1 = sRegex.Mid(nl + 1);
                m_sBugtraqRegex2 = sRegex.Left(nl);
            }
            else
                m_sBugtraqRegex1 = sRegex;
        }
        else if (it->first.compare(BUGTRAQPROPNAME_WARNIFNOISSUE) == 0)
        {
            CString sYesNo   = CUnicodeUtils::StdGetUnicode(it->second.value).c_str();
            m_bWarnIfNoIssue = ((sYesNo.CompareNoCase(L"yes") == 0) || ((sYesNo.CompareNoCase(L"true") == 0)));
        }
        else if (it->first.compare(BUGTRAQPROPNAME_APPEND) == 0)
        {
            CString sYesNo = CUnicodeUtils::StdGetUnicode(it->second.value).c_str();
            if ((sYesNo.CompareNoCase(L"no") == 0) || ((sYesNo.CompareNoCase(L"false") == 0)))
                CheckRadioButton(IDC_TOPRADIO, IDC_BOTTOMRADIO, IDC_TOPRADIO);
        }
        else if (it->first.compare(BUGTRAQPROPNAME_NUMBER) == 0)
        {
            CString sYesNo = CUnicodeUtils::StdGetUnicode(it->second.value).c_str();
            if ((sYesNo.CompareNoCase(L"no") == 0) || ((sYesNo.CompareNoCase(L"false") == 0)))
                CheckRadioButton(IDC_TEXTRADIO, IDC_NUMERICRADIO, IDC_TEXTRADIO);
        }
    }

    CString sWindowTitle;
    GetWindowText(sWindowTitle);
    CAppUtils::SetWindowTitle(m_hWnd, m_pathList.GetCommonRoot().GetUIPathString(), sWindowTitle);

    m_tooltips.AddTool(IDC_TESTREGEX, IDS_EDITPROPS_TESTREGEX_TT);
    UpdateData(false);

    AdjustControlSize(IDC_BUGTRAQWARN);
    AdjustControlSize(IDC_TEXTRADIO);
    AdjustControlSize(IDC_NUMERICRADIO);
    AdjustControlSize(IDC_TOPRADIO);
    AdjustControlSize(IDC_BOTTOMRADIO);
    AdjustControlSize(IDC_PROPRECURSIVE);

    GetDlgItem(IDC_PROPRECURSIVE)->EnableWindow(m_bFolder || m_bMultiple);
    GetDlgItem(IDC_PROPRECURSIVE)->ShowWindow(m_bRevProps || m_bRemote ? SW_HIDE : SW_SHOW);

    AddAnchor(IDC_ISSUETRACKERGROUP, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_URLLABEL, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_URLLABEL2, TOP_LEFT);
    AddAnchor(IDC_BUGTRAQURL, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_BUGTRAQWARN, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_MESSAGEGROUP, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_MESSAGEHINTLABEL, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_MESSAGEPATTERNLABEL, TOP_LEFT);
    AddAnchor(IDC_BUGTRAQMESSAGE, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_MESSAGELABEL, TOP_LEFT);
    AddAnchor(IDC_BUGTRAQLABEL, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_BUGIDLABEL, TOP_LEFT);
    AddAnchor(IDC_TEXTRADIO, TOP_LEFT);
    AddAnchor(IDC_NUMERICRADIO, TOP_LEFT);
    AddAnchor(IDC_INSERTLABEL, TOP_LEFT);
    AddAnchor(IDC_TOPRADIO, TOP_LEFT);
    AddAnchor(IDC_BOTTOMRADIO, TOP_LEFT);
    AddAnchor(IDC_REGEXGROUP, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_REGEXLABEL, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_REGEXIDLABEL, TOP_LEFT);
    AddAnchor(IDC_BUGTRAQLOGREGEX1, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_REGEXMSGLABEL, TOP_LEFT);
    AddAnchor(IDC_BUGTRAQLOGREGEX2, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_TESTREGEX, TOP_RIGHT);
    AddAnchor(IDC_IBUGTRAQPROVIDERGROUP, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_UUIDLABEL32, TOP_LEFT);
    AddAnchor(IDC_UUID32, TOP_LEFT);
    AddAnchor(IDC_UUIDLABEL64, TOP_LEFT);
    AddAnchor(IDC_UUID64, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_PARAMSLABEL, TOP_LEFT);
    AddAnchor(IDC_PARAMS, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_DWM, TOP_LEFT);
    AddAnchor(IDC_PROPRECURSIVE, BOTTOM_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDOK, BOTTOM_RIGHT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);
    AddAnchor(IDHELP, BOTTOM_RIGHT);
    EnableSaveRestore(L"EditPropBugtraq");

    GetDlgItem(IDC_BUGTRAQURL)->SetFocus();
    return FALSE;
}

void CEditPropBugtraq::OnOK()
{
    m_tooltips.Pop(); // hide the tooltips
    UpdateData();

    // check whether the entered regex strings are valid
    try
    {
        std::wregex r1 = std::wregex(m_sBugtraqRegex1);
        UNREFERENCED_PARAMETER(r1);
    }
    catch (std::exception&)
    {
        ShowEditBalloon(IDC_BUGTRAQLOGREGEX1, IDS_ERR_INVALIDREGEX, IDS_ERR_ERROR);
        return;
    }
    try
    {
        std::wregex r2 = std::wregex(m_sBugtraqRegex2);
        UNREFERENCED_PARAMETER(r2);
    }
    catch (std::exception&)
    {
        ShowEditBalloon(IDC_BUGTRAQLOGREGEX2, IDS_ERR_INVALIDREGEX, IDS_ERR_ERROR);
        return;
    }

    TProperties newProps;
    PropValue   pVal;

    // bugtraq:url
    std::string propVal = CUnicodeUtils::StdGetUTF8(static_cast<LPCWSTR>(m_sBugtraqUrl));
    pVal.value          = propVal;
    pVal.remove         = (pVal.value.empty());
    newProps.emplace(BUGTRAQPROPNAME_URL, pVal);

    // bugtraq:warnifnoissue
    if (m_bWarnIfNoIssue)
        pVal.value = "true";
    else
        pVal.value = "";
    pVal.remove = (pVal.value.empty());
    newProps.emplace(BUGTRAQPROPNAME_WARNIFNOISSUE, pVal);

    // bugtraq:message
    propVal     = CUnicodeUtils::StdGetUTF8(static_cast<LPCWSTR>(m_sBugtraqMessage));
    pVal.value  = propVal;
    pVal.remove = (pVal.value.empty());
    newProps.emplace(BUGTRAQPROPNAME_MESSAGE, pVal);

    // bugtraq:label
    propVal     = CUnicodeUtils::StdGetUTF8(static_cast<LPCWSTR>(m_sBugtraqLabel));
    pVal.value  = propVal;
    pVal.remove = (pVal.value.empty());
    newProps.emplace(BUGTRAQPROPNAME_LABEL, pVal);

    // bugtraq:number
    int checked = GetCheckedRadioButton(IDC_TEXTRADIO, IDC_NUMERICRADIO);
    if (checked == IDC_TEXTRADIO)
        pVal.value = "false";
    else
        pVal.value.clear();
    pVal.remove = (pVal.value.empty());
    newProps.emplace(BUGTRAQPROPNAME_NUMBER, pVal);

    // bugtraq:append
    checked = GetCheckedRadioButton(IDC_TOPRADIO, IDC_BOTTOMRADIO);
    if (checked == IDC_TOPRADIO)
        pVal.value = "false";
    else
        pVal.value.clear();
    pVal.remove = (pVal.value.empty());
    newProps.emplace(BUGTRAQPROPNAME_APPEND, pVal);

    // bugtraq:logregex
    CString sLogRegex = m_sBugtraqRegex2 + L"\n" + m_sBugtraqRegex1;
    if (m_sBugtraqRegex1.IsEmpty() && m_sBugtraqRegex2.IsEmpty())
        sLogRegex.Empty();
    if (m_sBugtraqRegex2.IsEmpty() && !m_sBugtraqRegex1.IsEmpty())
        sLogRegex = m_sBugtraqRegex1;
    if (m_sBugtraqRegex1.IsEmpty() && !m_sBugtraqRegex2.IsEmpty())
        sLogRegex = m_sBugtraqRegex2;
    propVal     = CUnicodeUtils::StdGetUTF8(static_cast<LPCWSTR>(sLogRegex));
    pVal.value  = propVal;
    pVal.remove = (pVal.value.empty());
    newProps.emplace(BUGTRAQPROPNAME_LOGREGEX, pVal);

    // bugtraq:providerparams
    propVal     = CUnicodeUtils::StdGetUTF8(static_cast<LPCWSTR>(m_sProviderParams));
    pVal.value  = propVal;
    pVal.remove = (pVal.value.empty());
    newProps.emplace(BUGTRAQPROPNAME_PROVIDERPARAMS, pVal);

    // bugtraq:provideruuid
    propVal     = CUnicodeUtils::StdGetUTF8(static_cast<LPCWSTR>(m_sProviderUuid));
    pVal.value  = propVal;
    pVal.remove = (pVal.value.empty());
    newProps.emplace(BUGTRAQPROPNAME_PROVIDERUUID, pVal);

    // bugtraq:provideruuid64
    propVal     = CUnicodeUtils::StdGetUTF8(static_cast<LPCWSTR>(m_sProviderUuid64));
    pVal.value  = propVal;
    pVal.remove = (pVal.value.empty());
    newProps.emplace(BUGTRAQPROPNAME_PROVIDERUUID64, pVal);

    m_bChanged = true;

    m_properties = newProps;

    CResizableStandAloneDialog::OnOK();
}

void CEditPropBugtraq::OnBnClickedHelp()
{
    OnHelp();
}

void CEditPropBugtraq::OnBnClickedTestregex()
{
    m_tooltips.Pop(); // hide the tooltips
    CBugtraqRegexTestDlg dlg(this);
    dlg.m_sBugtraqRegex1 = m_sBugtraqRegex1;
    dlg.m_sBugtraqRegex2 = m_sBugtraqRegex2;
    if (dlg.DoModal() == IDOK)
    {
        m_sBugtraqRegex1 = dlg.m_sBugtraqRegex1;
        m_sBugtraqRegex2 = dlg.m_sBugtraqRegex2;
        UpdateData(FALSE);
    }
}
