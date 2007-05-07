#include "StdAfx.h"
#include <objbase.h>
#include "SubWCRev_h.h"
#include "SubWCRev_i.c"
#include "SubWCRevCOM.h"

#include <tchar.h>
#include <windows.h>
#include <shlwapi.h>

#include <apr_pools.h>
#include "svn_error.h"
#include "svn_client.h"
#include "svn_path.h"
#include "SubWCRev.h"
#include "Register.h"


void AutomationMain()
{
	// initialize the COM library
	::CoInitialize(NULL);

	apr_pool_t * pool;
	svn_client_ctx_t ctx;

	apr_initialize();
	apr_pool_create_ex (&pool, NULL, NULL, NULL);
	memset (&ctx, 0, sizeof (ctx));

	size_t ret = 0;
	getenv_s(&ret, NULL, 0, "SVN_ASP_DOT_NET_HACK");
	if (ret)
	{
		svn_wc_set_adm_dir("_svn", pool);
	}

	// register ourself as a class object against the internal COM table
	DWORD nToken = CoEXEInitialize();



	//
	// (loop ends if WM_QUIT message is received)
	//
	MSG msg;
	while (GetMessage(&msg, 0, 0, 0) > 0) 
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}


	// unregister from the known table of class objects
	CoEXEUninitialize(nToken);

	apr_terminate2();

	// 
	::CoUninitialize();
}

static long g_cComponents = 0 ;     // Count of active components
static long g_cServerLocks = 0 ;    // Count of locks

static HMODULE g_hModule = NULL ;   // DLL module handle

//
// Constructor
//
SubWCRev::SubWCRev() : m_cRef(1)
{ 
	InterlockedIncrement(&g_cComponents) ; 

	m_ptinfo = NULL;
	LoadTypeInfo(&m_ptinfo, LIBID_LibSubWCRev, IID_ISubWCRev, 0);
}

//
// Destructor
//
SubWCRev::~SubWCRev() 
{ 
	InterlockedDecrement(&g_cComponents) ; 
}

//
// IUnknown implementation
//
HRESULT __stdcall SubWCRev::QueryInterface(const IID& iid, void** ppv)
{    
	if (iid == IID_IUnknown || iid == IID_ISubWCRev || iid == IID_IDispatch)
	{
		*ppv = static_cast<ISubWCRev*>(this) ; 
	}
	else
	{
		*ppv = NULL ;
		return E_NOINTERFACE ;
	}
	reinterpret_cast<IUnknown*>(*ppv)->AddRef() ;
	return S_OK ;
}

ULONG __stdcall SubWCRev::AddRef()
{
	return InterlockedIncrement(&m_cRef) ;
}

ULONG __stdcall SubWCRev::Release() 
{
	if (InterlockedDecrement(&m_cRef) == 0)
	{
		delete this ;
		::PostMessage(NULL,WM_QUIT,0,0);
		return 0 ;
	}
	return m_cRef ;
}

//
// ISubWCRev implementation
//
HRESULT __stdcall SubWCRev::GetWCInfo(/*[in]*/ BSTR wcPath, /*[in]*/VARIANT_BOOL folders, /*[in]*/VARIANT_BOOL externals)
{
	if (wcPath==NULL)
		return ERROR_INVALID_PARAMETER;

	HRESULT hr = S_OK;
	apr_pool_t * pool;
	svn_error_t * svnerr = NULL;
	svn_client_ctx_t ctx;
	const char * internalpath;
	memset (&SubStat, 0, sizeof (SubStat));
	SubStat.bFolders = folders;
	SubStat.bExternals = externals;

	apr_pool_create_ex (&pool, NULL, NULL, NULL);
	memset (&ctx, 0, sizeof (ctx));

	size_t ret = 0;
	getenv_s(&ret, NULL, 0, "SVN_ASP_DOT_NET_HACK");
	if (ret)
	{
		svn_wc_set_adm_dir("_svn", pool);
	}

	char *wc_utf8;
	wc_utf8 = Utf16ToUtf8((WCHAR*)wcPath, pool);
	internalpath = svn_path_internal_style (wc_utf8, pool);

	svnerr = svn_status(	internalpath,	//path
							&SubStat,		//status_baton
							TRUE,			//noignore
							&ctx,
							pool);

	if (svnerr)
	{
		hr = S_FALSE;
	}
	return hr;
}

