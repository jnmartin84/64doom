#include "mips.h"

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
/* void R_DrawSpan (void) */
	.set	nomips16
	.set	nomicromips
	.ent	R_DrawSpan
	.type	R_DrawSpan,	@function
R_DrawSpan:
	.global	R_DrawSpan
	.set	noreorder
	.set	nomacro

	lw	t0,	%gp_rel(ds_x1)(gp)
	lw	t1,	%gp_rel(ds_x2)(gp)
	lw	t2,	%gp_rel(ds_y)(gp)
	subu	t3,	t1,	t0
	blez	t3,	_R_DrawSpan_end
	sll	t2,	t2,	2
	lui	t4,	%hi(ylookup)
	addiu	t4,	t4,	%lo(ylookup)
	addu	t2,	t4,	t2
	lw	t5,	0(t2)
	lw	t2,	%gp_rel(ds_x1)(gp)
	lui	t4,	%hi(columnofs)
	sll	t2,	t2,	2
	addiu	t4,	t4,	%lo(columnofs)
	addu	t2,	t4,	t2
	lw	t6,	0(t2)
	lw	t7,	%gp_rel(ds_colormap)(gp)
	lw	t8,	%gp_rel(ds_source)(gp)
	lw	t1,	%gp_rel(ds_ystep)(gp)
	lw	t0,	%gp_rel(ds_xstep)(gp)
	addu	t9,	t5,	t6
	lw	t5,	%gp_rel(ds_xfrac)(gp)
	lw	t6,	%gp_rel(ds_yfrac)(gp)

_R_DrawSpan_count:    
	addi	t3,	t3,	-1 
	sra	a0,	t6,	10
	andi	a0,	a0,	0xfc0
	sra	a1,	t5,	16
	andi	a1,	a1,	0x3f
	addu	a2,	a0,	a1
	addu	a3,	t8,	a2
	addu	t6,	t6,	t1
	lbu	v0,	0(a3)
	addu	t5,	t5,	t0
	addu	a0,	t7,	v0
	lbu	a3,	0(a0)
	sb	a3,	0(t9)
	bgez	t3,	_R_DrawSpan_count
	addiu	t9,	t9,	1

_R_DrawSpan_end:
	jr	ra
	nop

	.set	macro
	.set	reorder
	.end	R_DrawSpan
