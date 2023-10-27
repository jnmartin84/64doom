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
//    The actual span/column drawing functions.
//    Here find the main potential for optimization,
//     e.g. inline assembly, different algorithms.
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

extern void *bufptr;

#define ytab(y) (((y)<<8)+((y)<<6))

extern void I_SetPalette(byte* palette);

extern surface_t* _dc;

// ?
#define MAXWIDTH            1120
#define MAXHEIGHT            832

// status bar height at bottom of screen
#define SBARHEIGHT            32

//
// All drawing to the view buffer is accomplished in this file.
// The other refresh files only know about ccordinates,
//  not the architecture of the frame buffer.
// Conveniently, the frame buffer is a linear one,
//  and we need only the base address,
//  and the total size == width*height*depth/8.,
//

byte*           viewimage;
int             viewwidth;
int             scaledviewwidth;
int             viewheight;
int             viewwindowx;
int             viewwindowy;

//int             ylookup[200];//MAXHEIGHT];
//int             ylookup2[200];//MAXHEIGHT];

int             columnofs[320];//MAXWIDTH];

// Color tables for different players,
//  translate a limited part to another
//  (color ramps used for  suit colors).
//
byte            translations[3][256];

//
// R_DrawColumn
// Source is the top of the column to scale.
//
lighttable_t*   dc_colormap;
int             x;
int             dc_x;
int             dc_yl;
int             dc_yh;
fixed_t         dc_iscale;
fixed_t         dc_texturemid;

// first pixel in a column (possibly virtual)
byte*           dc_source;

// just for profiling
int             dccount;

//
// Spectre/Invisibility.
//
#define FUZZTABLE             50
#define FUZZOFF    (SCREENWIDTH)

int    fuzzoffset[FUZZTABLE] =
{
    FUZZOFF,-FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
    FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
    FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,
    FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
    FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,
    FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,
    FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF 
};

int    fuzzpos = 0;

