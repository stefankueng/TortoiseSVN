// TortoiseIDiff - an image diff viewer in TortoiseSVN

// Copyright (C) 2006-2008, 2021 - TortoiseSVN

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

/**
 * \ingroup TortoiseIDiff
 * Subclassed trackbar control that jumps to the mouse click positions immediately, instead of
 * changing the value "towards it".
 */
class CNiceTrackbar
{
public:
    CNiceTrackbar()
        : m_window(nullptr)
        , m_origProc(nullptr)
        , m_dragging(false)
        , m_dragChanged(false)
    {
    }

    HWND GetWindow() const { return m_window; }
    bool IsValid() const { return m_window != nullptr; }

    void ConvertTrackbarToNice(HWND window);

private:
    static LRESULT CALLBACK NiceTrackbarProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    void                    PostMessageToParent(int tbCode) const;
    bool                    SetThumb(LPARAM lParamPoint) const;

private:
    HWND    m_window;
    WNDPROC m_origProc;
    bool    m_dragging;
    bool    m_dragChanged;
};
