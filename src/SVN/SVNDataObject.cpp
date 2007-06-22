// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007 - TortoiseSVN

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
#include "StdAfx.h"
#include "SVNDataObject.h"
#include "UnicodeUtils.h"
#include "PathUtils.h"
#include "TempFile.h"

CLIPFORMAT CF_FILECONTENTS = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILECONTENTS);
CLIPFORMAT CF_FILEDESCRIPTOR = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR);
CLIPFORMAT CF_PREFERREDDROPEFFECT = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);

SVNDataObject::SVNDataObject(const CTSVNPathList& svnpaths, SVNRev peg, SVNRev rev) : m_svnPaths(svnpaths)
	, m_pegRev(peg)
	, m_revision(rev)
	, m_bInOperation(FALSE)
	, m_bIsAsync(TRUE)
	, m_cRefCount(0)
{
}

SVNDataObject::~SVNDataObject()
{
}

//////////////////////////////////////////////////////////////////////////
// IUnknown
//////////////////////////////////////////////////////////////////////////

STDMETHODIMP SVNDataObject::QueryInterface(REFIID riid, void** ppvObject)
{
	*ppvObject = NULL;
	if (IID_IUnknown==riid || IID_IDataObject==riid)
		*ppvObject=this;
	if (riid == IID_IAsyncOperation)
		*ppvObject = (IAsyncOperation*)this;

	if (NULL!=*ppvObject)
	{
		((LPUNKNOWN)*ppvObject)->AddRef();
		return S_OK;
	}
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) SVNDataObject::AddRef(void)
{
	return ++m_cRefCount;
}

STDMETHODIMP_(ULONG) SVNDataObject::Release(void)
{
	--m_cRefCount;
	if (m_cRefCount == 0)
	{
		delete this;
		return 0;
	}
	return m_cRefCount;
}

