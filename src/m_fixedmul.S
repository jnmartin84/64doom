#include "mips.h"

/* fixed_t FixedMul ( fixed_t	a, fixed_t	b ) */
	.set	nomips16
	.set	nomicromips
	.ent	FixedMul
	.type	FixedMul, @function
FixedMul:
	.global	FixedMul
	.set	noreorder
	.set	nomacro

	daddu	a1,	a1,	zero
	addu	a0,	a0,	zero
	dmultu	a1,	a0
	mflo	v0
	dsrl	v0,	16
	# without the following line the result is wrong
	# even if it does look like a no-op
	sll	v0,	v0,	0
	jr	ra
	nop

	.set	macro
	.set	reorder
	.end	FixedMul
