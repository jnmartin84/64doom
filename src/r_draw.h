// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	System specific interface stuff.
//
//-----------------------------------------------------------------------------


#ifndef __R_DRAW__
#define __R_DRAW__


#ifdef __GNUG__
#pragma interface
#endif


extern lighttable_t*	dc_colormap;
extern int		dc_x;
extern int		dc_yl;
extern int		dc_yh;
extern fixed_t		dc_iscale;
extern fixed_t		dc_texturemid;

// first pixel in a column
extern byte*		dc_source;		


// The span blitting interface.
// Hook in assembler or system specific BLT
//  here.
void 	R_DrawColumn(void);

void 	R_DrawColumn_TrueColor_C(void);

//_TrueColor (void);
void 	R_DrawColumnLow(void);
//_TrueColor (void);

// The Spectre/Invisibility effect.
void 	R_DrawFuzzColumn(void);
//_TrueColor (void);
void 	R_DrawFuzzColumnLow(void);
//_TrueColor (void);

// Draw with color translation tables,
//  for player sprite rendering,
//  Green/Red/Blue/Indigo shirts.
void	R_DrawTranslatedColumn(void);
//_TrueColor (void);
void	R_DrawTranslatedColumnLow(void);
//_TrueColor (void);

void
R_VideoErase
( unsigned	ofs,
  int		count );

extern int		ds_y;
extern int		ds_x1;
extern int		ds_x2;

extern lighttable_t*	ds_colormap;

extern fixed_t		ds_xfrac;
extern fixed_t		ds_yfrac;
extern fixed_t		ds_xstep;
extern fixed_t		ds_ystep;

// start of a 64*64 tile image
extern byte*		ds_source;		

extern byte*		translationtables;
extern byte*		dc_translation;


// Span blitting for rows, floor/ceiling.
// No Sepctre effect needed.
void 	R_DrawSpan(void);
//_TrueColor (void);

// Low resolution mode, 160x200?
void 	R_DrawSpanLow(void);
//_TrueColor (void);


void
R_InitBuffer
( int		width,
  int		height );


// Initialize color translation tables,
//  for player rendering etc.
void	R_InitTranslationTables (void);



// Rendering function.
void R_FillBackScreen (void);

// If the view size is not full screen, draws a border around it.
void R_DrawViewBorder (void);
#if 0
static /*__attribute__((always_inline)) inline*/ void R_DrawColumn_TrueColor(void)
{
asm volatile(
"	.set	noreorder\n\t"
"	.set	nomacro\n\t"
"	lw	$t0,	%%gp_rel(dc_yh)($gp)\n\t"
"	lw	$t1,	%%gp_rel(dc_yl)($gp)\n\t"
"	lui	$t2,	%%hi(ylookup)\n\t"
"	subu	$t3,	$t0,	$t1\n\t"
"	bltz	$t3,	_R_DrawColumn_return%=\n\t"
"	lui	$v1,	%%hi(palarray)\n\t"
"	addiu	$v1,	$v1,	%%lo(palarray)\n\t"
"	addiu	$t2,	$t2,	%%lo(ylookup)\n\t"
"	addu	$t4,	$t1,	$zero\n\t"
"	sll	$t4,	$t4,	2\n\t"
"	addu	$t4,	$t2,	$t4\n\t"
"	lw	$t5,	0($t4)\n\t"
"	lw	$t6,	%%gp_rel(dc_x)($gp)\n\t"
"	lui	$t2,	%%hi(columnofs)\n\t"
"	addu	$t4,	$t6,	$zero\n\t"
"	sll	$t4,	$t4,	2\n\t"
"	addiu	$t2,	$t2,	%%lo(columnofs)\n\t"
"	addu	$t4,	$t2,	$t4\n\t"
"	lw	$t7,	0($t4)\n\t"
"	lui	$t4,	%%hi(buf16)\n\t"
"	addiu	$t4,	$t4,	%%lo(buf16)\n\t"
"	lw	$t4,	0($t4)\n\t"
"	lw	$t0,	%%gp_rel(dc_iscale)($gp)\n\t"
"	addu	$t4,	$t4,	$t5\n\t"
"	lw	$t6,	%%gp_rel(centery)($gp)\n\t"
"	addu	$t4,	$t4,	$t5\n\t"
"	subu	$t1,	$t1,	$t6\n\t"
"	addu	$t4,	$t4,	$t7\n\t"
"	multu	$t1,	$t0\n\t"
"	addu	$t4,	$t4,	$t7\n\t"
"	mflo	$t1\n\t"
"	addiu	$t4,	$t4, 0x6400\n\t"
"	lw	$t6,	%%gp_rel(dc_texturemid)($gp)\n\t"
"	addiu	$t4,	$t4, 0x5F00\n\t"
"	lw	$t8,	%%gp_rel(dc_source)($gp)\n\t"
"	lw	$t9,	%%gp_rel(dc_colormap)($gp)\n\t"
"	addu	$t6,	$t6,	$t1\n\t"
"   nop\n\t"
"_R_DrawColumn_dowhile%=:\n\t"
"	bltz   $t3, _R_DrawColumn_return%=\n\t"
"	sra	$t1,	$t6,	16\n\t"
"	andi	$t1,	$t1,	127\n\t"
"	addu	$t1,	$t8,	$t1\n\t"
"	lbu	$t5,	0($t1)\n\t"
"	addi	$t3,	$t3,	-1\n\t"
"	addu	$a0,	$t9,	$t5\n\t"
"	lbu	$t1,	0($a0)\n\t"
"	addiu	$t4,	$t4,	1280\n\t"
"	sll		$t1,	$t1,	2\n\t"
"	addu	$t1,	$t1,	$v1\n\t"
"	lw	$t2,	0($t1)\n\t"
"	addu	$t6,	$t6,	$t0\n\t"
"   j _R_DrawColumn_dowhile%=\n\t"
"	sh	$t2,	0($t4)\n\t"
"_R_DrawColumn_return%=:\n\t"
"	.set	macro\n\t"
"	.set	reorder"
:
:
: "v1","a0","t9","t8","t7","t6","t5","t4","t3","t2","t1","t0"
);

}