//////////////////////////////////////////////////////////////////////////
// IDataObject
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP SVNDataObject::GetData(FORMATETC* pformatetcIn, STGMEDIUM* pmedium)
{
	if (pformatetcIn == NULL || pmedium == NULL)
		return E_INVALIDARG;
	pmedium->hGlobal = NULL;

	if ((pformatetcIn->tymed & TYMED_ISTREAM) && (pformatetcIn->dwAspect == DVASPECT_CONTENT) && (pformatetcIn->cfFormat == CF_FILECONTENTS))
	{
		// supports the IStream format.
		// The lindex param is the index of the file to return

		// Note: we don't get called for directories since those are simply created and don't
		// need to be fetched.

		// Note2: It would be really nice if we could get a stream from the subversion library
		// from where we could read the file contents. But the Subversion lib is not implemented
		// to *read* from a remote stream but so that the library *writes* to a stream we pass.
		// Since we can't get such a read stream, we have to fetch the file in whole first to
		// a temp location and then pass the shell an IStream for that temp file.
		CTSVNPath filepath = CTempFiles::Instance().GetTempFilePath(true);
		if (!m_svn.Cat(CTSVNPath(m_allPaths[pformatetcIn->lindex].infodata.url), m_pegRev, m_revision, filepath))
		{
			DeleteFile(filepath.GetWinPath());
			return STG_E_ACCESSDENIED;
		}

		IStream * pIStream = NULL;
		HRESULT res = SHCreateStreamOnFile(filepath.GetWinPath(), STGM_READ, &pIStream);
		if (res == S_OK)
		{
			pmedium->pstm = pIStream;
			pmedium->tymed = TYMED_ISTREAM;
			return S_OK;
		}
		return res;
	}
	else if ((pformatetcIn->tymed & TYMED_HGLOBAL) && (pformatetcIn->dwAspect == DVASPECT_CONTENT) && (pformatetcIn->cfFormat == CF_FILEDESCRIPTOR))
	{
		// now it is time to get all subfolders for the directories we have
		SVNInfo svnInfo;
		// find the common directory of all the paths
		CTSVNPath commonDir;
		bool bAllUrls = true;
		for (int i=0; i<m_svnPaths.GetCount(); ++i)
		{
			if (!m_svnPaths[i].IsUrl())
				bAllUrls = false;
			if (commonDir.IsEmpty())
				commonDir = m_svnPaths[i].GetContainingDirectory();
			if (!commonDir.IsEquivalentTo(m_svnPaths[i].GetContainingDirectory()))
			{
				commonDir.Reset();
				break;
			}
		}
		if (bAllUrls && (m_svnPaths.GetCount() > 1) && !commonDir.IsEmpty())
		{
			// if all paths are in the same directory, we can fetch the info recursively
			// from the parent folder to save a lot of time.
			const SVNInfoData * infodata = svnInfo.GetFirstFileInfo(commonDir, m_pegRev, m_revision, true);
			while (infodata)
			{
				// check if the returned item is one in our list
				for (int i=0; i<m_svnPaths.GetCount(); ++i)
				{
					if (m_svnPaths[i].IsAncestorOf(CTSVNPath(infodata->url)))
					{
						SVNDataObject::SVNObjectInfoData id = {m_svnPaths[i], *infodata};
						m_allPaths.push_back(id);
						break;
					}
				}
				infodata = svnInfo.GetNextFileInfo();
			}
		}
		else
		{
			for (int i = 0; i < m_svnPaths.GetCount(); ++i)
			{
				const SVNInfoData * infodata = svnInfo.GetFirstFileInfo(m_svnPaths[i], m_pegRev, m_revision, true);
				while (infodata)
				{
					SVNDataObject::SVNObjectInfoData id = {m_svnPaths[i], *infodata};
					m_allPaths.push_back(id);
					infodata = svnInfo.GetNextFileInfo();
				}
			}
		}
		unsigned int dataSize = sizeof(FILEGROUPDESCRIPTOR) + ((m_allPaths.size() - 1) * sizeof(FILEDESCRIPTOR));
		HGLOBAL data = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE | GMEM_ZEROINIT, dataSize);

		FILEGROUPDESCRIPTOR* files = (FILEGROUPDESCRIPTOR*)GlobalLock(data);
		files->cItems = m_allPaths.size();
		int i = 0;
		for (vector<SVNDataObject::SVNObjectInfoData>::const_iterator it = m_allPaths.begin(); it != m_allPaths.end(); ++it)
		{
			CString temp = it->infodata.url.Mid(it->rootpath.GetContainingDirectory().GetSVNPathString().GetLength()+1);
			// we have to unescape the urls since the local filesystem doesn't need them
			// escaped and it would only look really ugly (and be wrong).
			temp = CPathUtils::PathUnescape(temp);
			_tcscpy_s(files->fgd[i].cFileName, MAX_PATH, (LPCTSTR)temp);
			files->fgd[i].dwFlags = FD_ATTRIBUTES | FD_PROGRESSUI;
			files->fgd[i].dwFileAttributes = (it->infodata.kind == svn_node_dir) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
			++i;
		}

		GlobalUnlock(data);

		pmedium->hGlobal = data;
		pmedium->tymed = TYMED_HGLOBAL;
		return S_OK;
	}
	// handling CF_PREFERREDDROPEFFECT is necessary to tell the shell that it should *not* ask for the
	// CF_FILEDESCRIPTOR until the drop actually occurs. If we don't handle CF_PREFERREDDROPEFFECT, the shell
	// will ask for the file descriptor for every object (file/folder) the mousepointer hoovers over and is
	// a potential drop target.
	else if ((pformatetcIn->tymed & TYMED_HGLOBAL) && (pformatetcIn->cfFormat == CF_PREFERREDDROPEFFECT))
	{
		HGLOBAL data = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE | GMEM_ZEROINIT, sizeof(DWORD));
		DWORD* effect = (DWORD*) GlobalLock(data);
		(*effect) = DROPEFFECT_COPY;
		GlobalUnlock(data);
		pmedium->hGlobal = data;
		pmedium->tymed = TYMED_HGLOBAL;
		return S_OK;
	}
	else if ((pformatetcIn->tymed & TYMED_HGLOBAL) && (pformatetcIn->dwAspect == DVASPECT_CONTENT) && (pformatetcIn->cfFormat == CF_TEXT))
	{
		// caller wants text
		// create the string from the pathlist
		CString text;
		if (m_svnPaths.GetCount())
		{
			// create a single string where the URLs are separated by newlines
			for (int i=0; i<m_svnPaths.GetCount(); ++i)
			{
				if (m_svnPaths[i].IsUrl())
					text += m_svnPaths[i].GetSVNPathString();
				else
					text += m_svnPaths[i].GetWinPathString();
				text += _T("\r\n");
			}
		}
		CStringA texta = CUnicodeUtils::GetUTF8(text);
		pmedium->tymed = TYMED_HGLOBAL;
		pmedium->hGlobal = GlobalAlloc(GHND, (texta.GetLength()+1)*sizeof(char));
		if (pmedium->hGlobal)
		{
			char* pMem = (char*)GlobalLock(pmedium->hGlobal);
			strcpy_s(pMem, texta.GetLength()+1, (LPCSTR)texta);
			GlobalUnlock(pmedium->hGlobal);
		}
		pmedium->pUnkForRelease = NULL;
		return S_OK;
	}
	else if ((pformatetcIn->tymed & TYMED_HGLOBAL) && (pformatetcIn->dwAspect == DVASPECT_CONTENT) && (pformatetcIn->cfFormat == CF_UNICODETEXT))
	{
		// caller wants Unicode text
		// create the string from the pathlist
		CString text;
		if (m_svnPaths.GetCount())
		{
			// create a single string where the URLs are separated by newlines
			for (int i=0; i<m_svnPaths.GetCount(); ++i)
			{
				if (m_svnPaths[i].IsUrl())
					text += m_svnPaths[i].GetSVNPathString();
				else
					text += m_svnPaths[i].GetWinPathString();
				text += _T("\r\n");
			}
		}
		pmedium->tymed = TYMED_HGLOBAL;
		pmedium->hGlobal = GlobalAlloc(GHND, (text.GetLength()+1)*sizeof(TCHAR));
		if (pmedium->hGlobal)
		{
			TCHAR* pMem = (TCHAR*)GlobalLock(pmedium->hGlobal);
			_tcscpy_s(pMem, text.GetLength()+1, (LPCTSTR)text);
			GlobalUnlock(pmedium->hGlobal);
		}
		pmedium->pUnkForRelease = NULL;
		return S_OK;
	}

	return DV_E_FORMATETC;
}

