// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009-2010, 2014 - TortoiseSVN

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

class COMError
{
public:
    COMError(HRESULT hr);
    virtual ~COMError();

    std::wstring GetMessage() const {return message;}
    std::wstring GetDescription() const {return description;}
    std::wstring GetMessageAndDescription() const {return message + L"\n" + description;}
    std::wstring GetSource() const {return source;}
    std::wstring GetUUID() const {return uuid;}

private:
    std::wstring message;
    std::wstring description;
    std::wstring source;
    std::wstring uuid;
};
