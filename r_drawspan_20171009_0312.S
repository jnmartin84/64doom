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
//# :: ifndef RDPRENDER
#if 1
R_DrawSpan:
	.global	R_DrawSpan
	.set	noreorder

	#	reserve stack
	#	save caller-saved registers to stack
	addiu	sp,	sp,	-48
	sd	s4,	40(sp)
	sd	s3,	32(sp)
	sd	s2,	24(sp)
	sd	s1,	16(sp)
	sd	s0,	8(sp)
	sd	ra,	0(sp)

	#	// We do not check for zero spans here?
	#	count = ds_x2 - ds_x1
	#	t0 = ds_x1
	lw	t0,	%gp_rel(ds_x1)(gp)
	#	{t0:ds_x1}
	#	t1 = ds_x2
	lw	t1,	%gp_rel(ds_x2)(gp)
	#	{t0:ds_x1, t1:ds_x2}
	
	#	count = t1 - t0
	#	s0 = t1 - t0
	#	s0 = count
	subu	s0,	t1,	t0
	#	{s0:count,	t0:ds_x1, t1:ds_x2}
	
	#	(count <= 0) ? (goto _R_DrawSpan_end; } : { /* do nothing */ ; }
	blezl	s0, _R_DrawSpan_end
	#	delay slot
	nop
	
	#	s1 = xfrac = ds_xfrac
	lwu	s1,	%gp_rel(ds_xfrac)(gp)
	#	{s0:count, s1:xfrac, t0:ds_x1, t1:ds_x2}

	#	s2 = yfrac = ds_yfrac
	lwu	s2,	%gp_rel(ds_yfrac)(gp)
	#	{s0:count, s1:xfrac, s2:yfrac, t0:ds_x1, t1:ds_x2}

	# dest = _ylookup(ds_y,scaledviewwidth,viewheight) + _columnofs(ds_x1,scaledviewwidth,viewheight);

	#	a0 = ds_y
	lw	a0,	%gp_rel(ds_y)(gp)
	#	{a0:ds_y, s0:count, s1:xfrac, s2:yfrac, t0:ds_x1, t1:ds_x2}
	
	#	a1 = scaledviewwidth
	lw	a1,	%gp_rel(scaledviewwidth)(gp)
	#	{a0:ds_y, a1:scaledviewwidth, s0:count, s1:xfrac, s2:yfrac, t0:ds_x1, 
	#	t1:ds_x2}

	#	a2 = viewheight
	lw	a2,	%gp_rel(viewheight)(gp)
	#	{a0:ds_y, a1:scaledviewwidth, a2:viewheight, s0:count, s1:xfrac, s2:yfrac, 
	#	t0:ds_x1, t1:ds_x2}

	#	_ylookup(a0,a1,a2)
	jal	_ylookup
	#	{a0:ds_y, a1:scaledviewwidth, a2:viewheight, s0:count, s1:xfrac, s2:yfrac, 
	#	t0:ds_x1, t1:ds_x2}
	#	delay slot
	nop
	#	{s0:count, s1:xfrac, s2:yfrac, v0:_ylookup}
	
	#	v0 now contains ylookup result
	#	store ylookup result in s3
	addu	s3,	v0,	zero
	#	{s0:count, s1:xfrac, s2:yfrac, s3:_ylookup, v0:_ylookup}

	#	t0 = ds_x1
	#	a0 = t0
	lw	t0,	%gp_rel(ds_x1)(gp)
	#	{s0:count, s1:xfrac, s2:yfrac, s3:_ylookup, t0:ds_x1, v0:_ylookup}
	addu	a0,	t0,	zero
	#	{a0:ds_x1, s0:count, s1:xfrac, s2:yfrac, s3:_ylookup, t0:ds_x1, v0:_ylookup}

	#	a1 = scaledviewwidth
	lw	a1,	%gp_rel(scaledviewwidth)(gp)
	#	{a0:ds_x1, a1:scaledviewwidth, s0:count, s1:xfrac, s2:yfrac,
	#	s3:_ylookup, t0:ds_x1, v0:_ylookup}

	#	a2 = viewheight
	lw	a2,	%gp_rel(viewheight)(gp)
	#	{a0:ds_x1, a1:scaledviewwidth, a2:viewheight, s0:count, s1:xfrac, s2:yfrac,
	#	s3:_ylookup, t0:ds_x1, v0:_ylookup}

	#	_columnofs(a0,a1,a2)
	jal	_columnofs
	#	{a0:ds_x1, a1:scaledviewwidth, a2:viewheight, s0:count, s1:xfrac, s2:yfrac,
	#	s3:_ylookup, t0:ds_x1, v0:_ylookup}
	#	delay slot
	nop
	#	{s0:count, s1:xfrac, s2:yfrac, s3:_ylookup, v0:_columnofs}

	#	v0 now contains columnofs lookup result
	#	store columnofs lookup result in s4
	addu	s4,	v0,	zero	
	#	{s0:count, s1:xfrac, s2:yfrac, s3:_ylookup, s4:_columnofs, v0:_columnofs}

	#	t8 = _ylookup + _columnofs
	#	t8 = s3 + s4
	#	t8 = dest
	addu	t8,	s3,	s4
	#	{s0:count, s1:xfrac, s2:yfrac, s3:_ylookup, s4:_columnofs, t8:dest, 
	#	v0:_columnofs}
	
	#	t0 = ds_xstep
	lwu	t0,	%gp_rel(ds_xstep)(gp)
	#	{s0:count, s1:xfrac, s2:yfrac, s3:_ylookup, s4:_columnofs, t0:ds_xstep, 
	#	t8:dest, v0:_columnofs}

	#	t1 = ds_ystep
	lwu	t1,	%gp_rel(ds_ystep)(gp)
	#	{s0:count, s1:xfrac, s2:yfrac, s3:_ylookup, s4:_columnofs, t0:ds_xstep, 
	#	t1:ds_ystep, t8:dest, v0:_columnofs}
	
	#	t2 = ds_source
	lw	t2,	%gp_rel(ds_source)(gp)
	#	{s0:count, s1:xfrac, s2:yfrac, s3:_ylookup, s4:_columnofs, t0:ds_xstep, 
	#	t1:ds_ystep, t2:ds_source, t8:dest, v0:_columnofs}

	#	t3 = ds_colormap
	lw	t3,	%gp_rel(ds_colormap)(gp)
	#	{s0:count, s1:xfrac, s2:yfrac, s3:_ylookup, s4:_columnofs, t0:ds_xstep, 
	#	t1:ds_ystep, t2:ds_source, t3:ds_colormap, t8:dest, v0:_columnofs}
	
	#	(s0 <= 0) ? goto _R_DrawSpan_end; : /* do nothing */ ;
