#include "mips.h"

#define SCREENWIDTH 320
#define FRACBITS 16

//
// A column is a vertical slice/span from a wall texture that,
//  given the DOOM style restrictions on the view orientation,
//  will always have constant z depth.
// Thus a special case loop for very fast rendering can
//  be used. It has also been used with Wolfenstein 3D.
//
/* void R_DrawColumn (void) */

R_DrawColumn:
	.global	R_DrawColumn
	.set	noreorder

	#	reserve stack
	#	save caller-saved registers to stack
	addiu	sp,	sp,	-32
	sd	s4,	24(sp)
	sd	s3,	16(sp)
	sd	s2,	8(sp)
	sd	ra,	0(sp)

	#	t1 == dc_yh
	lw	t1,	%gp_rel(dc_yh)(gp)
	#	s2 == dc_yl
	lw	s2,	%gp_rel(dc_yl)(gp)
	
	#	count == dc_yh - dc_yl
	subu	s3,	t1,	s2

	#	(count < 0) ? goto _R_DrawColumnEnd; : /* do nothing */ ;
	bltz	s3,	_R_DrawColumn_end
	#	delay slot
	#	a1 = scaledviewwidth
	lw	a1,	%gp_rel(scaledviewwidth)(gp)
	
	#	dest = _ylookup(dc_yl,scaledviewwidth,viewheight) + _columnofs(dc_x,scaledviewwidth,viewheight); 
	#	a0 = dc_yl
	addu	a0,	s2,	zero

	#	a2 = viewheight
	lw	a2,	%gp_rel(viewheight)(gp)

	#	_ylookup(a0,a1,a2)
	jal	_ylookup
	#	delay slot
	nop
	
	#	v0 now contains ylookup result
	#	store ylookup result in s4
	addu	s4,	v0,	zero

	#	t5 =  dc_x
	lw	t5,	%gp_rel(dc_x)(gp)	

	#	a0 = dc_x
	addu	a0,	t5,	zero

	#	a1 = scaledviewwidth
	lw	a1,	%gp_rel(scaledviewwidth)(gp)

	#	a2 = viewheight
	lw	a2,	%gp_rel(viewheight)(gp)

	#	_columnofs(a0,a1,a2)
	jal	_columnofs
	#	delay slot
	nop

	#	a0 now contains columnofs lookup result
	#	store columnofs lookup result in t6
	addu	t6,	v0,	zero

	#	fracstep = dc_iscale;
	#	t1 = fracstep
	lw	t1,	%gp_rel(dc_iscale)(gp)

	#	dest = ylookup + columnofs
	#	t6 = dest
	addu	t6,	s4,	t6 

	#	s2 = dc_yl
	#	t5 = centery
	lw	t5,	%gp_rel(centery)(gp)

	#	s2 = dc_yl - centery
	subu	s2,	s2,	t5

	#	frac = dc_texturemid + (dc_yl-centery)*fracstep;
	multu	s2,	t1
	#	s2 = (dc_yl-centery)*fracstep`
	mflo	s2

	#	t5 = dc_texturemid
	lw	t5,	%gp_rel(dc_texturemid)(gp)
	#	t5 = frac
	addu	t5,	t5,	s2

	#	subproc
	#	name:
	#		_R_DrawColumn_count
	#	inputs:
	#		t1 == fracstep
	#		s3 == count
	#		t5 == frac
	#		t6 == dest

	#	(s3 <= 0) ? goto _R_DrawColumn_end; : /* do nothing */ ;
_R_DrawColumn_count:	
	blez	s3,	_R_DrawColumn_end
	#	delay slot
	#	s2 = (frac>>FRACBITS)
	sra	s2,	t5,	FRACBITS

	#	s3--
	addi	s3,	s3,	-1 

	#	s2 = source_index = s2 & 127
	andi	s2,	s2,	127

	#	t7 = dc_source
	lw	t7,	%gp_rel(dc_source)(gp)
	#	s2 = src = dc_source+source_index
	addu	s2,	t7,	s2
	#	s4 = byte at src
	lbu	s4,	0(s2)
	#	t7 = dc_colormap
	lw	t7,	%gp_rel(dc_colormap)(gp)
	#	t7 = dst = dc_colormap+src
	addu	t7,	t7,	s4

	#	*dest = dst
	#	s2 = memory[dst]
	lbu	s2,	0(t7)
	#	memory[dest] = s2
	sb	s2,	0(t6)

	#	add SCREENWIDTH to dest
	addiu	t6,	t6,	SCREENWIDTH

	#	goto	_R_DrawColumn_count;
	b	_R_DrawColumn_count
	#	delay slot
	#	add fracstep to frac
	addu	t5,	t5,	t1

	#	restore caller-saved registers from stack
	#	return stack
	#	jump register to saved return address
_R_DrawColumn_end:
	ld	ra,	0(sp)
	ld	s2,	8(sp)
	ld	s3,	16(sp)
	ld	s4,	24(sp)

	addiu	sp,	sp,	32

	jr	ra
	#	delay slot
	nop

	.set	reorder