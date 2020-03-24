#include "mips.h"

//
// Swap 16bit, that is, MSB and LSB byte.
//

/* unsigned short SwapSHORT (unsigned short x) */
	.global	SwapSHORT
	.set	nomips16
	.set	nomicromips
	.ent	SwapSHORT
	.type	SwapSHORT, @function
SwapSHORT:
	.set	noreorder
	.set	nomacro

	srl	t0,	a0,	8
	sll	t1,	a0,	8
	or	v0,	t0,	t1
	jr	ra
	andi	v0,	v0,	0xFFFF

	.set	macro
	.set	reorder
	.end	SwapSHORT
	.size   SwapSHORT, .-SwapSHORT

//
// Swapping 32bit.
//

/* unsigned long SwapLONG (unsigned long x) */
	.global	SwapLONG
	.set	nomips16
	.set	nomicromips
	.ent	SwapLONG
	.type	SwapLONG, @function
SwapLONG:
	.set	noreorder
	.set	nomacro

	srl	t1,	a0,	8
	srl	t0,	a0,	24
	andi	t2,	a0,	0xff00
	andi	t1,	t1,	0xff00
	sll	t3,	a0,	24
	sll	t2,	t2,	8
	or	v0,	t0,	t1
	or	v0,	v0,	t2
	jr	ra
	or	v0,	v0,	t3
	
	.set	macro
	.set	reorder
	.end	SwapLONG
    .size   SwapLONG, .-SwapLONG