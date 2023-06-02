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
//    static const int qN = 13, qA= 16, qP= 15, qR= 2*qN-qP, qS= qN+qP+1-qA;

    // scale the input range
    // this makes it work as a replacement for the old finesine table
//    x <<= 2;
//    x = (x << (30 - qN));

    x = x << 19;

    if ((x ^ (x << 1)) < 0)
        x = (1 << 31) - x;

//    x = x >> (30 - qN);
    x = x >> 17;

//    return x * ((3 << qP) - ((x * x) >> qR)) >> qS;
    return x * (98304 - ((x * x) >> 11)) >> 13;
}

static inline int32_t __attribute__((always_inline)) finecosine(int32_t x)
{
    return finesine(x + 2048);
}

#if 0
extern int32_t finetangentf(int32_t x);
#endif
#if 1
// I don't want this to use floating point but I haven't figured out how to convert it to fixed-point yet
static inline int32_t  __attribute__((always_inline)) finetangentf(int32_t x)
{
    // [0,4095] -> [-2048,2047] -> [-1.0,1.0]
    float inx = (((float)x - 2048.0f)/2048.0f);
    float y = (1.0f - (inx*inx));
    // avoid div by zero
    if(y == 0) y = 0.0001f;
    float ret = inx * ((-0.0187108f * y) + 0.31583526f + (1.27365776f / y));
    // float to fixed
    return (int32_t)(ret*65536);
}
#endif
#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