HRESULT __stdcall SubWCRev::get_Revision(/*[out, retval]*/VARIANT* rev)
{
	rev->vt = VT_I4;
	rev->lVal = SubStat.CmtRev;
	return S_OK;
}

HRESULT __stdcall SubWCRev::get_MinRev(/*[out, retval]*/VARIANT* rev)
{
	rev->vt = VT_I4;
	rev->lVal = SubStat.MinRev;
	return S_OK;
}

HRESULT __stdcall SubWCRev::get_MaxRev(/*[out, retval]*/VARIANT* rev)
{
	rev->vt = VT_I4;
	rev->lVal = SubStat.MaxRev;
	return S_OK;
}

HRESULT __stdcall SubWCRev::get_Date(/*[out, retval]*/VARIANT* date)
{
	date->vt = VT_BSTR;

	__time64_t ttime;
	ttime = SubStat.CmtDate/1000000L;

	struct tm newtime;
	if (_localtime64_s(&newtime, &ttime))
		return FALSE;
	// Format the date/time in international format as yyyy/mm/dd hh:mm:ss
	WCHAR destbuf[32];
	_stprintf_s(destbuf, 32, _T("%04d/%02d/%02d %02d:%02d:%02d"),
		newtime.tm_year + 1900,
		newtime.tm_mon + 1,
		newtime.tm_mday,
		newtime.tm_hour,
		newtime.tm_min,
		newtime.tm_sec);
	date->bstrVal = SysAllocStringLen(destbuf, _tcslen(destbuf));
	return S_OK;
}

HRESULT __stdcall SubWCRev::get_Url(/*[out, retval]*/VARIANT* url)
{
	url->vt = VT_BSTR;

	WCHAR * buf;
	int len = strlen(SubStat.Url);
	buf = new WCHAR[len*4 + 1];
	ZeroMemory(buf, (len*4 + 1)*sizeof(WCHAR));
	MultiByteToWideChar(CP_UTF8, 0, SubStat.Url, -1, buf, len*4);
	url->bstrVal = SysAllocString(buf);
	delete [] buf;

	return S_OK;
}

HRESULT __stdcall SubWCRev::get_HasModifications(VARIANT_BOOL* modifications)
{
	*modifications = !!SubStat.HasMods;
	return S_OK;
}

HRESULT SubWCRev::LoadTypeInfo(ITypeInfo ** pptinfo, const CLSID &libid, const CLSID &iid, LCID lcid)
{
	HRESULT hr;
	LPTYPELIB ptlib = NULL;
	LPTYPEINFO ptinfo = NULL;

	*pptinfo = NULL;

	// Load type library.
	hr = LoadRegTypeLib(libid, 1, 0, lcid, &ptlib);
	if (FAILED(hr))
		return hr;

	// Get type information for interface of the object.
	hr = ptlib->GetTypeInfoOfGuid(iid, &ptinfo);
	if (FAILED(hr))
	{
		ptlib->Release();
		return hr;
	}

	ptlib->Release();
	*pptinfo = ptinfo;
	return NOERROR;
}

HRESULT __stdcall SubWCRev::GetTypeInfoCount(UINT* pctinfo)
{
	*pctinfo = 1;
	return S_OK;
}
HRESULT __stdcall SubWCRev::GetTypeInfo(UINT itinfo, LCID /*lcid*/, ITypeInfo** pptinfo)
{
	*pptinfo = NULL;

	if(itinfo != 0)
		return ResultFromScode(DISP_E_BADINDEX);

	m_ptinfo->AddRef();      // AddRef and return pointer to cached
	// typeinfo for this object.
	*pptinfo = m_ptinfo;

	return NOERROR;
}

HRESULT __stdcall SubWCRev::GetIDsOfNames(REFIID /*riid*/, LPOLESTR* rgszNames, UINT cNames,
											 LCID /*lcid*/, DISPID* rgdispid)
{
	return DispGetIDsOfNames(m_ptinfo, rgszNames, cNames, rgdispid);
}
HRESULT __stdcall SubWCRev::Invoke(DISPID dispidMember, REFIID /*riid*/,
									  LCID /*lcid*/, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
									  EXCEPINFO* pexcepinfo, UINT* puArgErr)
{
	return DispInvoke(
		this, m_ptinfo,
		dispidMember, wFlags, pdispparams,
		pvarResult, pexcepinfo, puArgErr); 
}

