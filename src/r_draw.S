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
//      Column texture mapping / rendering.
//

#define SCREEN_MUL 1
#define BASE_WIDTH 320
#define SCREENWIDTH BASE_WIDTH*SCREEN_MUL
#include "mips.h"
#define FRACBITS 16
#define thepal palarray

//
// A column is a vertical slice/span from a wall texture that,
//  given the DOOM style restrictions on the view orientation,
//  will always have constant z depth.
// Thus a special case loop for very fast rendering can
//  be used. It has also been used with Wolfenstein 3D.
//

/* void R_DrawColumn_TrueColor (void) */
	.global	R_DrawColumn_TrueColor
	.set	nomips16
	.set	nomicromips
	.ent	R_DrawColumn_TrueColor
	.type	R_DrawColumn_TrueColor,	@function
	.set	noreorder
	.set	nomacro
	.align  3
R_DrawColumn_TrueColor:
	lw	t0,	%gp_rel(dc_yh)(gp)
	lui	t2,	%hi(ylookup)
	lw	t1,	%gp_rel(dc_yl)(gp)
	lui	v1,	%hi(thepal)
	subu	t3,	t0,	t1
	bltz	t3,	_R_DrawColumn_TrueColor_return
	addiu	t2,	t2,	%lo(ylookup)
	addiu	v1,	v1,	%lo(thepal)
	addu	t4,	t1,	zero
	sll	t4,	t4,	2
	addu	t4,	t2,	t4
	lw	t5,	0(t4)
	lui	t2,	%hi(columnofs)
	lw	t6,	%gp_rel(dc_x)(gp)
	addiu	t2,	t2,	%lo(columnofs)
	addu	t4,	t6,	zero
	sll	t4,	t4,	2
	addu	t4,	t2,	t4
	lw	t7,	0(t4)
	lui	t4,	%hi(buf16)
	lw	t4,	%lo(buf16)(t4)
	lw	t0,	%gp_rel(dc_iscale)(gp)
	lw	t6,	%gp_rel(centery)(gp)
	addu	t4,	t4,	t5
	addu	t4,	t4,	t5
	subu	t1,	t1,	t6
	multu	t0,	t1
	lw	t6,	%gp_rel(dc_texturemid)(gp)
	lw	t8,	%gp_rel(dc_source)(gp)
	lw	t9,	%gp_rel(dc_colormap)(gp)
	addu	t4,	t4,	t7
	mflo	t1
	addu	t4,	t4,	t7
	addu	t6,	t6,	t1

// t4 is dest
_R_DrawColumn_TrueColor_dowhile:
	// update texture mapping coordinates
	sra	t1,	t6,	FRACBITS
	andi	t1,	t1,	127
	addu	t1,	t8,	t1
	lbu	t5,	0(t1)
	// decrement col pixel count
	addiu	t3,	t3,	-1
	addu	v0,	t9,	t5
	lbu	t1,	0(v0)
	// advance screen pointer
	addiu	t4,	t4,	(SCREENWIDTH*2)

	// palette lookup
	sll	t1,	t1,	2
	addu	t1,	t1,	v1
	lw	t2,	0(t1)
	addu	t6,	t6,	t0
	bge	t3,	zero,	_R_DrawColumn_TrueColor_dowhile
	sh	t2,	-(SCREENWIDTH*2)(t4)

_R_DrawColumn_TrueColor_return:
	jr	ra
	nop

	.set	macro
	.set	reorder
	.end	R_DrawColumn_TrueColor
    .size   R_DrawColumn_TrueColor, .-R_DrawColumn_TrueColor

/* void R_DrawColumnLow_TrueColor (void) */
	.global	R_DrawColumnLow_TrueColor
	.set	nomips16
	.set	nomicromips
	.ent	R_DrawColumnLow_TrueColor
	.type	R_DrawColumnLow_TrueColor,	@function
	.set	noreorder
	.set	nomacro
	.align  3