_R_DrawSpan_count:	
	bltz	s0,	_R_DrawSpan_end
	#	{s0:count, s1:xfrac, s2:yfrac, s3:_ylookup, s4:_columnofs, t0:ds_xstep, 
	#	t1:ds_ystep, t2:ds_source, t3:ds_colormap, t8:dest, v0:_columnofs}
	#	delay slot
	nop
	#	{s0:count, s1:xfrac, s2:yfrac, s3:_ylookup, s4:_columnofs, t0:ds_xstep, 
	#	t1:ds_ystep, t2:ds_source, t3:ds_colormap, t8:dest, v0:_columnofs}

	#	s0--
	addi	s0,	s0,	-1 
	#	{s0:count, s1:xfrac, s2:yfrac, s3:_ylookup, s4:_columnofs, t0:ds_xstep, 
	#	t1:ds_ystep, t2:ds_source, t3:ds_colormap, t8:dest, v0:_columnofs}

	#	// Current texture index in u,v.
	#	spot = ((yfrac>>(16-6))&(63*64)) + ((xfrac>>16)&63);
	#	t4 = yfrac >> (16-6)
	sra	t4,	s2, 10
	#	{s0:count, s1:xfrac, s2:yfrac, s3:_ylookup, s4:_columnofs, t0:ds_xstep, 
	#	t1:ds_ystep, t2:ds_source, t3:ds_colormap, t4:yfrac>>10, t8:dest, 
	#	v0:_columnofs}
	
	#	t4 = t4 & (63*64)
	andi	t4,	t4,	0xfc0 # (63*64)
	#	{s0:count, s1:xfrac, s2:yfrac, s3:_ylookup, s4:_columnofs, t0:ds_xstep, 
	#	t1:ds_ystep, t2:ds_source, t3:ds_colormap, t4:(yfrac>>10)&4032,
	#	t5:xfrac>>16, t8:dest, v0:_columnofs}

	#	t5 = xfrac >> 16
	sra	t5,	s1,	16
	#	{s0:count, s1:xfrac, s2:yfrac, s3:_ylookup, s4:_columnofs, t0:ds_xstep, 
	#	t1:ds_ystep, t2:ds_source, t3:ds_colormap, t4:yfrac>>10, t5:xfrac>>16,
	#	t8:dest, v0:_columnofs}
	
	#	t5 = t5 & 63
	andi	t5,	t5,	0x3f # 63
	#	{s0:count, s1:xfrac, s2:yfrac, s3:_ylookup, s4:_columnofs, t0:ds_xstep, 
	#	t1:ds_ystep, t2:ds_source, t3:ds_colormap, t4:(yfrac>>10)&4032,
	#	t5:(frac>>16)&63, t8:dest, v0:_columnofs}
	
	#	t6 = spot = t4 + t5
	addu	t6,	t4,	t5
	#	{s0:count, s1:xfrac, s2:yfrac, s3:_ylookup, s4:_columnofs, t0:ds_xstep, 
	#	t1:ds_ystep, t2:ds_source, t3:ds_colormap, t4:(yfrac>>10)&4032,
	#	t5:(frac>>16)&63, t6:spot, t8:dest, v0:_columnofs}

	#	last_spot = spot
	#	for debugging...
	sw	t6,	%gp_rel(last_spot)(gp)
	
	#	t7 = &ds_source[spot]
	addu	t7,	t2,	t6
	#	{s0:count, s1:xfrac, s2:yfrac, s3:_ylookup, s4:_columnofs, t0:ds_xstep, 
	#	t1:ds_ystep, t2:ds_source, t3:ds_colormap, t4:(yfrac>>10)&4032,
	#	t5:(frac>>16)&63, t6:spot, t7:&ds_source[spot], t8:dest, v0:_columnofs}

	#	*dest++ = ds_colormap[ds_source[spot]];

	#	t9 = ds_source[spot]
	lbu		t9,	0(t7)
	#	{s0:count, s1:xfrac, s2:yfrac, s3:_ylookup, s4:_columnofs, t0:ds_xstep, 
	#	t1:ds_ystep, t2:ds_source, t3:ds_colormap, t4:(yfrac>>10)&4032,
	#	t5:(frac>>16)&63, t6:spot, t7:&ds_source[spot], t8:dest, 
	#	t9:ds_source[spot], v0:_columnofs}
	
	#	t4 = &ds_colormap[t9]
	addu	t4,	t3,	t9
	#	{s0:count, s1:xfrac, s2:yfrac, s3:_ylookup, s4:_columnofs, t0:ds_xstep, 
	#	t1:ds_ystep, t2:ds_source, t3:ds_colormap, t4:&ds_colormap[t7],
	#	t5:(frac>>16)&63, t6:spot, t7:&ds_source[spot], t8:dest, 
	#	t9:ds_source[spot], v0:_columnofs}
	
	#	t7	= ds_colormap[t4]
	lbu		t7,	0(t4)
	#	{s0:count, s1:xfrac, s2:yfrac, s3:_ylookup, s4:_columnofs, t0:ds_xstep, 
	#	t1:ds_ystep, t2:ds_source, t3:ds_colormap, t4:&ds_colormap[t7],
	#	t5:(frac>>16)&63, t6:spot, t7:ds_coloramp[t4], t8:dest, 
	#	t9:ds_source[spot], v0:_columnofs}

	#	0(t8) = t7
	sb		t7,	0(t8)
	#	{s0:count, s1:xfrac, s2:yfrac, s3:_ylookup, s4:_columnofs, t0:ds_xstep, 
	#	t1:ds_ystep, t2:ds_source, t3:ds_colormap, t4:&ds_colormap[t7],
	#	t5:(frac>>16)&63, t6:spot, t7:ds_coloramp[t4], t8:dest, 
	#	t9:ds_source[spot], v0:_columnofs}

	#	dest++
	addiu	t8,	t8,	1
	#	{s0:count, s1:xfrac, s2:yfrac, s3:_ylookup, s4:_columnofs, t0:ds_xstep, 
	#	t1:ds_ystep, t2:ds_source, t3:ds_colormap, t4:&ds_colormap[t7],
	#	t5:(frac>>16)&63, t6:spot, t7:ds_coloramp[t4], t8:dest, 
	#	t9:ds_source[spot], v0:_columnofs}

	#	xfrac += ds_xstep
	#	s1 += t0
	addu	s1,	s1,	t0
	#	{s0:count, s1:xfrac, s2:yfrac, s3:_ylookup, s4:_columnofs, t0:ds_xstep, 
	#	t1:ds_ystep, t2:ds_source, t3:ds_colormap, t4:&ds_colormap[t7],
	#	t5:(frac>>16)&63, t6:spot, t7:ds_coloramp[t4], t8:dest, 
	#	t9:ds_source[spot], v0:_columnofs}
	
	#	trying to figure out why the "addu	s2,	s2,	t1" doesn't modify s2
	nop
	
	#	yfrac += ds_ystep
	#	s2 += t1
	addu	s2,	s2,	t1
	#	{s0:count, s1:xfrac, s2:yfrac, s3:_ylookup, s4:_columnofs, t0:ds_xstep, 
	#	t1:ds_ystep, t2:ds_source, t3:ds_colormap, t4:&ds_colormap[t7],
	#	t5:(frac>>16)&63, t6:spot, t7:ds_coloramp[t4], t8:dest, 
	#	t9:ds_source[spot], v0:_columnofs}

	#	trying to figure out why the "addu	s2,	s2,	t1" doesn't modify s2
	nop

	#	goto	_R_DrawSpan_count;
	b	_R_DrawSpan_count
	#	{s0:count, s1:xfrac, s2:yfrac, s3:_ylookup, s4:_columnofs, t0:ds_xstep, 
	#	t1:ds_ystep, t2:ds_source, t3:ds_colormap, t4:&ds_colormap[t7],
	#	t5:(frac>>16)&63, t6:spot, t7:ds_coloramp[t4], t8:dest, 
	#	t9:ds_source[spot], v0:_columnofs}
	#	delay slot
	nop
	#	{s0:count, s1:xfrac, s2:yfrac, s3:_ylookup, s4:_columnofs, t0:ds_xstep, 
	#	t1:ds_ystep, t2:ds_source, t3:ds_colormap, t4:&ds_colormap[t7],
	#	t5:(frac>>16)&63, t6:spot, t7:ds_coloramp[t4], t8:dest, 
	#	t9:ds_source[spot], v0:_columnofs}
	
	#	restore caller-saved registers from stack
	#	return stack
	#	jump register to saved return address
_R_DrawSpan_end:
	ld	ra,	0(sp)
	ld	s0,	8(sp)
	ld	s1,	16(sp)
	ld	s2,	24(sp)
	ld	s3,	32(sp)
	ld	s4,	40(sp)

	addiu	sp,	sp,	48

	jr	ra
	#	delay slot
	nop

	.set	reorder
#endif