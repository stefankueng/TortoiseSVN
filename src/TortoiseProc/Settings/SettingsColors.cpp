// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008, 2010, 2013 - TortoiseSVN

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
#include "SettingsColors.h"

IMPLEMENT_DYNAMIC(CSettingsColors, ISettingsPropPage)
CSettingsColors::CSettingsColors()
    : ISettingsPropPage(CSettingsColors::IDD)
{
}

CSettingsColors::~CSettingsColors()
{
}

void CSettingsColors::DoDataExchange(CDataExchange* pDX)
{
    ISettingsPropPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_CONFLICTCOLOR, m_cConflict);
    DDX_Control(pDX, IDC_ADDEDCOLOR, m_cAdded);
    DDX_Control(pDX, IDC_DELETEDCOLOR, m_cDeleted);
    DDX_Control(pDX, IDC_MERGEDCOLOR, m_cMerged);
    DDX_Control(pDX, IDC_MODIFIEDCOLOR, m_cModified);
    DDX_Control(pDX, IDC_FILTERMATCHCOLOR, m_cFilterMatch);
}


BEGIN_MESSAGE_MAP(CSettingsColors, ISettingsPropPage)
    ON_BN_CLICKED(IDC_RESTORE, OnBnClickedRestore)
    ON_BN_CLICKED(IDC_CONFLICTCOLOR, &CSettingsColors::OnBnClickedColor)
    ON_BN_CLICKED(IDC_ADDEDCOLOR, &CSettingsColors::OnBnClickedColor)
    ON_BN_CLICKED(IDC_DELETEDCOLOR, &CSettingsColors::OnBnClickedColor)
    ON_BN_CLICKED(IDC_MERGEDCOLOR, &CSettingsColors::OnBnClickedColor)
    ON_BN_CLICKED(IDC_MODIFIEDCOLOR, &CSettingsColors::OnBnClickedColor)
    ON_BN_CLICKED(IDC_FILTERMATCHCOLOR, &CSettingsColors::OnBnClickedColor)
END_MESSAGE_MAP()

BOOL CSettingsColors::OnInitDialog()
{
    ISettingsPropPage::OnInitDialog();

    m_cAdded.SetColor(m_Colors.GetColor(CColors::Added));
    m_cDeleted.SetColor(m_Colors.GetColor(CColors::Deleted));
    m_cMerged.SetColor(m_Colors.GetColor(CColors::Merged));
    m_cModified.SetColor(m_Colors.GetColor(CColors::Modified));
    m_cConflict.SetColor(m_Colors.GetColor(CColors::Conflict));
    m_cFilterMatch.SetColor(m_Colors.GetColor(CColors::FilterMatch));

    CString sDefaultText, sCustomText;
    sDefaultText.LoadString(IDS_COLOURPICKER_DEFAULTTEXT);
    sCustomText.LoadString(IDS_COLOURPICKER_CUSTOMTEXT);
    m_cAdded.EnableAutomaticButton(sDefaultText, m_Colors.GetColor(CColors::Added, true));
    m_cAdded.EnableOtherButton(sCustomText);
    m_cDeleted.EnableAutomaticButton(sDefaultText, m_Colors.GetColor(CColors::Deleted, true));
    m_cDeleted.EnableOtherButton(sCustomText);
    m_cMerged.EnableAutomaticButton(sDefaultText, m_Colors.GetColor(CColors::Merged, true));
    m_cMerged.EnableOtherButton(sCustomText);
    m_cModified.EnableAutomaticButton(sDefaultText, m_Colors.GetColor(CColors::Modified, true));
    m_cModified.EnableOtherButton(sCustomText);
    m_cConflict.EnableAutomaticButton(sDefaultText, m_Colors.GetColor(CColors::Conflict, true));
    m_cConflict.EnableOtherButton(sCustomText);
    m_cFilterMatch.EnableAutomaticButton(sDefaultText, m_Colors.GetColor(CColors::FilterMatch, true));
    m_cFilterMatch.EnableOtherButton(sCustomText);

    return TRUE;
}

void CSettingsColors::OnBnClickedRestore()
{
    m_cAdded.SetColor(m_Colors.GetColor(CColors::Added, true));
    m_cDeleted.SetColor(m_Colors.GetColor(CColors::Deleted, true));
    m_cMerged.SetColor(m_Colors.GetColor(CColors::Merged, true));
    m_cModified.SetColor(m_Colors.GetColor(CColors::Modified, true));
    m_cConflict.SetColor(m_Colors.GetColor(CColors::Conflict, true));
    m_cFilterMatch.SetColor(m_Colors.GetColor(CColors::FilterMatch, true));
    SetModified(TRUE);
}

BOOL CSettingsColors::OnApply()
{
    m_Colors.SetColor(CColors::Added, m_cAdded.GetColor() == -1 ? m_cAdded.GetAutomaticColor() : m_cAdded.GetColor());
    m_Colors.SetColor(CColors::Deleted, m_cDeleted.GetColor() == -1 ? m_cDeleted.GetAutomaticColor() : m_cDeleted.GetColor());
    m_Colors.SetColor(CColors::Merged, m_cMerged.GetColor() == -1 ? m_cMerged.GetAutomaticColor() : m_cMerged.GetColor());
    m_Colors.SetColor(CColors::Modified, m_cModified.GetColor() == -1 ? m_cModified.GetAutomaticColor() : m_cModified.GetColor());
    m_Colors.SetColor(CColors::Conflict, m_cConflict.GetColor() == -1 ? m_cConflict.GetAutomaticColor() : m_cConflict.GetColor());
    m_Colors.SetColor(CColors::PropertyChanged, m_cModified.GetColor() == -1 ? m_cModified.GetAutomaticColor() : m_cModified.GetColor());
    m_Colors.SetColor(CColors::FilterMatch, m_cFilterMatch.GetColor() == -1 ? m_cFilterMatch.GetAutomaticColor() : m_cFilterMatch.GetColor());

    return ISettingsPropPage::OnApply();
}

void CSettingsColors::OnBnClickedColor()
{
    SetModified();
}
