﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2008-2015, 2021, 2023 - TortoiseSVN

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
#include "resource.h"
#include "SVNStatusListCtrl.h"
#include <iterator>

#define SVNSLC_COL_VERSION 7

// assign property list

CSVNStatusListCtrl::PropertyList&
    CSVNStatusListCtrl::PropertyList::operator=(const char* rhs)
{
    // do you really want to replace the property list?

    assert(properties.empty());
    properties.clear();

    // add all properties in the list

    while ((rhs != nullptr) && (*rhs != 0))
    {
        const char* next = strchr(rhs, ' ');

        CString name(rhs, static_cast<int>(next == nullptr ? strlen(rhs) : next - rhs));
        properties.emplace(name, CString());

        rhs = next == nullptr ? nullptr : next + 1;
    }

    // done

    return *this;
}

// collect property names in a set

void CSVNStatusListCtrl::PropertyList::GetPropertyNames(std::set<CString>& names) const
{
    for (const auto& [name, value] : properties)
    {
        names.insert(name);
    }
}

// get a property value.

const CString& CSVNStatusListCtrl::PropertyList::operator[](const CString& name) const
{
    static const CString empty;
    auto                 iter = properties.find(name);

    return iter == properties.end()
               ? empty
               : iter->second;
}

// set a property value.

CString& CSVNStatusListCtrl::PropertyList::operator[](const CString& name)
{
    return properties[name];
}

/// check whether that property has been set on this item.

bool CSVNStatusListCtrl::PropertyList::HasProperty(const CString& name) const
{
    return properties.find(name) != properties.end();
}

// due to frequent use: special check for svn:needs-lock

bool CSVNStatusListCtrl::PropertyList::IsNeedsLockSet() const
{
    static const CString svnNeedsLock(SVN_PROP_NEEDS_LOCK);
    return HasProperty(svnNeedsLock);
}

// remove all entries

void CSVNStatusListCtrl::PropertyList::Clear()
{
    properties.clear();
}

// number of properties
size_t CSVNStatusListCtrl::PropertyList::Count() const
{
    return properties.size();
}

// registry access

void CSVNStatusListCtrl::ColumnManager::ReadSettings(DWORD defaultColumns, const CString& containerName)
{
    // defaults

    DWORD selectedStandardColumns = defaultColumns;

    columns.resize(SVNSLC_NUMCOLUMNS);
    for (size_t i = 0; i < SVNSLC_NUMCOLUMNS; ++i)
    {
        columns[i].index    = static_cast<int>(i);
        columns[i].width    = 0;
        columns[i].visible  = true;
        columns[i].relevant = true;
    }

    userProps.clear();

    // where the settings are stored within the registry

    registryPrefix = L"Software\\TortoiseSVN\\StatusColumns\\" + containerName;

    // we accept only the current version
    bool valid = static_cast<DWORD>(CRegDWORD(registryPrefix + L"Version", 0xff)) == SVNSLC_COL_VERSION;
    if (valid)
    {
        // read (possibly different) column selection

        selectedStandardColumns = CRegDWORD(registryPrefix, selectedStandardColumns);

        // read user-prop lists

        CString userPropList   = CRegString(registryPrefix + L"UserProps");
        CString shownUserProps = CRegString(registryPrefix + L"ShownUserProps");

        ParseUserPropSettings(userPropList, shownUserProps);

        // read column widths

        CString colWidths = CRegString(registryPrefix + L"_Width");

        ParseWidths(colWidths);
    }

    // process old-style visibility setting

    SetStandardColumnVisibility(selectedStandardColumns);

    // clear all previously set header columns

    int c = control->GetHeaderCtrl()->GetItemCount() - 1;
    while (c >= 0)
        control->DeleteColumn(c--);

    // create columns

    for (int i = 0, count = GetColumnCount(); i < count; ++i)
        control->InsertColumn(i, GetName(i), LVCFMT_LEFT, IsVisible(i) ? -1 : GetVisibleWidth(i, false));

    // restore column ordering

    if (valid)
        ParseColumnOrder(CRegString(registryPrefix + L"_Order"));
    else
        ParseColumnOrder(CString());

    ApplyColumnOrder();

    // auto-size the columns so we can see them while fetching status
    // (seems the same values will not take affect in InsertColumn)

    for (int i = 0, count = GetColumnCount(); i < count; ++i)
        if (IsVisible(i))
            control->SetColumnWidth(i, GetVisibleWidth(i, true));
}

