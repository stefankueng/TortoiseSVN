// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2007 - TortoiseSVN

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
#include "BLOBOutStream.h"

// write the (possible NULL) data we just got through Add()

void CBLOBOutStreamBase::WriteThisStream (CCacheFileOutBuffer* buffer)
{
	if ((data != NULL) && (size > 0))
		buffer->Add (data, size);
}

// construction: nothing special to do

CBLOBOutStreamBase::CBLOBOutStreamBase ( CCacheFileOutBuffer* aBuffer
									   , SUB_STREAM_ID anID)
	: CHierachicalOutStreamBase (aBuffer, anID)
	, data (NULL)
	, size (0)
{
}

// write local stream data and close the stream

void CBLOBOutStreamBase::Add (const unsigned char* source, size_t byteCount)
{
	assert (data == NULL);

	// this may fail under x64

	if (byteCount > (DWORD)(-1))
		throw std::exception ("BLOB to large for stream");

	// remember the buffer & size just for a few moments

	data = source;
	size = (DWORD)byteCount;

	// write them (and all sub-streams) to file

	AutoClose();
}
