// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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
#include "SVNProperties.h"
#include "UnicodeStrings.h"
#include "Utils.h"
#include "EditPropertyValueDlg.h"


// CEditPropertyValueDlg dialog

IMPLEMENT_DYNAMIC(CEditPropertyValueDlg, CResizableStandAloneDialog)

CEditPropertyValueDlg::CEditPropertyValueDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CEditPropertyValueDlg::IDD, pParent)
	, m_sPropValue(_T(""))
	, m_bRecursive(FALSE)
	, m_bFolder(false)
	, m_bMultiple(false)
	, m_bIsBinary(false)
{

}

CEditPropertyValueDlg::~CEditPropertyValueDlg()
{
}

void CEditPropertyValueDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROPNAMECOMBO, m_PropNames);
	DDX_Text(pDX, IDC_PROPVALUE, m_sPropValue);
	DDX_Check(pDX, IDC_PROPRECURSIVE, m_bRecursive);
}


BEGIN_MESSAGE_MAP(CEditPropertyValueDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDHELP, &CEditPropertyValueDlg::OnBnClickedHelp)
	ON_CBN_SELCHANGE(IDC_PROPNAMECOMBO, &CEditPropertyValueDlg::CheckRecursive)
	ON_CBN_EDITCHANGE(IDC_PROPNAMECOMBO, &CEditPropertyValueDlg::CheckRecursive)
	ON_BN_CLICKED(IDC_LOADPROP, &CEditPropertyValueDlg::OnBnClickedLoadprop)
	ON_EN_CHANGE(IDC_PROPVALUE, &CEditPropertyValueDlg::OnEnChangePropvalue)
END_MESSAGE_MAP()


// CEditPropertyValueDlg message handlers