void CSVNStatusListCtrl::ColumnManager::WriteSettings() const
{
    // ReSharper disable CppEntityAssignedButNoRead
    CRegDWORD regVersion(registryPrefix + L"Version", 0, TRUE);
    regVersion = SVNSLC_COL_VERSION;

    // write (possibly different) column selection

    CRegDWORD regStandardColumns(registryPrefix, 0, TRUE);
    regStandardColumns = GetSelectedStandardColumns();

    // write user-prop lists

    CRegString regUserProps(registryPrefix + L"UserProps", CString(), TRUE);
    regUserProps = GetUserPropList();

    CRegString regShownUserProps(registryPrefix + L"ShownUserProps", CString(), TRUE);
    regShownUserProps = GetShownUserProps();

    // write column widths

    CRegString regWidths(registryPrefix + L"_Width", CString(), TRUE);
    regWidths = GetWidthString();

    // write column ordering

    CRegString regColumnOrder(registryPrefix + L"_Order", CString(), TRUE);
    regColumnOrder = GetColumnOrderString();
    // ReSharper restore CppEntityAssignedButNoRead
}

// read column definitions

int CSVNStatusListCtrl::ColumnManager::GetColumnCount() const
{
    return static_cast<int>(columns.size());
}

bool CSVNStatusListCtrl::ColumnManager::IsVisible(int column) const
{
    size_t index = static_cast<size_t>(column);
    assert(columns.size() > index);

    return columns[index].visible;
}

int CSVNStatusListCtrl::ColumnManager::GetInvisibleCount() const
{
    int invisibleCount = 0;
    for (std::vector<ColumnInfo>::const_iterator it = columns.begin(); it != columns.end(); ++it)
    {
        if (!it->visible)
            invisibleCount++;
    }
    return invisibleCount;
}

bool CSVNStatusListCtrl::ColumnManager::IsRelevant(int column) const
{
    size_t index = static_cast<size_t>(column);
    assert(columns.size() > index);

    return columns[index].relevant;
}

bool CSVNStatusListCtrl::ColumnManager::IsUserProp(int column) const
{
    size_t index = static_cast<size_t>(column);
    assert(columns.size() > index);

    return columns[index].index >= SVNSLC_USERPROPCOLOFFSET;
}

const CString& CSVNStatusListCtrl::ColumnManager::GetName(int column) const
{
    static const UINT standardColumnNames[SVNSLC_NUMCOLUMNS] = {IDS_STATUSLIST_COLFILE

                                                                ,
                                                                IDS_STATUSLIST_COLFILENAME, IDS_STATUSLIST_COLEXT, IDS_STATUSLIST_COLSTATUS

                                                                ,
                                                                IDS_STATUSLIST_COLREMOTESTATUS, IDS_STATUSLIST_COLTEXTSTATUS, IDS_STATUSLIST_COLPROPSTATUS

                                                                ,
                                                                IDS_STATUSLIST_COLREMOTETEXTSTATUS, IDS_STATUSLIST_COLREMOTEPROPSTATUS, IDS_STATUSLIST_COLDEPTH, IDS_STATUSLIST_COLURL

                                                                ,
                                                                IDS_STATUSLIST_COLLOCK, IDS_STATUSLIST_COLLOCKCOMMENT, IDS_STATUSLIST_COLLOCKDATE

                                                                ,
                                                                IDS_STATUSLIST_COLAUTHOR, IDS_STATUSLIST_COLREVISION, IDS_STATUSLIST_COLREMOTEREVISION, IDS_STATUSLIST_COLDATE

                                                                ,
                                                                IDS_STATUSLIST_COLMODIFICATIONDATE, IDS_STATUSLIST_COLSIZE};

    static CString standardColumnNameStrings[SVNSLC_NUMCOLUMNS];

    // standard columns

    size_t index = static_cast<size_t>(column);
    if (index < SVNSLC_NUMCOLUMNS)
    {
        CString& result = standardColumnNameStrings[index];
        if (result.IsEmpty())
            result.LoadString(standardColumnNames[index]);

        return result;
    }

    // user-prop columns

    if (index < columns.size())
        return userProps[columns[index].index - SVNSLC_USERPROPCOLOFFSET].name;

    // default: empty

    static const CString empty;
    return empty;
}

