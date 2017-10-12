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


//extern void *n64_memcpy(void *d, const void *s, size_t n);
extern void *__n64_memcpy_ASM(void *d, const void *s, size_t n);
// special case, always fills with zero
//extern void *n64_memset(void *ptr, int value, size_t num);
extern void *__n64_memset_ZERO_ASM(void *ptr, int value, size_t num);
// the non-special case version that accepts arbitrary fill value
//extern void *n64_memset2(void *ptr, int value, size_t num);
extern void *__n64_memset_ASM(void *ptr, int value, size_t num);

extern uint32_t palarray[256];
extern void *__safe_buffer[];
extern display_context_t _dc;
extern void buffer_sprite_8bit(int x, int y, int w, int  h, uint8_t *sprite, uint8_t *buf, int buf_w, int buf_h);
extern void buffer_fast_line_8bit(int x, int y, int x2, int y2, byte color, uint8_t *buffer, int buf_w, int buf_h);
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

#define R_FIXED_POINT 16
#define R_ONE (1 << R_FIXED_POINT)

double fixed_to_double(int fixed)
{
        return ((double)fixed) / R_ONE;
}

int fixed_to_int(int fixed)
{
        return fixed >> R_FIXED_POINT;
}

/**
 * viewwindowx(i,width,height) = ((SCREENWIDTH-width) >> 1)
 * columnofs(i,width,height) = viewwindowx(width) + i
 * viewwindowy(i,width,height) = (width == SCREENWIDTH) ? 0 : ((SCREENHEIGHT-SBARHEIGHT-height) >> 1)
 * offset(i	,width,height) = viewwindowy(i,width,height) * SCREENWIDTH
 * ylookup(i,width,height) = screens[0] + (offset(i,width,height) + (SCREENWIDTH*i))
 * ylookup2(i,width,height) = ylookup(i,width,height) + viewwindowx(i,width,height)
 */
/*byte*		viewimage;
int		viewwidth;
int		scaledviewwidth;
int		viewheight;
int		viewwindowx;
int		viewwindowy;
byte*		ylookup[MAXHEIGHT];
byte*		ylookup2[MAXHEIGHT];

int		columnofs[MAXWIDTH];*/
#if 0
int _viewwindowx(int i, int width, int height)
{
    return ((SCREENWIDTH-width) >> 1);
}
 
int _columnofs(int i, int width, int height)
{
    return _viewwindowx(i,width,height) + i; 
}
 
int _viewwindowy(int i, int width, int height)
{
    return (width == SCREENWIDTH) ? 0 : ((SCREENHEIGHT-SBARHEIGHT-height) >> 1);
}

int _offset(int i, int width, int height)
{
    return _viewwindowy(i,width,height) * SCREENWIDTH;
}

byte *_ylookup(int i, int width, int height)
{
	return screens[0] + (_offset(i,width,height) + (SCREENWIDTH*i));
}

byte *_ylookup2(int i, int width, int height)
{
    return _ylookup(i,width,height) + _viewwindowx(i,width,height);
}
#endif

byte*		viewimage;
int		viewwidth;
int		scaledviewwidth;
int		viewheight;
int		viewwindowx;
int		viewwindowy;
byte*		ylookup[MAXHEIGHT];
byte*		ylookup2[MAXHEIGHT];

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
int			dc_x;
int			dc_yl;
int			dc_yh;
fixed_t			dc_iscale;
fixed_t			dc_texturemid;

// first pixel in a column (possibly virtual)
byte*			dc_source;

// just for profiling
int			dccount;
int sprite_alloc = 0;
sprite_t *column_sprite;// = n64_malloc(sizeof(sprite_t) + (2 * 128 * 128));
byte *column_texture;
extern fixed_t FixedDiv(fixed_t a, fixed_t b);

extern int colcount;
extern int spancount;

//
// A column is a vertical slice/span from a wall texture that,
//  given the DOOM style restrictions on the view orientation,
//  will always have constant z depth.
// Thus a special case loop for very fast rendering can
//  be used. It has also been used with Wolfenstein 3D.
//
//#ifdef RDPRENDER
#ifdef RDPRENDER
void R_DrawColumn (void);