static __attribute__((always_inline)) inline void R_DrawColumnLow_TrueColor(void)
{
asm volatile(
".set noreorder\n\t"
".set nomacro\n\t"
"		lw	$t0,	%%gp_rel(dc_yh)($gp)\n\t"
"	lw	$t1,	%%gp_rel(dc_yl)($gp)\n\t"
"	lui	$t2,	%%hi(ylookup2)\n\t"
"	subu	$t3,	$t0,	$t1\n\t"
"	bltz	$t3,	_R_DrawColumnLow_return%=\n\t"
"	addiu	$t2,	$t2,	%%lo(ylookup2)\n\t"
"	lui	$v1,	%%hi(palarray)\n\t"
"	addiu	$v1,	$v1,	%%lo(palarray)\n\t"
"	addu	$t4,	$t1,	$zero\n\t"
"	sll	$t4,	$t4,	2\n\t"
"	addu	$t4,	$t2,	$t4\n\t"
"	lw	$t5,	0($t4)\n\t"
"	lw	$t7,	%%gp_rel(dc_x)($gp)\n\t"
"	lui	$t4,	%%hi(buf16)\n\t"
"	sll $t7, $t7, 1\n\t"
"	addiu	$t4,	$t4,	%%lo(buf16)\n\t"
"	lw	$t4,	0($t4)\n\t"
"	lw	$t0,	%%gp_rel(dc_iscale)($gp)\n\t"
"	lw	$t6,	%%gp_rel(centery)($gp)\n\t"
"	addu	$t4,	$t4,	$t5\n\t"
"	addu	$t4,	$t4,	$t5\n\t"
"	addu	$t4,	$t4,	$t7\n\t"
"	addu	$t4,	$t4,	$t7\n\t"
"	addiu	$t4,	$t4, 0x6400\n\t"
"	addiu	$t4,	$t4, 0x5F00\n\t"
"	subu	$t1,	$t1,	$t6\n\t"
"	multu	$t1,	$t0\n\t"
"	sra		$t4, $t4, 2\n\t"
"	sll		$t4, $t4, 2\n\t"
"	mflo	$t1\n\t"
"	lw	$t6,	%%gp_rel(dc_texturemid)($gp)\n\t"
"	lw	$t8,	%%gp_rel(dc_source)($gp)\n\t"
"	lw	$t9,	%%gp_rel(dc_colormap)($gp)\n\t"
"	addu	$t6,	$t6,	$t1\n\t"
"_R_DrawColumnLow_dowhile%=:\n\t"
"	bltz   $t3, _R_DrawColumnLow_return%=\n\t"
"	sra	$t1,	$t6,	16\n\t"
"	addu	$t6,	$t6,	$t0\n\t"
"	andi	$t1,	$t1,	127\n\t"
"	addu	$t1,	$t8,	$t1\n\t"
"	lbu	$t5,	0($t1)\n\t"
"	addi	$t3,	$t3,	-1\n\t"
"	addu	$a0,	$t9,	$t5\n\t"
"	lbu	$t1,	0($a0)\n\t"
"	addiu	$t4,	$t4,	1280\n\t"
"	sll	$t1,	$t1,	2\n\t"
"	addu	$t1,	$t1,	$v1\n\t"
"	lw	$t2,	0($t1)\n\t"
"	j _R_DrawColumnLow_dowhile%=\n\t"
"	sw	$t2,	0($t4)\n\t"
"_R_DrawColumnLow_return%=:\n\t"
"	.set	macro\n\t"
"	.set	reorder"
:
:
: "v1","a0","t9","t8","t7","t6","t5","t4","t3","t2","t1","t0"
);	
}