int CSVNStatusListCtrl::ColumnManager::GetWidth(int column, bool useDefaults) const
{
    size_t index = static_cast<size_t>(column);
    assert(columns.size() > index);

    int width = columns[index].width;
    if ((width == 0) && useDefaults)
        width = LVSCW_AUTOSIZE_USEHEADER;

    return width;
}

int CSVNStatusListCtrl::ColumnManager::GetVisibleWidth(int column, bool useDefaults) const
{
    return IsVisible(column)
               ? GetWidth(column, useDefaults)
               : 0;
}

// switch columns on and off

void CSVNStatusListCtrl::ColumnManager::SetVisible(int column, bool visible)
{
    size_t index = static_cast<size_t>(column);
    assert(index < columns.size());

    if (columns[index].visible != visible)
    {
        columns[index].visible = visible;
        columns[index].relevant |= visible;
        if (!visible)
            columns[index].width = 0;

        control->SetColumnWidth(column, GetVisibleWidth(column, true));
        ApplyColumnOrder();

        control->Invalidate(FALSE);
    }
}

// tracking column modifications

void CSVNStatusListCtrl::ColumnManager::ColumnMoved(int column, int position)
{
    // in front of what column has it been inserted?

    int index = columns[column].index;

    std::vector<int> gridColumnOrder = GetGridColumnOrder();

    size_t visiblePosition = static_cast<size_t>(position);
    size_t columnCount     = gridColumnOrder.size();

    for (; (visiblePosition < columnCount) && !columns[gridColumnOrder[visiblePosition]].visible; ++visiblePosition)
    {
    }

    int next = visiblePosition == (columnCount - 1)
                   ? -1
                   : gridColumnOrder[visiblePosition];

    // move logical column index just in front of that "next" column

    columnOrder.erase(std::find(columnOrder.begin(), columnOrder.end(), index));
    columnOrder.insert(std::find(columnOrder.begin(), columnOrder.end(), next), index);

    // make sure, invisible columns are still put in front of all others

    ApplyColumnOrder();
}

void CSVNStatusListCtrl::ColumnManager::ColumnResized(int column)
{
    size_t index = static_cast<size_t>(column);
    assert(index < columns.size());
    assert(columns[index].visible);

    int width            = control->GetColumnWidth(column);
    columns[index].width = width;

    int propertyIndex = columns[index].index;
    if (propertyIndex >= SVNSLC_USERPROPCOLOFFSET)
        userProps[propertyIndex - SVNSLC_USERPROPCOLOFFSET].width = width;

    control->Invalidate(FALSE);
}

// call these to update the user-prop list
// (will also auto-insert /-remove new list columns)

