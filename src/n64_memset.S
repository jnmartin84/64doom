/* Copyright (C) 2002, 2003 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Hartvig Ekner <hartvige@mips.com>, 2002.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include "mips.h"

/* void *memset(void *s, int c, size_t n).  */

__n64_memset_ASM:
        .global __n64_memset_ASM

	.set	noreorder

	slti	t1, a2, 8		# Less than 8?
	bne	t1, zero, L_last8
	move	v0, a0			# Setup exit value before too late

	beq	a1, zero, L_ueven	# If zero pattern, no need to extend
	andi	a1, 0xff		# Avoid problems with bogus arguments
	sll	t0, a1, 8
	or	a1, t0
	sll	t0, a1, 16
	or	a1, t0			# a1 is now pattern in full word

L_ueven:	
	subu	t0, zero, a0		# Unaligned address?
	andi	t0, 0x3
	beq	t0, zero, L_chkw
	subu	a2, t0
	SWHI	a1, 0(a0)		# Yes, handle first unaligned part
	addu	a0, t0			# Now both a0 and a2 are updated

L_chkw:	
	andi	t0, a2, 0x7		# Enough left for one loop iteration?
	beq	t0, a2, L_chkl
	subu	a3, a2, t0
	addu	a3, a0			# a3 is last loop address +1
	move	a2, t0			# a2 is now # of bytes left after loop
L_loopw:	
	addiu	a0, 8			# Handle 2 words pr. iteration
	sw	a1, -8(a0)
	bne	a0, a3, L_loopw
	sw	a1, -4(a0)

L_chkl:	
	andi	t0, a2, 0x4		# Check if there is at least a full
	beq	t0, zero, L_last8	#  word remaining after the loop
	subu	a2, t0
	sw	a1, 0(a0)		# Yes...
	addiu	a0, 4

L_last8:	
	blez	a2, L_exit		# Handle last 8 bytes (if cnt>0)
	addu	a3, a2, a0		# a3 is last address +1
L_lst8l:	
	addiu	a0, 1
	bne	a0, a3, L_lst8l
	sb	a1, -1(a0)
L_exit:	
	j	ra			# Bye, bye
	nop

	.set	reorder


__n64_memset_ZERO_ASM:
        .global __n64_memset_ZERO_ASM

	.set	noreorder

	slti	t1, a2, 8		# Less than 8?
	bne	t1, zero, L_last8
	move	v0, a0			# Setup exit value before too late

	beq	a1, zero, L_uevenZERO	# If zero pattern, no need to extend

L_uevenZERO:	
	subu	t0, zero, a0		# Unaligned address?
	andi	t0, 0x3
	beq	t0, zero, L_chkwZERO
	subu	a2, t0
	SWHI	zero, 0(a0)		# Yes, handle first unaligned part
	addu	a0, t0			# Now both a0 and a2 are updated

L_chkwZERO:	
	andi	t0, a2, 0x7		# Enough left for one loop iteration?
	beq	t0, a2, L_chklZERO
	subu	a3, a2, t0
	addu	a3, a0			# a3 is last loop address +1
	move	a2, t0			# a2 is now # of bytes left after loop
L_loopwZERO:	
	addiu	a0, 8			# Handle 2 words pr. iteration
	sw	zero, -8(a0)
	bne	a0, a3, L_loopwZERO
	sw	zero, -4(a0)

L_chklZERO:	
	andi	t0, a2, 0x4		# Check if there is at least a full
	beq	t0, zero, L_last8ZERO	#  word remaining after the loop
	subu	a2, t0
	sw	zero, 0(a0)		# Yes...
	addiu	a0, 4

L_last8ZERO:	
	blez	a2, L_exitZERO		# Handle last 8 bytes (if cnt>0)
	addu	a3, a2, a0		# a3 is last address +1
L_lst8lZERO:	
	addiu	a0, 1
	bne	a0, a3, L_lst8lZERO
	sb	zero, -1(a0)
L_exitZERO:	
	j	ra			# Bye, bye
	nop

	.set	reorder
