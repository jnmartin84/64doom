// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	Endianess handling, swapping 16bit and 32bit.
//
//-----------------------------------------------------------------------------


#ifndef __M_SWAP__
#define __M_SWAP__


#ifdef __GNUG__
#pragma interface
#endif

// Endianess handling.
// WAD files are stored little endian.

#define SHORT(x)	((int16_t)(((uint16_t)(x)>>8)|((uint16_t)(x)<<8))) 
#define LONG(x)     ((int32_t)((((uint32_t)(x)>>24)&0xff) | \
					(((uint32_t)(x)<<8)&0xff0000) | \
					(((uint32_t)(x)>>8)&0xff00) | \
					(((uint32_t)(x)<<24))))
#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
