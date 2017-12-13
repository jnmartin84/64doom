#include "mips.h"

// Swap 16bit, that is, MSB and LSB byte.
/* unsigned short SwapSHORT (unsigned short x) */
	.set	nomips16
	.set	nomicromips
	.ent	SwapSHORT
	.type	SwapSHORT, @function
SwapSHORT:
	.global	SwapSHORT
	.set	noreorder
	.set	nomacro

	srl	t0,	a0,	8
	sll	t1,	a0,	8
	or	v0,	t0,	t1
	andi	v0,	v0,	0xFFFF
	
	jr	ra
	nop

	.set	macro
	.set	reorder
	.end	SwapSHORT

// Swapping 32bit.
/* unsigned long SwapLONG (unsigned long x) */
	.set	nomips16
	.set	nomicromips
	.ent	SwapLONG
	.type	SwapLONG, @function
SwapLONG:
	.global	SwapLONG
	.set	noreorder
	.set	nomacro

	srl	t0,	a0,	24
	srl	t1,	a0,	8
	andi	t1,	t1,	0xff00
	andi	t2,	a0,	0xff00
	sll	t2,	t2,	8
	sll	t3,	a0,	24
	or	v0,	t0,	t1
	or	v0,	v0,	t2
	or	v0,	v0,	t3

	jr	ra
	nop

	.set	macro
	.set	reorder
	.end	SwapLONG
