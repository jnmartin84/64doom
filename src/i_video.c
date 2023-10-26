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
extern void* bufptr;
uint8_t *screens[5];
//uint8_t __attribute__((aligned(64))) screen[0];//320*200];
uint16_t*  palarray;

// function prototypes

void I_SetPalette(byte* palette);

void I_FinishUpdate(void);

int display_ready = 0;

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

static    surface_t *disp;
surface_t *_surface;

void I_StartFrame(void)
{
//if(!display_ready) return;
//    _dc = lockVideo(1);
    // get the buffer address pointer from the surface once per frame instead of per every column/span
}

void I_ShutdownGraphics(void)
{
}


void I_UpdateNoBlit(void)
{
}


void I_FinishUpdate(void)
{
#if 1
if(!display_ready) return;
    while( !(disp = display_get()) );
    // Attach the RDP to the display buffer
    rdpq_attach_clear(disp, NULL);

//    data_cache_hit_writeback(screens[0], 320*200);

    // Load the palette
    rdpq_tex_load_tlut(palarray, 0, 256);

    // Set copy render mode, with palette lookup
    rdpq_set_mode_copy(false);
    rdpq_mode_tlut(TLUT_RGBA16);

    // Blit the surface onto the display
    surface_t src = surface_make(bufptr, FMT_CI8, 320, 200, 320);
    rdpq_tex_blit(&src, 0, 0, NULL);

    // Detach from the surface and display it once done
    rdpq_detach_show();
    return;
#endif

_surface = lockVideo(1);
#if 0
        // Nintendo 64 frame buffer, 64-bit pointer to 20th row
        uint64_t *dst64 = (uint64_t *)((uintptr_t)_surface->buffer + (uintptr_t)12800);
        // OpenTyrian frame buffer, 32-bit pointer to 1st row
        uint32_t *src32 = (uint32_t *)VGAScreen;

        // 64000 8-bit pixels
        // /
        // 16 pixels per loop
        // =
        // 4000 iterations
        for (uint n=0;n<4000;n++)
        {
                // read 4 pixels from 8-bit frame buffer
                uint32_t src_four1 = *src32++;
                // read 4 more pixels from 8-bit frame buffer
                uint32_t src_four2 = *src32++;
                // read 4 more pixels from 8-bit frame buffer
                uint32_t src_four3 = *src32++;
                // read 4 more pixels from 8-bit frame buffer
                uint32_t src_four4 = *src32++;
                // color index first two pixels to get 2 16-bit pixels
                uint32_t src12 = tworgb_palette[(src_four1 >> 16)];
                // color index second two pixels to get 2 more 16-bit pixels
                uint32_t src34 = tworgb_palette[(src_four1 & 0xffff)];
                // write 4 16-bit pixels to 16-bit frame buffer
                *dst64++ = (uint64_t)(((uint64_t)src12<<32)|(uint64_t)src34);
                // color index two more pixels to get 2 16-bit pixels
                src12 = tworgb_palette[(src_four2 >> 16)];
                // color index two more pixels to get 2 16-bit pixels
                src34 = tworgb_palette[(src_four2 & 0xffff)];
                // write 4 more 16-bit pixels to 16-bit frame buffer
                *dst64++ = (uint64_t)(((uint64_t)src12<<32)|(uint64_t)src34);
                // color index two more pixels to get 2 16-bit pixels
                src12 = tworgb_palette[(src_four3 >> 16)];
                // color index two more pixels to get 2 16-bit pixels
                src34 = tworgb_palette[(src_four3 & 0xffff)];
                // write 4 more 16-bit pixels to 16-bit frame buffer
                *dst64++ = (uint64_t)(((uint64_t)src12<<32)|(uint64_t)src34);
                // color index two more pixels to get 2 16-bit pixels
                src12 = tworgb_palette[(src_four4 >> 16)];
                // color index two more pixels to get 2 16-bit pixels
                src34 = tworgb_palette[(src_four4 & 0xffff)];
                // write 4 more 16-bit pixels to 16-bit frame buffer
                *dst64++ = (uint64_t)(((uint64_t)src12<<32)|(uint64_t)src34);
        }
#endif
for(int y=0;y<200;y++) {
for(int x=0;x<320;x++) {
((uint16_t*)_surface->buffer)[(y*320)+x] = palarray[screens[0][(y*320)+x]];
}
}
unlockVideo(_surface);
}

//
// I_ReadScreen
//
void I_ReadScreen(uint8_t* scr)
{
    memcpy(scr,bufptr,320*200);//*2);
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
//uint32_t*  palarray;
static uint16_t __attribute__((aligned(64))) default_palarray[256];
static uint16_t __attribute__((aligned(64))) current_palarray[256];
static uint16_t *old_palarray = NULL;

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
        //uint32_t packedcol = (unpackedcol << 16) | unpackedcol;
        current_palarray[i] = unpackedcol;
    }

    data_cache_hit_writeback(current_palarray, 256*2);
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
        rdpq_init();
//        rdpq_debug_start();

    I_SetDefaultPalette();

    palarray = current_palarray;
display_ready = 1;
printf("I_InitGraphics: Initialized display and RDPQ.\n");
    bufptr = (void*)((uintptr_t)screens[0] | 0xA0000000);//s[0];//(void*)_dc->buffer;

}
