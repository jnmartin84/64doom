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
// $Log:$
//
// DESCRIPTION:
//	The actual span/column drawing functions.
//	Here find the main potential for optimization,
//	 e.g. inline assembly, different algorithms.
//
//-----------------------------------------------------------------------------

#include "doomdef.h"

#include "i_system.h"
#include "z_zone.h"
#include "w_wad.h"

#include "r_local.h"

// Needs access to LFB (guess what).
#include "v_video.h"

// State.
#include "doomstat.h"

extern uint32_t ytab[];

#define n64_cfb_set_pixel( _dc, x, y, color ) \
	*(uint16_t *)(__safe_buffer[(_dc) - 1] + (( (x)+(ytab[y]) )<<1)) = (color)

#define n64_cfb_get_pixel( buffer, x, y ) \
	*(uint16_t *)(__safe_buffer[(_dc) - 1] + (( (x)+(ytab[y]) )<<1))

extern void I_SetPalette(byte* palette);
	
extern void *__n64_memcpy_ASM(void *d, const void *s, size_t n);
// special case, always fills with zero
extern void *__n64_memset_ZERO_ASM(void *ptr, int value, size_t num);
// the non-special case version that accepts arbitrary fill value
extern void *__n64_memset_ASM(void *ptr, int value, size_t num);

extern uint32_t palarray[256];
extern void *__safe_buffer[];
extern display_context_t _dc;
// stores W_CacheLumpName ("PLAYPAL",PU_CACHE);
byte *p;
uint32_t oldpal[256];

uint16_t *buf16;
// ?
#define MAXWIDTH			1120
#define MAXHEIGHT			832

// status bar height at bottom of screen
#define SBARHEIGHT		32

//
// All drawing to the view buffer is accomplished in this file.
// The other refresh files only know about ccordinates,
//  not the architecture of the frame buffer.
// Conveniently, the frame buffer is a linear one,
//  and we need only the base address,
//  and the total size == width*height*depth/8.,
//

byte*		viewimage;
int		viewwidth;
int		scaledviewwidth;
int		viewheight;
int		viewwindowx;
int		viewwindowy;

int		ylookup[MAXHEIGHT];
int		ylookup2[MAXHEIGHT];

int		columnofs[MAXWIDTH];

// Color tables for different players,
//  translate a limited part to another
//  (color ramps used for  suit colors).
//
byte		translations[3][256];

//
// R_DrawColumn
// Source is the top of the column to scale.
//
lighttable_t*		dc_colormap;
int			x;
int			dc_x;
int			dc_yl;
int			dc_yh;
fixed_t			dc_iscale;
fixed_t			dc_texturemid;

// first pixel in a column (possibly virtual)
byte*			dc_source;

// just for profiling
int			dccount;

//
// Spectre/Invisibility.
//
#define FUZZTABLE		50
#define FUZZOFF	(SCREENWIDTH)

int	fuzzoffset[FUZZTABLE] =
{
    FUZZOFF,-FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
    FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
    FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,
    FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
    FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,
    FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,
    FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF 
};

int	fuzzpos = 0;

//
// Framebuffer postprocessing.
// Creates a fuzzy image by copying pixels
//  from adjacent ones to left and right.
// Used with an all black colormap, this
//  could create the SHADOW effect,
//  i.e. spectres and invisible players.
//
void R_DrawFuzzColumn_TrueColor (void)
{
    int			count;
//    fixed_t		frac;
//    fixed_t		fracstep;
	uint16_t	*dest;

    // Adjust borders. Low...
    if (!dc_yl)
    {
	dc_yl = 1;
    }

    // .. and high.
    if (dc_yh == viewheight-1)
    {
	dc_yh = viewheight - 2;
    }
	
    count = dc_yh - dc_yl;

    // Zero length.
    if (count < 0)
    {
		return;
    }

	dest = (&((uint16_t *)__safe_buffer[(_dc)-1])[ylookup[dc_yl]+(40*SCREENWIDTH) + columnofs[dc_x]]);

    // Looks familiar.
//    fracstep = dc_iscale;
//    frac = dc_texturemid + (dc_yl-centery)*fracstep;

    // Looks like an attempt at dithering,
    //  using the colormap #6 (of 0-31, a bit
    //  brighter than average).
    do
    {
		// Lookup framebuffer, and retrieve
		//  a pixel that is either one column
		//  left or right of the current one.
		// Add index from colormap to index.
		*dest = dest[fuzzoffset[fuzzpos]];
		
		// Clamp table lookup index.
		if (++fuzzpos == FUZZTABLE)
		{
			fuzzpos = 0;
		}
		dest += SCREENWIDTH;
//		frac += fracstep;
    }
    while (count--);
}

