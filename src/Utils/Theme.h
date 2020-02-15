﻿// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2020 - TortoiseSVN

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
#include "registry.h"
#include <string>
#include <unordered_map>
#include <functional>

using ThemeChangeCallback = std::function<void(void)>;

/**
 * Singleton to handle Theme related methods.
 * provides callbacks to allow different parts of the app
 * to receive notifications when the theme changes.
 */
class CTheme
{
private:
    CTheme();
    ~CTheme();

public:
    static CTheme& Instance();

    /// returns true if dark mode is even allowed. We only allow dark mode on Win10 1809 or later.
    bool     IsDarkModeAllowed();
    /// sets the theme and calls all registered callbacks.
    void     SetDarkTheme(bool b = true);
    /// returns true if dark theme is enabled. If false, then the normal theme is active.
    bool     IsDarkTheme() const { return m_dark; }
    /// converts a color to the theme color. For normal theme the \b clr is returned unchanged.
    /// for dark theme, the color is adjusted in brightness.
    COLORREF GetThemeColor(COLORREF clr) const;
    /// registers a callback function that's called for every theme change.
    /// returns an id that can be used to unregister the callback function.
    int      RegisterThemeChangeCallback(ThemeChangeCallback&& cb);
    /// unregisters a callback function.
    bool     RemoveRegisteredCallback(int id);

    static void     RGBToHSB(COLORREF rgb, BYTE& hue, BYTE& saturation, BYTE& brightness);
    static void     RGBtoHSL(COLORREF color, float& h, float& s, float& l);
    static COLORREF HSLtoRGB(float h, float s, float l);

private:
    void Load();

private:
    bool                                         m_bLoaded;
    bool                                         m_dark;
    bool                                         m_bDarkModeIsAllowed;
    std::unordered_map<int, ThemeChangeCallback> m_themeChangeCallbacks;
    int                                          m_lastThemeChangeCallbackId;
    CRegStdDWORD                                 m_regDarkTheme;
};
