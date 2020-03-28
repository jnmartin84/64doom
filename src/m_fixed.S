//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2014-2017 Jason Martin (jnmartin84)
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
//	Fixed point implementation.
//

#include "mips.h"
#define	FRACBITS	16

//
// FixedDiv
//
// 16.16 Fixed-point divide.
//
#if 1
/*fixed_t FixedDiv(fixed_t a, fixed_t b)*/
	.global	FixedDiv
	.set	nomips16
	.set	nomicromips
	.ent	FixedDiv
	.type	FixedDiv, @function
FixedDiv:
	.set	noreorder
	.set	nomacro

//	# see: https://stackoverflow.com/a/16059870
//	# takes absolute value of a0 (a) and stores in t1
	sra	t0,	a0,	31
	sra	t2,	a1,	31
	xor	t1,	a0,	t0
	xor	t3,	a1,	t2
	subu	t1,	t1,	t0
//	# takes absolute value of a1 (b) and stores in t3
	subu	t3,	t3,	t2
//	# t1 = abs(a) >> 14
	srl	t1,	t1,	14
//	# t0 = a ^ b
//	# this gets used if the next branch is taken
	xor	t0,	a0,	a1
//	# so if ((abs(a) >> 14) >= abs(b))
//	#     goto __FixedDiv_test
	slt	t4,	t3,	t1
	bne	t4,	zero,	_FixedDiv_test
//	# a0 = ((int64_t) a)
//#	dadd	a0,	a0,	zero
//	# a1 = ((int64_t) b)
//#	dadd	a1,	a1,	zero
//	# a0 = (a0 << 16)
	dsll	a0,	a0,	FRACBITS
//	# lo = a0 / a1
	ddiv	zero,	a0,	a1
//	# v0 = ((int64_t) a << FRACBITS) / ((int64_t) b)
	jr	ra
	mflo	v0
	
_FixedDiv_test:
//	#  (a^b) < 0 ? return INT_MIN : return INT_MAX
	bltz	t0,	_FixedDiv_return_INT_MIN
	# do useful work in delay slot...
	# this is part of setup for returning INT_MAX
	lui	v0,	0x7FFF
	jr	ra
	ori	v0,	v0,	0xFFFF

_FixedDiv_return_INT_MIN:
	addi	v0,	zero,	0x8000
	jr	ra
	sll	v0,	v0,	16

	.set	macro
	.set	reorder
	.end	FixedDiv
    .size   FixedDiv, .-FixedDiv
#endif

#if 1
/*fixed_t FixedMul(fixed_t a, fixed_t b)*/
	.global	FixedMul
	.set	nomips16
	.set	nomicromips
	.ent	FixedMul
	.type	FixedMul, @function
FixedMul:
	.set	noreorder
	.set	nomacro	
#if 1
	dmultu	a1,	a0
	mflo	v0
	dsrl     v0, 16
	sll	v0,	v0,	0
	jr	ra
	nop
#endif
#if 0
	daddu	a1,	a1,	zero
	addu	a0,	a0,	zero
	dmultu	a1,	a0
	mflo	v0
	dsrl	v0,	16
	# without the following line the result is wrong
	# even if it does look like a no-op
	sll	v0,	v0,	0
	jr	ra
	nop
#endif
	.set	macro
	.set	reorder
	.end	FixedMul
    .size   FixedMul, .-FixedMul
#endif
