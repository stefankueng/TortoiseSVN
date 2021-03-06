// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2009, 2017, 2021 - TortoiseSVN

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
#pragma once

/**
 * \ingroup TortoiseProc
 * Maintains a list of X string items in the registry and provides methods
 * to add new items. The list can be used as a 'recently used' or 'recent items' list.
 */
class CRegHistory
{
public:
    CRegHistory();
    virtual ~CRegHistory();

    /// Loads the history
    /// \param lpszSection the section in the registry, e.g., "Software\\CompanyName\\History"
    /// \param lpszKeyPrefix the name of the registry values, e.g., "historyItem"
    /// \return the number of history items loaded
    size_t Load(LPCWSTR lpszSection, LPCWSTR lpszKeyPrefix);
    /// Saves the history.
    bool Save() const;
    /// Adds a new string to the history list.
    virtual bool AddEntry(LPCWSTR szText);
    /// Removes the entry at index \c pos.
    void RemoveEntry(int pos);
    void RemoveEntry(LPCWSTR str);
    /// Sets the maximum number of items in the history. Default is 25.
    void SetMaxHistoryItems(int nMax) { m_nMaxHistoryItems = nMax; }
    /// Returns the number of items in the history.
    size_t GetCount() const { return m_arEntries.size(); }
    /// Returns the entry at index \c pos
    LPCWSTR GetEntry(size_t pos) { return m_arEntries[pos].c_str(); }

protected:
    std::wstring              m_sSection;
    std::wstring              m_sKeyPrefix;
    std::vector<std::wstring> m_arEntries;
    int                       m_nMaxHistoryItems;
};
