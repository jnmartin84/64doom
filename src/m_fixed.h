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
//    Fixed point arithemtics, implementation.
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
#define FRACBITS        16
#define FRACUNIT        (1<<FRACBITS)

typedef int32_t fixed_t;

static inline int D_abs(fixed_t x)
{
    fixed_t _s = x >> 31;
    return (x ^ _s) - _s;
}

static inline fixed_t __attribute__((always_inline)) FixedMul(fixed_t a, fixed_t b)
{
    return (fixed_t)(((uint64_t)a * (uint64_t)b) >> 16);
}

static inline fixed_t __attribute__((always_inline)) FixedDiv(fixed_t a, fixed_t b)
{
    return (fixed_t) ((int64_t)((int64_t)a<<16) / ((int64_t)b));
}

// http://www.coranac.com/2009/07/sines/
// third-order approximation
static inline int32_t  __attribute__((always_inline)) finesine(int32_t x)
{
    // original has qA = 12 (output range [-4095,4095])
    // we need 16 (output range [-65535,65535])
    static const int qN = 13, qA= 16, qP= 15, qR= 2*qN-qP, qS= qN+qP+1-qA;

    // scale the input range
    // this makes it work as a replacement for the old finesine table
    x <<= 2;

    x = (x << (30 - qN));

    if ((x ^ (x << 1)) < 0)
        x = (1 << 31) - x;

    x = x >> (30 - qN);

    return x * ((3 << qP) - ((x * x) >> qR)) >> qS;
}

static inline int32_t  __attribute__((always_inline)) finecosine(int32_t x)
{
    return finesine(x + 2048);
}

#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
