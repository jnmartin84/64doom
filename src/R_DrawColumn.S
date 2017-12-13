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
	.set	nomips16
	.set	nomicromips
	.ent	R_DrawColumn
	.type	R_DrawColumn,	@function
R_DrawColumn:
	.global	R_DrawColumn
	.set	noreorder
	.set	nomacro

	lw	t0,	%gp_rel(dc_yh)(gp)
	lw	t1,	%gp_rel(dc_yl)(gp)
	lui	t2,	%hi(ylookup)
	subu	t3,	t0,	t1
	bltz	t3,	_R_DrawColumn_end
	nop
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
	lw	t0,	%gp_rel(dc_iscale)(gp)
	lw	t6,	%gp_rel(centery)(gp)
	addu	t7,	t5,	t7 
	subu	t1,	t1,	t6
	multu	t1,	t0
	mflo	t1
	lw	t6,	%gp_rel(dc_texturemid)(gp)
	lw	t8,	%gp_rel(dc_source)(gp)
	lw	t9,	%gp_rel(dc_colormap)(gp)
	addu	t6,	t6,	t1
    
_R_DrawColumn_count:
	addi	t3,	t3,	-1 
	sra	t1,	t6,	FRACBITS
	andi	t1,	t1,	127
	addu	t1,	t8,	t1
	lbu	t5,	0(t1)
	addu	t6,	t6,	t0
	addu	a0,	t9,	t5
	lbu	t1,	0(a0)
	sb	t1,	0(t7)
	bgez	t3,	_R_DrawColumn_count
	addiu	t7,	t7,	SCREENWIDTH

_R_DrawColumn_end:
	jr	ra
	nop

	.set	macro
	.set	reorder
	.end	R_DrawColumn
