#include "mips.h"

//
// Swap 16bit, that is, MSB and LSB byte.
//

#if 1
/* unsigned short SwapSHORT (unsigned short x) */
	.global	SwapSHORT
	.set	nomips16
	.set	nomicromips
	.ent	SwapSHORT
	.type	SwapSHORT, @function
	.set	noreorder
	.set	nomacro
SwapSHORT:
	srl	t0,	a0,	8
	sll	t1,	a0,	8
	or	v0,	t0,	t1
	jr	ra
 	andi	v0,	v0,	0xFFFF
	.set	macro
	.set	reorder
	.end	SwapSHORT
	.size   SwapSHORT, .-SwapSHORT
#endif

//
// Swapping 32bit.
//

/* unsigned long SwapLONG (unsigned long x) */
	.global	SwapLONG
	.set	nomips16
	.set	nomicromips
	.ent	SwapLONG
	.type	SwapLONG, @function
	.set	noreorder
	.set	nomacro
SwapLONG:
	srl	t1,	a0,	8
	srl	t0,	a0,	24
	andi	t2,	a0,	0xff00
	andi	t1,	t1,	0xff00
	sll	t3,	a0,	24
	sll	t2,	t2,	8
	or	v0,	t0,	t1
	or	v1,	t2,	t3
	jr	ra
	or	v0,	v0,	v1
	.set	macro
	.set	reorder
	.end	SwapLONG
	.size   SwapLONG, .-SwapLONG
