// Emacs style mode select   -*- C++ -*- h
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//	DOOM graphics stuff for Nintendo 64, libdragon
//
//-----------------------------------------------------------------------------

#include <libdragon.h>
#include <stdlib.h>
#include <math.h>

#include "doomstat.h"
#include "i_system.h"
#include "v_video.h"
#include "m_argv.h"
#include "d_main.h"

#include "doomdef.h"

// function prototypes

void I_SetPalette(byte* palette);

void I_FinishUpdate(void);


// globals

// I don't want to rename this across the code base, it used to be display_context_t 
// I started this port with libdragon in 2014
// it did not expose a pointer to the buffer display_context_t was associated with
// and I had to use __safe_buffer[_dc-1] to access it directly
// I really like libdragon 9 years later though, much nicer
surface_t *_dc;

surface_t *lockVideo(int wait)
{
    if (wait)
    {
        return display_get();
    }
    else
    {
        return display_try_get();
    }
}

void unlockVideo(surface_t *dc)
{
    if (dc)
    {
        display_show(dc);
    }
}

extern void* bufptr;

void I_StartFrame(void)
{
    _dc = lockVideo(1);
    // get the buffer address pointer from the surface once per frame instead of per every column/span
    bufptr = (void*)_dc->buffer;
}

void I_ShutdownGraphics(void)
{
}


void I_UpdateNoBlit(void)
{
}

void I_FinishUpdate(void)
{
    unlockVideo(_dc);
}
extern void* bufptr;
//
// I_ReadScreen
//
void I_ReadScreen(uint16_t* scr)
{
    memcpy(scr,bufptr,320*200*2);
}

//
// Palette stuff.
//


void I_ForcePaletteUpdate(void)
{
}


//
// I_SetPalette
//
// this is intentionally uint32_t
// graphics_make_color will pack two 16-bit colors into a 32-bit word
// and we use that full word when rendering columns and spans in low-detail mode
// the color lookup is a single index into palarray
// no shifting and masking necessary to create a doubled pixel
// and it gets written to the 16-bit framebuffer as two pixels with a single 32-bit write
uint32_t*  palarray;
static uint32_t __attribute__((aligned(8))) default_palarray[256];
static uint32_t __attribute__((aligned(8))) current_palarray[256];
static uint32_t *old_palarray = NULL;

void I_SavePalette(void)
{
    old_palarray = current_palarray;
    palarray = default_palarray;
}

void I_RestorePalette(void)
{
    palarray = current_palarray;
    old_palarray = default_palarray;
}

void I_SetPalette(byte* palette)
{
    const byte *gammaptr = gammatable[usegamma];

    unsigned int i;
    
    for (i = 0; i < 256; i++)
    {
        int r = *palette++;
        int g = *palette++;
        int b = *palette++;

        r = gammaptr[r];
        g = gammaptr[g];
        b = gammaptr[b];

        uint16_t unpackedcol = ((r >> 3) << 11) | ((g >> 3) << 6) | ((b >> 3) << 1);
        uint32_t packedcol = (unpackedcol << 16) | unpackedcol;
        current_palarray[i] = packedcol;
    }
}

#include "z_zone.h"
#include "w_wad.h"
void I_SetDefaultPalette(void)
{
    I_SetPalette(W_CacheLumpName ("PLAYPAL",PU_CACHE));
    memcpy(default_palarray, current_palarray, sizeof(current_palarray));
}

void I_InitGraphics(void)
{
    display_init( (resolution_t)
	{
		.width = SCREENWIDTH,
		.height = SCREENHEIGHT,
		.interlaced = false,
	}, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE );

    I_SetDefaultPalette();

    palarray = current_palarray;
}