static /*__attribute__((always_inline)) inline*/ void R_DrawSpan_TrueColor(void)
{
	asm volatile(
".set noreorder\n\t"
".set nomacro\n\t"
"	lw	$t0,	%%gp_rel(ds_x1)($gp)\n\t"
"	lw	$t1,	%%gp_rel(ds_x2)($gp)\n\t"
"	lw	$t2,	%%gp_rel(ds_y)($gp)\n\t"
"	subu	$t3,	$t1,	$t0\n\t"
"	lui	$v1,	%%hi(palarray)\n\t"
"	lui	$t4,	%%hi(ylookup)\n\t"
"	sll	$t2,	$t2,	2\n\t"
"	addiu	$v1,	$v1,	%%lo(palarray)\n\t"
"	lw	$t7,	%%gp_rel(ds_colormap)($gp)\n\t"
"	addiu	$t4,	$t4,	%%lo(ylookup)\n\t"
"	lui	$t9,	%%hi(buf16)\n\t"
"	addu	$t2,	$t4,	$t2\n\t"
"	lw	$t5,	0($t2)\n\t"
"	lw	$t2,	%%gp_rel(ds_x1)($gp)\n\t"
"	lui	$t4,	%%hi(columnofs)\n\t"
"	sll	$t2,	$t2,	2\n\t"
"	addiu	$t4,	$t4,	%%lo(columnofs)\n\t"
"	addiu	$t9,	$t9,	%%lo(buf16)\n\t"
"	addu	$t2,	$t4,	$t2\n\t"
"	lw	$t9, 0($t9)\n\t"
"	lw	$t6,	0($t2)\n\t"
"	lw	$t8,	%%gp_rel(ds_source)($gp)\n\t"
"	lw	$t1,	%%gp_rel(ds_ystep)($gp)\n\t"
"	lw	$t0,	%%gp_rel(ds_xstep)($gp)\n\t"
"	addu	$t9,	$t9,	$t5\n\t"
"	addu	$t9,	$t9,	$t5\n\t"
"	addiu	$t9,	$t9, 0x6400\n\t"
"	addiu	$t9,	$t9, 0x63FE\n\t"
"	addu	$t9, $t9, $t6\n\t"
"	addu	$t9, $t9, $t6\n\t"
"	lw	$t6,	%%gp_rel(ds_yfrac)($gp)\n\t"
"	lw	$t5,	%%gp_rel(ds_xfrac)($gp)\n\t"
"   bne $t1, $zero, _R_DrawSpan_dowhile\n\t"
"	nop\n\t"
"	sra	$t6,	$t6,	10\n\t"
"	andi $t6, $t6, 0xfc0\n\t"
"_R_DrawSpan_NoY_dowhile:\n\t"
"    bltz $t3, _R_DrawSpan_return\n\t"
"	sra	$a1,	$t5,	16\n\t"
"	andi	$a1,	$a1,	0x3f\n\t"
"	addu	$a2,	$t6,	$a1\n\t"
"	addu	$t5,	$t5,	$t0\n\t"
"	addu	$a3,	$t8,	$a2\n\t"
"	lbu		$v0,	0($a3)\n\t"
"	addiu	$t9,	$t9,	2\n\t"
"	addu	$a0,	$t7,	$v0\n\t"
"	lbu		$a3,	0($a0)\n\t"
"	addi	$t3,	$t3,	-1 \n\t"
"	sll		$a3,	$a3,	2\n\t"
"	addu	$a3, $a3,	$v1\n\t"
"	lw	$t2, 0($a3)\n\t"
"	j	_R_DrawSpan_NoY_dowhile\n\t"
"	sh	$t2, 0($t9)\n\t"
"_R_DrawSpan_dowhile:\n\t"
"	bltz $t3, _R_DrawSpan_return\n\t"
"	sra	$a0,	$t6,	10\n\t"
"	sra	$a1,	$t5,	16\n\t"
"	andi	$a0,	$a0,	0xfc0\n\t"
"	andi	$a1,	$a1,	0x3f\n\t"
"	addu	$a2,	$a0,	$a1\n\t"
"	addu	$t5,	$t5,	$t0\n\t"
"	addu	$a3,	$t8,	$a2\n\t"
"	lbu		$v0,	0($a3)\n\t"
"	addiu	$t9,	$t9,	2\n\t"
"	addu	$a0,	$t7,	$v0\n\t"
"	lbu		$a3,	0($a0)\n\t"
"	addu	$t6,	$t6,	$t1\n\t"
"	addi	$t3,	$t3,	-1 \n\t"
"	sll		$a3,	$a3,	2\n\t"
"	addu	$a3, $a3,	$v1\n\t"
"	lw	$t2, 0($a3)\n\t"
"	j	_R_DrawSpan_dowhile\n\t"
"	sh	$t2, 0($t9)\n\t"
"    nop\n\t"
"    nop\n\t"
"    nop\n\t"
"    nop\n\t"
"    nop\n\t"
"_R_DrawSpan_return:\n\t"
"	.set	macro\n\t"
"	.set	reorder"
:
:
: "v1","v0","a3","a2","a1","a0","t9","t8","t7","t6","t5","t4","t3","t2","t1","t0"
);
}
static __attribute__((always_inline)) inline void R_DrawSpan_TrueColor_fancy(void)
{
	asm volatile(
".set noreorder\n\t"
".set nomacro\n\t"
	"lw	$t0,	%%gp_rel(ds_x1)($gp)\n\t"
	"lw	$t1,	%%gp_rel(ds_x2)($gp)\n\t"
	"lw	$t2,	%%gp_rel(ds_y)($gp)\n\t"
	"lui	$v1,	%%hi(palarray)\n\t"
	"subu	$t3,	$t1,	$t0\n\t"
	"bltz	$t3,	_R_DrawSpan_return\n\t"
	"lui	$t4,	%%hi(ylookup)\n\t"
	"sll	$t2,	$t2,	2\n\t"
	"addiu	$v1,	$v1,	%%lo(palarray)\n\t"
	"lw	$t7,	%%gp_rel(ds_colormap)($gp)\n\t"
	"addiu	$t4,	$t4,	%%lo(ylookup)\n\t"
	"lui	$t9,	%%hi(buf16)\n\t"
	"addu	$t2,	$t4,	$t2\n\t"
	"lw	$t5,	0($t2)\n\t"
	"lw	$t2,	%%gp_rel(ds_x1)($gp)\n\t"
	"lui	$t4,	%%hi(columnofs)\n\t"
	"sll	$t2,	$t2,	2\n\t"
	"addiu	$t4,	$t4,	%%lo(columnofs)\n\t"
	"addiu	$t9,	$t9,	%%lo(buf16)\n\t"
	"addu	$t2,	$t4,	$t2\n\t"
	"lw	$t9, 0($t9)\n\t"
	"lw	$t6,	0($t2)\n\t"
	"lw	$t8,	%%gp_rel(ds_source)($gp)\n\t"
	"lw	$t1,	%%gp_rel(ds_ystep)($gp)\n\t"
	"lw	$t0,	%%gp_rel(ds_xstep)($gp)\n\t"
	"addu	$t9,	$t9,	$t5\n\t"
	"addu	$t9,	$t9,	$t5\n\t"
	"addiu	$t9,	$t9, 0x6400\n\t"
	"addiu	$t9,	$t9, 0x6400\n\t"
	"addu	$t9, $t9, $t6\n\t"
	"addu	$t9, $t9, $t6\n\t"
	"lw	$t5,	%%gp_rel(ds_xfrac)($gp)\n\t"
	"lw	$t6,	%%gp_rel(ds_yfrac)($gp)\n\t"
	"andi	$at, $t9, 3\n\t"
	"andi	$t4, $t3, 1\n\t"
"beqz	$t3,	singled_after\n\t"
"sra	$t3, $t3, 1\n\t"
"beqz	$at, doubled_middle\n\t"
"nop\n\t"
"singled_before:\n\t"
"	sra	$a0,	$t6,	10\n\t"
"	sra	$a1,	$t5,	16\n\t"
"	andi	$a0,	$a0,	0xfc0\n\t"
"	andi	$a1,	$a1,	0x3f\n\t"
"	addu	$t6,	$t6,	$t1\n\t"
"	addu	$a2,	$a0,	$a1\n\t"
"	addu	$t5,	$t5,	$t0\n\t"
"	addu	$a3,	$t8,	$a2\n\t"
"	lbu		$v0,	0($a3)\n\t"
"	nop\n\t"
"	addu	$a0,	$t7,	$v0\n\t"
"	lbu		$a3,	0($a0)\n\t"
"	addiu	$t9,	$t9,	2\n\t"
"	sll		$a3,	$a3,	2\n\t"
"	addu	$a3, $a3,	$v1\n\t"
"	lw	$t2, 0($a3)\n\t"
"	sh	$t2, -2($t9)\n\t"
"	beqz	$t4,	even_at_test\n\t"
"	nop\n\t"
"	b		doubled_middle\n\t"
"	nop\n\t"
"even_at_test:\n\t"
"	beqz	$at,	doubled_middle\n\t"
"	nop\n\t"
"	subu	$t3,	$t3, 1\n\t"
"doubled_middle:\n\t"
"	addu	$t4, $t4, $at\n\t"
"doubled_middle_LOOP:\n\t"
"    bltz	$t3,	dmdone\n\t"
"	sra	$a0,	$t6,	10\n\t"
"	sra	$a1,	$t5,	16\n\t"
"	andi	$a0,	$a0,	0xfc0\n\t"
"	andi	$a1,	$a1,	0x3f\n\t"
"	addu	$t6,	$t6,	$t1\n\t"
"	addu	$a2,	$a0,	$a1\n\t"
"	addu	$t5,	$t5,	$t0\n\t"
"	addu	$a3,	$t8,	$a2\n\t"
"	lbu		$v0,	0($a3)\n\t"
"	nop\n\t"
"	addu	$a0,	$t7,	$v0\n\t"
"	lbu		$a3,	0($a0)\n\t"
"	nop\n\t"
"	sll		$a3,	$a3,	2\n\t"
"	addu	$a3, $a3,	$v1\n\t"
"	lw	$t2, 0($a3)\n\t"
"	sra	$a0,	$t6,	10\n\t"
"	sra	$a1,	$t5,	16\n\t"
"	andi	$a0,	$a0,	0xfc0\n\t"
"	andi	$a1,	$a1,	0x3f\n\t"
"	addu	$t6,	$t6,	$t1\n\t"
"	addu	$a2,	$a0,	$a1\n\t"
"	addu	$t5,	$t5,	$t0\n\t"
"	addu	$a3,	$t8,	$a2\n\t"
"	lbu		$v0,	0($a3)\n\t"
"	nop\n\t"
"	addu	$a0,	$t7,	$v0\n\t"
"	lbu		$a3,	0($a0)\n\t"
"	nop\n\t"
"	sll		$a3,	$a3,	2\n\t"
"	addu	$a3, $a3,	$v1\n\t"
"	lw	$at, 0($a3)\n\t"
"	andi	$at,	$at, 0xFFFF\n\t"
"	sll		$t2,	$t2,	16\n\t"
"	or		$t2, $t2, $at\n\t"
"	sw	$t2, 0($t9)\n\t"
"	addi	$t3,	$t3,	-1\n\t"
"	j doubled_middle_LOOP\n\t"
"	addiu	$t9,	$t9,	4\n\t"
"dmdone:\n\t"
"	beqz	$t4,	_R_DrawSpan_return\n\t"
"	nop\n\t"
"	addi	$t4,	$t4, -2\n\t"
"	beqz	$t4, _R_DrawSpan_return\n\t"
"	nop\n\t"
"singled_after:\n\t"
"	sra	$a0,	$t6,	10\n\t"
"	sra	$a1,	$t5,	16\n\t"
"	andi	$a0,	$a0,	0xfc0\n\t"
"	andi	$a1,	$a1,	0x3f\n\t"
"	addu	$t6,	$t6,	$t1\n\t"
"	addu	$a2,	$a0,	$a1\n\t"
"	addu	$t5,	$t5,	$t0\n\t"
"	addu	$a3,	$t8,	$a2\n\t"
"	lbu		$v0,	0($a3)\n\t"
"	nop\n\t"
"	addu	$a0,	$t7,	$v0\n\t"
"	lbu		$a3,	0($a0)\n\t"
"	nop\n\t"
"	sll		$a3,	$a3,	2\n\t"
"	addu	$a3, $a3,	$v1\n\t"
"	lw	$t2, 0($a3)\n\t"
"	sh	$t2, 0($t9)\n\t"
"	addiu	$t9,	$t9,	2\n\t"
"_R_DrawSpan_return:\n\t"
"	.set	macro\n\t"
"	.set	reorder"
:
:
: "v1","v0","a3","a2","a1","a0","t9","t8","t7","t6","t5","t4","t3","t2","t1","t0","at"
);	
}

