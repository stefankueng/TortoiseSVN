﻿// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2013, 2020-2021 - TortoiseSVN

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
#include "EncodingDlg.h"

const CFileTextLines::UnicodeType uctArray[] =
    {
        CFileTextLines::ASCII,
        CFileTextLines::UTF16_LE,
        CFileTextLines::UTF16_BE,
        CFileTextLines::UTF16_LEBOM,
        CFileTextLines::UTF16_BEBOM,
        CFileTextLines::UTF32_LE,
        CFileTextLines::UTF32_BE,
        CFileTextLines::UTF8,
        CFileTextLines::UTF8BOM};

const EOL eolArray[] =
    {
        EOL_AUTOLINE,
        EOL_CRLF,
        EOL_LF,
        EOL_CR,
        EOL_LFCR,
        EOL_VT,
        EOL_FF,
        EOL_NEL,
        EOL_LS,
        EOL_PS};

// dialog

IMPLEMENT_DYNAMIC(CEncodingDlg, CStandAloneDialog)

CEncodingDlg::CEncodingDlg(CWnd* pParent)
    : CStandAloneDialog(CEncodingDlg::IDD, pParent)
    , texttype(CFileTextLines::ASCII)
    , m_lineEndings(EOL_AUTOLINE)
{
}

CEncodingDlg::~CEncodingDlg()
{
}

void CEncodingDlg::DoDataExchange(CDataExchange* pDX)
{
    CStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_COMBO_ENCODING, m_encoding);
    DDX_Control(pDX, IDC_COMBO_EOL, m_eol);
}

BEGIN_MESSAGE_MAP(CEncodingDlg, CStandAloneDialog)
END_MESSAGE_MAP()

// message handlers

void CEncodingDlg::OnCancel()
{
    __super::OnCancel();
}

void CEncodingDlg::OnOK()
{
    UpdateData();
    texttype      = uctArray[m_encoding.GetCurSel()];
    m_lineEndings = eolArray[m_eol.GetCurSel()];
    __super::OnOK();
}

BOOL CEncodingDlg::OnInitDialog()
{
    CStandAloneDialog::OnInitDialog();

    CString sTitle;
    GetWindowText(sTitle);
    sTitle = view + sTitle;
    SetWindowText(sTitle);
    SetDlgItemText(IDC_VIEW, view);

    for (int i = 0; i < _countof(uctArray); i++)
    {
        m_encoding.AddString(CFileTextLines::GetEncodingName(uctArray[i]));
        if (texttype == uctArray[i])
        {
            m_encoding.SetCurSel(i);
        }
    }

    for (int i = 0; i < _countof(eolArray); i++)
    {
        m_eol.AddString(GetEolName(eolArray[i]));
        if (m_lineEndings == eolArray[i])
        {
            m_eol.SetCurSel(i);
        }
    }

    return FALSE;
}