void CSVNStatusListCtrl::ColumnManager::UpdateUserPropList(const std::map<CTSVNPath, PropertyList>& propertyMap)
{
    // collect all user-defined props

    std::set<CString> aggregatedProps;

    for (auto it = propertyMap.cbegin(); it != propertyMap.cend(); ++it)
        it->second.GetPropertyNames(aggregatedProps);

    itemProps = aggregatedProps;

    // add new ones to the internal list

    std::set<CString> newProps = aggregatedProps;
    for (size_t i = 0, count = userProps.size(); i < count; ++i)
        newProps.erase(userProps[i].name);

    while (newProps.size() && (newProps.size() + userProps.size() > SVNSLC_MAXCOLUMNCOUNT - SVNSLC_USERPROPCOLOFFSET))
        newProps.erase(--newProps.end());

    for (auto iter = newProps.begin(), end = newProps.end(); iter != end; ++iter)
    {
        int index = static_cast<int>(userProps.size()) + SVNSLC_USERPROPCOLOFFSET;
        if (columnOrder.size() < SVNSLC_MAXCOLUMNCOUNT)
            columnOrder.push_back(index);

        UserProp userProp;
        userProp.name  = *iter;
        userProp.width = 0;

        userProps.push_back(userProp);
    }

    // remove unused columns from control.
    // remove used ones from the set of aggregatedProps.

    for (size_t i = columns.size(); i > 0; --i)
        if ((columns[i - 1].index >= SVNSLC_USERPROPCOLOFFSET) && (aggregatedProps.erase(GetName(static_cast<int>(i) - 1)) == 0))
        {
            // this user-prop has not been set on any item

            if (!columns[i - 1].visible)
            {
                control->DeleteColumn(static_cast<int>(i - 1));
                columns.erase(columns.begin() + i - 1);
            }
        }

    // aggregatedProps now contains new columns only.
    // we can't use newProps here because some props may have been used
    // earlier but were not in the recent list of used props.
    // -> they are neither in columns[] nor in newProps.

    for (auto iter = aggregatedProps.begin(), end = aggregatedProps.end(); iter != end; ++iter)
    {
        // get the logical column index / ID

        int index = -1;
        int width = 0;
        for (size_t i = 0, count = userProps.size(); i < count; ++i)
            if (userProps[i].name == *iter)
            {
                index = static_cast<int>(i) + SVNSLC_USERPROPCOLOFFSET;
                width = userProps[i].width;
                break;
            }

        if (index == -1)
        {
            // property got removed because there were more than SVNSLC_MAXCOLUMNCOUNT-SVNSLC_USERPROPCOLOFFSET
            continue;
        }

        // find insertion position

        std::vector<ColumnInfo>::iterator columnIter = columns.begin();
        std::vector<ColumnInfo>::iterator end2       = columns.end();
        for (; (columnIter != end2) && columnIter->index < index; ++columnIter)
            ;
        int pos = static_cast<int>(columnIter - columns.begin());

        ColumnInfo column;
        column.index   = index;
        column.width   = width;
        column.visible = false;

        columns.insert(columnIter, column);

        // update control

        int result = control->InsertColumn(pos, *iter, LVCFMT_LEFT, GetVisibleWidth(pos, false));
        assert(result != -1);
        UNREFERENCED_PARAMETER(result);
    }

    // update column order

    ApplyColumnOrder();
}

void CSVNStatusListCtrl::ColumnManager::UpdateRelevance(const std::vector<FileEntry*>& files, const std::vector<size_t>& visibleFiles, const std::map<CTSVNPath, PropertyList>& propertyMap)
{
    // collect all user-defined props that belong to shown files

    std::set<CString> aggregatedProps;
    for (size_t i = 0, count = visibleFiles.size(); i < count; ++i)
    {
        auto propEntry = propertyMap.find(files[visibleFiles[i]]->GetPath());
        if (propEntry != propertyMap.end())
        {
            propEntry->second.GetPropertyNames(aggregatedProps);
        }
    }

    itemProps = aggregatedProps;

    // invisible columns for unused props are not relevant

    for (int i = 0, count = GetColumnCount(); i < count; ++i)
        if (IsUserProp(i) && !IsVisible(i))
        {
            columns[i].relevant = aggregatedProps.find(GetName(i)) != aggregatedProps.end();
        }
}

// don't clutter the context menu with irrelevant prop info

bool CSVNStatusListCtrl::ColumnManager::AnyUnusedProperties() const
{
    return columns.size() < userProps.size() + SVNSLC_NUMCOLUMNS;
}