static __attribute__((always_inline)) inline void R_DrawSpanLow_TrueColor(void)
{
	asm volatile(
	".set noreorder\n\t"
	".set nomacro\n\t"
	"lw	$t0,	%%gp_rel(ds_x1)($gp)\n\t"
	"lw	$t1,	%%gp_rel(ds_x2)($gp)\n\t"
	"lw	$t2,	%%gp_rel(ds_y)($gp)\n\t"
	"subu	$t3,	$t1,	$t0\n\t"
	"bltz	$t3,	_R_DrawSpanLow_return\n\t"
	"sll	$t2,	$t2,	2\n\t"
	"lui	$v1,	%%hi(palarray)\n\t"
	"lui	$t4,	%%hi(ylookup2)\n\t"
	"addiu	$v1,	$v1,	%%lo(palarray)\n\t"
	"addiu	$t4,	$t4,	%%lo(ylookup2)\n\t"
	"addu	$t2,	$t4,	$t2\n\t"
	"lw	$t5,	0($t2)\n\t"
	"lw	$t2,	%%gp_rel(ds_x1)($gp)\n\t"
	"lui	$t9,	%%hi(buf16)\n\t"
	"sll $t2, $t2, 1\n\t"
	"addiu	$t9,	$t9,	%%lo(buf16)\n\t"
	"lw 		$t9, 0($t9)\n\t"
	"lw	$t7,	%%gp_rel(ds_colormap)($gp)\n\t"
	"addu	$t9,	$t9,	$t5\n\t"
	"addu	$t9,	$t9,	$t5\n\t"
	"addiu	$t9,	$t9, 0x6400\n\t"
	"addiu	$t9,	$t9, 0x6400\n\t"
	"addu	$t9, $t9, $t2\n\t"
	"addu	$t9, $t9, $t2\n\t"
	"sra		$t9, $t9, 2\n\t"
	"sll		$t9, $t9, 2\n\t"
	"lw	$t8,	%%gp_rel(ds_source)($gp)\n\t"
	"lw	$t1,	%%gp_rel(ds_ystep)($gp)\n\t"
	"lw	$t0,	%%gp_rel(ds_xstep)($gp)\n\t"
	"lw	$t5,	%%gp_rel(ds_xfrac)($gp)\n\t"
	"lw	$t6,	%%gp_rel(ds_yfrac)($gp)\n\t"
    "nop\n\t"
"_R_DrawSpanLow_dowhile:\n\t"
"	bltz $t3, _R_DrawSpanLow_return\n\t"
"	sra	$a0,	$t6,	10\n\t"
"	sra	$a1,	$t5,	16\n\t"
"	andi	$a0,	$a0,	0xfc0\n\t"
"	andi	$a1,	$a1,	0x3f\n\t"
"	addu	$t6,	$t6,	$t1\n\t"
"	addu	$a2,	$a0,	$a1\n\t"
"	addu	$t5,	$t5,	$t0\n\t"
"	addu	$a3,	$t8,	$a2\n\t"
"	lbu		$v0,	0($a3)\n\t"
"	addi	$t3,	$t3,	-1\n\t"
"	addu	$a0,	$t7,	$v0\n\t"
"	lbu		$a3,	0($a0)\n\t"
"	addiu	$t9,	$t9,	4\n\t"
"	sll		$a3,	$a3,	2\n\t"
"	addu	$a3, $a3,	$v1\n\t"
"	lw	$t2, 0($a3)\n\t"
"	j	_R_DrawSpanLow_dowhile\n\t"
"	sw	$t2, 0($t9)\n\t"
"_R_DrawSpanLow_return:\n\t"
".set macro\n\t"
".set reorder"
:
:
: "v1","v0","a3","a2","a1","a0","t9","t8","t7","t6","t5","t4","t3","t2","t1","t0"
);
}
#endif
#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
