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

#include "mips.h"
#if 1
#define SCREENWIDTH 640
#define FRACBITS 16

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
R_DrawColumn_TrueColor:
	.set	noreorder
	.set	nomacro

	lw	t0,	%gp_rel(dc_yh)(gp)
	lw	t1,	%gp_rel(dc_yl)(gp)
	lui	t2,	%hi(ylookup)
	subu	t3,	t0,	t1
	bltz	t3,	_R_DrawColumn_return
	lui	v1,	%hi(palarray)
	addiu	v1,	v1,	%lo(palarray)
	addiu	t2,	t2,	%lo(ylookup)
	addu	t4,	t1,	zero
	sll	t4,	t4,	2
	addu	t4,	t2,	t4
	lw	t5,	0(t4)
	lw	t6,	%gp_rel(dc_x)(gp)
	addu	t4,	t6,	zero
	lui	t2,	%hi(columnofs)
	sll	t4,	t4,	2
	addiu	t2,	t2,	%lo(columnofs)
	addu	t4,	t2,	t4
	lw	t7,	0(t4)

	lui	t4,	%hi(buf16)
	addiu	t4,	t4,	%lo(buf16)
	lw	t4,	0(t4)
	
	lw	t0,	%gp_rel(dc_iscale)(gp)
	lw	t6,	%gp_rel(centery)(gp)
	addu	t4,	t4,	t5 
	addu	t4,	t4,	t5
	addu	t4,	t4,	t7
	addu	t4,	t4,	t7
	addiu	t4,	t4, 0x6400 // 20*SCREENWIDTH*2

	subu	t1,	t1,	t6
	multu	t1,	t0
	mflo	t1
	lw	t6,	%gp_rel(dc_texturemid)(gp)
	lw	t8,	%gp_rel(dc_source)(gp)
	lw	t9,	%gp_rel(dc_colormap)(gp)
	addu	t6,	t6,	t1

// t4 is dest
_R_DrawColumn_dowhile:
	sra	t1,	t6,	FRACBITS
	addu	t6,	t6,	t0
	andi	t1,	t1,	127
	addi	t3,	t3,	-1 
	addu	t1,	t8,	t1
	lbu	t5,	0(t1)
	nop
	addu	a0,	t9,	t5
	lbu	t1,	0(a0)
	nop
	sll		t1,	t1,	2
	addu	t1, t1,	v1
	lw	t2, 0(t1)
	sh	t2, 0(t4)
	bgez	t3,	_R_DrawColumn_dowhile
	addiu	t4,	t4,	(SCREENWIDTH*2)
	
_R_DrawColumn_return:
	jr	ra
	nop

	.set	macro
	.set	reorder
	.end	R_DrawColumn_TrueColor
    .size   R_DrawColumn_TrueColor, .-R_DrawColumn_TrueColor
#endif
#if 1		
/* void R_DrawColumnLow_TrueColor (void) */
	.global	R_DrawColumnLow_TrueColor
	.set	nomips16
	.set	nomicromips
	.ent	R_DrawColumnLow_TrueColor
	.type	R_DrawColumnLow_TrueColor,	@function
R_DrawColumnLow_TrueColor:
	.set	noreorder
	.set	nomacro
	
	lw	t0,	%gp_rel(dc_yh)(gp)
	lw	t1,	%gp_rel(dc_yl)(gp)
	subu	t3,	t0,	t1
	bltz	t3,	_R_DrawColumnLow_return
	lui	t2,	%hi(ylookup2)

	addiu	t2,	t2,	%lo(ylookup2)
	lui	v1,	%hi(palarray)
	addiu	v1,	v1,	%lo(palarray)

	addu	t4,	t1,	zero
	sll	t4,	t4,	2
	addu	t4,	t2,	t4
	lw	t5,	0(t4)
	// t5 is ylookup2[dc_yl]
		
	lw	t7,	%gp_rel(dc_x)(gp)
	sll t7, t7, 1
	// t7 is dc_x << 1
	
	lui	t4,	%hi(buf16)
	addiu	t4,	t4,	%lo(buf16)
	lw	t4,	0(t4)
	
	// t4 = &screen[0]
	
	lw	t0,	%gp_rel(dc_iscale)(gp)
	lw	t6,	%gp_rel(centery)(gp)

	addu	t4,	t4,	t5 
	addu	t4,	t4,	t5

	addu	t4,	t4,	t7
	addu	t4,	t4,	t7

	addiu	t4,	t4, 0x6400 // 20*SCREENWIDTH*2
	sra		t4, t4, 2
	sll		t4, t4, 2

	subu	t1,	t1,	t6
	multu	t1,	t0
	mflo	t1
	lw	t6,	%gp_rel(dc_texturemid)(gp)
	lw	t8,	%gp_rel(dc_source)(gp)
	lw	t9,	%gp_rel(dc_colormap)(gp)
	addu	t6,	t6,	t1

_R_DrawColumnLow_dowhile:
	sra	t1,	t6,	FRACBITS
	addu	t6,	t6,	t0
	andi	t1,	t1,	127
	addi	t3,	t3,	-1 
	addu	t1,	t8,	t1
	lbu	t5,	0(t1)
	nop
	addu	a0,	t9,	t5
	lbu	t1,	0(a0)
	nop
	sll		t1,	t1,	2
	addu	t1, t1,	v1
	lw	t2, 0(t1)
	sw	t2, 0(t4)
	bgez	t3,	_R_DrawColumnLow_dowhile
	addiu	t4,	t4,	(SCREENWIDTH*2)

_R_DrawColumnLow_return:
	jr	ra
	nop
	
	.set	macro
	.set	reorder
	.end	R_DrawColumnLow_TrueColor
	.size	R_DrawColumnLow_TrueColor, .-R_DrawColumnLow_TrueColor
#endif