//
// Framebuffer postprocessing.
// Creates a fuzzy image by copying pixels
//  from adjacent ones to left and right.
// Used with an all black colormap, this
//  could create the SHADOW effect,
//  i.e. spectres and invisible players.
//
void R_DrawFuzzColumn (int cyl,int cyh,int cx)
{
    int             count;
    uint8_t*        dest;

    // Adjust borders. Low...
    if (!cyl)
    {
        cyl = 1;
    }

    // .. and high.
    if (cyh == viewheight-1)
    {
        cyh = viewheight - 2;
    }

    count = cyh - cyl;

    // Zero length.
    if (count < 0)
    {
        return;
    }

    dest = (uint8_t*)(((uint8_t *)bufptr) + ((viewwindowy + cyl)*SCREENWIDTH) + (viewwindowx + cx));

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
void R_DrawFuzzColumnLow (int cyl,int cyh,int cx)
{
    int              count;
    uint16_t*        dest;

    // Adjust borders. Low...
    if (!cyl)
    {
        cyl = 1;
    }

    // .. and high.
    if (cyh == viewheight-1)
    {
        cyh = viewheight - 2;
    }

    count = cyh - cyl;

    // Zero length.
    if (count < 0)
    {
        return;
    }

    dest = (uint16_t*)(((uint8_t *)bufptr) + ((viewwindowy + cyl)*SCREENWIDTH) + viewwindowx + (cx<<1));

    // Looks like an attempt at dithering,
    //  using the colormap #6 (of 0-31, a bit
    //  brighter than average).
    do
    {
        // Lookup framebuffer, and retrieve
        //  a pixel that is either one column
        //  left or right of the current one.
        // Add index from colormap to index.

        // 16-bit write, double-pixel write with one store instruction
        *dest = dest[fuzzoffset[fuzzpos] >> 1];

        // Clamp table lookup index.
        if (++fuzzpos == FUZZTABLE)
        {
            fuzzpos = 0;
        }

        dest += (SCREENWIDTH >> 1);
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
byte*        dc_translation;
byte*        translationtables;

void R_DrawTranslatedColumn (int cyl,int cyh,int cx)
{
    int              count;
    fixed_t          frac;
    fixed_t          fracstep;
    uint8_t*         dest;

#ifdef RANGECHECK
    if ((unsigned)cx >= SCREENWIDTH
    || cyl < 0
    || cyh >= SCREENHEIGHT)
    {
        I_Error("R_DrawTranslatedColumn: %i to %i at %i", cyl, cyh, cx);
    }
#endif

    count = cyh - cyl;

    // Framebuffer destination address.
    dest = (uint8_t*)(((uint8_t *)bufptr) + ((viewwindowy + cyl)*SCREENWIDTH) + (viewwindowx + cx));

    // Looks familiar.
    fracstep = dc_iscale;
    frac = dc_texturemid + (cyl-centery)*fracstep;

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
//byte*    dc_translation;
//byte*    translationtables;

void R_DrawTranslatedColumnLow (int cyl,int cyh,int cx)
{
    int              count;
    fixed_t          frac;
    fixed_t          fracstep;
    uint16_t*        dest;

#ifdef RANGECHECK
    if ((unsigned)cx >= SCREENWIDTH
    || cyl < 0
    || cyh >= SCREENHEIGHT)
    {
        I_Error("R_DrawTranslatedColumnLow: %i to %i at %i", cyl, cyh, cx);
    }
#endif

    count = cyh - cyl;

    dest = (uint16_t*)(((uint8_t *)bufptr) + ((viewwindowy + cyl)*SCREENWIDTH) + viewwindowx + (cx<<1));

    // Looks familiar.
    fracstep = dc_iscale;
    frac = dc_texturemid + (cyl-centery)*fracstep;

    // Here we do an additional index re-mapping.
    do
    {
        // Translation tables are used
        //  to map certain colorramps to other ones,
        //  used with PLAY sprites.
        // Thus the "green" ramp of the player 0 sprite
        //  is mapped to gray, red, black/indigo.

        // 16-bit write, double-pixel write with one store instruction
	uint8_t spot = dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]];
	uint16_t widespot = (spot << 8) | spot;
	*dest = widespot;
        dest += (SCREENWIDTH >> 1);
        frac += fracstep;
    }
    while (count--);
}

void R_DrawColumn (int cyl,int cyh,int cx)
{
    int              count;
    byte*            dest;
    unsigned         frac;
    unsigned         fracstep;
    unsigned         fracstep2;
    unsigned         fracstep3;
    unsigned         fracstep4;

    count = cyh - cyl + 1;

    dest = (uint8_t*)(bufptr + ((viewwindowy + cyl)*SCREENWIDTH) + (viewwindowx + cx));

    fracstep = dc_iscale << 9;
    frac = (dc_texturemid + (cyl-centery)*dc_iscale) << 9;

    fracstep2 = fracstep + fracstep;
    fracstep3 = fracstep2 + fracstep;
    fracstep4 = fracstep3 + fracstep;

    while (count >= 8)
    {
        dest[0] = dc_colormap[dc_source[frac >> 25]];
        dest[SCREENWIDTH] = dc_colormap[dc_source[(frac + fracstep) >> 25]];
        dest[SCREENWIDTH*2] = dc_colormap[dc_source[(frac + fracstep2) >> 25]];
        dest[SCREENWIDTH*3] = dc_colormap[dc_source[(frac + fracstep3) >> 25]];

        frac += fracstep4;

        dest[SCREENWIDTH*4] = dc_colormap[dc_source[frac >> 25]];
        dest[SCREENWIDTH*5] = dc_colormap[dc_source[(frac + fracstep) >> 25]];
        dest[SCREENWIDTH*6] = dc_colormap[dc_source[(frac + fracstep2) >> 25]];
        dest[SCREENWIDTH*7] = dc_colormap[dc_source[(frac + fracstep3) >> 25]];

        frac += fracstep4;
        dest += SCREENWIDTH*8;
        count -= 8;
    }

    while (count > 0)
    {
        *dest = dc_colormap[dc_source[frac >> 25]];
        dest += SCREENWIDTH;
        frac += fracstep;
        count--;
    }
}

void R_DrawColumnLow (int cyl,int cyh,int cx)
{
    int         count;
    unsigned    frac;
    unsigned    fracstep;
    uint16_t    *dest;

#ifdef RANGECHECK
    if ((unsigned)cx >= SCREENWIDTH
    || cyl < 0
    || cyh >= SCREENHEIGHT)
    {
        I_Error("R_DrawColumnLow: %i to %i at %i", cyl, cyh, cx);
    }
#endif

    count = cyh - cyl;

    dest = (uint16_t*)(bufptr + ((viewwindowy + cyl)*SCREENWIDTH) + viewwindowx + (cx<<1));

    fracstep = dc_iscale << 9;
    frac = (dc_texturemid + (cyl-centery)*dc_iscale) << 9;

    do
    {
        // 16-bit write, double-pixel write with one store instruction
	uint16_t widespot = dc_colormap[dc_source[frac >> 25]];
	widespot |= (widespot << 8);
	*dest = widespot;
        dest += (SCREENWIDTH >> 1);
        frac += fracstep;
    } while (count--);
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
int                  ds_y;
int                  ds_x1;
int                  ds_x2;

fixed_t              ds_xfrac;
fixed_t              ds_yfrac;
fixed_t              ds_xstep;
fixed_t              ds_ystep;

// start of a 64*64 tile image
lighttable_t*        ds_colormap;
byte*                ds_source;

void R_DrawSpan (int sx1,int sx2,int sy)
{
    unsigned         position;
    unsigned         step;
    byte*	dest;
    unsigned	count;
    unsigned	spot;
    unsigned	xtemp;
    unsigned	ytemp;

    position = ((ds_xfrac << 10) & 0xffff0000) | ((ds_yfrac >> 6) & 0xffff);
    step = ((ds_xstep << 10) & 0xffff0000) | ((ds_ystep >> 6) & 0xffff);

    dest = (uint8_t*)(bufptr + ((viewwindowy + sy)*SCREENWIDTH) + (viewwindowx + sx1));

    count = sx2 - sx1 + 1;

    while (count >= 4)
    {
        ytemp = position >> 4;
        ytemp = ytemp & 4032;
        xtemp = position >> 26;
        spot = xtemp | ytemp;
        position += step;
        dest[0] = ds_colormap[ds_source[spot]];

        ytemp = position >> 4;
        ytemp = ytemp & 4032;
        xtemp = position >> 26;
        spot = xtemp | ytemp;
        position += step;
        dest[1] = ds_colormap[ds_source[spot]];

        ytemp = position >> 4;
        ytemp = ytemp & 4032;
        xtemp = position >> 26;
        spot = xtemp | ytemp;
        position += step;
        dest[2] = ds_colormap[ds_source[spot]];

        ytemp = position >> 4;
        ytemp = ytemp & 4032;
        xtemp = position >> 26;
        spot = xtemp | ytemp;
        position += step;
        dest[3] = ds_colormap[ds_source[spot]];

        count -= 4;
        dest += 4;
    }
    while (count > 0)
    {
        ytemp = position >> 4;
        ytemp = ytemp & 4032;
        xtemp = position >> 26;
        spot = xtemp | ytemp;
        position += step;
        *dest++ = ds_colormap[ds_source[spot]];
        count--;
    }
}

void R_DrawSpanLow (int sx1,int sx2,int sy)
{
    fixed_t          xfrac;
    fixed_t          yfrac;
    uint16_t*        dest;
    int              count;
    int              spot;

#ifdef RANGECHECK
    if (sx2 < sx1
    || sx1<0
    || sx2>=SCREENWIDTH
    || (unsigned)sy>SCREENHEIGHT)
    {
    I_Error( "R_DrawSpan: %i to %i at %i",sx1,sx2,sy);
    }
#endif

    count = sx2 - sx1;

    xfrac = ds_xfrac;
    yfrac = ds_yfrac;

    dest = (uint16_t*)(bufptr + ((viewwindowy + sy)*SCREENWIDTH) + viewwindowx + (sx1<<1));

    do
    {
        // Current texture index in u,v.
        spot = ((yfrac >> 10) & 4032) + ((xfrac >> 16) & 63);
        // Next step in u,v.
        xfrac += (ds_xstep);
        yfrac += (ds_ystep);

        // Lookup pixel from flat texture tile,
        //  re-index using light/colormap.
        // 16-bit write, double-pixel write with one store instruction
        uint16_t widespot = ds_colormap[ds_source[spot]];
        widespot |= (widespot << 8);
        *dest++ = widespot;
    } while (count--);
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
    int    i;

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
            translationtables[i] = translationtables[i+256] = translationtables[i+512] = i;
        }
    }
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
    int        i;
//    int        offset;

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
        viewwindowy = ((SCREENHEIGHT-(SBARHEIGHT)-height) >> 1);
    }

#if 0
    offset = ytab(viewwindowy);

    for (i=0 ; i<height ; i++)
    {
        ylookup[i] = offset;
        ylookup2[i] = ylookup[i] + viewwindowx;
        offset += SCREENWIDTH;
    }
#endif
}


