// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2007 - TortoiseSVN

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
#include "StdAfx.h"
#include "ILogReceiver.h"

// construction

StandardRevProps::StandardRevProps 
    ( const CString& author
    , const CString& message
    , apr_time_t timeStamp)
    : author (author)
    , message (message)
    , timeStamp (timeStamp)
{
}

// construction

UserRevPropArray::UserRevPropArray()
{
}

UserRevPropArray::UserRevPropArray (size_t initialCapacity)
{
    reserve (initialCapacity);
}

// modification

void UserRevPropArray::Add
    ( const CString& name
    , const CString& value)
{
    push_back (UserRevProp());

    UserRevProp& item = back();
    item.name = name;
    item.value = value;
}

