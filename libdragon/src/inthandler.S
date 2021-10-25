/*
   Simple interrupt handler, hands off MIPS interrupts to higher level processes.
   Based on INITS.inc from Neon64.

   It is not reentrant, so interrupts are disabled for the duration.
   Safe for doing most things, including FPU operations, within handlers.
*/

#include "regs.S"

inthandler:
	.align 8
	.global inthandler

	/*Save $2 before using*/
	sd $2,save02

	.set noat
	/*Fetch exception pc off cop#0*/
	mfc0 $2,C0_EPC
	nop

	la k1,save01
	sd $1,(k1)

	mfc0 k1,C0_SR
	la $1,saveSR
	sw k1,($1)
	la $1, ~1
	and k1,$1
	mtc0 k1,C0_SR
	.set at

	/*Save EPC now*/
	sw $2,saveEPC

	/* save GPRs */
	/*sd $2,save02 - removed , saved already.At this point it contains epc*/
	sd $3,save03
	sd $4,save04
	sd $5,save05
	sd $6,save06
	sd $7,save07
	sd $8,save08
	sd $9,save09
	sd $10,save10
	sd $11,save11
	sd $12,save12
	sd $13,save13
	sd $14,save14
	sd $15,save15
	sd $16,save16
	sd $17,save17
	sd $18,save18
	sd $19,save19
	sd $20,save20
	sd $21,save21
	sd $22,save22
	sd $23,save23
	sd $24,save24
	sd $25,save25
	sd $26,save26
	sd $27,save27
	sd $28,save28
	sd $29,save29
	sd $30,save30
	sd $31,save31
	mflo $30
	sd $30,saveLO
	mfhi $30
	sd $30,saveHI
	cfc1 $30,$f31
	nop
	sd $30,saveFC31

	sdc1 $f0,saveFR00
	sdc1 $f1,saveFR01
	sdc1 $f2,saveFR02
	sdc1 $f3,saveFR03
	sdc1 $f4,saveFR04
	sdc1 $f5,saveFR05
	sdc1 $f6,saveFR06
	sdc1 $f7,saveFR07
	sdc1 $f8,saveFR08
	sdc1 $f9,saveFR09
	sdc1 $f10,saveFR10
	sdc1 $f11,saveFR11
	sdc1 $f12,saveFR12
	sdc1 $f13,saveFR13
	sdc1 $f14,saveFR14
	sdc1 $f15,saveFR15
	sdc1 $f16,saveFR16
	sdc1 $f17,saveFR17
	sdc1 $f18,saveFR18
	sdc1 $f19,saveFR19
	sdc1 $f20,saveFR20
	sdc1 $f21,saveFR21
	sdc1 $f22,saveFR22
	sdc1 $f23,saveFR23
	sdc1 $f24,saveFR24
	sdc1 $f25,saveFR25
	sdc1 $f26,saveFR26
	sdc1 $f27,saveFR27
	sdc1 $f28,saveFR28
	sdc1 $f29,saveFR29
	sdc1 $f30,saveFR30
	sdc1 $f31,saveFR31

	la sp,(exception_stack+65*1024-8)

	mfc0 k1,C0_CAUSE
	andi $30,k1,0xff
	beqz $30, justaninterrupt
	nop

	/*:(*/
//	jal __onCriticalException
	la    $4,  reg_dump
	sw    $0,  0($4)
	sw    $1,  4($4)
	sw    $2,  8($4)
	sw    $3, 12($4)
	sw    $4, 16($4)
	sw    $5, 20($4)
	sw    $6, 24($4)
	sw    $7, 28($4)
	sw    $8, 32($4)
	sw    $9, 36($4)
	sw   $10, 40($4)
	sw   $11, 44($4)
	sw   $12, 48($4)
	sw   $13, 52($4)
	sw   $14, 56($4)
	sw   $15, 60($4)
	sw   $16, 64($4)
	sw   $17, 68($4)
	sw   $18, 72($4)
	sw   $19, 76($4)
	sw   $20, 80($4)
	sw   $21, 84($4)
	sw   $22, 88($4)
	sw   $23, 92($4)
	sw   $24, 96($4)
	sw   $25, 100($4)
	sw   $26, 104($4)
	sw   $27, 108($4)
	sw   $28, 112($4)
	sw   $29, 116($4)
	sw   $30, 120($4)
	sw   $31, 124($4)
	
	mfhi $1
	sw   $1,  128($4)
	mflo $1
	sw   $1,  132($4)

    mfc0 $1,  $0
	sw   $1,  136($4)
	
    mfc0 $1,  $1
	sw   $1,  140($4)

	mfc0 $1,  $2
	sw   $1,  144($4)
	
	mfc0 $1,  $3
	sw   $1,  148($4)

	mfc0 $1,  $4
	sw   $1,  152($4)
	
	mfc0 $1,  $5
	sw   $1,  156($4)
	
	mfc0 $1,  $6
	sw   $1,  160($4)