R_DrawColumnLow_TrueColor:
	lw	t0,	%gp_rel(dc_yh)(gp)
	lui	t2,	%hi(ylookup2)
	lw	t1,	%gp_rel(dc_yl)(gp)
	addiu	t2,	t2,	%lo(ylookup2)
	subu	t3,	t0,	t1
	bltz	t3,	_R_DrawColumnLow_TrueColor_return
	lui	v1,	%hi(thepal)
	addiu	v1,	v1,	%lo(thepal)

	addu	t4,	t1,	zero
	sll	t4,	t4,	2
	addu	t4,	t2,	t4
	lw	t5,	0(t4)
	// t5 is ylookup2[dc_yl]
	lw	t7,	%gp_rel(dc_x)(gp)
	// t4 = &screen[0]
	lui	t4,	%hi(buf16)
	lw	t4, 	%lo(buf16)(t4)
	// t7 is dc_x << 1
	sll	t7,	t7,	1
	lw	t0,	%gp_rel(dc_iscale)(gp)
	lw	t6,	%gp_rel(centery)(gp)
	addu	t4,	t4,	t5
	addu	t4,	t4,	t5
	subu	t1,	t1,	t6
	multu	t0,	t1
	addu	t4,	t4,	t7
	addu	t4,	t4,	t7
	mflo	t1
	lw	t6,	%gp_rel(dc_texturemid)(gp)
	lw	t8,	%gp_rel(dc_source)(gp)
	lw	t9,	%gp_rel(dc_colormap)(gp)
	addu	t6,	t6,	t1

_R_DrawColumnLow_TrueColor_dowhile:
	// update texture mapping coordinates
	sra	t1,	t6,	FRACBITS
	andi	t1,	t1,	127
	addu	t6,	t6,	t0
	addu	t1,	t8,	t1

	lbu	t5,	0(t1)

	addiu	t3,	t3,	-1

	addu	v0,	t9,	t5
	lbu	t1,	0(v0)

	addiu	t4,	t4,	((BASE_WIDTH*SCREEN_MUL)*2)

	sll	t1,	t1,	2
	addu	t1,	t1,	v1
	lw	t2,	0(t1)

	bge	t3,	zero,	_R_DrawColumnLow_TrueColor_dowhile
	sw	t2,	-(SCREENWIDTH*2)(t4)

_R_DrawColumnLow_TrueColor_return:
	jr	ra
	nop

	.set	macro
	.set	reorder
	.end	R_DrawColumnLow_TrueColor
	.size	R_DrawColumnLow_TrueColor, .-R_DrawColumnLow_TrueColor

/* void R_DrawSpan_TrueColor (void) */
	.global	R_DrawSpan_TrueColor
	.set	nomips16
	.set	nomicromips
	.set	noreorder
	.set	nomacro
	.set	noat
	.ent	R_DrawSpan_TrueColor
	.type	R_DrawSpan_TrueColor,	@function
	.align  3
R_DrawSpan_TrueColor:
	lw	t0,	%gp_rel(ds_x1)(gp)
	lui	t9,	%hi(buf16)
	lw	t1,	%gp_rel(ds_x2)(gp)
	lui	v1,	%hi(thepal)

	subu	t3,	t1,	t0
	bltz	t3,	_R_DrawSpan_TrueColor_return
	sll	t0,	t0,	2

	lw	t9,	%lo(buf16)(t9)

	addiu	v1,	v1,	%lo(thepal)

	lw	t2,	%gp_rel(ds_y)(gp)


	lui	t4,	%hi(ylookup)
	addiu	t4,	t4,	%lo(ylookup)

	sll	t2,	t2,	2
	addu	t2,	t4,	t2
	lw	t5,	0(t2)

	lui	t4,	%hi(columnofs)
	addiu	t4,	t4,	%lo(columnofs)

	addu	t0,	t4,	t0
	lw	t6,	0(t0)

	addu	t9,	t9,	t5
	addu	t9,	t9,	t5
	addu	t9,	t9,	t6
	addu	t9,	t9,	t6

	lw	t0,	%gp_rel(ds_xstep)(gp)
	lw	t1,	%gp_rel(ds_ystep)(gp)
	lw	t5,	%gp_rel(ds_xfrac)(gp)
	lw	t6,	%gp_rel(ds_yfrac)(gp)
	lw	t7,	%gp_rel(ds_colormap)(gp)
	lw	t8,	%gp_rel(ds_source)(gp)

