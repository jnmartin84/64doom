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

#include "z_zone.h"
#include "w_wad.h"

#include "doomdef.h"

// externs
extern void* bufptr;
extern uint8_t *screens[2];

// function prototypes
void I_SetPalette(byte* palette);
void I_FinishUpdate(void);

// globals
int curscr = 0;

// locals
static surface_t* disp;
static uint16_t __attribute__((aligned(64))) current_palarray[256];

// functions

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


void I_StartFrame(void)
{
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
    //while( !(disp = display_get()) );
    disp = display_get();
    // Attach the RDP to the display buffer
    rdpq_attach_clear(disp, NULL);
    // we do all drawing to uncached view of bufptr
    //data_cache_hit_writeback(bufptr, SCREENWIDTH*SCREENHEIGHT);

    // Blit the surface onto the display
    surface_t src = surface_make(screens[curscr], FMT_CI8, SCREENWIDTH, SCREENHEIGHT, SCREENWIDTH);
    curscr = (curscr + 1) & 1;
    bufptr = screens[curscr];
    rdpq_tex_blit(&src, 0, 0, NULL);

    // Detach from the surface and display it once done
    rdpq_detach_show();
    return;
#endif
}

//
// I_ReadScreen
//
void I_ReadScreen(uint8_t* scr)
{
    memcpy(scr,bufptr,320*200);
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

        uint16_t col = ((r >> 3) << 11) | ((g >> 3) << 6) | ((b >> 3) << 1);
        current_palarray[i] = col;
    }

    // do the writeback here so we don't have to every time we submit the screen
    data_cache_hit_writeback(current_palarray, 256*2);
    // Load the palette
    rdpq_tex_load_tlut(current_palarray, 0, 256);
}

void I_SetDefaultPalette(void)
{
    I_SetPalette(W_CacheLumpName ("PLAYPAL",PU_CACHE));
}

void I_InitGraphics(void)
{
    console_clear();
    display_init((resolution_t) {
            .width = SCREENWIDTH,
            .height = SCREENHEIGHT,
            .interlaced = false,
        }, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE);

    rdpq_init();

    I_SetDefaultPalette();

    // use it uncached everywhere so we don't have to writeback every time we submit the screen
    screens[0] = (void*)((uintptr_t)screens[0] | 0xA0000000);
    screens[1] = (void*)((uintptr_t)screens[1] | 0xA0000000);

    curscr = 0;
    bufptr = screens[curscr];

    // Set copy render mode, with palette lookup
    rdpq_set_mode_copy(false);
    rdpq_mode_tlut(TLUT_RGBA16);

    printf("I_InitGraphics: Initialized display and RDPQ.\n");
}