//
// R_FillBackScreen
// Fills the back screen with a pattern
//  for variable screen sizes
// Also draws a beveled edge.
//

// pre-color-indexed back screen flat for easier R_VideoErase
static uint8_t srcp[64*64];
static int drew_bg_before=0;

//
// Copy a screen buffer.
//

void R_VideoErase ( unsigned ofs, int count )
{
//if(1)
//return;
//if(bufptr == 0) {
//I_Error("why is buffer null");
//}
    uint8_t* dst = (uint8_t *)((uintptr_t)bufptr + ((ofs)/*<<1*/));

    for (int i=0;i<count;i++)
    {
        *dst++ = srcp[((((ofs+i)/SCREENWIDTH)&63)<<6) + (((ofs+i)%SCREENWIDTH)&63)];
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
    int        top;
    int        side;
    int        ofs;
    int        i;
//if(1) return;
    patch_t*    patch;

    unsigned int x,y;

    if (scaledviewwidth == SCREENWIDTH)
    {
        return;
    }

    if (!drew_bg_before)
    {
        // DOOM border patch.
        char    name1[] = "FLOOR7_2";

        // DOOM II border patch.
        char    name2[] = "GRNROCK";

        char*    name;
        if ( gamemode == commercial)
        {
            name = name2;
        }
        else
        {
            name = name1;
        }

        byte* src = W_CacheLumpName (name, PU_CACHE);

//        for (i=0;i<64*64;i++)
//        {
//            srcp[i] = src[i];
//        }
        memcpy(srcp, src, 4096);

        Z_Free(src);

        drew_bg_before = 1;
    }

    top = (((SCREENHEIGHT-SBARHEIGHT)-viewheight)/2);

    side = (SCREENWIDTH-scaledviewwidth)/2;


    // copy top and one line of left side
    R_VideoErase (0, ytab(top)+side);

    // copy one line of right side and bottom
    ofs = ytab(viewheight+top)-side;
    R_VideoErase (ofs, ytab(top)+side);

    // copy sides using wraparound
    ofs = ytab(top) + SCREENWIDTH-side;
    side <<= 1;

    for (i=1 ; i<viewheight ; i++)
    {
        R_VideoErase (ofs, side);
        ofs += SCREENWIDTH;
    }

    patch = W_CacheLumpName ("brdr_t",PU_CACHE);
    for (x=0 ; x<scaledviewwidth ; x+=8)
    {
        V_DrawPatch (viewwindowx+x,viewwindowy-8,patch);
    }

    patch = W_CacheLumpName ("brdr_b",PU_CACHE);
    for (x=0 ; x<scaledviewwidth ; x+=8)
    {
        V_DrawPatch (viewwindowx+x,viewwindowy+viewheight,patch);
    }

    patch = W_CacheLumpName ("brdr_l",PU_CACHE);
    for (y=0 ; y<viewheight ; y+=8)
    {
        V_DrawPatch (viewwindowx-8,viewwindowy+y,patch);
    }

    patch = W_CacheLumpName ("brdr_r",PU_CACHE);
    for (y=0 ; y<viewheight ; y+=8)
    {
        V_DrawPatch (viewwindowx+scaledviewwidth,viewwindowy+y,patch);
    }

    // Draw beveled edge.
    V_DrawPatch (viewwindowx-8,
         viewwindowy-8,
         W_CacheLumpName ("brdr_tl",PU_CACHE));

    V_DrawPatch (viewwindowx+scaledviewwidth,
         viewwindowy-8,
         W_CacheLumpName ("brdr_tr",PU_CACHE));

    V_DrawPatch (viewwindowx-8,
         viewwindowy+viewheight,
         W_CacheLumpName ("brdr_bl",PU_CACHE));

    V_DrawPatch (viewwindowx+scaledviewwidth,
         viewwindowy+viewheight,
         W_CacheLumpName ("brdr_br",PU_CACHE));    
}
