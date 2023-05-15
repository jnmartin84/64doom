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

// externs
extern uint16_t *buf16;

//extern short* pcmbuf;

// function prototypes
int global_do_invul = 0;
void I_SetPalette(byte* palette);

void I_FinishUpdate(void);


// globals

// this is intentionally uint32_t
// graphics_make_color will pack two 16-bit colors into a 32-bit word
// and we use that full word when rendering columns and spans in low-detail mode
// the color lookup is a single index into palarray
// no shifting and masking necessary to create a doubled pixel
// and it gets written to the 16-bit framebuffer as two pixels with a single 32-bit write
uint32_t __attribute__((aligned(8))) palarray[256];

// I don't want to rename this across the code base, it used to be display_context_t 
// I started this port with libdragon in 2014
// it did not expose a pointer to the buffer display_context_t was associated with
// and I had to use __safe_buffer[_dc-1] to access it directly
// I really like libdragon 9 years later though, much nicer
surface_t *_dc;

surface_t *lockVideo(int wait)
{
    surface_t *dc;

    if (wait)
    {
        while (!(dc = display_lock()));
    }
    else
    {
        dc = display_lock();
    }

    return dc;
}

void unlockVideo(surface_t *dc)
{
    if (dc)
    {
        display_show(dc);
    }
}


void I_ShutdownGraphics(void)
{
}


void I_UpdateNoBlit(void)
{
}

void I_FinishUpdate(void)
{
}

//
// I_ReadScreen
//
void I_ReadScreen(byte* scr)
{
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
	byte p1,p2,p3;
    byte r,g,b;
    unsigned int i;

    // this is ugly from a well-intentioned but probably pointless attempt to make it oPtImIzEd 
    for (i = 0; i < 768/12; i++) {

		uint32_t fc1 = *(uint32_t *)(&palette[i*12]);
		uint32_t fc2 = *(uint32_t *)(&palette[(i*12)+4]);
		uint32_t fc3 = *(uint32_t *)(&palette[(i*12)+8]);
		
		p1 = fc1 >> 24;
		p2 = fc1 >> 16;
		p3 = fc1 >> 8;
		
        r = gammaptr[p1];
        g = gammaptr[p2];
        b = gammaptr[p3];
        palarray[i*4] = graphics_make_color(r,g,b,0xff);

		p1 = fc1 & 0xff;
		p2 = fc2 >> 24;
		p3 = fc2 >> 16;
		
        r = gammaptr[p1];
        g = gammaptr[p2];
        b = gammaptr[p3];
        palarray[(i*4)+1] = graphics_make_color(r,g,b,0xff);

		p1 = fc2 >> 8;
		p2 = fc2 & 0xff;
		p3 = fc3 >> 24;
		
        r = gammaptr[p1];
        g = gammaptr[p2];
        b = gammaptr[p3];
        palarray[(i*4)+2] = graphics_make_color(r,g,b,0xff);

		p1 = fc3 >> 16;
		p2 = fc3 >> 8;
		p3 = fc3 & 0xff;
		
        r = gammaptr[p1];
        g = gammaptr[p2];
        b = gammaptr[p3];
        palarray[(i*4)+3] = graphics_make_color(r,g,b,0xff);
	}

	palette += 256*3;
}


void I_InitGraphics(void)
{
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE);
}


void DebugOutput_String_For_IError(const char *str, int lineNumber, int good)
{
    #define ERROR_LINE_LEN 32
    int error_string_length = strlen(str);
    int error_string_line_count = (error_string_length / ERROR_LINE_LEN) + 1;

    graphics_set_color(graphics_make_color(0xFF,0xFF,0xFF,0x00), graphics_make_color(0x00,0x00,0x00,0x00));

    for (int i=0;i<=error_string_line_count;i++)
    {
        if (!good)
        {
            graphics_draw_box(_dc, 18, 12+((lineNumber+i)*8), 284, 16, graphics_make_color(0xFF,0x00,0x00,0x00));
        }
        else
        {
            graphics_draw_box(_dc, 18, 12+((lineNumber+i)*8), 284, 16, graphics_make_color(0x00,0x00,0xFF,0x00));
        }
    }

    for (int i=0;i<=error_string_line_count;i++)
    {
        char copied_line[ERROR_LINE_LEN + 1] = {'\0'};
        if (0 == i)
        {
            strncpy(copied_line, "I_Error:", ERROR_LINE_LEN);
        }
        else
        {
            strncpy(copied_line, str + ((i-1)*ERROR_LINE_LEN), ERROR_LINE_LEN);
        }
        graphics_draw_text(_dc, 20, 16+((lineNumber+i)*8), copied_line);
    }
}