BOOL CEditPropertyValueDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	// fill the combobox control with all the
	// known properties
	if ((!m_bFolder)&&(!m_bMultiple))
		m_PropNames.AddString(_T("svn:eol-style"));
	if ((!m_bFolder)&&(!m_bMultiple))
		m_PropNames.AddString(_T("svn:executable"));
	if ((m_bFolder)||(m_bMultiple))
		m_PropNames.AddString(_T("svn:externals"));
	if ((m_bFolder)||(m_bMultiple))
		m_PropNames.AddString(_T("svn:ignore"));
	if ((!m_bFolder)&&(!m_bMultiple))
		m_PropNames.AddString(_T("svn:keywords"));
	if ((!m_bFolder)&&(!m_bMultiple))
		m_PropNames.AddString(_T("svn:needs-lock"));
	if ((!m_bFolder)&&(!m_bMultiple))
		m_PropNames.AddString(_T("svn:mime-type"));

	if ((m_bFolder)||(m_bMultiple))
	{
		m_PropNames.AddString(_T("bugtraq:url"));
		m_PropNames.AddString(_T("bugtraq:logregex"));
		m_PropNames.AddString(_T("bugtraq:label"));
		m_PropNames.AddString(_T("bugtraq:message"));
		m_PropNames.AddString(_T("bugtraq:number"));
		m_PropNames.AddString(_T("bugtraq:warnifnoissue"));
		m_PropNames.AddString(_T("bugtraq:append"));

		m_PropNames.AddString(_T("tsvn:logtemplate"));
		m_PropNames.AddString(_T("tsvn:logwidthmarker"));
		m_PropNames.AddString(_T("tsvn:logminsize"));
		m_PropNames.AddString(_T("tsvn:logfilelistenglish"));
		m_PropNames.AddString(_T("tsvn:projectlanguage"));
	}
	else
		GetDlgItem(IDC_PROPRECURSIVE)->EnableWindow(FALSE);

	// select the pre-set property in the combobox
	bool bFound = false;
	for (int i=0; i<m_PropNames.GetCount(); ++i)
	{
		CString sText;
		m_PropNames.GetLBText(i, sText);
		if (m_sPropName.Compare(sText)==0)
		{
			m_PropNames.SetCurSel(i);
			bFound = true;
			break;
		}
	}

	if (!bFound)
	{
		m_PropNames.SetWindowText(m_sPropName);
	}

	GetDlgItem(IDC_PROPVALUE)->EnableToolTips();
	GetDlgItem(IDC_PROPNAMECOMBO)->EnableToolTips();

	m_tooltips.Create(this);

	UpdateData(FALSE);
	CheckRecursive();

	if (!m_sTitle.IsEmpty())
		SetWindowText(m_sTitle);

	AddAnchor(IDC_PROPNAME, TOP_LEFT, TOP_CENTER);
	AddAnchor(IDC_PROPNAMECOMBO, TOP_CENTER, TOP_RIGHT);
	AddAnchor(IDC_PROPVALUEGROUP, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_PROPVALUE, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_LOADPROP, BOTTOM_RIGHT);
	AddAnchor(IDC_PROPRECURSIVE, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CEditPropertyValueDlg::SetPropertyValue(const std::string& sValue)
{
	if (SVNProperties::IsBinary(sValue))
	{
		m_bIsBinary = true;
		m_sPropValue.LoadString(IDS_EDITPROPS_BINVALUE);
	}
	else
	{
		m_bIsBinary = false;
		m_sPropValue = MultibyteToWide(sValue.c_str()).c_str();
	}
}

void CEditPropertyValueDlg::OnBnClickedHelp()
{
	OnHelp();
}

void CEditPropertyValueDlg::OnCancel()
{
	m_sPropName.Empty();
	CDialog::OnCancel();
}

void CEditPropertyValueDlg::OnOK()
{
	UpdateData();
	if (!m_bIsBinary)
	{
		m_PropValue = WideToMultibyte((LPCTSTR)m_sPropValue);
	}
	m_PropNames.GetWindowText(m_sPropName);
	CDialog::OnOK();
}

void CEditPropertyValueDlg::CheckRecursive()
{
	// some properties can only be applied to files
	// if the properties are edited for a folder or
	// multiple items, then such properties must be
	// applied recursively.
	// Here, we check the property the user selected
	// and check the "recursive" checkbox automatically
	// if it needs to be set.
	int idx = m_PropNames.GetCurSel();
	if (idx >= 0)
	{
		CString sName;
		m_PropNames.GetLBText(idx, sName);
		if ((m_bFolder)||(m_bMultiple))
		{
			// folder or multiple, now check for file-only props
			if (sName.Compare(_T("svn:eol-style"))==0)
				m_bRecursive = TRUE;
			if (sName.Compare(_T("svn:executable"))==0)
				m_bRecursive = TRUE;
			if (sName.Compare(_T("svn:keywords"))==0)
				m_bRecursive = TRUE;
			if (sName.Compare(_T("svn:needs-lock"))==0)
				m_bRecursive = TRUE;
			if (sName.Compare(_T("svn:mime-type"))==0)
				m_bRecursive = TRUE;
		}
		UINT nText = 0;
		if (sName.Compare(_T("svn:externals"))==0)
			nText = IDS_TT_EXTERNALS;
		if (sName.Compare(_T("svn:executable"))==0)
			nText = IDS_TT_EXECUTABLE;
		if (sName.Compare(_T("svn:needs-lock"))==0)
			nText = IDS_TT_NEEDSLOCK;
		if (sName.Compare(_T("svn:mime-type"))==0)
			nText = IDS_TT_MIMETYPE;
		if (sName.Compare(_T("svn:ignore"))==0)
			nText = IDS_TT_IGNORE;
		if (sName.Compare(_T("svn:keywords"))==0)
			nText = IDS_TT_KEYWORDS;
		if (sName.Compare(_T("svn:eol-style"))==0)
			nText = IDS_TT_EOLSTYLE;
		if (sName.Compare(_T("bugtraq:label"))==0)
			nText = IDS_TT_BQLABEL;
		if (sName.Compare(_T("bugtraq:message"))==0)
			nText = IDS_TT_BQMESSAGE;
		if (sName.Compare(_T("bugtraq:number"))==0)
			nText = IDS_TT_BQNUMBER;
		if (sName.Compare(_T("bugtraq:url"))==0)
			nText = IDS_TT_BQURL;
		if (sName.Compare(_T("bugtraq:warnifnoissue"))==0)
			nText = IDS_TT_BQWARNNOISSUE;
		if (sName.Compare(_T("bugtraq:append"))==0)
			nText = IDS_TT_BQAPPEND;
		if (sName.Compare(_T("tsvn:logtemplate"))==0)
			nText = IDS_TT_TSVNLOGTEMPLATE;
		if (sName.Compare(_T("tsvn:logwidthmarker"))==0)
			nText = IDS_TT_TSVNLOGWIDTHMARKER;
		if (sName.Compare(_T("tsvn:logminsize"))==0)
			nText = IDS_TT_TSVNLOGMINSIZE;
		if (sName.Compare(_T("tsvn:logfilelistenglish"))==0)
			nText = IDS_TT_TSVNLOGFILELISTENGLISH;
		if (sName.Compare(_T("tsvn:projectlanguage"))==0)
			nText = IDS_TT_TSVNPROJECTLANGUAGE;

		if (nText)
		{
			m_tooltips.AddTool(IDC_PROPVALUE, nText);
			m_tooltips.AddTool(GetDlgItem(IDC_PROPNAMECOMBO)->GetWindow(GW_CHILD), nText);
		}
	}
}

BOOL CEditPropertyValueDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_RETURN:
			{
				if (GetAsyncKeyState(VK_CONTROL)&0x8000)
				{
					PostMessage(WM_COMMAND, IDOK);
				}
			}
			break;
		default:
			break;
		}
	}

	m_tooltips.RelayEvent(pMsg);
	return __super::PreTranslateMessage(pMsg);
}

