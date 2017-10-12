	.section .mdebug.abiO64
	.previous
	.section .gcc_compiled_long32
	.previous
	.nan	legacy
	.module	fp=64
	.module	oddspreg
	.text
$Ltext0:
	.cfi_sections	.debug_frame
	.align	2
	.globl	FixedMul
$LFB0 = .
	.file 1 "m_fixed.c"
	.loc 1 46 0
	.cfi_startproc
	.set	nomips16
	.set	nomicromips
	.ent	FixedMul
	.type	FixedMul, @function
FixedMul:
	.frame	$fp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x40000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	.cfi_def_cfa_offset 8
	sd	$fp,0($sp)
	.cfi_offset 30, -8
	move	$fp,$sp
	.cfi_def_cfa_register 30
	sw	$4,8($fp)
	sw	$5,16($fp)
	.loc 1 47 0
	lw	$3,8($fp)
	lw	$2,16($fp)
	dmult	$3,$2
	mflo	$2
	dsra	$2,$2,16
	sll	$2,$2,0
	.loc 1 48 0
	move	$sp,$fp
	.cfi_def_cfa_register 29
	ld	$fp,0($sp)
	addiu	$sp,$sp,8
	.cfi_restore 30
	.cfi_def_cfa_offset 0
	j	$31
	nop

	.set	macro
	.set	reorder
	.end	FixedMul
	.cfi_endproc
$LFE0:
	.size	FixedMul, .-FixedMul
	.align	2
	.globl	FixedDiv
$LFB1 = .
	.loc 1 68 0
	.cfi_startproc
	.set	nomips16
	.set	nomicromips
	.ent	FixedDiv
	.type	FixedDiv, @function
FixedDiv:
	.frame	$fp,16,$31		# vars= 8, regs= 1/0, args= 0, gp= 0
	.mask	0x40000000,-8
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-16
	.cfi_def_cfa_offset 16
	sd	$fp,8($sp)
	.cfi_offset 30, -8
	move	$fp,$sp
	.cfi_def_cfa_register 30
	sw	$4,16($fp)
	sw	$5,24($fp)
	.loc 1 69 0
	lw	$2,16($fp)
	sra	$3,$2,31
	lw	$2,16($fp)
	xor	$2,$3,$2
	subu	$2,$2,$3
	sra	$4,$2,14
	lw	$2,24($fp)
	sra	$3,$2,31
	lw	$2,24($fp)
	xor	$2,$3,$2
	subu	$2,$2,$3
	slt	$2,$4,$2
	bne	$2,$0,$L4
	nop

	.loc 1 71 0
	lw	$3,16($fp)
	lw	$2,24($fp)
	xor	$2,$3,$2
	bgez	$2,$L5
	nop

	.loc 1 71 0 is_stmt 0 discriminator 1
	li	$2,-2147483648			# 0xffffffff80000000
	b	$L7
	nop

$L5:
	.loc 1 71 0 discriminator 2
	li	$2,2147418112			# 0x7fff0000
	ori	$2,$2,0xffff
	b	$L7
	nop

$L4:
$LBB2 = .
	.loc 1 77 0 is_stmt 1
	lw	$2,16($fp)
	dsll	$3,$2,16
	lw	$2,24($fp)
	ddiv	$0,$3,$2
	teq	$2,$0,7
	mfhi	$2
	mflo	$2
	sd	$2,0($fp)
	.loc 1 79 0
	ld	$2,0($fp)
	sll	$2,$2,0
$L7:
$LBE2 = .
	.loc 1 81 0
	move	$sp,$fp
	.cfi_def_cfa_register 29
	ld	$fp,8($sp)
	addiu	$sp,$sp,16
	.cfi_restore 30
	.cfi_def_cfa_offset 0
	j	$31
	nop

	.set	macro
	.set	reorder
	.end	FixedDiv
	.cfi_endproc
$LFE1:
	.size	FixedDiv, .-FixedDiv
$Letext0:
	.file 2 "/home/Jason/n64dev/n64inst/mips64-elf/include/machine/_default_types.h"
	.file 3 "/home/Jason/n64dev/n64inst/mips64-elf/include/stdint.h"
	.file 4 "m_fixed.h"
	.section	.debug_info,"",@progbits
