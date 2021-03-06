// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007, 2009, 2011-2012, 2014, 2017, 2021 - TortoiseSVN

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
#include "RegHistory.h"

CRegHistory::CRegHistory()
    : m_nMaxHistoryItems(25)
{
}

CRegHistory::~CRegHistory()
{
}

bool CRegHistory::AddEntry(LPCWSTR szText)
{
    if (szText[0] == 0)
        return false;

    if ((!m_sSection.empty()) && (!m_sKeyPrefix.empty()))
    {
        // refresh the history from the registry
        Load(m_sSection.c_str(), m_sKeyPrefix.c_str());
    }

    for (size_t i = 0; i < m_arEntries.size(); ++i)
    {
        if (wcscmp(szText, m_arEntries[i].c_str()) == 0)
        {
            m_arEntries.erase(m_arEntries.begin() + i);
            m_arEntries.insert(m_arEntries.begin(), szText);
            return false;
        }
    }
    m_arEntries.insert(m_arEntries.begin(), szText);
    return true;
}

void CRegHistory::RemoveEntry(int pos)
{
    m_arEntries.erase(m_arEntries.begin() + pos);
}

void CRegHistory::RemoveEntry(LPCWSTR str)
{
    if (str == nullptr)
        return;
    for (std::vector<std::wstring>::iterator it = m_arEntries.begin(); it != m_arEntries.end(); ++it)
    {
        if (it->compare(str) == 0)
        {
            m_arEntries.erase(it);
            break;
        }
    }
}

size_t CRegHistory::Load(LPCWSTR lpszSection, LPCWSTR lpszKeyPrefix)
{
    if (lpszSection == nullptr || lpszKeyPrefix == nullptr || *lpszSection == '\0')
        return static_cast<size_t>(-1);

    m_arEntries.clear();

    m_sSection   = lpszSection;
    m_sKeyPrefix = lpszKeyPrefix;

    int          n = 0;
    std::wstring sText;
    do
    {
        //keys are of form <lpszKeyPrefix><entrynumber>
        wchar_t sKey[4096] = {0};
        swprintf_s(sKey, L"%s\\%s%d", lpszSection, lpszKeyPrefix, n++);
        sText = CRegStdString(sKey);
        if (!sText.empty())
        {
            m_arEntries.push_back(sText);
        }
    } while (!sText.empty() && n < m_nMaxHistoryItems);

    return m_arEntries.size();
}

bool CRegHistory::Save() const
{
    if (m_sSection.empty())
        return false;

    // save history to registry
    int nMax = min(static_cast<int>(m_arEntries.size()), m_nMaxHistoryItems + 1);
    for (int n = 0; n < static_cast<int>(m_arEntries.size()); n++)
    {
        wchar_t sKey[4096] = {0};
        swprintf_s(sKey, L"%s\\%s%d", m_sSection.c_str(), m_sKeyPrefix.c_str(), n);
        CRegStdString regKey(sKey);
        regKey = m_arEntries[n];
    }
    // remove items exceeding the max number of history items
    for (int n = nMax;; n++)
    {
        wchar_t sKey[4096] = {0};
        swprintf_s(sKey, L"%s\\%s%d", m_sSection.c_str(), m_sKeyPrefix.c_str(), n);
        CRegStdString regKey = CRegStdString(sKey);
        if (static_cast<std::wstring>(regKey).empty())
            break;
        regKey.removeValue(); // remove entry
    }
    return true;
}