STDMETHODIMP SVNDataObject::GetDataHere(FORMATETC* /*pformatetc*/, STGMEDIUM* /*pmedium*/)
{
	return E_NOTIMPL;
}

STDMETHODIMP SVNDataObject::QueryGetData(FORMATETC* pformatetc)
{ 
	if (pformatetc == NULL)
		return E_INVALIDARG;

	if (!(DVASPECT_CONTENT & pformatetc->dwAspect))
		return DV_E_DVASPECT;

	if ((pformatetc->tymed & TYMED_ISTREAM) || (pformatetc->tymed & TYMED_HGLOBAL))
	{
		if (pformatetc->dwAspect == DVASPECT_CONTENT)
		{
			if ((pformatetc->cfFormat == CF_FILECONTENTS) ||
				(pformatetc->cfFormat == CF_FILEDESCRIPTOR) ||
				(pformatetc->cfFormat == CF_PREFERREDDROPEFFECT) ||
				(pformatetc->cfFormat == CF_TEXT) ||
				(pformatetc->cfFormat == CF_UNICODETEXT))
				return S_OK;
			return DV_E_FORMATETC;
		}
		else
			return DV_E_DVASPECT;
	}
	return DV_E_TYMED;
}

STDMETHODIMP SVNDataObject::GetCanonicalFormatEtc(FORMATETC* /*pformatectIn*/, FORMATETC* pformatetcOut)
{ 
	if (pformatetcOut == NULL)
		return E_INVALIDARG;
	return DATA_S_SAMEFORMATETC;
}

STDMETHODIMP SVNDataObject::SetData(FORMATETC* /*pformatetc*/, STGMEDIUM* /*pmedium*/, BOOL /*fRelease*/)
{ 
	return E_NOTIMPL;
}

STDMETHODIMP SVNDataObject::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppenumFormatEtc)
{ 
	if (ppenumFormatEtc == NULL)
		return E_POINTER;

	*ppenumFormatEtc = NULL;
	switch (dwDirection)
	{
	case DATADIR_GET:
		*ppenumFormatEtc= new CSVNEnumFormatEtc();
		if (*ppenumFormatEtc == NULL)
			return E_OUTOFMEMORY;
		(*ppenumFormatEtc)->AddRef(); 
		break;
	default:
		return E_NOTIMPL;
	}
	return S_OK;
}

