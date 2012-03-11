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

#ifndef __LOG_H
#define __LOG_H

#include <windows.h>
#include <string>
#include <vector>
#include <stdarg.h>
#include <tchar.h>
#include "synchro.h"
#include <memory>

#pragma warning (disable: 4251) // need dll-linkage

void InitializeLog();
void FreeLogThreadName();
void SetLogThreadName(LPCTSTR pszThreadName);

//! ���� ���������.
enum ELogMessageType
{
	eLM_Error,		//!< "������" - ��������� ���� � ������, ���������� ���������� �������� �������� ����������.
	eLM_Warning,	//!< "��������������" - ��������� ���� � ������, ������� ������� ���������.
	eLM_Info,		//!< "����������" - ��������� � ���������� ���������� �������� ����������.
	eLM_Debug,		//!< "�������" - ���������, ���������� ���������� ����������.
	eLM_DirectOutput //!< ��������������� � ��� ������ �������� ����������� ����������
};

//! ������� �������� ���������.
enum ELogMessageLevel
{
	L0 = 0,	//!< �������� ������.
	L1 = 1,
	L2 = 2,
	L3 = 3,
	L4 = 4,
	L5 = 5,
	LMAX = 255
};

//! ��������� "�����������" ����. ������������ ������ �� ��������.
class ILogMedia
{
public:
	virtual ~ILogMedia() {}
	//! ���������� ���������.
	virtual void Write(
		ELogMessageType type,	//!< [in] ��� ���������.
		ELogMessageLevel nLevel,//!< [in] ������� �������� (L0 - �������� ������).
		LPCTSTR pszDate,		//!< [in] ���� ������ ���������.
		LPCTSTR pszTime,		//!< [in] ����� ������ ���������.
		LPCTSTR pszThreadId,	//!< [in] ������������� ������, �� �������� �������� ���������.
		LPCTSTR pszThreadName,	//!< [in] ��� ������, �� �������� �������� ���������.
		LPCTSTR pszModule,		//!< [in] ������, �� �������� �������� ���������.
		LPCTSTR pszMessage		//!< [in] ���������.
		) = 0;
	//! ���������, ��������� ��� ������������ ���������.
	//!   \return false - ��������� ������������.
	virtual bool Check(
		ELogMessageType type,	//!< [in] ��� ���������.
		ELogMessageLevel nLevel,//!< [in] ������� �������� (L0 - �������� ������).
		LPCTSTR pszModule		//!< [in] ������, �� �������� �������� ���������.
		)
	{
		type; nLevel; pszModule;
		return true;
	};
	//! ����������, �������� �� ���.
	//! ���� ��� ��� �������� ������ (��������, ������ ��� �����), ������ �� ���� ����� �����.
	virtual bool IsWorking() const { return true; }
};

typedef std::shared_ptr<ILogMedia> LogMediaPtr;

//! ���-��������.
class LogMediaProxy: public ILogMedia
{
public:
	LogMediaProxy(const LogMediaPtr& pLog = LogMediaPtr());
	~LogMediaProxy();
	void SetLog(const LogMediaPtr& pLog);
	virtual void Write(ELogMessageType type, ELogMessageLevel nLevel, LPCTSTR pszDate, LPCTSTR pszTime, LPCTSTR pszThreadId, LPCTSTR pszThreadName, LPCTSTR pszModule, LPCTSTR pszMessage);
	virtual bool Check(ELogMessageType type, ELogMessageLevel nLevel, LPCTSTR pszModule);
private:
	LogMediaPtr m_pLog;
	CriticalSection m_cs;
};

//! ����������� "����������" �����.
class LogMediaColl: public ILogMedia
{
	typedef std::vector<LogMediaPtr> MediaColl;
public:
	LogMediaColl();
	~LogMediaColl();
	void Add(const LogMediaPtr& pMedia);
	virtual void Write(ELogMessageType type, ELogMessageLevel nLevel, LPCTSTR pszDate, LPCTSTR pszTime, LPCTSTR pszThreadId, LPCTSTR pszThreadName, LPCTSTR pszModule, LPCTSTR pszMessage);
	virtual bool Check(ELogMessageType type, ELogMessageLevel nLevel, LPCTSTR pszModule);
private:
	MediaColl m_MediaColl;
	CriticalSection m_cs;
};

//! ��������� ���������� ��������� ����.
class IFilter
{
public:
	virtual ~IFilter() {}
	//! ���������, ��������� ��� ������������ ���������.
	//!   \return false - ��������� ������������.
	virtual bool Check(
		ELogMessageType& type,		//!< [in/out] ��� ���������.
		ELogMessageLevel& nLevel,	//!< [in/out] ������� �������� (L0 - �������� ������).
		LPCTSTR pszModule			//!< [in] ������, �� �������� �������� ���������.
		) = 0;
};

