// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006-2007 - TortoiseSVN

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
#include "StdAfx.h"
#include "Resource.h"
#include ".\leftview.h"

IMPLEMENT_DYNCREATE(CLeftView, CBaseView)

CLeftView::CLeftView(void)
{
	m_pwndLeft = this;
	m_nStatusBarID = ID_INDICATOR_LEFTVIEW;
}

CLeftView::~CLeftView(void)
{
}

void CLeftView::OnContextMenu(CPoint point, int /*nLine*/, DiffStates state)
{
	if (!m_pwndRight->IsWindowVisible())
		return;

	CMenu popup;
	if (popup.CreatePopupMenu())
	{
#define ID_USEBLOCK 1
#define ID_USEFILE 2
#define ID_USETHEIRANDYOURBLOCK 3
#define ID_USEYOURANDTHEIRBLOCK 4
#define ID_USEBOTHTHISFIRST 5
#define ID_USEBOTHTHISLAST 6

		UINT uFlags = MF_ENABLED;
		if ((m_nSelBlockStart == -1)||(m_nSelBlockEnd == -1))
			uFlags |= MF_DISABLED | MF_GRAYED;
		CString temp;
		
		bool bImportantBlock = true;
		switch (state)
		{
		case DIFFSTATE_UNKNOWN:
			bImportantBlock = false;
			break;
		}
		temp.LoadString(IDS_VIEWCONTEXTMENU_USETHISBLOCK);
		popup.AppendMenu(MF_STRING | uFlags | (bImportantBlock ? MF_ENABLED : MF_DISABLED|MF_GRAYED), ID_USEBLOCK, temp);

		temp.LoadString(IDS_VIEWCONTEXTMENU_USETHISFILE);
		popup.AppendMenu(MF_STRING | MF_ENABLED, ID_USEFILE, temp);

		if (m_pwndBottom->IsWindowVisible())
		{
			temp.LoadString(IDS_VIEWCONTEXTMENU_USEYOURANDTHEIRBLOCK);
			popup.AppendMenu(MF_STRING | uFlags | (bImportantBlock ? MF_ENABLED : MF_DISABLED|MF_GRAYED), ID_USEYOURANDTHEIRBLOCK, temp);
			temp.LoadString(IDS_VIEWCONTEXTMENU_USETHEIRANDYOURBLOCK);
			popup.AppendMenu(MF_STRING | uFlags | (bImportantBlock ? MF_ENABLED : MF_DISABLED|MF_GRAYED), ID_USETHEIRANDYOURBLOCK, temp);
		}
		else
		{
			temp.LoadString(IDS_VIEWCONTEXTMENU_USEBOTHTHISFIRST);
			popup.AppendMenu(MF_STRING | uFlags | (bImportantBlock ? MF_ENABLED : MF_DISABLED|MF_GRAYED), ID_USEBOTHTHISFIRST, temp);
			temp.LoadString(IDS_VIEWCONTEXTMENU_USEBOTHTHISLAST);
			popup.AppendMenu(MF_STRING | uFlags | (bImportantBlock ? MF_ENABLED : MF_DISABLED|MF_GRAYED), ID_USEBOTHTHISLAST, temp);
		}

		popup.AppendMenu(MF_SEPARATOR, NULL);

		temp.LoadString(IDS_EDIT_COPY);
		popup.AppendMenu(MF_STRING | (HasTextSelection() ? MF_ENABLED : MF_DISABLED|MF_GRAYED), ID_EDIT_COPY, temp);

		int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
		viewstate leftstate;
		viewstate bottomstate;
		viewstate rightstate;
		switch (cmd)
		{
		case ID_EDIT_COPY:
			OnEditCopy();
			break;
		case ID_USEFILE:
			{
				if (m_pwndBottom->IsWindowVisible())
				{
					for (int i=0; i<GetLineCount(); i++)
					{
						bottomstate.difflines[i] = m_pwndBottom->m_pViewData->GetLine(i);
						m_pwndBottom->m_pViewData->SetLine(i, m_pViewData->GetLine(i));
						bottomstate.linestates[i] = m_pwndBottom->m_pViewData->GetState(i);
						m_pwndBottom->m_pViewData->SetState(i, m_pViewData->GetState(i));
						if (m_pwndBottom->IsLineConflicted(i))
							m_pwndBottom->m_pViewData->SetState(i, DIFFSTATE_CONFLICTRESOLVED);
					}
					m_pwndBottom->SetModified();
				}
				else
				{
					for (int i=0; i<GetLineCount(); i++)
					{
						rightstate.difflines[i] = m_pwndRight->m_pViewData->GetLine(i);
						m_pwndRight->m_pViewData->SetLine(i, m_pViewData->GetLine(i));
						DiffStates state = m_pViewData->GetState(i);
						switch (state)
						{
						case DIFFSTATE_CONFLICTEMPTY:
						case DIFFSTATE_UNKNOWN:
						case DIFFSTATE_EMPTY:
							rightstate.linestates[i] = m_pwndRight->m_pViewData->GetState(i);
							m_pwndRight->m_pViewData->SetState(i, state);
							break;
						case DIFFSTATE_YOURSADDED:
						case DIFFSTATE_IDENTICALADDED:
						case DIFFSTATE_NORMAL:
						case DIFFSTATE_THEIRSADDED:
						case DIFFSTATE_ADDED:
						case DIFFSTATE_CONFLICTADDED:
						case DIFFSTATE_CONFLICTED:
						case DIFFSTATE_CONFLICTED_IGNORED:
						case DIFFSTATE_IDENTICALREMOVED:
						case DIFFSTATE_REMOVED:
						case DIFFSTATE_THEIRSREMOVED:
						case DIFFSTATE_YOURSREMOVED:
							rightstate.linestates[i] = DIFFSTATE_NORMAL;
							m_pwndRight->m_pViewData->SetState(i, DIFFSTATE_NORMAL);
							m_pViewData->SetState(i, DIFFSTATE_NORMAL);
							break;
						default:
							break;
						}
					}
					m_pwndRight->SetModified();
					if (m_pwndLocator)
						m_pwndLocator->DocumentUpdated();
				}
			}
			break;
		case ID_USEBLOCK:
			{
				if (m_pwndBottom->IsWindowVisible())
				{
					for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
					{
						bottomstate.difflines[i] = m_pwndBottom->m_pViewData->GetLine(i);
						m_pwndBottom->m_pViewData->SetLine(i, m_pViewData->GetLine(i));
						bottomstate.linestates[i] = m_pwndBottom->m_pViewData->GetState(i);
						m_pwndBottom->m_pViewData->SetState(i, m_pViewData->GetState(i));
						if (m_pwndBottom->IsLineConflicted(i))
							m_pwndBottom->m_pViewData->SetState(i, DIFFSTATE_CONFLICTRESOLVED);
					}
					m_pwndBottom->SetModified();
				}
				else
				{
					for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
					{
						rightstate.difflines[i] = m_pwndRight->m_pViewData->GetLine(i);
						m_pwndRight->m_pViewData->SetLine(i, m_pViewData->GetLine(i));
						DiffStates state = m_pViewData->GetState(i);
						switch (state)
						{
						case DIFFSTATE_ADDED:
						case DIFFSTATE_CONFLICTADDED:
						case DIFFSTATE_CONFLICTED:
						case DIFFSTATE_CONFLICTED_IGNORED:
						case DIFFSTATE_CONFLICTEMPTY:
						case DIFFSTATE_IDENTICALADDED:
						case DIFFSTATE_NORMAL:
						case DIFFSTATE_THEIRSADDED:
						case DIFFSTATE_UNKNOWN:
						case DIFFSTATE_YOURSADDED:
						case DIFFSTATE_EMPTY:
							rightstate.linestates[i] = m_pwndRight->m_pViewData->GetState(i);
							m_pwndRight->m_pViewData->SetState(i, state);
							break;
						case DIFFSTATE_IDENTICALREMOVED:
						case DIFFSTATE_REMOVED:
						case DIFFSTATE_THEIRSREMOVED:
						case DIFFSTATE_YOURSREMOVED:
							rightstate.linestates[i] = m_pwndRight->m_pViewData->GetState(i);
							m_pwndRight->m_pViewData->SetState(i, DIFFSTATE_ADDED);
							break;
						default:
							break;
						}
					}
					m_pwndRight->SetModified();
				}
			} 
			break;
		case ID_USEYOURANDTHEIRBLOCK:
			{
				UseYourAndTheirBlock(rightstate, bottomstate, leftstate);
			}
			break;
		case ID_USETHEIRANDYOURBLOCK:
			{
				UseTheirAndYourBlock(rightstate, bottomstate, leftstate);
			}
			break;
		case ID_USEBOTHTHISLAST:
			{
				UseBothRightFirst(rightstate, leftstate);
			}
			break;
		case ID_USEBOTHTHISFIRST:
			{
				UseBothLeftFirst(rightstate, leftstate);
			}
			break;
		} // switch (cmd) 
		CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate);
	} // if (popup.CreatePopupMenu()) 
}
