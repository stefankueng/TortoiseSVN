// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010-2011, 2013, 2015, 2021 - TortoiseSVN

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
#include "ProjectProperties.h"
#include "AeroControls.h"

class PropValue
{
public:
    PropValue()
        : count(0)
        , allTheSameValue(true)
        , isBinary(false)
        , remove(false)
        , isInherited(false){};

    std::string  value;
    std::wstring valueWithoutNewlines;
    int          count;
    bool         allTheSameValue;
    bool         isBinary;
    bool         remove;
    bool         isInherited;
    std::wstring inheritedFrom;
};

using TProperties = std::multimap<std::string, PropValue>;

/**
 * \ingroup TortoiseProc
 * Base class for all the edit dialogs for properties.
 */

class EditPropBase
{
public:
    EditPropBase();
    virtual ~EditPropBase();

    virtual void        SetPropertyName(const std::string& sName);
    virtual void        SetPropertyValue(const std::string& sValue);
    virtual std::string GetPropertyValue() const { return m_propValue; }
    virtual std::string GetPropertyName() const { return m_propName; }
    virtual bool        IsBinary() const { return m_bIsBinary; }
    virtual bool        IsChanged() const { return m_bChanged; }
    virtual bool        GetRecursive() const { return !!m_bRecursive && !m_bRevProps; }
    virtual void        SetProperties(const TProperties& props) { m_properties = props; }
    virtual TProperties GetProperties() const { return m_properties; }
    virtual bool        HasMultipleProperties() { return false; }
    virtual bool        IsFolderOnlyProperty() { return false; }

    virtual void SetFolder() { m_bFolder = true; }
    virtual void SetMultiple() { m_bMultiple = true; }
    virtual void SetPathList(const CTSVNPathList& pathList);

    virtual void SetDialogTitle(const CString& sTitle) { m_sTitle = sTitle; }
    virtual void RevProps(bool bRevProps = false) { m_bRevProps = bRevProps; }

    virtual INT_PTR DoModal() = 0;

protected:
    std::string   m_propValue;
    std::string   m_propName;
    bool          m_bFolder;
    bool          m_bMultiple;
    bool          m_bIsBinary;
    bool          m_bChanged;
    BOOL          m_bRecursive;
    bool          m_bRemote;
    bool          m_bRevProps;
    CTSVNPathList m_pathList;
    CString       m_sTitle;
    TProperties   m_properties;
};