void CSVNStatusListCtrl::ColumnManager::RemoveUnusedProps()
{
    // determine what column indexes / IDs to keep.
    // map them onto new IDs (we may delete some IDs in between)

    std::map<int, int> validIndices;
    int                userPropID = SVNSLC_USERPROPCOLOFFSET;

    for (size_t i = 0, count = columns.size(); i < count; ++i)
    {
        int index = columns[i].index;

        if (itemProps.find(GetName(static_cast<int>(i))) != itemProps.end() || columns[i].visible || index < SVNSLC_USERPROPCOLOFFSET)
        {
            validIndices[index] = index < SVNSLC_USERPROPCOLOFFSET
                                      ? index
                                      : userPropID++;
        }
    }

    // remove everything else:

    // remove from columns and control.
    // also update index values in columns

    for (size_t i = columns.size(); i > 0; --i)
    {
        std::map<int, int>::const_iterator iter = validIndices.find(columns[i - 1].index);

        if (iter == validIndices.end())
        {
            control->DeleteColumn(static_cast<int>(i - 1));
            columns.erase(columns.begin() + i - 1);
        }
        else
        {
            columns[i - 1].index = iter->second;
        }
    }

    // remove from user props

    for (size_t i = userProps.size(); i > 0; --i)
    {
        int index = static_cast<int>(i) - 1 + SVNSLC_USERPROPCOLOFFSET;
        if (validIndices.find(index) == validIndices.end())
            userProps.erase(userProps.begin() + i - 1);
    }

    // remove from and update column order

    for (size_t i = columnOrder.size(); i > 0; --i)
    {
        std::map<int, int>::const_iterator iter = validIndices.find(columnOrder[i - 1]);

        if (iter == validIndices.end())
            columnOrder.erase(columnOrder.begin() + i - 1);
        else
            columnOrder[i - 1] = iter->second;
    }
}

// bring everything back to its "natural" order

void CSVNStatusListCtrl::ColumnManager::ResetColumns(DWORD defaultColumns)
{
    // update internal data

    std::sort(columnOrder.begin(), columnOrder.end());

    for (size_t i = 0, count = columns.size(); i < count; ++i)
    {
        columns[i].width   = 0;
        columns[i].visible = (i < 32) && (((defaultColumns >> i) & 1) != 0);
    }

    for (size_t i = 0, count = userProps.size(); i < count; ++i)
        userProps[i].width = 0;

    // update UI

    for (int i = 0, count = GetColumnCount(); i < count; ++i)
        control->SetColumnWidth(i, GetVisibleWidth(i, true));

    ApplyColumnOrder();

    control->Invalidate(FALSE);
}

// initialization utilities

void CSVNStatusListCtrl::ColumnManager::ParseUserPropSettings(const CString& userPropList, const CString& shownUserProps)
{
    assert(userProps.empty());

    static CString delimiters(L" ");

    // parse list of visible user-props

    std::set<CString> visibles;

    int     pos  = 0;
    CString name = shownUserProps.Tokenize(delimiters, pos);
    while (!name.IsEmpty())
    {
        visibles.insert(name);
        name = shownUserProps.Tokenize(delimiters, pos);
    }

    // create list of all user-props

    pos  = 0;
    name = userPropList.Tokenize(delimiters, pos);
    while (!name.IsEmpty())
    {
        bool visible = visibles.find(name) != visibles.end();

        UserProp newEntry;
        newEntry.name  = name;
        newEntry.width = 0;

        userProps.push_back(newEntry);

        // auto-create columns for visible user-props
        // (others may be added later)

        if (visible)
        {
            ColumnInfo newColumn;
            newColumn.width    = 0;
            newColumn.visible  = true;
            newColumn.relevant = true;
            newColumn.index    = static_cast<int>(userProps.size()) + SVNSLC_USERPROPCOLOFFSET - 1;

            columns.push_back(newColumn);
        }

        name = userPropList.Tokenize(delimiters, pos);
    }
}

void CSVNStatusListCtrl::ColumnManager::ParseWidths(const CString& widths)
{
    for (int i = 0, count = widths.GetLength() / 8; i < count; ++i)
    {
        long width = wcstol(widths.Mid(i * 8, 8), nullptr, 16);
        if (i < SVNSLC_NUMCOLUMNS)
        {
            // a standard column

            columns[i].width = width;
        }
        else if (i >= SVNSLC_USERPROPCOLOFFSET)
        {
            // a user-prop column

            size_t index = static_cast<size_t>(i - SVNSLC_USERPROPCOLOFFSET);

            // someone may have tweaked the registry settings by hand ...

            if (index < userProps.size())
            {
                userProps[index].width = width;

                for (size_t k = 0, count2 = columns.size(); k < count2; ++k)
                    if (columns[k].index == i)
                        columns[k].width = width;
            }
        }
        else
        {
            // there is no such column

            assert(width == 0);
        }
    }
}

