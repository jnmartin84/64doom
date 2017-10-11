#include "mips.h"

#if 1
#define SCREENWIDTH 320
#endif
#if 0
#define SCREENWIDTH 256
#endif
#define FRACBITS 16

//
// A column is a vertical slice/span from a wall texture that,
//  given the DOOM style restrictions on the view orientation,
//  will always have constant z depth.
// Thus a special case loop for very fast rendering can
//  be used. It has also been used with Wolfenstein 3D.
//
/* void R_DrawColumn (void) */
#ifndef RDPRENDER
	.set	nomips16
	.set	nomicromips
	.ent	R_DrawColumn
	.type	R_DrawColumn, @function
R_DrawColumn:
	.global	R_DrawColumn
	.set	noreorder
	.set	nomacro

	lw	t1,	%gp_rel(dc_yh)(gp)
	lw	a2,	%gp_rel(dc_yl)(gp)
	
	subu	v0,	t1,	a2

	bltz	v0,	_R_DrawColumn_end
	nop
	
	addu	a0,	a2,	zero
	
	lui	a1,	%hi(ylookup)
	sll	a0,	a0,	2
	addiu	a1,	a1,	%lo(ylookup)
	addu	a0,	a1,	a0
	lw	v1,	0(a0)

	lw	t5,	%gp_rel(dc_x)(gp)	
	addu	a0,	t5,	zero

	lui	a1,	%hi(columnofs)
	sll	a0,	a0,	2
	addiu	a1,	a1,	%lo(columnofs)
	addu	a0,	a1,	a0
	lw	t6,	0(a0)

	lw	t1,	%gp_rel(dc_iscale)(gp)

	lw	t5,	%gp_rel(centery)(gp)

	addu	t6,	v1,	t6 
	subu	a2,	a2,	t5

_R_DrawColumn_mul_start:
	blez	a2,	R_DrawColumn_mul_done
	nop

	addu	t1,	t1,	t1

	b	_R_DrawColumn_mul_start
	addiu	a2,	a2,	-1

	addu	a2,	t1,	zero

_R_DrawColumn_mul_done:
	lw	t5,	%gp_rel(dc_texturemid)(gp)
	addu	t5,	t5,	a2

_R_DrawColumn_count:	
	bltz	v0,	_R_DrawColumn_end
	nop

	addi	v0,	v0,	-1 

	sra	a2,	t5,	FRACBITS
	andi	a2,	a2,	127

	lw	t7,	%gp_rel(dc_source)(gp)
	addu	a2,	t7,	a2
	lbu	v1,	0(a2)
	lw	t7,	%gp_rel(dc_colormap)(gp)
	addu	t5,	t5,	t1
	addu	t7,	t7,	v1
	lbu	a2,	0(t7)
	sb	a2,	0(t6)

	b	_R_DrawColumn_count
	addiu	t6,	t6,	SCREENWIDTH
	
_R_DrawColumn_end:

	jr	ra
	nop

	.set	macro
	.set	reorder
	.end	R_DrawColumn
#endif
