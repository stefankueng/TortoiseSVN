// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2005 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
#pragma once

class CPatch;

/**
 * \ingroup TortoiseMerge
 * Virtual class providing the callback interface which
 * is used by CFilePatchesDlg.
 */
class CPatchFilesDlgCallBack
{
public:
	/**
	 * Callback function. Called when the user doubleclicks on a
	 * specific file to patch. The framework the has to process
	 * the patching/viewing.
	 * \param sFilePath the full path to the file to patch
	 * \param sVersion the revision number of the file to patch
	 * \return TRUE if patching was successful
	 */
	virtual BOOL PatchFile(CString sFilePath, CString sVersion, BOOL bAutoPatch = FALSE) = 0;
};

#define	FPDLG_FILESTATE_GOOD		0x0000
#define	FPDLG_FILESTATE_CONFLICTED	0x0001
#define FPDLG_FILESTATE_PATCHED		0x0002

#define ID_PATCHALL					1
/**
 * \ingroup TortoiseMerge
 *
 * Dialog class, showing all files to patch from a unified diff file.
 */
class CFilePatchesDlg : public CDialog
{
	DECLARE_DYNAMIC(CFilePatchesDlg)

public:
	CFilePatchesDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CFilePatchesDlg();

	/**
	 * Call this method to initialize the dialog.
	 * \param pPatch The CPatch object used to get the filenames from the unified diff file
	 * \param pCallBack The Object providing the callback interface (CPatchFilesDlgCallBack)
	 * \param sPath The path to the "parent" folder where the patch gets applied to
	 * \return TRUE if successful
	 */
	BOOL	Init(CPatch * pPatch, CPatchFilesDlgCallBack * pCallBack, CString sPath);

	BOOL	SetFileStatusAsPatched(CString sPath);
	enum { IDD = IDD_FILEPATCHES };
protected:
	CPatch *					m_pPatch;
	CPatchFilesDlgCallBack *	m_pCallBack;
	CString						m_sPath;
	CListCtrl					m_cFileList;
	CDWordArray					m_arFileStates;
	CImageList					m_ImgList;
	BOOL						m_bMinimized;
	int							m_nWindowHeight;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLvnGetInfoTipFilelist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkFilelist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMCustomdrawFilelist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRclickFilelist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNcLButtonDblClk(UINT nHitTest, CPoint point);

	DECLARE_MESSAGE_MAP()

	CString GetFullPath(int nIndex);
};