void R_DrawColumn_C (void)
{
	R_DrawColumn();
}	

void R_DrawColumn (void)
#endif

#ifndef RDPRENDER
void R_DrawColumn_C (void)
#endif

{
    int			count;
    byte*		dest;
    fixed_t		frac;
    fixed_t		fracstep;
#ifdef RDPRENDER
    byte color_pixel = dc_colormap[dc_source[0]];
#endif

    // dc_yh is top pixel?
    // dc_yl is bottom pixel?

    count = dc_yh - dc_yl;

    // Zero length, column does not exceed a pixel.
    if (count < 0)
    {
	return;
    }

#if 0
if(1)
{


                /* Load the sprite into texture slot 0, at the beginning of memory, without mirroring */
//                rdp_load_texture_stride( 0, 0, MIRROR_DISABLED, column_sprite, 0);

                /* Display a stationary sprite to demonstrate backwards compatibility */
//                rdp_draw_sprite( 0, (x*32), (y*24) );
//              rdp_draw_textured_rectangle_scaled(0, dc_x, dc_yl, dc_x + 1, dc_yh, 0.0078125, fixed_to_double(dc_iscale));
                rdp_set_primitive_color(palarray[color_pixel]);
                rdp_draw_filled_rectangle(dc_x*2, dc_yl*2, (dc_x + 1)*2, dc_yh*2);
				return;
}	
#endif

#ifdef RANGECHECK
    if ((unsigned)dc_x >= SCREENWIDTH
	|| dc_yl < 0
	|| dc_yh >= SCREENHEIGHT)
    {
        char ermac[256];
        sprintf(ermac, "R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
        I_Error(ermac);
//	I_Error ("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
    }
#endif


#if 1
colcount++;
#endif	
	
#ifdef RDPRENDER
    // Determine scaling,
    //  which is the only mapping to be done.
    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl-centery)*fracstep;

	if(!sprite_alloc)
	{
#if 1
		column_texture = (byte*)n64_malloc(sizeof(byte)*128);
#endif
#if 0
		column_texture = (byte*)malloc(sizeof(byte)*128);
#endif
		column_sprite = n64_malloc(sizeof(sprite_t) + (4 * 128));
		column_sprite->width = 1;
		column_sprite->height = 128;
		column_sprite->bitdepth = 2;
		column_sprite->hslices = 1;
		column_sprite->vslices = 1;
		sprite_alloc = 1;
	}

/*	for(int i=0;i<128;i++)
	{
		column_texture[i] = dc_colormap[dc_source[(fixed_to_int(frac) + i)&127]];
	}*/

	for(int i=0;i<128;i++)
	{
		column_sprite->data[i] = palarray[dc_colormap[dc_source[(fixed_to_int(frac) + i)&127]]];//column_texture[i]];
	}
    
	// Assure RDP is ready for new commands
#if 0
    rdp_sync( SYNC_PIPE );
    // Remove any clipping windows
    rdp_set_default_clipping();
	rdp_set_texture_flush( FLUSH_STRATEGY_AUTOMATIC);
	// Enable sprite display instead of solid color fill
    rdp_enable_texture_copy();
	// Attach RDP to display
	rdp_attach_display( _dc );
#endif
	rdp_set_texture_flush( FLUSH_STRATEGY_AUTOMATIC);
	// Enable sprite display instead of solid color fill
    rdp_enable_texture_copy();

	// Load the sprite into texture slot 0, at the beginning of memory, without mirroring
	rdp_load_texture( 0, 0, MIRROR_DISABLED, column_sprite);

	// Display a stationary sprite to demonstrate backwards compatibility
#if 0
	rdp_draw_textured_rectangle_scaled(0, dc_x*2, dc_yl*2, (dc_x + 1)*2, dc_yh*2, 2.0, (1.0 / fixed_to_double(dc_iscale)) /*/ 2.0*/);
#endif
#if 1
	rdp_draw_textured_rectangle_scaled(0, dc_x, dc_yl, (dc_x + 1), dc_yh, 2.0, (1.0 / fixed_to_double(dc_iscale)) / 2.0);
#endif
#if 0
	rdp_detach_display();
#endif
#endif

#ifndef RDPRENDER
    // Framebuffer destination address.
    // Use ylookup LUT to avoid multiply with ScreenWidth.
    // Use columnofs LUT for subwindows?
#if 1
    dest = ylookup[dc_yl] + columnofs[dc_x];
#endif
#if 0
    dest = _ylookup(dc_yl,scaledviewwidth,viewheight) + _columnofs(dc_x,scaledviewwidth,viewheight); 
#endif
	
    // Determine scaling,
    //  which is the only mapping to be done.
    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl-centery)*fracstep;

    // Inner loop that does the actual texture mapping,
    //  e.g. a DDA-lile scaling.
    // This is as fast as it gets.

    do
    {
        // Re-map color indices from wall texture column
        //  using a lighting/special effects LUT.
        *dest = dc_colormap[dc_source[(frac>>FRACBITS)&127]];

        dest += SCREENWIDTH;
        frac += fracstep;
    }
    while (count--);
