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
	.type	R_DrawSpan, @function
R_DrawSpan:
	.global	R_DrawSpan
	.set	noreorder
	.set	nomacro

	lw		t0,	%gp_rel(ds_x1)(gp)
	lw		t1,	%gp_rel(ds_x2)(gp)
	lw		a0,	%gp_rel(ds_y)(gp)
	subu	v0,	t1,	t0
	# return early if possible
	blez	v0,	_R_DrawSpan_end
	sll		a0,	a0,	2
	lui     a1,	%hi(ylookup)
	addiu	a1,	a1,	%lo(ylookup)
	addu	a0,	a1,	a0
	lw      a2,	0(a0)
	lw      a0,	%gp_rel(ds_x1)(gp)
	lui     a1,	%hi(columnofs)
	sll     a0,	a0,	2
	addiu	a1,	a1,	%lo(columnofs)
	addu	a0,	a1,	a0
	lw      a3,	0(a0)
	lw      t3,	%gp_rel(ds_colormap)(gp)
	lw      t2,	%gp_rel(ds_source)(gp)
	lw      t1,	%gp_rel(ds_ystep)(gp)
	lw      t0,	%gp_rel(ds_xstep)(gp)
	addu	t8,	a2,	a3
	lw		a2,	%gp_rel(ds_xfrac)(gp)
	lw		a3,	%gp_rel(ds_yfrac)(gp)

_R_DrawSpan_count:    
	# assume early return means there is at least one loop iteration to handle
	addi	v0,	v0,	-1 
	sra     t4,	a3,	10
	andi	t4,	t4,	0xfc0 # (63*64)
	sra     t5,	a2,	16
	andi	t5,	t5,	0x3f # 63
	addu	t6,	t4,	t5
	addu	t7,	t2,	t6
	addu	a3,	a3,	t1
	lbu		t9,	0(t7)
	# avoid pipeline stall hazard due to t9
	addu	a2,	a2,	t0
	addu	t4,	t3,	t9
	# memory-memory forwarding
	lbu		t7,	0(t4)
	# *dest = ds_colormap[ds_source[spot]]
	sb		t7,	0(t8)
	# do { ... } while(count--);
	bgez    v0, _R_DrawSpan_count
	# dest++
	addiu	t8,	t8,	1

_R_DrawSpan_end:
	jr		ra
	nop

	.set	macro
	.set	reorder
	.end	R_DrawSpan