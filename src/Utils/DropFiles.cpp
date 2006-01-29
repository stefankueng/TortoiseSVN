#include "stdafx.h"
#include "DropFiles.h"

CDropFiles::CDropFiles()
{
	m_pBuffer = NULL;
	m_nBufferSize = 0;
}

CDropFiles::~CDropFiles()
{
	delete m_pBuffer;
}

void CDropFiles::AddFile(const CString &sFile)
{
	m_arFiles.Add(sFile);
}

int CDropFiles::GetCount()
{
	return m_arFiles.GetCount();
}

void CDropFiles::CreateBuffer()
{
	ASSERT(m_pBuffer == NULL);
	ASSERT(m_nBufferSize == 0);
	ASSERT(m_arFiles.GetCount()>0);

	int nLength = 0;

	for(int i=0;i<m_arFiles.GetSize();i++)
	{
		nLength += m_arFiles[i].GetLength();
		nLength += 1; // '\0' separator
	}

	m_nBufferSize = sizeof(DROPFILES) + (nLength+1)*sizeof(TCHAR);
	m_pBuffer = new char[m_nBufferSize];
	
	ZeroMemory(m_pBuffer, m_nBufferSize);

	DROPFILES* df = (DROPFILES*)m_pBuffer;
	df->pFiles = sizeof(DROPFILES);
	df->fWide = 1;

	TCHAR* pFilenames = (TCHAR*)(m_pBuffer + sizeof(DROPFILES));
	TCHAR* pCurrentFilename = pFilenames;

	for(int i=0;i<m_arFiles.GetSize();i++)
	{
		CString str = m_arFiles[i];
		wcscpy_s(pCurrentFilename,str.GetLength()+1,str.GetBuffer());
		pCurrentFilename += str.GetLength(); 
		*pCurrentFilename = '\0'; // separator between file names
		pCurrentFilename++;
	}
	*pCurrentFilename = '\0'; // terminate array
}

void* CDropFiles::GetBuffer() const
{
	return (void*)m_pBuffer;
}

int	CDropFiles::GetBufferSize() const
{
	return m_nBufferSize;
}

void CDropFiles::CreateStructure()
{
	CreateBuffer();
	
	COleDataSource dropData;
	HGLOBAL hMem = ::GlobalAlloc(GMEM_ZEROINIT|GMEM_MOVEABLE|GMEM_DDESHARE, GetBufferSize()); 
	memcpy( ::GlobalLock(hMem), GetBuffer(), GetBufferSize() );
	::GlobalUnlock(hMem);
	dropData.CacheGlobalData( CF_HDROP, hMem );
	dropData.DoDragDrop(DROPEFFECT_COPY|DROPEFFECT_MOVE,NULL);
}