// uses t0, t1, t3, t5, t6, t7, t9, a0, a1, a2, a3, v0, v1
// t9 is dest, reuse t9 above to compute buf16 + (ylookup[ds_y] + columnofs[ds_x1])*2
//	.align  3
_R_DrawSpan_TrueColor_dowhile:
	sra	a0,	t6,	10
	andi	a0,	a0,	0xfc0

	sra	a1,	t5,	16
	andi	a1,	a1,	0x3f

	addu	v0,	a0,	a1
	addu	v0,	t8,	v0

	add	t5,	t5,	t0

	lbu		v0,	0(v0)
	addiu	t9,	t9,	2

	addu	a0,	t7,	v0
	addu	t6,	t6,	t1
	lbu		v0,	0(a0)
	addiu	t3,	t3,	-1

	sll		v0,	v0,	2
	addu	v0, v0,	v1
	lw	t2, 0(v0)

	bge	t3,	zero,	_R_DrawSpan_TrueColor_dowhile
	sh	t2, -2(t9)

_R_DrawSpan_TrueColor_return:
	jr	ra
	nop

	.set	macro
	.set	reorder
	.end	R_DrawSpan_TrueColor
	.size   R_DrawSpan_TrueColor, .-R_DrawSpan_TrueColor

/* void R_DrawSpanLow_TrueColor (void) */
	.global	R_DrawSpanLow_TrueColor
	.set	nomips16
	.set	nomicromips
	.ent	R_DrawSpanLow_TrueColor
	.type	R_DrawSpanLow_TrueColor,	@function
	.set	noreorder
	.set	nomacro
	.set	noat
	.align  3
R_DrawSpanLow_TrueColor:
	lw	t0,	%gp_rel(ds_x1)(gp)
	lui	t9,	%hi(buf16)
	lw	t1,	%gp_rel(ds_x2)(gp)
	lui	v1,	%hi(thepal)

	subu	t3,	t1,	t0
	bltz	t3,	_R_DrawSpanLow_TrueColor_return
	sll	t0,	t0,	2

	lw	t2,	%gp_rel(ds_y)(gp)

	addiu	v1,	v1,	%lo(thepal)
	lw	t9,	%lo(buf16)(t9)


	lui	t4,	%hi(ylookup2)
	addiu	t4,	t4,	%lo(ylookup2)

	sll	t2,	t2,	2
	addu	t2,	t4,	t2
	lw	t5,	0(t2)

	addu	t9,	t9,	t0
	addu	t9,	t9,	t5
	addu	t9,	t9,	t5

	lw	t0,	%gp_rel(ds_xstep)(gp)
	lw	t1,	%gp_rel(ds_ystep)(gp)
	lw	t5,	%gp_rel(ds_xfrac)(gp)
	lw	t6,	%gp_rel(ds_yfrac)(gp)
	lw	t7,	%gp_rel(ds_colormap)(gp)
	lw	t8,	%gp_rel(ds_source)(gp)

_R_DrawSpanLow_TrueColor_dowhile:
	sra	a0,	t6,	10
	andi	a0,	a0,	0xfc0
	sra	a1,	t5,	16
	andi	a1,	a1,	0x3f

	addu	t6,	t6,	t1
	addu	t5,	t5,	t0
	addu	a2,	a0,	a1
	addu	a2,	t8,	a2

	lbu	v0,	0(a2)
	addiu	t3,	t3,	-1

	addu	a0,	t7,	v0
	lbu	a2,	0(a0)
	addiu	t9,	t9,	4

	sll	a2,	a2,	2
	addu	a2,	a2,	v1
	lw	t2,	0(a2)
	bge	t3, zero,  _R_DrawSpanLow_TrueColor_dowhile
	sw	t2, -4(t9)
_R_DrawSpanLow_TrueColor_return:
	jr	ra
	nop

	.set	reorder
	.set	macro
	.end	R_DrawSpanLow_TrueColor
	.size   R_DrawSpanLow_TrueColor, .-R_DrawSpanLow_TrueColor