$Ldebug_info0:
	.4byte	0x10f
	.2byte	0x4
	.4byte	$Ldebug_abbrev0
	.byte	0x4
	.uleb128 0x1
	.4byte	$LASF14
	.byte	0xc
	.4byte	$LASF15
	.4byte	$LASF16
	.4byte	$Ltext0
	.4byte	$Letext0-$Ltext0
	.4byte	$Ldebug_line0
	.uleb128 0x2
	.byte	0x1
	.byte	0x6
	.4byte	$LASF0
	.uleb128 0x2
	.byte	0x1
	.byte	0x8
	.4byte	$LASF1
	.uleb128 0x2
	.byte	0x2
	.byte	0x5
	.4byte	$LASF2
	.uleb128 0x2
	.byte	0x2
	.byte	0x7
	.4byte	$LASF3
	.uleb128 0x2
	.byte	0x4
	.byte	0x5
	.4byte	$LASF4
	.uleb128 0x2
	.byte	0x4
	.byte	0x7
	.4byte	$LASF5
	.uleb128 0x3
	.4byte	$LASF9
	.byte	0x2
	.byte	0x59
	.4byte	0x5a
	.uleb128 0x2
	.byte	0x8
	.byte	0x5
	.4byte	$LASF6
	.uleb128 0x2
	.byte	0x8
	.byte	0x7
	.4byte	$LASF7
	.uleb128 0x4
	.byte	0x4
	.byte	0x5
	.ascii	"int\000"
	.uleb128 0x2
	.byte	0x4
	.byte	0x7
	.4byte	$LASF8
	.uleb128 0x3
	.4byte	$LASF10
	.byte	0x3
	.byte	0x38
	.4byte	0x4f
	.uleb128 0x2
	.byte	0x4
	.byte	0x7
	.4byte	$LASF11
	.uleb128 0x2
	.byte	0x1
	.byte	0x6
	.4byte	$LASF12
	.uleb128 0x3
	.4byte	$LASF13
	.byte	0x4
	.byte	0x2a
	.4byte	0x68
	.uleb128 0x5
	.4byte	$LASF17
	.byte	0x1
	.byte	0x2b
	.4byte	0x8f
	.4byte	$LFB0
	.4byte	$LFE0-$LFB0
	.uleb128 0x1
	.byte	0x9c
	.4byte	0xcc
	.uleb128 0x6
	.ascii	"a\000"
	.byte	0x1
	.byte	0x2c
	.4byte	0x8f
	.uleb128 0x2
	.byte	0x91
	.sleb128 0
	.uleb128 0x6
	.ascii	"b\000"
	.byte	0x1
	.byte	0x2d
	.4byte	0x8f
	.uleb128 0x2
	.byte	0x91
	.sleb128 8
	.byte	0
	.uleb128 0x7
	.4byte	$LASF18
	.byte	0x1
	.byte	0x43
	.4byte	0x8f
	.4byte	$LFB1
	.4byte	$LFE1-$LFB1
	.uleb128 0x1
	.byte	0x9c
	.uleb128 0x6
	.ascii	"a\000"
	.byte	0x1
	.byte	0x43
	.4byte	0x8f
	.uleb128 0x2
	.byte	0x91
	.sleb128 0
	.uleb128 0x6
	.ascii	"b\000"
	.byte	0x1
	.byte	0x43
	.4byte	0x8f
	.uleb128 0x2
	.byte	0x91
	.sleb128 8
	.uleb128 0x8
	.4byte	$LBB2
	.4byte	$LBE2-$LBB2
	.uleb128 0x9
	.4byte	$LASF19
	.byte	0x1
	.byte	0x4b
	.4byte	0x76
	.uleb128 0x2
	.byte	0x91
	.sleb128 -16
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_abbrev,"",@progbits
$Ldebug_abbrev0:
	.uleb128 0x1
	.uleb128 0x11
	.byte	0x1
	.uleb128 0x25
	.uleb128 0xe
	.uleb128 0x13
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x1b
	.uleb128 0xe
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x6
	.uleb128 0x10
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x2
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.byte	0
	.byte	0
	.uleb128 0x3
	.uleb128 0x16
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x4
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0x8
	.byte	0
	.byte	0
	.uleb128 0x5
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x6
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x2117
	.uleb128 0x19
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x6
	.uleb128 0x5
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x18
	.byte	0
	.byte	0
	.uleb128 0x7
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x6
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x2117
	.uleb128 0x19
	.byte	0
	.byte	0
	.uleb128 0x8
	.uleb128 0xb
	.byte	0x1
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x6
	.byte	0
	.byte	0
	.uleb128 0x9
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x18
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_aranges,"",@progbits
	.4byte	0x1c
	.2byte	0x2
	.4byte	$Ldebug_info0
	.byte	0x4
	.byte	0
	.2byte	0
	.2byte	0
	.4byte	$Ltext0
	.4byte	$Letext0-$Ltext0
	.4byte	0
	.4byte	0
	.section	.debug_line,"",@progbits
$Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
$LASF6:
	.ascii	"long long int\000"
$LASF8:
	.ascii	"unsigned int\000"
$LASF15:
	.ascii	"m_fixed.c\000"
$LASF16:
	.ascii	"/home/jason/n64dev/doom\000"
$LASF13:
	.ascii	"fixed_t\000"
$LASF5:
	.ascii	"long unsigned int\000"
$LASF7:
	.ascii	"long long unsigned int\000"
$LASF12:
	.ascii	"char\000"
$LASF1:
	.ascii	"unsigned char\000"
$LASF18:
	.ascii	"FixedDiv\000"
$LASF19:
	.ascii	"result\000"
$LASF4:
	.ascii	"long int\000"
$LASF9:
	.ascii	"__int64_t\000"
$LASF3:
	.ascii	"short unsigned int\000"
$LASF0:
	.ascii	"signed char\000"
$LASF10:
	.ascii	"int64_t\000"
$LASF17:
	.ascii	"FixedMul\000"
$LASF2:
	.ascii	"short int\000"
$LASF14:
	.ascii	"GNU C99 5.1.0 -G 4 -march=vr4300 -mtune=vr4300 -mips3 -g"
	.ascii	" -O0 -std=gnu99 -fno-strict-aliasing\000"
$LASF11:
	.ascii	"sizetype\000"
	.ident	"GCC: (GNU) 5.1.0"
