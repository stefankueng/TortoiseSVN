// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2011, 2013, 2015-2016, 2021 - TortoiseSVN

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
#include "UserProperties.h"
#include <regex>

bool UserProp::Parse(const CString& line)
{
    propType = UserPropTypeUnknown;

    int equalpos = line.Find('=');
    if (equalpos < 0)
    {
        propName = line;
        return true;
    }
    // format is
    // propertyname=propertytype;labeltext(...)
    propName     = line.Left(equalpos);
    int colonPos = line.Find(';');
    if (colonPos < 0)
        return false;
    CString temp = line.Mid(equalpos + 1, colonPos - equalpos - 1);
    if (temp.Compare(L"bool") == 0)
        propType = UserPropTypeBool;
    else if (temp.Compare(L"state") == 0)
        propType = UserPropTypeState;
    else if (temp.Compare(L"singleline") == 0)
        propType = UserPropTypeSingleLine;
    else if (temp.Compare(L"multiline") == 0)
        propType = UserPropTypeMultiLine;
    else
        return false;
    int bracketPos = line.Find('(');
    if (bracketPos < 0)
    {
        // property has invalid format!
        propType = UserPropTypeUnknown;
        return false;
    }
    temp      = line.Mid(colonPos + 1, bracketPos - colonPos - 1);
    labelText = temp;

    temp = line.Mid(bracketPos + 1);
    if (temp.Right(1) != ")")
    {
        // property has invalid format!
        propType = UserPropTypeUnknown;
        return false;
    }
    temp = temp.Left(temp.GetLength() - 1);
    switch (propType)
    {
        case UserPropTypeBool:
        {
            // format is
            // propertyname=bool;labeltext(YESVALUE;NOVALUE;Checkboxtext)
            int cPos = temp.Find(';');
            if (cPos >= 0)
            {
                boolYes   = temp.Left(cPos);
                int cPos2 = temp.Find(';', cPos + 1);
                if (cPos2)
                {
                    boolNo        = temp.Mid(cPos + 1, cPos2 - cPos - 1);
                    boolCheckText = temp.Mid(cPos2 + 1);
                }
            }
            if (boolCheckText.IsEmpty())
            {
                // property has invalid format!
                propType = UserPropTypeUnknown;
                return false;
            }
            return true;
        }
        case UserPropTypeState:
        {
            // format is
            // propertyname=state;labeltext(DEFVAL;VAL1;TEXT1;VAL2;TEXT2;VAL3;TEXT3;...)
            int cPos = temp.Find(';');
            if (cPos >= 0)
            {
                stateDefaultVal  = temp.Left(cPos);
                temp             = temp.Mid(cPos + 1);
                int     curPos   = 0;
                CString resToken = temp.Tokenize(L";", curPos);
                while (!resToken.IsEmpty())
                {
                    CString sVal, sText;
                    sVal     = resToken;
                    resToken = temp.Tokenize(L";", curPos);
                    sText    = resToken;
                    if (curPos < 0)
                    {
                        // property has invalid format!
                        propType = UserPropTypeUnknown;
                        return false;
                    }
                    resToken = temp.Tokenize(L";", curPos);
                    if (!sText.IsEmpty() && !sVal.IsEmpty())
                        stateEntries.emplace_back(sVal, sText);
                    else
                    {
                        // property has invalid format!
                        propType = UserPropTypeUnknown;
                        return false;
                    }
                }
                return true;
            }
            propType = UserPropTypeUnknown;
            return false;
        }
        case UserPropTypeSingleLine:
        {
            // format is
            // propertyname=singleline;labeltext(regex)
            try
            {
                std::wregex regCheck = std::wregex(temp);
                validationRegex      = temp;
            }
            catch (std::exception&)
            {
            }
        }
        break;
        case UserPropTypeMultiLine:
        {
            // format is
            // propertyname=multiline;labeltext(regex)
            try
            {
                std::wregex regCheck = std::wregex(temp);
                validationRegex      = temp;
            }
            catch (std::exception&)
            {
            }
        }
        break;
        case UserPropTypeUnknown:
            break;
        default:
            break;
    }
    return true;
}