//
// Framebuffer postprocessing.
// Creates a fuzzy image by copying pixels
//  from adjacent ones to left and right.
// Used with an all black colormap, this
//  could create the SHADOW effect,
//  i.e. spectres and invisible players.
//
void R_DrawFuzzColumnLow_TrueColor (void)
{
    int			count;
//    fixed_t		frac;
//    fixed_t		fracstep;
	uint32_t	*dest32;
	
    // Adjust borders. Low...
    if (!dc_yl)
    {
	dc_yl = 1;
    }

    // .. and high.
    if (dc_yh == viewheight-1)
    {
	dc_yh = viewheight - 2;
    }
	
    count = dc_yh - dc_yl;
    // Zero length.
    if (count < 0)
    {
		return;
    }

    x = dc_x << 1;
	dest32 = (uint32_t *)(__safe_buffer[_dc - 1] + (( (x)+(ylookup2[dc_yl]+(40*SCREENWIDTH)) )<<1));
	
    // Looks familiar.
//    fracstep = dc_iscale;
//    frac = dc_texturemid + (dc_yl-centery)*fracstep;

    // Looks like an attempt at dithering,
    //  using the colormap #6 (of 0-31, a bit
    //  brighter than average).
    do
    {
		// Lookup framebuffer, and retrieve
		//  a pixel that is either one column
		//  left or right of the current one.
		// Add index from colormap to index.
		*dest32 = dest32[fuzzoffset[fuzzpos] >> 1];
		
		// Clamp table lookup index.
		if (++fuzzpos == FUZZTABLE)
		{
			fuzzpos = 0;
		}

		dest32 += SCREENWIDTH>>1;
//		frac += fracstep;
    }
    while (count--);
}


//
// R_DrawTranslatedColumn
// Used to draw player sprites
//  with the green colorramp mapped to others.
// Could be used with different translation
//  tables, e.g. the lighter colored version
//  of the BaronOfHell, the HellKnight, uses
//  identical sprites, kinda brightened up.
//
byte*	dc_translation;
byte*	translationtables;

void R_DrawTranslatedColumn_TrueColor (void)
{
    int			count;
    fixed_t		frac;
    fixed_t		fracstep;
	uint16_t	*dest;
	
    count = dc_yh - dc_yl;

    if (count < 0)
    {
		return;
    }
//	int not_count = count;

#ifdef RANGECHECK
    if ((unsigned)dc_x >= SCREENWIDTH
	|| dc_yl < 0
	|| dc_yh >= SCREENHEIGHT)
    {
        char ermac[256];
        sprintf(ermac, "R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
        I_Error(ermac);
/*	I_Error ( "R_DrawColumn: %i to %i at %i",
		  dc_yl, dc_yh, dc_x);*/
    }
#endif

    // Framebuffer destination address.
    // Use ylookup LUT to avoid multiply with ScreenWidth.
    // Use columnofs LUT for subwindows?
	dest = (&((uint16_t *)__safe_buffer[(_dc)-1])[ylookup[dc_yl]+(40*SCREENWIDTH) + columnofs[dc_x]]);

    // Looks familiar.
    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl-centery)*fracstep;

    // Here we do an additional index re-mapping.
    do
    {
	// Translation tables are used
	//  to map certain colorramps to other ones,
	//  used with PLAY sprites.
	// Thus the "green" ramp of the player 0 sprite
	//  is mapped to gray, red, black/indigo.
	//*dest = palarray[dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]]];
	//n64_cfb_set_pixel(_dc, dc_x, dc_yl + (not_count - count), palarray[dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]]]);
	*dest = palarray[dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]]];
	dest += SCREENWIDTH;
	frac += fracstep;
    }
    while (count--);
}

//
// R_DrawTranslatedColumnLow
// Used to draw player sprites
//  with the green colorramp mapped to others.
// Could be used with different translation
//  tables, e.g. the lighter colored version
//  of the BaronOfHell, the HellKnight, uses
//  identical sprites, kinda brightened up.
//
//byte*	dc_translation;
//byte*	translationtables;