typedef std::shared_ptr<IFilter> FilterPtr;

//! ������ �� ���� � ������ ���������.
//! ��������� ��������� ����� ������������� ���� � ��������� ���������� ���� � ��������� �� ���� ���������.
class TypeLevelFilter: public IFilter
{
	ELogMessageType m_type;
	ELogMessageLevel m_nLevel;
public:
	//! �����������.
	TypeLevelFilter(
		ELogMessageType type = eLM_Info,	//!< [in] ��������� ��������� ����������ee type.
		ELogMessageLevel nLevel = LMAX	//!< [in] ��������� ���� type � ��������� �� ���� nLevel.
		): m_type(type), m_nLevel(nLevel) {}
	virtual bool Check(ELogMessageType& type, ELogMessageLevel& nLevel, LPCTSTR)
	{
		return type < m_type || (type == m_type && nLevel <= m_nLevel);
	}
};

//! ����������� ���.
class FilterLogMedia: public ILogMedia
{
	typedef std::vector<FilterPtr> FilterColl;
public:
	//! �����������.
	FilterLogMedia(
		const LogMediaPtr& pMedia	//!< [in] ���, �� ������� ������������� ������.
		);
	virtual ~FilterLogMedia();
	//! ��������� ������.
	void AddFilter(
		FilterPtr pFilter	//!< [in] ������.
		);
	virtual void Write(ELogMessageType type, ELogMessageLevel nLevel, LPCTSTR pszDate, LPCTSTR pszTime, LPCTSTR pszThreadId, LPCTSTR pszThreadName, LPCTSTR pszModule, LPCTSTR pszMessage);
	virtual bool Check(ELogMessageType type, ELogMessageLevel nLevel, LPCTSTR pszModule);

private:
	LogMediaPtr m_pMedia;
	FilterColl m_FilterColl;
	CriticalSection m_cs;
};

//! ��������� �������� ����.
struct LogParam
{
	LogParam(
		LogMediaPtr pMedia = LogMediaPtr(),	//!< [in]
		LPCTSTR pszModule = NULL	//!< [in]
		): m_pMedia(pMedia), m_pszModule(pszModule) {}
	LogMediaPtr m_pMedia;
	LPCTSTR m_pszModule;
};

//! "����������" ���.
class LogBase
{
public:
	virtual ~LogBase();
	//! ���������� "����������" ��� ������� ����.
	LogMediaPtr GetMedia() const throw() { return m_pMedia; }

	//! ���������� ��������������� ���������.
	void Write(
		ELogMessageType type,	//!< [in] ��� ���������.
		ELogMessageLevel nLevel,//!< [in] ������� �������� (L0 - �������� ������).
		LPCTSTR pszMessage,		//!< [in] ���������.
		...
		) throw()
	{
		va_list ap;
		va_start(ap, pszMessage);
		WriteVA(type, nLevel, NULL, pszMessage, ap);
		va_end(ap);
	}

	//! ���������� ��������������� Unicode-���������.
	void WriteW(
		ELogMessageType type,	//!< [in] ��� ���������.
		ELogMessageLevel nLevel,//!< [in] ������� �������� (L0 - �������� ������).
		LPCWSTR pszModule,		//!< [in] ��� ������. (NULL - ��� �� ���������)
		LPCWSTR pszMessage,		//!< [in] ���������.
		...
		) throw()
	{
		va_list ap;
		va_start(ap, pszMessage);
		WriteVAW(type, nLevel, pszModule, pszMessage, ap);
		va_end(ap);
	}

	//! ���������� ��������������� ���������.
	virtual void WriteVA(
		ELogMessageType type,	//!< [in] ��� ���������.
		ELogMessageLevel nLevel,//!< [in] ������� �������� (L0 - �������� ������).
		LPCTSTR pszModule,		//!< [in] ��� ������. (NULL - ��� �� ���������)
		LPCTSTR pszMessage,		//!< [in] ���������.
		va_list args
		) throw();

	//! ���������� ��������������� Unicode-���������.
	virtual void WriteVAW(
		ELogMessageType type,	//!< [in] ��� ���������.
		ELogMessageLevel nLevel,//!< [in] ������� �������� (L0 - �������� ������).
		LPCWSTR pszModule,		//!< [in] ��� ������. (NULL - ��� �� ���������)
		LPCWSTR pszMessage,		//!< [in] ���������.
		va_list args
		) throw();