//	mfc0 $1,  $7
//	sw   $1,  164($4)

	mfc0 $1,  $8
	sw   $1,  168($4)
	
	mfc0 $1,  $9
	sw   $1,  172($4)

    mfc0 $1,  $10
	sw   $1,  176($4)
	
    mfc0 $1,  $11
	sw   $1,  180($4)

	mfc0 $1,  $12
	sw   $1,  184($4)
	
	mfc0 $1,  $13
	sw   $1,  188($4)

	mfc0 $1,  $14
	sw   $1,  192($4)
	
	mfc0 $1,  $15
	sw   $1,  196($4)
	
	mfc0 $1,  $16
	sw   $1,  200($4)

	mfc0 $1,  $17
	sw   $1,  204($4)

	mfc0 $1,  $18
	sw   $1,  208($4)
	
	mfc0 $1,  $19
	sw   $1,  212($4)
	
    mfc0 $1,  $20
	sw   $1,  216($4)
	
//    mfc0 $1,  $21
//	sw   $1,  220($4)
//
//	mfc0 $1,  $22
//	sw   $1,  224($4)
//	
//	mfc0 $1,  $23
//	sw   $1,  228($4)
//
//	mfc0 $1,  $24
//	sw   $1,  232($4)
//	
//	mfc0 $1,  $25
//	sw   $1,  236($4)
	
	mfc0 $1,  $26
	sw   $1,  240($4)

	mfc0 $1,  $27
	sw   $1,  244($4)

	mfc0 $1,  $28
	sw   $1,  248($4)
	
	mfc0 $1,  $29
	sw   $1,  252($4)

	mfc0 $1,  $30
	sw   $1,  256($4)	
	
//	mfc0 $1,  $31
//	sw   $1,  260($4)	
	
/*	swc0 $3,  148($4)
	swc0 $4,  152($4)
	swc0 $5,  156($4)
	swc0 $6,  160($4)
	//swc0 $7,  164($4)
	swc0 $8,  168($4)
	swc0 $9,  172($4)
	swc0 $10, 176($4)
	swc0 $11, 180($4)
	swc0 $12, 184($4)
	swc0 $13, 188($4)
	swc0 $14, 192($4)
	swc0 $15, 196($4)
	swc0 $16, 200($4)
	swc0 $17, 204($4)
	swc0 $18, 208($4)
	swc0 $19, 212($4)
	swc0 $20, 216($4)
	//swc0 $21, 220($4)
	//swc0 $22, 224($4)
	//swc0 $23, 228($4)
	//swc0 $24, 232($4)
	//swc0 $25, 236($4)
	swc0 $26, 240($4)
	swc0 $27, 244($4)
	swc0 $28, 248($4)
	swc0 $29, 252($4)
	swc0 $30, 256($4)
	//swc0 $31, 260($4)*/

	jal register_dump
//l:
//        j l
	nop	
	
	j	endint
	nop
	
justaninterrupt:
	/* check for "pre-NMI" (reset) */
	andi $30,k1,0x1000
	beqz $30, notprenmi
	nop

	/* handle reset */
	jal __onResetException
	nop
	j	endint
	nop

notprenmi:

	/* check for count=compare */
	and $30,k1,0x8000
	beqz $30,notcount
	nop
	mtc0 $0,$11

	/* handle timer exception */
	jal __TI_handler
	nop

	j endint
	nop

notcount:

	/* pass anything else along to handler */
	jal __MI_handler
	nop