//
// Class factory IUnknown implementation
//
HRESULT __stdcall CFactory::QueryInterface(const IID& iid, void** ppv)
{    
	if ((iid == IID_IUnknown) || (iid == IID_IClassFactory))
	{
		*ppv = static_cast<IClassFactory*>(this) ; 
	}
	else
	{
		*ppv = NULL ;
		return E_NOINTERFACE ;
	}
	reinterpret_cast<IUnknown*>(*ppv)->AddRef() ;
	return S_OK ;
}

ULONG __stdcall CFactory::AddRef()
{
	return InterlockedIncrement(&m_cRef) ;
}

ULONG __stdcall CFactory::Release() 
{
	if (InterlockedDecrement(&m_cRef) == 0)
	{
		delete this ;
		return 0 ;
	}
	return m_cRef ;
}

//
// IClassFactory implementation
//
HRESULT __stdcall CFactory::CreateInstance(IUnknown* pUnknownOuter,
										   const IID& iid,
										   void** ppv) 
{
	// Cannot aggregate.
	if (pUnknownOuter != NULL)
	{
		return CLASS_E_NOAGGREGATION ;
	}

	// Create component.
	SubWCRev* pA = new SubWCRev();
	if (pA == NULL)
	{
		return E_OUTOFMEMORY ;
	}

	// Get the requested interface.
	HRESULT hr = pA->QueryInterface(iid, ppv) ;

	// Release the IUnknown pointer.
	// (If QueryInterface failed, component will delete itself.)
	pA->Release() ;
	return hr ;
}

// LockServer
HRESULT __stdcall CFactory::LockServer(BOOL bLock) 
{
	if (bLock)
	{
		InterlockedIncrement(&g_cServerLocks) ; 
	}
	else
	{
		InterlockedDecrement(&g_cServerLocks) ;
	}
	return S_OK ;
}


///////////////////////////////////////////////////////////
//
// Exported functions
//

//
// Can DLL unload now?
//
STDAPI DllCanUnloadNow()
{
	if ((g_cComponents == 0) && (g_cServerLocks == 0))
	{
		return S_OK ;
	}
	else
	{
		return S_FALSE ;
	}
}

//
// Get class factory
//
STDAPI DllGetClassObject(const CLSID& clsid,
						 const IID& iid,
						 void** ppv)
{
	// Can we create this component?
	if (clsid != CLSID_SubWCRev)
	{
		return CLASS_E_CLASSNOTAVAILABLE ;
	}

	// Create class factory.
	CFactory* pFactory = new CFactory ;  // Reference count set to 1
	// in constructor
	if (pFactory == NULL)
	{
		return E_OUTOFMEMORY ;
	}

	// Get requested interface.
	HRESULT hr = pFactory->QueryInterface(iid, ppv) ;
	pFactory->Release() ;

	return hr ;
}

CFactory gClassFactory;

DWORD CoEXEInitialize()
{
	DWORD nReturn;

	::CoRegisterClassObject(CLSID_SubWCRev,
		&gClassFactory,
		CLSCTX_SERVER, 
		REGCLS_MULTIPLEUSE, 
		&nReturn);

	return nReturn;
}

void CoEXEUninitialize(DWORD nToken)
{
	::CoRevokeClassObject(nToken);
}

//
// Server registration
//
STDAPI DllRegisterServer()
{

	g_hModule = ::GetModuleHandle(NULL);

	HRESULT hr= RegisterServer(g_hModule, 
		CLSID_SubWCRev,
		_T("SubWCRev Server Object"),
		_T("SubWCRev.object"),
		_T("SubWCRev.object.1"),
		LIBID_LibSubWCRev) ;
	if (SUCCEEDED(hr))
	{
		RegisterTypeLib( g_hModule, NULL);
	}
	return hr;
}

//
// Server unregistration
//
STDAPI DllUnregisterServer()
{

	g_hModule = ::GetModuleHandle(NULL);

	HRESULT hr= UnregisterServer(CLSID_SubWCRev,
		_T("SubWCRev.object"),
		_T("SubWCRev.object.1"),
		LIBID_LibSubWCRev) ;
	if (SUCCEEDED(hr))
	{
		UnRegisterTypeLib( g_hModule, NULL);
	}
	return hr;
}

///////////////////////////////////////////////////////////
//
// DLL module information
//
BOOL APIENTRY DllMain(HANDLE hModule,
					  DWORD dwReason,
					  void* /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		g_hModule = (HMODULE)hModule ;
	}

	return TRUE ;
}