	void Debug(LPCTSTR pszMessage, ...) throw() { va_list ap; va_start(ap, pszMessage); WriteVA(eLM_Debug, L0, NULL, pszMessage, ap); va_end(ap); }
	void Debug(ELogMessageLevel nLevel, LPCTSTR pszMessage, ...) throw() { va_list ap; va_start(ap, pszMessage); WriteVA(eLM_Debug, nLevel, NULL, pszMessage, ap); va_end(ap); }

	void Info(LPCTSTR pszMessage, ...) throw() { va_list ap; va_start(ap, pszMessage); WriteVA(eLM_Info, L0, NULL, pszMessage, ap); va_end(ap); }
	void Info(ELogMessageLevel nLevel, LPCTSTR pszMessage, ...) throw() { va_list ap; va_start(ap, pszMessage); WriteVA(eLM_Info, nLevel, NULL, pszMessage, ap); va_end(ap); }

	void Warning(LPCTSTR pszMessage, ...) throw() { va_list ap; va_start(ap, pszMessage); WriteVA(eLM_Warning, L0, NULL, pszMessage, ap); va_end(ap); }
	void Warning(ELogMessageLevel nLevel, LPCTSTR pszMessage, ...) throw() { va_list ap; va_start(ap, pszMessage); WriteVA(eLM_Warning, nLevel, NULL, pszMessage, ap); va_end(ap); }

	void Error(LPCTSTR pszMessage, ...) throw() { va_list ap; va_start(ap, pszMessage); WriteVA(eLM_Error, L0, NULL, pszMessage, ap); va_end(ap); }
	void Error(ELogMessageLevel nLevel, LPCTSTR pszMessage, ...) throw() { va_list ap; va_start(ap, pszMessage); WriteVA(eLM_Error, nLevel, NULL, pszMessage, ap); va_end(ap); }
	
	//! ���������, ������ �� ��� ��������� � ���.
	bool IsFiltered(ELogMessageType type, ELogMessageLevel nLevel);
	
	//! ������������� �������� ������.
	static void SetThreadName(
		LPCTSTR pszThreadName	//!< [in] ��� ������. ��������� ������ ���� �������� � ������� ������ ������.
		);
	
	//! ��������� ������������� �������� ������.
	static LPCTSTR GetThreadName();
	
	//! ���������� "����������" ��� ����������. (������������ ����� LogMediaProxy)
	static LogMediaPtr GetAppLogMedia();
	
	//! ������������� "����������" ��� ����������.
	static void SetAppLogMedia(LogMediaPtr pLog);
	
	static LogMediaPtr CreateConsoleMedia();
	
	static LogMediaPtr CreateFileMedia(
		LPCTSTR pszFilename,	//!< [in] ��� �����.
		bool bAppend,	//!< [in] ���� ���������� � �����.
		bool bFlush	= false,	//!< [in] ���� ������ �� ���� ����� ������� Write.
		bool bNewFileDaily = false
		);
	
	enum EDebugMediaStartParams
	{
		DEBUGMEDIA_NORMAL = 0x1,
		DEBUGMEDIA_FORCE = 0x2,			//!< ������� ���� ���� ��� ���������.
		DEBUGMEDIA_REPORTERRORS = 0x4,	//!< �������� ������ ����� _CrtDbgReport.
	};
	static LogMediaPtr CreateDebugMedia(
		DWORD dwParam = DEBUGMEDIA_NORMAL	//!< [in] ������� ���� ���� ��� ���������.
		);
	
