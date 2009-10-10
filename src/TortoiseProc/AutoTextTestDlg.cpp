// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009 - TortoiseSVN

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
#include "AutoTextTestDlg.h"
#include "HighResClock.h"
#include <regex>
#include <string>

using namespace std;
// CAutoTextTestDlg dialog

IMPLEMENT_DYNAMIC(CAutoTextTestDlg, CDialog)

CAutoTextTestDlg::CAutoTextTestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAutoTextTestDlg::IDD, pParent)
	, m_sRegex(_T(""))
	, m_sResult(_T(""))
	, m_sTimingLabel(_T(""))
{

}

CAutoTextTestDlg::~CAutoTextTestDlg()
{
}

void CAutoTextTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_AUTOTEXTREGEX, m_sRegex);
	DDX_Text(pDX, IDC_TESTRESULT, m_sResult);
	DDX_Text(pDX, IDC_TIMINGLABEL, m_sTimingLabel);
	DDX_Control(pDX, IDC_AUTOTEXTCONTENT, m_cContent);
}


BEGIN_MESSAGE_MAP(CAutoTextTestDlg, CDialog)
	ON_BN_CLICKED(IDC_AUTOTEXTSCAN, &CAutoTextTestDlg::OnBnClickedAutotextscan)
END_MESSAGE_MAP()


BOOL CAutoTextTestDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_cContent.LimitText(200*1024);

	return TRUE;
}

void CAutoTextTestDlg::OnBnClickedAutotextscan()
{
	UpdateData();

	m_sResult.Empty();
	m_sTimingLabel.Empty();
	m_cContent.GetTextRange(0, m_cContent.GetTextLength(), m_sContent);
	if ((!m_sContent.IsEmpty())&&(!m_sRegex.IsEmpty()))
	{
		try
		{
			std::set<CString> autolist;
			wstring s = m_sContent;
			CHighResClock timer;

			tr1::wregex regCheck;
			std::map<CString, tr1::wregex>::const_iterator regIt;
			regCheck = tr1::wregex(m_sRegex, tr1::regex_constants::icase | tr1::regex_constants::ECMAScript);
			const tr1::wsregex_iterator end;
			for (tr1::wsregex_iterator it(s.begin(), s.end(), regCheck); it != end; ++it)
			{
				const tr1::wsmatch match = *it;
				for (size_t i=1; i<match.size(); ++i)
				{
					if (match[i].second-match[i].first)
					{
						ATLTRACE(_T("matched keyword : %s\n"), wstring(match[i]).c_str());
						wstring result = wstring(match[i]);
						if (!result.empty())
						{
							autolist.insert(result.c_str());
						}
					}
				}
			}
			timer.Stop();
			for (std::set<CString>::iterator it = autolist.begin(); it != autolist.end(); ++it)
			{
				m_sResult += *it;
				m_sResult += _T("\r\n");
			}
			m_sTimingLabel.Format(_T("Parse time: %ld uSecs"), timer.GetMusecsTaken());
		}
		catch (exception) 
		{
			m_sResult = _T("Regex is invalid!");
		}
	}
	UpdateData(FALSE);
}

