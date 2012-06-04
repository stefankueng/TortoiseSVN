// Copyright 2012 Idol Software, Inc.
//
// This file is part of CrashHandler library.
//
// CrashHandler library is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include "generated\soapUploaderSoapProxy.h"

class DumpUploaderWebService : public UploaderSoapProxy
{
    std::string m_serviceUrl;
public:
    DumpUploaderWebService(int responseTimeoutSec = 0);
    std::wstring GetErrorText();

    typedef void (*pfnProgressCallback)(BOOL send, SIZE_T bytesCount, LPVOID context);
    void SetProgressCallback(pfnProgressCallback progressCallback, LPVOID context);
};
