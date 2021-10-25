/*
 * Copyright (c) 2013
 *	MIPS Technologies, Inc., California.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the MIPS Technologies, Inc., nor the names of its
 *	contributors may be used to endorse or promote products derived from
 *	this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE MIPS TECHNOLOGIES, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE MIPS TECHNOLOGIES, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
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

#define __mips 3

#ifndef L
#define L(label) $L ## label
#endif
#ifndef PTR_ADDIU
#define PTR_ADDIU addiu
#endif

#define PTR_SUBU subu
#define PTR_ADDU addu

#ifndef PTR_SRA
#define PTR_SRA sra
#endif

#define C_ST sw
#define C_STHI swl
#define NSIZE 4
#define NSIZEMASK 0x1f
#define NSIZEDMASK 0x3f
#define UNIT(unit) ((unit)*NSIZE)
#define UNITM1(unit) (((unit)*NSIZE)-1)

	.align 3
	.global __n64_memset_ZERO_ASM
	.global	__n64_memset_ASM
	.set	nomips16
	.set	nomicromips
	.set	noreorder
	.set	nomacro
	.ent	__n64_memset_ASM
	.type	__n64_memset_ASM,	@function

__n64_memset_ZERO_ASM:
__n64_memset_ASM:
	slti	t2,a2,(2 * NSIZE)
	bne	t2,zero,L(lastb)
	move	v0,a0
	beq	a1,zero,L(set0)
	PTR_SUBU a3,zero,a0
	nop
	and	 a1,0xff
	sll	t2,a1,8
	or	a1,t2
	sll	t2,a1,16
	or	a1,t2
L(set0):
	andi	t2,a3,(NSIZE-1)
	beq	t2,zero,L(aligned)
	PTR_SUBU a2,a2,t2
	C_STHI	a1,0(a0)
	PTR_ADDU a0,a0,t2
L(aligned):
	andi	t8,a2,NSIZEDMASK
	beq	a2,t8,L(chkw)
	PTR_SUBU a3,a2,t8
	PTR_ADDU a3,a0,a3
L(loop16w):
L(skip_pref):
	C_ST	a1,UNIT(0)(a0)
	C_ST	a1,UNIT(1)(a0)
	C_ST	a1,UNIT(2)(a0)
	C_ST	a1,UNIT(3)(a0)
	C_ST	a1,UNIT(4)(a0)
	C_ST	a1,UNIT(5)(a0)
	C_ST	a1,UNIT(6)(a0)
	C_ST	a1,UNIT(7)(a0)
	C_ST	a1,UNIT(8)(a0)
	C_ST	a1,UNIT(9)(a0)
	C_ST	a1,UNIT(10)(a0)
	C_ST	a1,UNIT(11)(a0)
	C_ST	a1,UNIT(12)(a0)
	C_ST	a1,UNIT(13)(a0)
	C_ST	a1,UNIT(14)(a0)
	C_ST	a1,UNIT(15)(a0)
	PTR_ADDIU a0,a0,UNIT(16)
	bne	a0,a3,L(loop16w)
	nop
	move	a2,t8
L(chkw):
	andi	t8,a2,NSIZEMASK
	beq	a2,t8,L(chk1w)
	nop
	C_ST	a1,UNIT(0)(a0)
	C_ST	a1,UNIT(1)(a0)
	C_ST	a1,UNIT(2)(a0)
	C_ST	a1,UNIT(3)(a0)
	C_ST	a1,UNIT(4)(a0)
	C_ST	a1,UNIT(5)(a0)
	C_ST	a1,UNIT(6)(a0)
	C_ST	a1,UNIT(7)(a0)
	PTR_ADDIU a0,a0,UNIT(8)
L(chk1w):
	andi	a2,t8,(NSIZE-1)
	beq	a2,t8,L(lastb)
	PTR_SUBU a3,t8,a2
	PTR_ADDU a3,a0,a3
L(wordCopy_loop):
	PTR_ADDIU a0,a0,UNIT(1)
	bne	a0,a3,L(wordCopy_loop)
	C_ST	a1,UNIT(-1)(a0)
L(lastb):
	blez	a2,L(leave)
	PTR_ADDU a3,a0,a2
L(lastbloop):
	PTR_ADDIU a0,a0,1
	bne	a0,a3,L(lastbloop)
	sb	a1,-1(a0)
L(leave):
	j	ra
	nop

	.set	macro
	.set	reorder
	.end	__n64_memset_ASM
    .size   __n64_memset_ASM, .-__n64_memset_ASM