void CSVNStatusListCtrl::ColumnManager::SetStandardColumnVisibility(DWORD visibility)
{
    for (size_t i = 0; i < SVNSLC_NUMCOLUMNS; ++i)
    {
        columns[i].visible = (visibility & 1) > 0;
        visibility /= 2;
    }
}

void CSVNStatusListCtrl::ColumnManager::ParseColumnOrder(const CString& widths)
{
    std::set<int> alreadyPlaced;
    columnOrder.clear();

    // place columns according to valid entries in orderString

    int limit = static_cast<int>(SVNSLC_USERPROPCOLOFFSET + userProps.size());
    for (int i = 0, count = widths.GetLength() / 2; i < count; ++i)
    {
        int index = wcstol(widths.Mid(i * 2, 2), nullptr, 16);
        if ((index < SVNSLC_NUMCOLUMNS) || ((index >= SVNSLC_USERPROPCOLOFFSET) && (index < limit)))
        {
            alreadyPlaced.insert(index);
            columnOrder.push_back(index);
        }
    }

    // place the remaining colums behind it

    for (int i = 0; i < SVNSLC_NUMCOLUMNS; ++i)
        if (alreadyPlaced.find(i) == alreadyPlaced.end())
            columnOrder.push_back(i);

    for (int i = SVNSLC_USERPROPCOLOFFSET; i < limit; ++i)
        if (alreadyPlaced.find(i) == alreadyPlaced.end())
            columnOrder.push_back(i);
}

// map internal column order onto visible column order
// (all invisibles in front)

std::vector<int> CSVNStatusListCtrl::ColumnManager::GetGridColumnOrder() const
{
    // extract order of used columns from order of all columns

    std::vector<int> result;
    result.reserve(SVNSLC_MAXCOLUMNCOUNT + 1);

    size_t colCount = columns.size();
    bool   visible  = false;

    do
    {
        // put invisible cols in front

        for (size_t i = 0, count = columnOrder.size(); i < count; ++i)
        {
            int index = columnOrder[i];
            for (size_t k = 0; k < colCount; ++k)
            {
                const ColumnInfo& column = columns[k];
                if ((column.index == index) && (column.visible == visible))
                    result.push_back(static_cast<int>(k));
            }
        }

        visible = !visible;
    } while (visible);

    return result;
}

void CSVNStatusListCtrl::ColumnManager::ApplyColumnOrder() const
{
    // extract order of used columns from order of all columns

    int order[SVNSLC_MAXCOLUMNCOUNT + 1] = {0};

    std::vector<int> gridColumnOrder = GetGridColumnOrder();
    std::ranges::copy(gridColumnOrder, order);

    // we must have placed all columns or something is really fishy ..

    assert(gridColumnOrder.size() == columns.size());
    assert(GetColumnCount() == control->GetHeaderCtrl()->GetItemCount());

    // o.k., apply our column ordering

    control->SetColumnOrderArray(GetColumnCount(), order);
}

// utilities used when writing data to the registry

DWORD CSVNStatusListCtrl::ColumnManager::GetSelectedStandardColumns() const
{
    DWORD result = 0;
    for (size_t i = SVNSLC_NUMCOLUMNS; i > 0; --i)
        result = result * 2 + (columns[i - 1].visible ? 1 : 0);

    return result;
}

CString CSVNStatusListCtrl::ColumnManager::GetUserPropList() const
{
    CString result;

    for (size_t i = 0, count = userProps.size(); i < count; ++i)
        result += userProps[i].name + ' ';

    return result;
}

CString CSVNStatusListCtrl::ColumnManager::GetShownUserProps() const
{
    CString result;

    for (size_t i = 0, count = columns.size(); i < count; ++i)
    {
        size_t index = static_cast<size_t>(columns[i].index);
        if (columns[i].visible && (index >= SVNSLC_USERPROPCOLOFFSET))
            result += userProps[index - SVNSLC_USERPROPCOLOFFSET].name + ' ';
    }

    return result;
}

