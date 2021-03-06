// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2010, 2012, 2016-2017, 2020-2021 - TortoiseSVN

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

#include "SVNRev.h"
#include "HistoryCombo.h"
#include "Tooltip.h"
#include "ThemeControls.h"

class CRepositoryTree;

/**
 * \ingroup TortoiseProc
 * Interface definition
 */
class IRepo
{
public:
    virtual ~IRepo()                                                             = default;
    virtual bool    ChangeToUrl(CString& url, SVNRev& rev, bool bAlreadyChecked) = 0;
    virtual CString GetRepoRoot()                                                = 0;
    virtual void    OnCbenDragbeginUrlcombo(NMHDR* pNMHDR, LRESULT* pResult)     = 0;
    virtual HWND    GetHWND() const                                              = 0;
    virtual size_t  GetHistoryForwardCount() const                               = 0;
    virtual size_t  GetHistoryBackwardCount() const                              = 0;
    virtual bool    IsThreadRunning() const                                      = 0;
};

/**
 * \ingroup TortoiseProc
 * Provides a CReBarCtrl which can be used as a "control center" for the
 * repository browser. To associate the repository bar with any repository
 * tree control, call AssocTree() once after both objects are created. As
 * long as they are associated, the bar and the tree form a "team" of
 * controls that work together.
 */
class CRepositoryBar : public CReBarCtrl
{
    DECLARE_DYNAMIC(CRepositoryBar)

public:
    CRepositoryBar();
    ~CRepositoryBar() override;

public:
    /**
     * Creates the repository bar. Set \a in_dialog to TRUE when the bar
     * is placed inside of a dialog. Otherwise it is assumed that the
     * bar is placed in a frame window.
     */
    bool Create(CWnd* parent, UINT id, bool inDialog = true);

    /**
     * Show the given \a svn_url in the URL combo and the revision button.
     */
    void ShowUrl(const CString& url, const SVNRev& rev);

    /**
     * Show the given \a svn_url in the URL combo and the revision button,
     * and select it in the associated repository tree. If no \a svn_url
     * is given, the current values are used (which effectively refreshes
     * the tree).
     */
    void GotoUrl(const CString& url = CString(), const SVNRev& rev = SVNRev(), bool bAlreadyChecked = false);

    /**
     * Returns the current URL.
     */
    CString GetCurrentUrl() const;

    /**
     * Returns the current revision
     */
    SVNRev GetCurrentRev() const;

    /**
     * Saves the URL history of the HistoryCombo.
     */
    void SaveHistory();

    /**
     * Set the revision
     */
    void SetRevision(const SVNRev& rev);

    /**
     * Sets the head revision for the tooltip
     */
    void SetHeadRevision(svn_revnum_t rev);

    void SetFocusToURL() const;

    void SetIRepo(IRepo* pRepo) { m_pRepo = pRepo; }

    SVNRev GetHeadRevision() const { return m_headRev; }

    HWND GetComboWindow() const { return m_cbxUrl.GetSafeHwnd(); }

    afx_msg void OnGoUp();

protected:
    BOOL         PreTranslateMessage(MSG* pMsg) override;
    ULONG        GetGestureStatus(CPoint ptTouch) override;
    afx_msg void OnCbnSelEndOK();
    afx_msg void OnBnClicked();
    afx_msg void OnDestroy();
    afx_msg void OnCbenDragbeginUrlcombo(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnHistoryBack();
    afx_msg void OnHistoryForward();

    DECLARE_MESSAGE_MAP()

private:
    void SetTheme(bool bDark) const;

    CString m_url;
    SVNRev  m_rev;

    IRepo* m_pRepo;

    class CRepositoryCombo : public CHistoryCombo
    {
        CRepositoryBar* m_bar;

    public:
        CRepositoryCombo(CRepositoryBar* bar)
            : m_bar(bar)
        {
        }
        bool OnReturnKeyPressed() override;
    } m_cbxUrl;

    CThemeMFCButton m_btnRevision;
    CThemeMFCButton m_btnUp;
    CThemeMFCButton m_btnBack;
    CThemeMFCButton m_btnForward;
    CStatic         m_revText;

    SVNRev    m_headRev;
    CToolTips m_tooltips;
    int       m_themeCallbackId;
};

/**
 * \ingroup TortoiseProc
 * Implements a visual container for a CRepositoryBar which may be added to a
 * dialog. A CRepositoryBarCnr is not needed if the CRepositoryBar is attached
 * to a frame window.
 */
class CRepositoryBarCnr : public CStatic
{
    DECLARE_DYNAMIC(CRepositoryBarCnr)

public:
    CRepositoryBarCnr(CRepositoryBar* repositoryBar);
    ~CRepositoryBarCnr() override;

    // Generated message map functions
protected:
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg UINT OnGetDlgCode();
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);

    void DrawItem(LPDRAWITEMSTRUCT) override;

    DECLARE_MESSAGE_MAP()

private:
    CRepositoryBar* m_pbarRepository;
};