void R_DrawTranslatedColumnLow_TrueColor (void)
{
    int			count;
    fixed_t		frac;
    fixed_t		fracstep;
    uint32_t    *dest32;
    count = dc_yh - dc_yl;

    if (count < 0)
    {
	return;
    }

    x = dc_x << 1;
    dest32 = (uint32_t *)(__safe_buffer[_dc - 1] + (( (x)+(ylookup2[dc_yl]+(40*SCREENWIDTH)) )<<1));
    // Looks familiar.
    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl-centery)*fracstep;

    // Here we do an additional index re-mapping.
    do
    {
	// Translation tables are used
	//  to map certain colorramps to other ones,
	//  used with PLAY sprites.
	// Thus the "green" ramp of the player 0 sprite
	//  is mapped to gray, red, black/indigo.
	*dest32 = palarray[dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]]];
	dest32 += SCREENWIDTH>>1;
	frac += fracstep;
    }
    while (count--);
}


//
// R_InitTranslationTables
// Creates the translation tables to map
//  the green color ramp to gray, brown, red.
// Assumes a given structure of the PLAYPAL.
// Could be read from a lump instead.
//
void R_InitTranslationTables (void)
{
    int		i;

    translationtables = Z_Malloc (256*3+255, PU_STATIC, 0);
    translationtables = (byte *)(( (int)translationtables + 255 )& ~255);

    // translate just the 16 green colors
    for (i=0 ; i<256 ; i++)
    {
	if (i >= 0x70 && i<= 0x7f)
	{
	    // map green ramp to gray, brown, red
	    translationtables[i    ] = 0x60 + (i&0xf);
	    translationtables[i+256] = 0x40 + (i&0xf);
	    translationtables[i+512] = 0x20 + (i&0xf);
	}
	else
	{
	    // Keep all other colors as is.
	    translationtables[i    ] =
            translationtables[i+256] =
            translationtables[i+512] = i;
	}
    }
}


//
// R_DrawSpan
// With DOOM style restrictions on view orientation,
//  the floors and ceilings consist of horizontal slices
//  or spans with constant z depth.
// However, rotation around the world z axis is possible,
//  thus this mapping, while simpler and faster than
//  perspective correct texture mapping, has to traverse
//  the texture at an angle in all but a few cases.
// In consequence, flats are not stored by column (like walls),
//  and the inner loop has to step in texture space u and v.
//
int			ds_y;
int			ds_x1;
int			ds_x2;

lighttable_t*		ds_colormap;

fixed_t			ds_xfrac;
fixed_t			ds_yfrac;
fixed_t			ds_xstep;
fixed_t			ds_ystep;

// start of a 64*64 tile image
byte*			ds_source;

// just for profiling
int			dscount;

//
// R_InitBuffer
// Creats lookup tables that avoid
//  multiplies and other hazzles
//  for getting the framebuffer address
//  of a pixel to draw.
//
void R_InitBuffer ( int width, int height )
{
    int		i;
    int		offset;

    // Handle resize,
    //  e.g. smaller view windows
    //  with border and/or status bar.
    viewwindowx = (SCREENWIDTH-width) >> 1;

    // Column offset. For windows.
    for (i=0 ; i<width ; i++)
    {
        columnofs[i] = viewwindowx + i;
    }

    // Samw with base row offset.
    if (width == SCREENWIDTH)
    {
        viewwindowy = 0;
    }
    else
    {
        viewwindowy = ((SCREENHEIGHT-(SBARHEIGHT*2)-height) >> 1);//(SCREENHEIGHT-SBARHEIGHT-height) >> 1;
    }

    // Preclaculate all row offsets.
    offset = (((viewwindowy) * SCREENWIDTH) /** 2*/);

    for (i=0 ; i<height ; i++)
    {
        ylookup[i] = offset;
        ylookup2[i] = ylookup[i] + viewwindowx;
        offset += SCREENWIDTH;
    }
	
    p = W_CacheLumpName ("PLAYPAL",PU_CACHE);	
}