CString CSVNStatusListCtrl::ColumnManager::GetWidthString() const
{
    CString result;

    // regular columns

    wchar_t buf[10] = {0};
    for (size_t i = 0; i < SVNSLC_NUMCOLUMNS; ++i)
    {
        swprintf_s(buf, L"%08X", columns[i].width);
        result += buf;
    }

    // range with no column IDs

    result += CString('0', 8 * (SVNSLC_USERPROPCOLOFFSET - SVNSLC_NUMCOLUMNS));

    // user-prop columns

    for (size_t i = 0, count = userProps.size(); i < count; ++i)
    {
        swprintf_s(buf, L"%08X", userProps[i].width);
        result += buf;
    }

    return result;
}

CString CSVNStatusListCtrl::ColumnManager::GetColumnOrderString() const
{
    CString result;

    wchar_t buf[3] = {0};
    for (size_t i = 0, count = columnOrder.size(); i < count; ++i)
    {
        if (columnOrder[i] < SVNSLC_MAXCOLUMNCOUNT)
        {
            swprintf_s(buf, L"%02X", columnOrder[i]);
            result += buf;
        }
    }

    return result;
}

// sorter utility class

CSVNStatusListCtrl::CSorter::CSorter(ColumnManager* columnManager, CSVNStatusListCtrl* listControl, int sortedColumn, bool ascending)
    : columnManager(columnManager)
    , control(listControl)
    , sortedColumn(sortedColumn)
    , ascending(ascending)
{
    m_bSortLogical = !CRegDWORD(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\NoStrCmpLogical", 0, false, HKEY_CURRENT_USER);
    if (m_bSortLogical)
        m_bSortLogical = !CRegDWORD(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\NoStrCmpLogical", 0, false, HKEY_LOCAL_MACHINE);
}

bool CSVNStatusListCtrl::CSorter::operator()(const FileEntry* entry1, const FileEntry* entry2) const
{
#define SGN(x) ((x) == 0 ? 0 : ((x) > 0 ? 1 : -1))

    int result = 0;
    switch (sortedColumn)
    {
        case 19:
        {
            if (result == 0)
            {
                __int64 fileSize1 = entry1->isFolder ? 0 : entry1->workingSize != (-1) ? entry1->workingSize
                                                                                       : entry1->GetPath().GetFileSize();
                __int64 fileSize2 = entry2->isFolder ? 0 : entry2->workingSize != (-1) ? entry2->workingSize
                                                                                       : entry2->GetPath().GetFileSize();

                result = static_cast<int>(fileSize1 - fileSize2);
            }
        }
            [[fallthrough]];
        case 18:
        {
            if (result == 0)
            {
                __int64 writetime1 = entry1->GetPath().GetLastWriteTime();
                __int64 writetime2 = entry2->GetPath().GetLastWriteTime();

                FILETIME* filetime1 = reinterpret_cast<FILETIME*>(static_cast<long long*>(&writetime1));
                FILETIME* filetime2 = reinterpret_cast<FILETIME*>(static_cast<long long*>(&writetime2));

                result = CompareFileTime(filetime1, filetime2);
            }
        }
            [[fallthrough]];
        case 17:
        {
            if (result == 0)
            {
                result = SGN(entry1->lastCommitDate - entry2->lastCommitDate);
            }
        }
            [[fallthrough]];
        case 16:
        {
            if (result == 0)
            {
                result = entry1->remoteRev - entry2->remoteRev;
            }
        }
            [[fallthrough]];
        case 15:
        {
            if (result == 0)
            {
                result = entry1->lastCommitRev - entry2->lastCommitRev;
            }
        }
            [[fallthrough]];
        case 14:
        {
            if (result == 0)
            {
                if (m_bSortLogical)
                    result = StrCmpLogicalW(entry1->lastCommitAuthor, entry2->lastCommitAuthor);
                else
                    result = StrCmpI(entry1->lastCommitAuthor, entry2->lastCommitAuthor);
            }
        }
            [[fallthrough]];
        case 13:
        {
            if (result == 0)
            {
                result = static_cast<int>(entry1->lockDate - entry2->lockDate);
            }
        }
            [[fallthrough]];
        case 12:
        {
            if (result == 0)
            {
                if (m_bSortLogical)
                    result = StrCmpLogicalW(entry1->lockComment, entry2->lockComment);
                else
                    result = StrCmpI(entry1->lockComment, entry2->lockComment);
            }
        }
            [[fallthrough]];
        case 11:
        {
            if (result == 0)
            {
                if (m_bSortLogical)
                    result = StrCmpLogicalW(entry1->lockOwner, entry2->lockOwner);
                else
                    result = StrCmpI(entry1->lockOwner, entry2->lockOwner);
            }
        }
            [[fallthrough]];
        case 10:
        {
            if (result == 0)
            {
                if (m_bSortLogical)
                    result = StrCmpLogicalW(entry1->url, entry2->url);
                else
                    result = StrCmpI(entry1->url, entry2->url);
            }
        }
            [[fallthrough]];
        case 9:
        {
            if (result == 0)
            {
                result = entry1->depth - entry2->depth;
            }
        }
            [[fallthrough]];
        case 8:
        {
            if (result == 0)
            {
                result = entry1->remotePropStatus - entry2->remotePropStatus;
            }
        }
            [[fallthrough]];
        case 7:
        {
            if (result == 0)
            {
                result = entry1->remoteTextStatus - entry2->remoteTextStatus;
            }
        }
            [[fallthrough]];
        case 6:
        {
            if (result == 0)
            {
                result = entry1->propStatus - entry2->propStatus;
            }
        }
            [[fallthrough]];
        case 5:
        {
            if (result == 0)
            {
                result = entry1->textStatus - entry2->textStatus;
            }
        }
            [[fallthrough]];
        case 4:
        {
            if (result == 0)
            {
                result = entry1->remoteStatus - entry2->remoteStatus;
            }
        }
            [[fallthrough]];
        case 3:
        {
            if (result == 0)
            {
                result = entry1->status - entry2->status;
            }
        }
            [[fallthrough]];
        case 2:
        {
            if (result == 0)
            {
                if (m_bSortLogical)
                    result = StrCmpLogicalW(entry1->path.GetFileExtension(), entry2->path.GetFileExtension());
                else
                    result = StrCmpI(entry1->path.GetFileExtension(), entry2->path.GetFileExtension());
            }
        }
            [[fallthrough]];
        case 1:
        {
            // do not sort by file/dirname if the sorting isn't done by this specific column but let the second-order
            // sorting be done by path
            if ((result == 0) && (sortedColumn == 1))
            {
                if (m_bSortLogical)
                    result = StrCmpLogicalW(entry1->path.GetFileOrDirectoryName(), entry2->path.GetFileOrDirectoryName());
                else
                    result = StrCmpI(entry1->path.GetFileOrDirectoryName(), entry2->path.GetFileOrDirectoryName());
            }
        }
            [[fallthrough]];
        case 0: // path column
        {
            if (result == 0)
            {
                if (m_bSortLogical)
                    result = StrCmpLogicalW(entry1->path.GetWinPath(), entry2->path.GetWinPath());
                else
                    result = StrCmpI(entry1->path.GetWinPath(), entry2->path.GetWinPath());
            }
        }
            [[fallthrough]];
        default:
            if ((result == 0) && (sortedColumn > 0))
            {
                // N/A props are "less than" empty props

                const CString& propName = columnManager->GetName(sortedColumn);

                auto propEntry1    = control->m_propertyMap.find(entry1->GetPath());
                auto propEntry2    = control->m_propertyMap.find(entry2->GetPath());
                bool entry1HasProp = (propEntry1 != control->m_propertyMap.end()) && propEntry1->second.HasProperty(propName);
                bool entry2HasProp = (propEntry2 != control->m_propertyMap.end()) && propEntry2->second.HasProperty(propName);

                if (entry1HasProp)
                {
                    result = entry2HasProp
                                 ? propEntry1->second[propName].Compare(propEntry2->second[propName])
                                 : 1;
                }
                else
                {
                    result = entry2HasProp ? -1 : 0;
                }
            }
    } // switch (m_nSortedColumn)
    if (!ascending)
        result = -result;

    return result < 0;
}