endint:
	/* restore GPRs */
	ld $2,save02
	ld $3,save03
	ld $4,save04
	ld $5,save05
	ld $6,save06
	ld $7,save07
	ld $8,save08
	ld $9,save09
	ld $10,save10
	ld $11,save11
	ld $12,save12
	ld $13,save13
	ld $14,save14
	ld $15,save15
	ld $16,save16
	ld $17,save17
	ld $18,save18
	ld $19,save19
	ld $20,save20
	ld $21,save21
	ld $22,save22
	ld $23,save23
	ld $24,save24
	ld $25,save25
	ld $26,save26
	ld $27,save27
	ld $28,save28
	ld $29,save29
	ld $31,save31
	lw $30,saveSR
	mtc0 $30,C0_SR
	ld $30,saveLO
	mtlo $30
	ld $30,saveHI
	mthi $30

	ldc1 $f0,saveFR00
	ldc1 $f1,saveFR01
	ldc1 $f2,saveFR02
	ldc1 $f3,saveFR03
	ldc1 $f4,saveFR04
	ldc1 $f5,saveFR05
	ldc1 $f6,saveFR06
	ldc1 $f7,saveFR07
	ldc1 $f8,saveFR08
	ldc1 $f9,saveFR09
	ldc1 $f10,saveFR10
	ldc1 $f11,saveFR11
	ldc1 $f12,saveFR12
	ldc1 $f13,saveFR13
	ldc1 $f14,saveFR14
	ldc1 $f15,saveFR15
	ldc1 $f16,saveFR16
	ldc1 $f17,saveFR17
	ldc1 $f18,saveFR18
	ldc1 $f19,saveFR19
	ldc1 $f20,saveFR20
	ldc1 $f21,saveFR21
	ldc1 $f22,saveFR22
	ldc1 $f23,saveFR23
	ldc1 $f24,saveFR24
	ldc1 $f25,saveFR25
	ldc1 $f26,saveFR26
	ldc1 $f27,saveFR27
	ldc1 $f28,saveFR28
	ldc1 $f29,saveFR29
	ldc1 $f30,saveFR30
	ldc1 $f31,saveFR31

	ld $30,saveFC31
	nop

	ctc1 $30,$f31

	ld $30,save30
	.set noat
	la $1,save01
	ld $1,($1)

	eret
	nop
	.set at

	.section .data
	.global __baseRegAddr

	.align 8
	__baseRegAddr:
	.lcomm save01, 8
	.lcomm save02, 8
	.lcomm save03, 8
	.lcomm save04, 8
	.lcomm save05, 8
	.lcomm save06, 8
	.lcomm save07, 8
	.lcomm save08, 8
	.lcomm save09, 8
	.lcomm save10, 8
	.lcomm save11, 8
	.lcomm save12, 8
	.lcomm save13, 8
	.lcomm save14, 8
	.lcomm save15, 8
	.lcomm save16, 8
	.lcomm save17, 8
	.lcomm save18, 8
	.lcomm save19, 8
	.lcomm save20, 8
	.lcomm save21, 8
	.lcomm save22, 8
	.lcomm save23, 8
	.lcomm save24, 8
	.lcomm save25, 8
	.lcomm save26, 8
	.lcomm save27, 8
	.lcomm save28, 8
	.lcomm save29, 8
	.lcomm save30, 8
	.lcomm save31, 8
	.lcomm saveSR, 4
	.lcomm saveEPC, 4
	.lcomm saveHI, 8
	.lcomm saveLO, 8
	.lcomm saveFC31, 8
	.lcomm saveFR00, 8
	.lcomm saveFR01, 8
	.lcomm saveFR02, 8
	.lcomm saveFR03, 8
	.lcomm saveFR04, 8
	.lcomm saveFR05, 8
	.lcomm saveFR06, 8
	.lcomm saveFR07, 8
	.lcomm saveFR08, 8
	.lcomm saveFR09, 8
	.lcomm saveFR10, 8
	.lcomm saveFR11, 8
	.lcomm saveFR12, 8
	.lcomm saveFR13, 8
	.lcomm saveFR14, 8
	.lcomm saveFR15, 8
	.lcomm saveFR16, 8
	.lcomm saveFR17, 8
	.lcomm saveFR18, 8
	.lcomm saveFR19, 8
	.lcomm saveFR20, 8
	.lcomm saveFR21, 8
	.lcomm saveFR22, 8
	.lcomm saveFR23, 8
	.lcomm saveFR24, 8
	.lcomm saveFR25, 8
	.lcomm saveFR26, 8
	.lcomm saveFR27, 8
	.lcomm saveFR28, 8
	.lcomm saveFR29, 8
	.lcomm saveFR30, 8
	.lcomm saveFR31, 8
	.lcomm exception_stack, 65*1024