//
// R_FillBackScreen
// Fills the back screen with a pattern
//  for variable screen sizes
// Also draws a beveled edge.
//
void R_FillBackScreen (void)
{
    byte*	src;
    byte*	dest;
    int		x;
    int		y;
    patch_t*	patch;

    // DOOM border patch.
    char	name1[] = "FLOOR7_2";

    // DOOM II border patch.
    char	name2[] = "GRNROCK";

    char*	name;

    if (scaledviewwidth == SCREENWIDTH)
    {
	return;
    }

    if ( gamemode == commercial)
    {
	name = name2;
    }
    else
    {
	name = name1;
    }

    src = W_CacheLumpName (name, PU_CACHE);
    dest = screens[1];

    for (y=0 ; y<SCREENHEIGHT-SBARHEIGHT ; y++)
    {
	for (x=0 ; x<SCREENWIDTH>>6 ; x++)
	{
	    __n64_memcpy_ASM (dest, src+((y&63)<<6), 64);
	    dest += 64;
	}

	if (SCREENWIDTH&63)
	{
	    __n64_memcpy_ASM (dest, src+((y&63)<<6), SCREENWIDTH&63);
	    dest += (SCREENWIDTH&63);
	}
    }

    patch = W_CacheLumpName ("brdr_t",PU_CACHE);

    for (x=0 ; x<scaledviewwidth ; x+=8)
    {
	V_DrawPatch (viewwindowx+x,viewwindowy-8,1,patch);
    }
    patch = W_CacheLumpName ("brdr_b",PU_CACHE);

    for (x=0 ; x<scaledviewwidth ; x+=8)
    {
	V_DrawPatch (viewwindowx+x,viewwindowy+viewheight,1,patch);
    }
    patch = W_CacheLumpName ("brdr_l",PU_CACHE);

    for (y=0 ; y<viewheight ; y+=8)
    {
	V_DrawPatch (viewwindowx-8,viewwindowy+y,1,patch);
    }
    patch = W_CacheLumpName ("brdr_r",PU_CACHE);

    for (y=0 ; y<viewheight ; y+=8)
    {
	V_DrawPatch (viewwindowx+scaledviewwidth,viewwindowy+y,1,patch);
    }

    // Draw beveled edge.
    V_DrawPatch (viewwindowx-8,
		 viewwindowy-8,
		 1,
		 W_CacheLumpName ("brdr_tl",PU_CACHE));

    V_DrawPatch (viewwindowx+scaledviewwidth,
		 viewwindowy-8,
		 1,
		 W_CacheLumpName ("brdr_tr",PU_CACHE));

    V_DrawPatch (viewwindowx-8,
		 viewwindowy+viewheight,
		 1,
		 W_CacheLumpName ("brdr_bl",PU_CACHE));

    V_DrawPatch (viewwindowx+scaledviewwidth,
		 viewwindowy+viewheight,
		 1,
		 W_CacheLumpName ("brdr_br",PU_CACHE));
}



//
// Copy a screen buffer.
//
void R_VideoErase ( unsigned ofs, int count )
{
	if((byte*)&palarray[0] != p) {
		// TODO figure out a way to do this without the memcpy, switch pointers or something
		__n64_memcpy_ASM(oldpal, palarray, 1024);

		I_SetPalette(p);

		for(int i=0;i<count;i+=2)
		{
			*(uint32_t *)(__safe_buffer[_dc - 1] + (((40*SCREENWIDTH)+ofs+i)<<1)) = palarray[screens[1][ofs+i]];
		}

		__n64_memcpy_ASM(palarray, oldpal, 1024);
	}
}


//
// R_DrawViewBorder
// Draws the border around the view
//  for different size windows?
//
void V_MarkRect ( int x, int y, int width, int height );

void R_DrawViewBorder (void)
{
    int		top;
    int		side;
    int		ofs;
    int		i;

    if (scaledviewwidth == SCREENWIDTH)
    {
	return;
    }

    top = (((SCREENHEIGHT-SBARHEIGHT*2)-viewheight)/2);
    side = (SCREENWIDTH-scaledviewwidth)/2;
	
    // copy top and one line of left side
    R_VideoErase (0, top*SCREENWIDTH+side);

    // copy one line of right side and bottom
    ofs = (viewheight+top)*SCREENWIDTH-side;
    R_VideoErase (ofs, top*SCREENWIDTH+side);

    // copy sides using wraparound
    ofs = top*SCREENWIDTH + SCREENWIDTH-side;
    side <<= 1;

    for (i=1 ; i<viewheight ; i++)
    {
	R_VideoErase (ofs, side);
	ofs += SCREENWIDTH;
    }

    // ?
    //V_MarkRect (0,0,SCREENWIDTH, SCREENHEIGHT-SBARHEIGHT);
}
