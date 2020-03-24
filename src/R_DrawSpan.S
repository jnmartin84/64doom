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
//      Span texture mapping / rendering.
//

#include "mips.h"
#if 1
//
// R_DrawSpan
// With DOOM style restrictions on view orientation,
//  the floors and ceilings consist of horizontal slices
//  or spans with constant z depth.
// However, rotation around the world z axis is possible,
//  thus this mapping, while simpler and faster than
//  perspective correct texture mapping, has to traverse
//  the texture at an angle in all but a few cases.
// In consequence, flats are not stored by column (like walls),
//  and the inner loop has to step in texture space u and v.
//
// Draws the actual span.
//

/* void R_DrawSpan_TrueColor (void) */
.set nomips16
.set nomicromips
.ent R_DrawSpan_TrueColor
.type R_DrawSpan_TrueColor, @function
.global R_DrawSpan_TrueColor
.set noreorder
.set nomacro
R_DrawSpan_TrueColor:

	lw	t0,	%gp_rel(ds_x1)(gp)
	lw	t1,	%gp_rel(ds_x2)(gp)
	lw	t2,	%gp_rel(ds_y)(gp)
	lui	v1,	%hi(palarray)
	subu	t3,	t1,	t0
	blez	t3,	_R_DrawSpan_return
	lui	t4,	%hi(ylookup)
	sll	t2,	t2,	2
	addiu	v1,	v1,	%lo(palarray)
	lw	t7,	%gp_rel(ds_colormap)(gp)
	addiu	t4,	t4,	%lo(ylookup)
	lui	t9,	%hi(buf16)
	addu	t2,	t4,	t2
	lw	t5,	0(t2)
	lw	t2,	%gp_rel(ds_x1)(gp)
	lui	t4,	%hi(columnofs)
	sll	t2,	t2,	2
	addiu	t4,	t4,	%lo(columnofs)
	addiu	t9,	t9,	%lo(buf16)
	addu	t2,	t4,	t2
	lw	t9, 0(t9)
	
	lw	t6,	0(t2)
	lw	t8,	%gp_rel(ds_source)(gp)
	lw	t1,	%gp_rel(ds_ystep)(gp)
	lw	t0,	%gp_rel(ds_xstep)(gp)
	addu	t9,	t9,	t5
	addu	t9,	t9,	t5
	addiu	t9,	t9, 0x6400 // 20*SCREENWIDTH*2
	addu	t9, t9, t6
	addu	t9, t9, t6
	lw	t5,	%gp_rel(ds_xfrac)(gp)
	lw	t6,	%gp_rel(ds_yfrac)(gp)

// uses t0, t1, t3, t5, t6, t7, t9, a0, a1, a2, a3, v0, v1
// t9 is dest, reuse t9 above to compute buf16 + (ylookup[ds_y] + columnofs[ds_x1])*2
_R_DrawSpan_dowhile:
	sra	a0,	t6,	10
	sra	a1,	t5,	16
	andi	a0,	a0,	0xfc0
	andi	a1,	a1,	0x3f
	addu	t6,	t6,	t1
	addu	a2,	a0,	a1
	addu	t5,	t5,	t0
	addu	a3,	t8,	a2
	lbu		v0,	0(a3)
	nop
	addu	a0,	t7,	v0
	lbu		a3,	0(a0)
	nop
	addi	t3,	t3,	-1 
	sll		a3,	a3,	2
	addu	a3, a3,	v1
	lw	t2, 0(a3)
	sh	t2, 0(t9)
	bgez	t3,	_R_DrawSpan_dowhile
	addiu	t9,	t9,	2

_R_DrawSpan_return:
	jr	ra
	nop

	.set	macro
	.set	reorder
	.end	R_DrawSpan_TrueColor
    .size   R_DrawSpan_TrueColor, .-R_DrawSpan_TrueColor

#endif	
#if 1
/* void R_DrawSpanLow_TrueColor (void) */
	.global	R_DrawSpanLow_TrueColor
	.set	nomips16
	.set	nomicromips
	.ent	R_DrawSpanLow_TrueColor
	.type	R_DrawSpanLow_TrueColor,	@function
R_DrawSpanLow_TrueColor:
	.set	noreorder
	.set	nomacro

	lw	t0,	%gp_rel(ds_x1)(gp)
	lw	t1,	%gp_rel(ds_x2)(gp)
	lw	t2,	%gp_rel(ds_y)(gp)
	subu	t3,	t1,	t0
	blez	t3,	_R_DrawSpanLow_return
	sll	t2,	t2,	2
	lui	v1,	%hi(palarray)
	lui	t4,	%hi(ylookup2)
	addiu	v1,	v1,	%lo(palarray)
	addiu	t4,	t4,	%lo(ylookup2)
	addu	t2,	t4,	t2
	lw	t5,	0(t2)
	lw	t2,	%gp_rel(ds_x1)(gp)
	lui	t9,	%hi(buf16)
	sll t2, t2, 1
	addiu	t9,	t9,	%lo(buf16)
	lw 		t9, 0(t9)

	addu	t9,	t9,	t5
	addu	t9,	t9,	t5
	addiu	t9,	t9, 0x6400 // 20*SCREENWIDTH*2
	addu	t9, t9, t2
	addu	t9, t9, t2
	sra		t9, t9, 2
	sll		t9, t9, 2
	lw	t7,	%gp_rel(ds_colormap)(gp)
	lw	t8,	%gp_rel(ds_source)(gp)
	lw	t1,	%gp_rel(ds_ystep)(gp)
	lw	t0,	%gp_rel(ds_xstep)(gp)
//	addu	t9,	t5,	t6
	lw	t5,	%gp_rel(ds_xfrac)(gp)
	lw	t6,	%gp_rel(ds_yfrac)(gp)

_R_DrawSpanLow_dowhile:
	sra	a0,	t6,	10
	sra	a1,	t5,	16
	andi	a0,	a0,	0xfc0
	andi	a1,	a1,	0x3f
	addu	t6,	t6,	t1
	addu	a2,	a0,	a1
	addu	t5,	t5,	t0
	addu	a3,	t8,	a2
	lbu		v0,	0(a3)
	nop
	addu	a0,	t7,	v0
	lbu		a3,	0(a0)
	nop
	addi	t3,	t3,	-1 
	sll		a3,	a3,	2
	addu	a3, a3,	v1
	lw	t2, 0(a3)
	sw	t2, 0(t9)
	bgez	t3,	_R_DrawSpanLow_dowhile
	addiu	t9,	t9,	4
	
_R_DrawSpanLow_return:
	jr	ra
	nop

	.set	macro
	.set	reorder
	.end	R_DrawSpanLow_TrueColor
    .size   R_DrawSpanLow_TrueColor, .-R_DrawSpanLow_TrueColor	
#endif