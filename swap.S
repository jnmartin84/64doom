#include "mips.h"

SWAPSHORT:
	.global	SWAPSHORT
	.set	noreorder

	srl	t0,	a0,	8
	sll	t1,	a0,	8
	or	v0,	t0,	t1
	jr	ra	
	nop

	.set	reorder

SWAPLONG:
	.global	SWAPLONG
	.set	noreorder

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

	.set	reorder
