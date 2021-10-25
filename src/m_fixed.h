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
//	Fixed point arithemtics, implementation.
//
//-----------------------------------------------------------------------------


#ifndef __M_FIXED__
#define __M_FIXED__


#ifdef __GNUG__
#pragma interface
#endif
#include <inttypes.h>

//
// Fixed point, 32bit as 16.16.
//
#define FRACBITS		16
#define FRACUNIT		(1<<FRACBITS)

#define float_to_fixed(val) (val * FRACUNIT)
#define fixed_to_float(val) (val / FRACUNIT)

typedef int fixed_t;

static inline fixed_t __attribute__((always_inline)) FixedMul(fixed_t a, fixed_t b)
{
    return (fixed_t)(((uint64_t)((uint64_t)a*(uint64_t)b))>>16);
}

static inline fixed_t __attribute__((always_inline)) FixedDiv(fixed_t a, fixed_t b)
{
    long long c;
    c = ((long long)a<<16) / ((long long)b);
    return (fixed_t) c;
}

#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
