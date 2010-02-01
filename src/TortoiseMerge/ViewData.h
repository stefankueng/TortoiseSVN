// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2007-2010 - TortoiseSVN

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
#include "DiffStates.h"
#include "EOL.h"

#include <vector>

enum HIDESTATE
{
	HIDESTATE_SHOWN,
	HIDESTATE_HIDDEN,
	HIDESTATE_MARKER,
};

/**
 * \ingroup TortoiseMerge
 * Holds the information which is required to define a single line of text.
 */
typedef struct
{
	CString						sLine;
	DiffStates					state;
	int							linenumber; 
	EOL							ending;
	HIDESTATE					hidestate;
} viewdata;

/**
 * \ingroup TortoiseMerge
 * Handles the view and diff data a TortoiseMerge view needs.
 */
class CViewData
{
public:
	CViewData(void);
	~CViewData(void);

	void			AddData(const CString& sLine, DiffStates state, int linenumber, EOL ending, HIDESTATE hide);
	void			AddData(const viewdata& data);
	void			InsertData(int index, const CString& sLine, DiffStates state, int linenumber, EOL ending, HIDESTATE hide);
	void			InsertData(int index, const viewdata& data);
	void			RemoveData(int index) {m_data.erase(m_data.begin() + index);}

	const viewdata&	GetData(int index) {return m_data[index];}
	const CString&	GetLine(int index) const {return m_data[index].sLine;}
	DiffStates		GetState(int index) const {return m_data[index].state;}
	HIDESTATE		GetHideState(int index) {return m_data[index].hidestate;}
	int				GetLineNumber(int index) {return m_data.size() ? m_data[index].linenumber : 0;}
	int				FindLineNumber(int number);
	EOL				GetLineEnding(int index) const {return m_data[index].ending;}

	int				GetCount() const {return (int)m_data.size();}

	void			SetState(int index, DiffStates state) {m_data[index].state = state;}
	void			SetLine(int index, const CString& sLine) {m_data[index].sLine = sLine;}
	void			SetLineNumber(int index, int linenumber) {m_data[index].linenumber = linenumber;}
	void			SetLineEnding(int index, EOL ending) {m_data[index].ending = ending;}
	void			SetLineHideState(int index, HIDESTATE state) {m_data[index].hidestate = state;}

	void			Clear() {m_data.clear();}
	void			Reserve(int length) {m_data.reserve(length);}

protected:
	std::vector<viewdata>		m_data;
};