void CEditPropertyValueDlg::OnBnClickedLoadprop()
{
	// now save the property value
	OPENFILENAME ofn;		// common dialog box structure
	TCHAR szFile[MAX_PATH];  // buffer for file name
	_tcscpy_s(szFile, (LPCTSTR)m_sPropName);
	CString temp;
	ZeroMemory(szFile, sizeof(szFile));
	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = m_hWnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
	temp.LoadString(IDS_REPOBROWSE_OPEN);
	CUtils::RemoveAccelerators(temp);
	if (temp.IsEmpty())
		ofn.lpstrTitle = NULL;
	else
		ofn.lpstrTitle = temp;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_EXPLORER;

	// Display the Open dialog box. 
	if (GetOpenFileName(&ofn)==FALSE)
	{
		return;
	}
	// first check the size of the file
	HANDLE hFile = CreateFile(ofn.lpstrFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD size = GetFileSize(hFile, NULL);
		FILE * stream;
		_tfopen_s(&stream, ofn.lpstrFile, _T("rbS"));
		char * buf = new char[size];
		if (fread(buf, sizeof(char), size, stream)==size)
		{
			m_PropValue.assign(buf, size);
		}
		delete [] buf;
		fclose(stream);
		// see if the loaded file contents are binary
		SetPropertyValue(m_PropValue);
		UpdateData(FALSE);
	}

}

void CEditPropertyValueDlg::OnEnChangePropvalue()
{
	UpdateData();
	CString sTemp;
	sTemp.LoadString(IDS_EDITPROPS_BINVALUE);
	if ((m_bIsBinary)&&(m_sPropValue.CompareNoCase(sTemp)!=0))
	{
		m_sPropValue.Empty();
		m_PropValue.clear();
		UpdateData(FALSE);
		m_bIsBinary = false;
	}
}

