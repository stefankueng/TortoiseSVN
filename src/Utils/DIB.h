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

#include <afxctl.h>
#include <afxwin.h>
#include <vfw.h>

/**
 * \ingroup Utils
 * 
 * A wrapper class for DIB's. It provides only a very small
 * amount of methods (just the ones I need). Especially for
 * creating 32bit 'image fields' which can be used for
 * implementing image filters.
 *
 * \par requirements
 * win95 or later
 * winNT4 or later
 *
 * \version 1.1
 * found a great class for DIB's from Chris Maunder
 * at http://www.codeproject.com which has much more
 * functions. Changed this code with the help of that
 * class a bit i.e. more error checking, even fixed
 * a memory leak.
 *
 * \version 1.0
 * first version
 *
 * \date 10-18-2000
 *
 * \par license
 * This code is absolutely free to use and modify. The code is provided "as is" with
 * no expressed or implied warranty. The author accepts no liability if it causes
 * any damage to your computer, causes your pet to fall ill, increases baldness
 * or makes your car start emitting strange noises when you start it up.
 * This code has no bugs, just undocumented features!
 * 
 * \todo 
 *
 * \bug 
 *
 */
class CDib : public CObject
{
public:
	CDib();
	virtual ~CDib();

    /**
     * Clears all member variables and frees allocated memory.
     */
    void		DeleteObject();
    /**
     * Gets the number of bytes per horizontal line in the image.
     * \param nWidth the width of the image
     * \param nBitsPerPixel number of bits per pixel (color depth)
     */
    static int	BytesPerLine(int nWidth, int nBitsPerPixel);
    /**
     * Returns the height of the image in pixels
     */
    int			GetHeight() const { return m_BMinfo.bmiHeader.biHeight; } 
    /**
     * Returns the width of the image in pixels
     */
    int			GetWidth() const { return m_BMinfo.bmiHeader.biWidth; }
    /**
     * Returns the size of the image in pixels
     */
    CSize		GetSize() const { return CSize(GetWidth(), GetHeight()); }
    /**
     * Returns the image bytefield which can be used to work on.
     */
    LPVOID		GetDIBits() { return m_pBits; }
	/**
	 * Creates a DIB from a CPictureHolder object with the specified width and height.
	 * \param pPicture the CPictureHolder object
	 * \param iWidth the width of the resulting picture
	 * \param iHeight the heigth of the resulting picture
	 */
	void		Create32BitFromPicture (CPictureHolder* pPicture, int iWidth, int iHeight);

	/**
	 * Returns a 32-bit RGB color
	 */
	COLORREF	FixColorRef		(COLORREF clr);
    /**
     * Sets the created Bitmap-image (from Create32BitFromPicture) to the internal
	 * member variables and fills in all required values for this class.
     * \param lpBitmapInfo a pointer to a BITMAPINFO structure
     * \param lpBits pointer to the image bytefield
     */
    BOOL		SetBitmap(LPBITMAPINFO lpBitmapInfo, LPVOID lpBits);   

public:
    /**
     * Draws the image on the specified device context at the specified point.
	 * No streching is done!
     * \param pDC the device context to draw on
     * \param ptDest the upper left corner to where the picture should be drawn to
     */
    BOOL		Draw(CDC* pDC, CPoint ptDest);

protected:
    HBITMAP		m_hBitmap;
    BITMAPINFO  m_BMinfo;
    VOID		*m_pBits;
};