STDMETHODIMP SVNDataObject::DAdvise(FORMATETC* /*pformatetc*/, DWORD /*advf*/, IAdviseSink* /*pAdvSink*/, DWORD* /*pdwConnection*/)
{ 
	return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP SVNDataObject::DUnadvise(DWORD /*dwConnection*/)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE SVNDataObject::EnumDAdvise(IEnumSTATDATA** /*ppenumAdvise*/)
{
	return OLE_E_ADVISENOTSUPPORTED;
}

//////////////////////////////////////////////////////////////////////////
// IAsyncOperation
//////////////////////////////////////////////////////////////////////////
HRESULT STDMETHODCALLTYPE SVNDataObject::SetAsyncMode(BOOL fDoOpAsync)
{
	m_bIsAsync = fDoOpAsync;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE SVNDataObject::GetAsyncMode(BOOL* pfIsOpAsync)
{
	if (!pfIsOpAsync)
		return E_FAIL;

	*pfIsOpAsync = m_bIsAsync;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE SVNDataObject::StartOperation(IBindCtx* /*pbcReserved*/)
{
	m_bInOperation = TRUE;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE SVNDataObject::InOperation(BOOL* pfInAsyncOp)
{
	if (!pfInAsyncOp)
		return E_FAIL;

	*pfInAsyncOp = m_bInOperation;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE SVNDataObject::EndOperation(HRESULT /*hResult*/, IBindCtx* /*pbcReserved*/, DWORD /*dwEffects*/)
{
	m_bInOperation = FALSE;
	Release();
	return S_OK;
}





CSVNEnumFormatEtc::CSVNEnumFormatEtc() : m_cRefCount(0)
	, m_iCur(0)
{
	m_formats[0].cfFormat = CF_UNICODETEXT;
	m_formats[0].dwAspect = DVASPECT_CONTENT;
	m_formats[0].lindex = -1;
	m_formats[0].ptd = NULL;
	m_formats[0].tymed = TYMED_HGLOBAL;

	m_formats[1].cfFormat = CF_TEXT;
	m_formats[1].dwAspect = DVASPECT_CONTENT;
	m_formats[1].lindex = -1;
	m_formats[1].ptd = NULL;
	m_formats[1].tymed = TYMED_HGLOBAL;

	m_formats[2].cfFormat = CF_FILECONTENTS;
	m_formats[2].dwAspect = DVASPECT_CONTENT;
	m_formats[2].lindex = -1;
	m_formats[2].ptd = NULL;
	m_formats[2].tymed = TYMED_ISTREAM;

	m_formats[3].cfFormat = CF_FILEDESCRIPTOR;
	m_formats[3].dwAspect = DVASPECT_CONTENT;
	m_formats[3].lindex = -1;
	m_formats[3].ptd = NULL;
	m_formats[3].tymed = TYMED_HGLOBAL;

	m_formats[4].cfFormat = CF_PREFERREDDROPEFFECT;
	m_formats[4].dwAspect = DVASPECT_CONTENT;
	m_formats[4].lindex = -1;
	m_formats[4].ptd = NULL;
	m_formats[4].tymed = TYMED_HGLOBAL;
}

STDMETHODIMP  CSVNEnumFormatEtc::QueryInterface(REFIID refiid, void** ppv)
{
	*ppv = NULL;
	if (IID_IUnknown==refiid || IID_IEnumFORMATETC==refiid)
		*ppv=this;

	if (*ppv != NULL)
	{
		((LPUNKNOWN)*ppv)->AddRef();
		return S_OK;
	}
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CSVNEnumFormatEtc::AddRef(void)
{
	return ++m_cRefCount;
}

STDMETHODIMP_(ULONG) CSVNEnumFormatEtc::Release(void)
{
	--m_cRefCount;
	if (m_cRefCount == 0)
	{
		delete this;
		return 0;
	}
	return m_cRefCount; 
}

STDMETHODIMP CSVNEnumFormatEtc::Next(ULONG celt, LPFORMATETC lpFormatEtc, ULONG* pceltFetched)
{
	if (pceltFetched != NULL)
		*pceltFetched = 0;

	ULONG cReturn = celt;

	if(celt <= 0 || lpFormatEtc == NULL || m_iCur >= SVNDATAOBJECT_NUMFORMATS)
		return S_FALSE;

	if (pceltFetched == NULL && celt != 1) // pceltFetched can be NULL only for 1 item request
		return S_FALSE;

	while (m_iCur < SVNDATAOBJECT_NUMFORMATS && cReturn > 0)
	{
		*lpFormatEtc++ = m_formats[m_iCur++];
		--cReturn;
	}

	if (pceltFetched != NULL)
		*pceltFetched = celt - cReturn;

	return (cReturn == 0) ? S_OK : S_FALSE;
}

STDMETHODIMP CSVNEnumFormatEtc::Skip(ULONG celt)
{
	if ((m_iCur + int(celt)) >= SVNDATAOBJECT_NUMFORMATS)
		return S_FALSE;
	m_iCur += celt;
	return S_OK;
}

STDMETHODIMP CSVNEnumFormatEtc::Reset(void)
{
	m_iCur = 0;
	return S_OK;
}

STDMETHODIMP CSVNEnumFormatEtc::Clone(IEnumFORMATETC** ppCloneEnumFormatEtc)
{
	if (ppCloneEnumFormatEtc == NULL)
		return E_POINTER;

	CSVNEnumFormatEtc *newEnum = new CSVNEnumFormatEtc();
	if (newEnum == NULL)
		return E_OUTOFMEMORY;

	newEnum->AddRef();
	newEnum->m_iCur = m_iCur;
	*ppCloneEnumFormatEtc = newEnum;
	return S_OK;
}