	/*! ������ ��� �� �������� � �������.
	� ��������� ����� ����� ��������� ��������� ����������:
	  REG_SZ	"TraceHeader" - ������ ������� � ��� ����� �������� ���� �����
	  REG_SZ	"TraceFile" - ��� ����� ��������� � �����, ���������� � �������� ����������
	  REG_DWORD	"TraceFileAppend" (def: 1) - ���� ���������� � ����� �����
	  REG_DWORD	"TraceFileFlush" (def: 0) - ���� ������ �� ���� ����� ������ ������
	  REG_DWORD	"TraceToApp" = 1 - ��� ����� ��������� � ���� ����������.
	  REG_DWORD	"TraceToConsole" = 1 - ��� ����� ��������� � ���� �������
	  REG_DWORD	"TraceToDebug" = 1 - ��� ����� ��������� � ���� ��������� (Debug Output).
	  �� ��� ��� ���� ����� �������� ������.
	    �������� ��������� �������:
			REG_SZ		"TraceType" = ["debug"|"info"|"warning"|"error"] - ���, ��������� ���� �������� ������������ �� �����.
			REG_DWORD	"TraceLevel" = [0|1|2|...] - ������� ���� TraceType, ��������� ���� �������� ������������ �� �����.
	  ���� ���������� ����������� ������� ������������� ��� ������� ����, ��� ����������
	  ��������� ����� ������ ���� (��������, ������ � ��� �����), �� � ��������� �����
	  ����� ������� �������� "Trace1" "Trace2" � �.�. (������ ���� ��������������� ������� � 1)
	  � ������� ����� ������� �������������� ���� � �������.
	  ������� � ����� ������ � �� �������� ����!
	  ������:
		HLKM\....\SomeComp
			REG_SZ		"TraceFile" = "c:\my.log"
		HLKM\....\SomeComp\Trace1
			REG_DWORD	"TraceToDebug" = 1
			REG_SZ		"TraceType" = "warning"
			REG_DWORD	"TraceLevel" = 5
		��������: ������ � ���� ��, � ����� ��� ������ � �������������� ������ 5 � ����.


	   \return ���, ������� ����� �������� � Log.
	*/
	static LogMediaPtr CreateMediaFromRegistry(
		HKEY hRoot,				//!< [in] �������� ������� �������.
		LPCTSTR pszPath,		//!< [in] ���� �� ��������� ��������, �� �������, ����������� �������� ����.
		bool bAppLog = false	//!< [in] ��� ���� ���������� ������ ���� true. ����� ������������� ��� ���������� (SetAppLogMedia()).
		);

protected:
	//! �����������.
	LogBase(
		LogMediaPtr pMedia,	//!< [in] ���.
		LPCTSTR pszModule		//!< [in] ��� ������.
		);
	LogMediaPtr m_pMedia;
	std::basic_string<TCHAR> m_szModule;
};

class LocalLog: public LogBase
{
public:
	//! ����������� ��� ���-����������.
	LocalLog(
		const LogParam& param,	//!< [in] ��������� �������� ����.
		LPCTSTR pszModule		//!< [in] ��� ������ �� ���������.
		);
	//! �����������.
	LocalLog(
		LogMediaPtr pMedia,	//!< [in] ���.
		LPCTSTR pszModule		//!< [in] ��� ������.
		);
	//! �����������.
	LocalLog(
		const LogBase& log,	//!< [in] ���.
		LPCTSTR pszModule		//!< [in] ��� ������.
		);
	virtual ~LocalLog();
};

class Log: public LogBase
{
public:
	//! ����������� ��� ���-����������.
	Log(
		const LogParam& param,	//!< [in] ��������� �������� ����.
		LPCTSTR pszModule		//!< [in] ��� ������ �� ���������.
		);
	//! ����������� ��� �������� ���� ���������� ��� ����������.
	Log(
		LogMediaPtr pMedia,	//!< [in] ���.
		LPCTSTR pszModule		//!< [in] ��� ������.
		);
	//! ����������� ��� ���-����������.
	Log(
		const LogBase& log,	//!< [in] ���.
		LPCTSTR pszModule		//!< [in] ��� ������.
		);
	virtual ~Log();
	void SetParams(
		LogMediaPtr pMedia,		//!< [in] ����� ���.
		LPCTSTR pszModule = NULL	//!< [in] ����� ��� ������. ����� ���� NULL.
		);
	//! ���������� ��������������� ���������.
	virtual void WriteVA(
		ELogMessageType type,	//!< [in] ��� ���������.
		ELogMessageLevel nLevel,//!< [in] ������� �������� (L0 - �������� ������).
		LPCTSTR pszModule,		//!< [in] ��� ������.
		LPCTSTR pszMessage,		//!< [in] ���������.
		va_list args
		) throw();
	//! ���������� ��������������� Unicode-���������.
	virtual void WriteVAW(
		ELogMessageType type,	//!< [in] ��� ���������.
		ELogMessageLevel nLevel,//!< [in] ������� �������� (L0 - �������� ������).
		LPCWSTR pszModule,		//!< [in] ��� ������.
		LPCWSTR pszMessage,		//!< [in] ���������.
		va_list args
		) throw();
private:
	CriticalSection m_cs;
};


//! "����������" ��� � �����������.
class FilterLog: public Log
{
	typedef std::shared_ptr<FilterLogMedia> TFilterLogMediaPtr;
	TFilterLogMediaPtr GetFilterLogMedia() { return std::static_pointer_cast<FilterLogMedia>(GetMedia()); }
public:
	FilterLog(const LogParam& param, LPCTSTR pszModule): Log(param, pszModule)
	{
		LogMediaPtr pMedia = GetMedia();
		if (pMedia)
			SetParams(LogMediaPtr(new FilterLogMedia(pMedia)));
	}
	virtual ~FilterLog();
	void AddFilter(FilterPtr pFilter) { if (GetFilterLogMedia()) GetFilterLogMedia()->AddFilter(pFilter); }
};

#endif