#endif
}

void R_DrawColumnLow (void)
{
    int			count;
    byte*		dest;
    byte*		dest2;
    fixed_t		frac;
    fixed_t		fracstep;

    count = dc_yh - dc_yl;

    // Zero length.
    if (count < 0)
    {
	return;
    }

#ifdef RANGECHECK
    if ((unsigned)dc_x >= SCREENWIDTH
	|| dc_yl < 0
	|| dc_yh >= SCREENHEIGHT)
  {
        char ermac[256];
        sprintf(ermac, "R_DrawColumnLow: %i to %i at %i", dc_yl, dc_yh, dc_x);
        I_Error(ermac);
//	I_Error ("R_DrawColumnLow: %i to %i at %i", dc_yl, dc_yh, dc_x);
    }
    //	dccount++;
#endif

//    dest = ylookup[dc_yl] + columnofs[dc_x<<1];
//    dest2 = ylookup[dc_yl] + columnofs[(dc_x<<1)+1];
#if 1
    dest = ylookup2[dc_yl] + (dc_x<<1);
    dest2 = ylookup2[dc_yl] + (dc_x<<1)+1;
#endif
#if 0
    dest = _ylookup2(dc_yl,scaledviewwidth,viewheight) + (dc_x<<1);
    dest2 = _ylookup2(dc_yl,scaledviewwidth,viewheight) + (dc_x<<1)+1;
#endif

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl-centery)*fracstep;

    do
    {
	*dest2 = *dest = dc_colormap[dc_source[(frac>>FRACBITS)&127]];
	dest += SCREENWIDTH;
	dest2 += SCREENWIDTH;
	frac += fracstep;
    }
    while (count--);
}



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
void R_DrawFuzzColumn (void)
{
    int			count;
    byte*		dest;
    fixed_t		frac;
    fixed_t		fracstep;

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


#ifdef RANGECHECK
    if ((unsigned)dc_x >= SCREENWIDTH
	|| dc_yl < 0 || dc_yh >= SCREENHEIGHT)
    {
        char ermac[256];
        sprintf(ermac, "R_DrawFuzzColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
        I_Error(ermac);
/*	I_Error ("R_DrawFuzzColumn: %i to %i at %i",
		 dc_yl, dc_yh, dc_x);*/
    }
#endif


    // Keep till detailshift bug in blocky mode fixed,
    //  or blocky mode removed.
    /* WATCOM code
    if (detailshift)
    {
	if (dc_x & 1)
	{
	    outpw (GC_INDEX,GC_READMAP+(2<<8) );
	    outp (SC_INDEX+1,12);
	}
	else
	{
	    outpw (GC_INDEX,GC_READMAP);
	    outp (SC_INDEX+1,3);
	}
	dest = destview + dc_yl*80 + (dc_x>>1);
    }
    else
    {
	outpw (GC_INDEX,GC_READMAP+((dc_x&3)<<8) );
	outp (SC_INDEX+1,1<<(dc_x&3));
	dest = destview + dc_yl*80 + (dc_x>>2);
    }*/

    // Does not work with blocky mode.
#if 1
    dest = ylookup[dc_yl] + columnofs[dc_x];
#endif
#if 0
    dest = _ylookup(dc_yl,scaledviewwidth,viewheight) + _columnofs(dc_x,scaledviewwidth,viewheight); 
#endif

    // Looks familiar.
    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl-centery)*fracstep;

    // Looks like an attempt at dithering,
    //  using the colormap #6 (of 0-31, a bit
    //  brighter than average).
    do
    {
	// Lookup framebuffer, and retrieve
	//  a pixel that is either one column
	//  left or right of the current one.
	// Add index from colormap to index.
	*dest = colormaps[6*256+dest[fuzzoffset[fuzzpos]]];

	// Clamp table lookup index.
	if (++fuzzpos == FUZZTABLE)
	    fuzzpos = 0;

	dest += SCREENWIDTH;

	frac += fracstep;
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
void R_DrawFuzzColumnLow (void)
{
    int			count;
    byte*		dest;
    byte*		dest2;
    fixed_t		frac;
    fixed_t		fracstep;

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


#ifdef RANGECHECK
    if (((unsigned)dc_x << 1) >= SCREENWIDTH
	|| dc_yl < 0 || dc_yh >= SCREENHEIGHT)
    {
        char ermac[256];
        sprintf(ermac, "R_DrawFuzzColum: %i to %i at %i", dc_yl, dc_yh, dc_x);
        I_Error(ermac);
/*	I_Error ("R_DrawFuzzColumn: %i to %i at %i",
		 dc_yl, dc_yh, dc_x);*/
    }
#endif

//    dest = ylookup[dc_yl] + columnofs[dc_x << 1];
//    dest2 = ylookup[dc_yl] + columnofs[(dc_x << 1) + 1];
#if 1
    dest = ylookup2[dc_yl] + (dc_x<<1);
    dest2 = ylookup2[dc_yl] + (dc_x<<1)+1;
#endif
#if 0
    dest = _ylookup2(dc_yl,scaledviewwidth,viewheight) + (dc_x<<1);
    dest2 = _ylookup2(dc_yl,scaledviewwidth,viewheight) + (dc_x<<1)+1;
#endif

    // Looks familiar.
    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl-centery)*fracstep;

    // Looks like an attempt at dithering,
    //  using the colormap #6 (of 0-31, a bit
    //  brighter than average).
    do
    {
	// Lookup framebuffer, and retrieve
	//  a pixel that is either one column
	//  left or right of the current one.
	// Add index from colormap to index.
	*dest2 = *dest = colormaps[6*256+dest[fuzzoffset[fuzzpos]]];

	// Clamp table lookup index.
	if (++fuzzpos == FUZZTABLE)
        {
	    fuzzpos = 0;
        }

	dest += SCREENWIDTH;
	dest2 += SCREENWIDTH;

	frac += fracstep;
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

void R_DrawTranslatedColumn (void)
{
    int			count;
    byte*		dest;
    fixed_t		frac;
    fixed_t		fracstep;

    count = dc_yh - dc_yl;

    if (count < 0)
    {
	return;
    }

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

    // WATCOM VGA specific.
    /* Keep for fixing.
    if (detailshift)
    {
	if (dc_x & 1)
	    outp (SC_INDEX+1,12);
	else
	    outp (SC_INDEX+1,3);

	dest = destview + dc_yl*80 + (dc_x>>1);
    }
    else
    {
	outp (SC_INDEX+1,1<<(dc_x&3));

	dest = destview + dc_yl*80 + (dc_x>>2);
    }*/

    // FIXME. As above.
    dest = ylookup[dc_yl] + columnofs[dc_x];

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
	*dest = dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]];
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

void R_DrawTranslatedColumnLow (void)
{
    int			count;
    byte*		dest;
    byte*		dest2;
    fixed_t		frac;
    fixed_t		fracstep;

    count = dc_yh - dc_yl;

    if (count < 0)
    {
	return;
    }

#ifdef RANGECHECK
    if (((unsigned)dc_x << 1) >= SCREENWIDTH
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

//    dest = ylookup[dc_yl] + columnofs[dc_x << 1];
//    dest2 = ylookup[dc_yl] + columnofs[(dc_x << 1) + 1];
    dest = ylookup2[dc_yl] + (dc_x<<1);
    dest2 = ylookup2[dc_yl] + (dc_x<<1)+1;

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
	*dest2 = *dest = dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]];
	dest += SCREENWIDTH;
	dest2 += SCREENWIDTH;

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

// for assembly debugging purposes
int last_spot = -1;

//
// Draws the actual span.
#ifdef RDPRENDER
void R_DrawSpan (void);

void R_DrawSpan_C (void)
{
	R_DrawSpan();
}	

void R_DrawSpan (void)
#endif

#ifndef RDPRENDER
void R_DrawSpan_C (void)
#endif

{
    fixed_t		xfrac;
    fixed_t		yfrac;
    byte*		dest;
    int			count;
    int			spot;

#ifdef RANGECHECK
    if (ds_x2 < ds_x1
	|| ds_x1<0
	|| ds_x2>=SCREENWIDTH
	|| (unsigned)ds_y>SCREENHEIGHT)
    {
        char ermac[256];
        sprintf(ermac, "R_DrawSpan: %i to %i at %i", ds_x1, ds_x2, ds_y);
        I_Error(ermac);
/*	I_Error( "R_DrawSpan: %i to %i at %i",
		 ds_x1,ds_x2,ds_y);*/
    }
//	dscount++;
#endif

#if 1
spancount++;
#endif

#ifdef RDPRENDER
    count = ds_x2 - ds_x1;
byte color_pixel = ds_colormap[ds_source[0]];

/*
//buffer_fast_line_8bit(ds_x1, ds_y, ds_x2, ds_y, color_pixel, ylookup[0], 320,200);
byte *texture = (byte*)malloc(sizeof(byte)*count * count);

for(int i=0;i<count*count;i+=count)
{
    texture[i] = ds_source[i&(63*count)];
}

buffer_sprite_8bit(ds_x1, ds_y, count, 1, texture, ylookup[0], 320, 200);

free(texture);*/
#if 0
               /* Assure RDP is ready for new commands */
                rdp_sync( SYNC_PIPE );

                /* Remove any clipping windows */
                rdp_set_default_clipping();

                /* Enable sprite display instead of solid color fill */
//                rdp_enable_texture_copy();
                rdp_enable_primitive_fill();
                /* Attach RDP to display */
                rdp_attach_display( _dc );
                rdp_sync( SYNC_PIPE );
#endif
                rdp_enable_primitive_fill();

                /* Load the sprite into texture slot 0, at the beginning of memory, without mirroring */
//                rdp_load_texture_stride( 0, 0, MIRROR_DISABLED, column_sprite, 0);

                /* Display a stationary sprite to demonstrate backwards compatibility */
//                rdp_draw_sprite( 0, (x*32), (y*24) );
//              rdp_draw_textured_rectangle_scaled(0, dc_x, dc_yl, dc_x + 1, dc_yh, 0.0078125, fixed_to_double(dc_iscale));
                rdp_set_primitive_color(palarray[color_pixel]);
#if 0
                rdp_draw_filled_rectangle(ds_x1*2, ds_y*2, ds_x2*2, (ds_y+1)*2);
#endif
#if 1
                rdp_draw_filled_rectangle(ds_x1, ds_y, ds_x2, (ds_y+1));
#endif
#if 0
				rdp_detach_display();
#endif
#endif

#ifndef RDPRENDER

    xfrac = ds_xfrac;
    yfrac = ds_yfrac;

#if 1
    dest = ylookup[ds_y] + columnofs[ds_x1];
#endif
#if 0
    dest = _ylookup(ds_y,scaledviewwidth,viewheight) + _columnofs(ds_x1,scaledviewwidth,viewheight);
#endif
    // We do not check for zero spans here?
    count = ds_x2 - ds_x1;

#if 0
    if(count > 0)
    {
//	spot = ((yfrac>>(16-6))&(63*64)) + ((xfrac>>16)&63);
//	byte spot_ds_source = ds_source[spot];
//	byte spot_ds_colormap = ds_colormap[spot_ds_source];

	byte __spot_ds_colormap = ds_colormap[ds_source[((yfrac>>(16-6))&(63*64)) + ((xfrac>>16)&63)]];

	// the non-special case version that accepts arbitrary fill value
	//void *n64_memset2(void *ptr, int value, size_t num)
//	n64_memset2(dest, __spot_ds_colormap, count);
	__n64_memset_ASM(dest, __spot_ds_colormap, count);
    }
#endif

#if 0

//	n64_memset(dest, 0, count);
	__n64_memset_ZERO_ASM(dest, 0, count);

#endif


#if 1
    do
    {
	// Current texture index in u,v.
	spot = ((yfrac>>(16-6))&(63*64)) + ((xfrac>>16)&63);
//	byte spanc = ds_colormap[ds_source[spot]];
	// Lookup pixel from flat texture tile,
	//  re-index using light/colormap.
	*dest++ = ds_colormap[ds_source[spot]];

	// Next step in u,v.
	xfrac += ds_xstep;
	yfrac += ds_ystep;
    }
    while (count--);
#endif
#endif
}



//
// Again..
//
void R_DrawSpanLow (void)
{
    fixed_t		xfrac;
    fixed_t		yfrac;
    byte*		dest;
    int			count;
    int			spot;

#ifdef RANGECHECK
    if (ds_x2 < ds_x1
	|| ds_x1<0
	|| ds_x2>=SCREENWIDTH
	|| (unsigned)ds_y>SCREENHEIGHT)
    {
        char ermac[256];
        sprintf(ermac, "R_DrawSpanLow: %i to %i at %i", ds_x1, ds_x2, ds_y);
        I_Error(ermac);
/*	I_Error( "R_DrawSpanLow: %i to %i at %i",
		 ds_x1,ds_x2,ds_y);*/
    }
//	dscount++;
#endif

    xfrac = ds_xfrac;
    yfrac = ds_yfrac;

//    dest = ylookup[ds_y] + columnofs[ds_x1<<1];
#if 1
    dest = ylookup2[ds_y] + (ds_x1<<1);
#endif
#if 0
    dest = _ylookup2(ds_y,scaledviewwidth,viewheight) + (ds_x1<<1);
#endif
    count = ds_x2 - ds_x1;

#if 0
    if(count > 0)
    {
//	spot = ((yfrac>>(16-6))&(63*64)) + ((xfrac>>16)&63);
//	byte spot_ds_source = ds_source[spot];
//	byte spot_ds_colormap = ds_colormap[spot_ds_source];

	byte __spot_ds_colormap = ds_colormap[ds_source[((yfrac>>(16-6))&(63*64)) + ((xfrac>>16)&63)]];

	// the non-special case version that accepts arbitrary fill value
	//void *n64_memset2(void *ptr, int value, size_t num)
//	n64_memset2(dest, __spot_ds_colormap, count);
	__n64_memset_ASM(dest, __spot_ds_colormap, count);
    }
#endif

#if 0

//    n64_memset(dest, 0, count);
    __n64_memset_ZERO_ASM(dest, 0, count);

#endif

#if 1
    do
    {
	spot = ((yfrac>>(16-6))&(63*64)) + ((xfrac>>16)&63);
	// Lowres/blocky mode does it twice,
	//  while scale is adjusted appropriately.
	*dest++ = ds_colormap[ds_source[spot]];
	*dest++ = ds_colormap[ds_source[spot]];

	xfrac += ds_xstep;
	yfrac += ds_ystep;
    }
    while (count--);
#endif
}


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
        viewwindowy = (SCREENHEIGHT-SBARHEIGHT-height) >> 1;
    }

    // Preclaculate all row offsets.
    offset = viewwindowy * SCREENWIDTH;

    for (i=0 ; i<height ; i++)
    {
        ylookup[i] = screens[0] + offset;
        ylookup2[i] = ylookup[i] + viewwindowx;
        offset += SCREENWIDTH;
    }
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
	for (x=0 ; x<SCREENWIDTH/64 ; x++)
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
    // LFB copy.
    // This might not be a good idea if memcpy
    //  is not optiomal, e.g. byte by byte on
    //  a 32bit CPU, as GNU GCC/Linux libc did
    //  at one point.
    __n64_memcpy_ASM (screens[0]+ofs, screens[1]+ofs, count);
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

    top = ((SCREENHEIGHT-SBARHEIGHT)-viewheight)/2;
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
    V_MarkRect (0,0,SCREENWIDTH, SCREENHEIGHT-SBARHEIGHT);
}
