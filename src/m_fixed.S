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
/*fixed_t FixedDiv(fixed_t a, fixed_t b)*/
	.global	FixedDiv
	.set	nomips16
	.set	nomicromips
	.ent	FixedDiv
	.type	FixedDiv, @function
	.set	noreorder
	.set	nomacro
//	.align 8

FixedDiv:
	xor	t7,	a0,	a1
	sra	t0,	a0,	31
	sra	t2,	a1,	31
	xor	t1,	a0,	t0
	xor	t3,	a1,	t2
	subu	t1,	t1,	t0
	subu	t3,	t3,	t2
	sra	t1,	t1,	14
	slt	t4,	t3,	t1
	beq	t4,	zero,	_FixedDiv_work
	dsll	a0,	a0,	FRACBITS
_FixedDiv_test:
	bltz	t7,	_FixedDiv_return_INT_MIN
	lui	v0,	0x7FFF
	jr	ra
	ori	v0,	v0,	0xFFFF
	nop
_FixedDiv_return_INT_MIN:
	addi	v0,	zero,	0x8000
	jr	ra
	sll	v0,	v0,	16
	nop
_FixedDiv_work:
	ddiv	zero,	a0,	a1
	jr	ra
	mflo	v0
	nop

	.set	macro
	.set	reorder
	.end	FixedDiv
    .size   FixedDiv, .-FixedDiv

/*fixed_t FixedMul(fixed_t a, fixed_t b)*/
	.global	FixedMul
	.set	nomips16
	.set	nomicromips
	.ent	FixedMul
	.type	FixedMul, @function
	.set	noreorder
	.set	nomacro
//	.align 8

FixedMul:

	dmultu	a1,	a0
	mflo	v0
	jr	ra
	dsra     v0, 16

	.set	macro
	.set	reorder
	.end	FixedMul
    .size   FixedMul, .-FixedMul
