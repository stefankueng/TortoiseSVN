// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Tim Kemp and Stefan Kueng

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

#pragma once

class CTSVNPath;

/**
 * \ingroup Utils
 * Provides simplified access to the system icons. Only small system icons
 * are supported.
 *
 * \note This class is implemented as a singleton.
 * The singleton instance is created when first accessed. See SYS_IMAGE_LIST() function
 * easy access of the singleton instance. All 
 *
 * \par requirements
 * win95 or later
 * winNT4 or later
 * MFC
 *
 * \version 1.0
 * first version
 *
 * \date MAR-2004
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
class CSysImageList : public CImageList
{
// Singleton constructor and destructor (private)
private:
	CSysImageList();
	~CSysImageList();

// Singleton specific operations
public:
	/**
	 * Returns a reference to the one and only instance of this class.
	 */
	static CSysImageList& GetInstance();
	/**
	 * Frees all allocated resources (if necessary). Don't call this
	 * function when the image list is currently bound to any control!
	 */
	static void Cleanup();

// Operations
public:
	/**
	 * Returns the icon index for a directory.
	 */
	int GetDirIconIndex() const;
	/**
	 * Returns the icon index for a file which has no special icon associated.
	 */
	int GetDefaultIconIndex() const;
	/**
	 * Returns the icon index for the specified \a file. Only the file extension
	 * is used to determine the file's icon.
	 */
	int GetFileIconIndex(const CString& file) const;

	/**
	 * Get the index for a SVN-style path file.  
	 * Uses a cache to speed things up
	 */
	int GetPathIconIndex(const CTSVNPath& file) const;

private:
	static CSysImageList *instance;

	typedef std::map<CString, int> IconIndexMap;
	mutable IconIndexMap m_indexCache;
};


/**
 * \relates CSysImageList
 * Singleton access for CSysImageList.
 */
inline CSysImageList& SYS_IMAGE_LIST() { return CSysImageList::GetInstance(); }
