// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2005 - Stefan Kueng

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
#ifndef __WaitCursorEx_h
#define __WaitCursorEx_h

#pragma once

/**
 * \ingroup Utils
 * Guard class for the system's wait cursor. Works very much like the CWaitCursor
 * class from MFC, but has additional functions to show or hide the wait cursor
 * when needed.
 * 
 * Simply instantiate an object of this class to make the wait cursor visible.
 * When the objects gets destructed for any reason, the wait cursor is automatically
 * hidden.
 *
 * \par requirements
 * win95 or later
 * winNT4 or later
 * MFC
 *
 * \version 1.0
 * first version
 *
 * \date MAY-2004
 *
 * \author Thomas Epting
 *
 * \par license
 * This code is absolutely free to use and modify. The code is provided "as is" with
 * no expressed or implied warranty. The author accepts no liability if it causes
 * any damage to your computer, causes your pet to fall ill, increases baldness
 * or makes your car start emitting strange noises when you start it up.
 * This code has no bugs, just undocumented features!
 */
class CWaitCursorEx
{
public:
	CWaitCursorEx(bool start_visible = true);
	~CWaitCursorEx();

	void Show();	//!< Show the hourglass cursor if not already visible
	void Hide();	//!< Hide the hourglass cursor if not already hidden

private:
	bool m_bVisible;
};

#endif /*__WaitCursorEx_h*/
