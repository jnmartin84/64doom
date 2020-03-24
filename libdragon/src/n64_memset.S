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

/* void *memset(void *s, int c, size_t n).  */

# define SWHI	swl		/* high part is left in big-endian	*/


#define zero $0
#define at $1
#define v0 $2
#define v1 $3
#define a0 $4
#define a1 $5
#define a2 $6
#define a3 $7
#define t0 $8
#define t1 $9
#define t2 $10
#define t3 $11
#define t4 $12
#define t5 $13
#define t6 $14
#define t7 $15
#define s0 $16
#define s1 $17
#define s2 $18
#define s3 $19
#define s4 $20
#define s5 $21
#define s6 $22
#define s7 $23
#define t8 $24
#define t9 $25
#define k0 $26
#define k1 $27
#define gp $28
#define sp $29
#define fp $30
#define ra $31

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


#experimental results: 32.0099 megaticks to run compared to 4.0015 megaticks for memsetASM
#__n64_zero_ASM:
#	.global __n64_zero_ASM
#	.set	noreorder
#
#	beq	a2, zero, L_endZero	# if length is zero, go to end
#	addu	t1, a2, zero		# put length into t1
#	addu	t2, a0, zero		# put pointer into t2
#L_nextZero:
#	sb	zero, 0(t2)		# store zero at pointer
#	addiu	t2, 1			# increment pointer by 1 byte
#	bne	t1, zero, L_nextZero	# if length is not zero, go again
#	addiu	t1, -1			# decrement length
#L_endZero:
#	j	ra
#	nop
#	.set	reorder





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
