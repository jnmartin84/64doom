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
	addiu	sp,	sp,	-112
	sd	s7,	0(sp)
	sd	s6,	8(sp)
	sd	s5,	16(sp)
	sd	s4,	24(sp)
	sd	s3,	32(sp)
	sd	s2,	40(sp)
	sd	s1,	48(sp)
	sd	s0,	56(sp)
	sd	a2,	64(sp)
	sd	a1,	72(sp)
	sd	a0,	80(sp)
	sd	v1,	88(sp)
	sd	v0,	96(sp)
	sd	ra,	104(sp)

	#	s1 == dc_yh
	lw	s1,	%gp_rel(dc_yh)(gp)
	#	s2 == dc_yl
	lw	s2,	%gp_rel(dc_yl)(gp)
	
	#	count == dc_yh - dc_yl
	subu	s3,	s1,	s2

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
	#	s5 =  dc_x
	lw	s5,	%gp_rel(dc_x)(gp)	

	#	v0 now contains ylookup result
	#	store ylookup result in s4
	addu	s4,	v0,	zero

	#	a0 = dc_x
	addu	a0,	s5,	zero

	#	a1 = scaledviewwidth
	lw	a1,	%gp_rel(scaledviewwidth)(gp)

	#	a2 = viewheight
	lw	a2,	%gp_rel(viewheight)(gp)

	#	_columnofs(a0,a1,a2)
	jal	_columnofs
	#	delay slot
	#	fracstep = dc_iscale;
	#	s1 = fracstep
	lw	s1,	%gp_rel(dc_iscale)(gp)

	#	a0 now contains columnofs lookup result
	#	store columnofs lookup result in s6
	addu	s6,	v0,	zero

	#	dest = ylookup + columnofs
	#	s6 = dest
	addu	s6,	s4,	s6 

	#	s2 = dc_yl
	#	s5 = centery
	lw	s5,	%gp_rel(centery)(gp)

	#	s2 = dc_yl - centery
	subu	s2,	s2,	s5

	#	frac = dc_texturemid + (dc_yl-centery)*fracstep;
	mult	s2,	s1
	#	s2 = (dc_yl-centery)*fracstep`
	mflo	s2

	#	s5 = dc_texturemid
	lw	s5,	%gp_rel(dc_texturemid)(gp)
	#	s5 = frac
	addu	s5,	s5,	s2

	#	subproc
	#	name:
	#		_R_DrawColumn_count
	#	inputs:
	#		s1 == fracstep
	#		s3 == count
	#		s5 == frac
	#		s6 == dest

	#	(s3 <= 0) ? goto _R_DrawColumn_end; : /* do nothing */ ;
_R_DrawColumn_count:	
	blez	s3,	_R_DrawColumn_end
	#	delay slot
	#	s2 = (frac>>FRACBITS)
	sra	s2,	s5,	FRACBITS

	#	s3--
	addi	s3,	s3,	-1 

	#	s2 = source_index = s2 & 127
	andi	s2,	s2,	127

	#	s7 = dc_source
	lw	s7,	%gp_rel(dc_source)(gp)
	#	s2 = src = dc_source+source_index
	add	s2,	s7,	s2
	#	s4 = byte at src
	lb	s4,	0(s2)
	#	s7 = dc_colormap
	lw	s7,	%gp_rel(dc_colormap)(gp)
	#	s7 = dst = dc_colormap+src
	addu	s7,	s7,	s4

	#	*dest = dst
	#	s2 = memory[dst]
	lb	s2,	0(s7)
	#	memory[dest] = s2
	sb	s2,	0(s6)

	#	add SCREENWIDTH to dest
	addiu	s6,	s6,	SCREENWIDTH

	#	goto	_R_DrawColumn_count;
	b	_R_DrawColumn_count
	#	delay slot
	#	add fracstep to frac
	addu	s5,	s5,	s1

	#	restore caller-saved registers from stack
	#	return stack
	#	jump register to saved return address
_R_DrawColumn_end:
	ld	ra,	104(sp)
	ld	v0,	96(sp)
	ld	v1,	88(sp)
	ld	a0,	80(sp)
	ld	a1,	72(sp)
	ld	a2,	64(sp)
	ld	s0,	56(sp)
	ld	s1,	48(sp)
	ld	s2,	40(sp)
	ld	s3,	32(sp)
	ld	s4,	24(sp)
	ld	s5,	16(sp)
	ld	s6,	8(sp)
	ld	s7,	0(sp)

	addiu	sp,	sp,	112

	jr	ra
	#	delay slot
	nop

	.set	reorder