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
#include "colorbar1.h"


// externs
extern void *__n64_memcpy_ASM(void *d, const void *s, size_t n);
extern void *__n64_memset_ASM(void *p, int v, size_t n);
extern void *__n64_memset_ZERO_ASM(void *p, int v, size_t n);

extern void buffer_fast_line_5551(int x, int y, int x2, int y2, uint32_t color, uint16_t *buffer, int buf_w, int buf_h);
extern void buffer_dot_horiz_line_5551(int x1, int x2, int y, uint32_t fill, uint16_t *buf, int buf_w, int buf_h);
extern void buffer_horiz_line_5551(int x1, int x2, int y, uint32_t fill, uint16_t *buf, int buf_w, int buf_h);

extern void *__safe_buffer[];
extern uint16_t *buf16;

extern byte* big_pal;

extern short* pcmbuf;

// function prototypes
int global_do_invul = 0;
void I_SetPalette(byte* palette);

void I_FinishUpdate(void);


// globals
uint16_t __attribute__((aligned(8))) colorbar[256];
uint32_t __attribute__((aligned(8))) palarray[256];

display_context_t _dc;
static uint32_t BLACK_COL;
static uint32_t WHITE_COL;
uint32_t RED_COL;
uint32_t GREEN_COL;
uint32_t BLUE_COL;
static uint32_t TOTAL_AVAIL_COL;
static uint32_t TOTAL_ALLOC_COL;
static uint32_t ZONE_ALLOC_COL;
static uint32_t ZONE_USED_COL;
static uint32_t MUS_USED_COL;
static uint32_t MAX_ZONE_USED_COL;
static uint32_t MAX_MUS_USED_COL;
static uint32_t UNMAP_FREE_COL;
static uint32_t USED_INSTR_COL;
static uint32_t NULL_FREE_COL;


display_context_t lockVideo(int wait)
{
    display_context_t dc;

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

void unlockVideo(display_context_t dc)
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
    __n64_memcpy_ASM(scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
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

    unsigned int r,g,b;
    unsigned int i;

    for (i = 0; i < 256; i++)
    {
        r = gammaptr[*palette++];
        g = gammaptr[*palette++];
        b = gammaptr[*palette++];
        palarray[i] = graphics_make_color(r,g,b,0xFF);
    }
}


void I_InitGraphics(void)
{
    __n64_memset_ZERO_ASM(palarray, 0, 256*sizeof(uint32_t));
#if SCREENWIDTH == 320
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE);
#elif SCREENWIDTH == 640
    display_init(RESOLUTION_640x480, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE);
#endif
    BLACK_COL         = graphics_make_color(  0,   0,   0,   0);
    WHITE_COL         = graphics_make_color(255, 255, 255,   0);
    RED_COL           = graphics_make_color(255,   0,   0,   0);
    GREEN_COL         = graphics_make_color(  0, 255,   0,   0);
    BLUE_COL          = graphics_make_color(  0,   0, 255,   0);
    TOTAL_AVAIL_COL   = graphics_make_color( 64,  64,  64,   0);
    TOTAL_ALLOC_COL   = graphics_make_color(255, 255, 255,   0);
    ZONE_ALLOC_COL    = graphics_make_color(  0, 255,   0,   0);
    ZONE_USED_COL     = graphics_make_color(  0,   0, 255,   0);
    MUS_USED_COL      = graphics_make_color(255,   0,   0,   0);
    MAX_ZONE_USED_COL = graphics_make_color(255,   0, 255,   0);
    MAX_MUS_USED_COL  = graphics_make_color(255, 255,   0,   0);
    UNMAP_FREE_COL    = graphics_make_color(  0, 255, 255,   0);
    USED_INSTR_COL    = graphics_make_color( 63, 127, 255,   0);
    NULL_FREE_COL     = graphics_make_color(255, 127,  63,   0);

    char *dataPointer = colorbar_data;

    for (int i=0;i<256;i++)
    {
        char rgb[3];
        HEADER_PIXEL(dataPointer,rgb);
        colorbar[i] = graphics_make_color(rgb[0],rgb[1],rgb[2],0xFF);
    }
}


void DebugOutput_Hex(const int number)
{
    char error_string[256];
    sprintf(error_string, "DEBUG: %08X\n", number);

    graphics_set_color(graphics_make_color(0xFF,0x00,0xFF,0x00), graphics_make_color(0x00,0x00,0x00,0x00));
    graphics_draw_box(_dc, 0, 16, 320, 24, graphics_make_color(0xFF,0x00,0xFF,0x00));
    graphics_draw_text(_dc, 0, 16, error_string);
}


void DoNothing (void)
{
}


void DebugOutput_String(const char *str, int good)
{
    graphics_set_color(graphics_make_color(0xFF,0xFF,0xFF,0x00), graphics_make_color(0x00,0x00,0x00,0x00));
    if(good)
    {
        graphics_draw_box(_dc, 0, 16, 320, 24, graphics_make_color(0x00,0x00,0xFF,0x00));
    }
    else
    {
        graphics_draw_box(_dc, 0, 16, 320, 24, graphics_make_color(0xFF,0x00,0x00,0x00));
    }
    graphics_draw_text(_dc, 0, 16, str);
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
            graphics_draw_box(_dc, 18, 12+((lineNumber+i)*8), 284, 16/*(16+8)+((lineNumber+i)*8)*/, graphics_make_color(0xFF,0x00,0x00,0x00));
        }
        else
        {
            graphics_draw_box(_dc, 18, 12+((lineNumber+i)*8), 284, 16/*(16+8)+((lineNumber+i)*8)*/, graphics_make_color(0x00,0x00,0xFF,0x00));
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


// because I don't want to lose this code again, I'm leaving it here
void render_sound_buffer_like_rcs(uint16_t *buffer)
{
    short *sound_buffer = pcmbuf;

    int first;
    int second;
    int third;
    int fourth;

    int min_sampleL = 65536;
    int max_adjusted_sampleL = -1;

    int min_sampleR = 65536;
    int max_adjusted_sampleR = -1;

    int x;
    int xL;
    int xR;
    for (xL = 0,xR=1; xL < 316*2; xL+=2,xR+=2)
    {
        if ( (sound_buffer[xL] + 32768) < min_sampleL)
        {
            min_sampleL = (sound_buffer[xL] + 32768);
        }
        if ( (sound_buffer[xR] + 32768) < min_sampleR)
        {
            min_sampleR = (sound_buffer[xR] + 32768);
        }
    }
    for (xL = 0,xR=1; xL < 316*2; xL+=2,xR+=2)
    {
        if ( ((sound_buffer[xL] + 32768) - min_sampleL) > max_adjusted_sampleL)
        {
            max_adjusted_sampleL = ((sound_buffer[xL] + 32768) - min_sampleL);
        }
        if ( ((sound_buffer[xR] + 32768) - min_sampleR) > max_adjusted_sampleR)
        {
            max_adjusted_sampleR = ((sound_buffer[xR] + 32768) - min_sampleR);
        }
    }

    // div by zero avoidance
    if (0 == max_adjusted_sampleL)
    {
        max_adjusted_sampleL = 1;
    }
    if (0 == max_adjusted_sampleR)
    {
        max_adjusted_sampleR = 1;
    }


    buffer_fast_line_5551(0,94-16-32-1,320,94-16-32-1,  BLACK_COL,buffer,320,240);
    buffer_fast_line_5551(0,94-16-32,320,94-16-32,  BLACK_COL,buffer,320,240);
    buffer_fast_line_5551(0,94-16-32+1,320,94-16-32+1,  BLACK_COL,buffer,320,240);

    buffer_fast_line_5551(0,94-16-32,320,94-16-32,  TOTAL_ALLOC_COL,buffer,320,240);

    buffer_fast_line_5551(0,94-16+32-1,320,94-16+32-1,  BLACK_COL,buffer,320,240);
    buffer_fast_line_5551(0,94-16+32,320,94-16+32,  BLACK_COL,buffer,320,240);
    buffer_fast_line_5551(0,94-16+32+1,320,94-16+32+1,  BLACK_COL,buffer,320,240);

    buffer_fast_line_5551(0,94-16+32,320,94-16+32,  TOTAL_ALLOC_COL,buffer,320,240);

    buffer_dot_horiz_line_5551(0, 320, 94-16-16, colorbar[255], buffer, 320, 240);
    buffer_dot_horiz_line_5551(0, 320, 94-16+16, colorbar[0], buffer, 320, 240);

    buffer_horiz_line_5551(0, 320, 94-16-1, BLACK_COL, buffer, 320, 240);
    buffer_horiz_line_5551(0, 320, 94-16, BLACK_COL, buffer, 320, 240);
    buffer_horiz_line_5551(0, 320, 94-16+1, BLACK_COL, buffer, 320, 240);

    buffer_dot_horiz_line_5551(0, 320, 94-16, TOTAL_ALLOC_COL, buffer, 320, 240);

    buffer_fast_line_5551(0,180-16-32-1,320,180-16-32-1, BLACK_COL,buffer,320,240);
    buffer_fast_line_5551(0,180-16-32,320,180-16-32, BLACK_COL,buffer,320,240);
    buffer_fast_line_5551(0,180-16-32+1,320,180-16-32+1, BLACK_COL,buffer,320,240);

    buffer_fast_line_5551(0,180-16-32,320,180-16-32, TOTAL_ALLOC_COL,buffer,320,240);

    buffer_fast_line_5551(0,180-16+32-1,320,180-16+32-1, BLACK_COL,buffer,320,240);
    buffer_fast_line_5551(0,180-16+32,320,180-16+32, BLACK_COL,buffer,320,240);
    buffer_fast_line_5551(0,180-16+32+1,320,180-16+32+1, BLACK_COL,buffer,320,240);

    buffer_fast_line_5551(0,180-16+32,320,180-16+32, TOTAL_ALLOC_COL,buffer,320,240);

    buffer_dot_horiz_line_5551(0, 320, 180-16-16, colorbar[255], buffer, 320, 240);
    buffer_dot_horiz_line_5551(0, 320, 180-16+16, colorbar[0], buffer, 320, 240);

    buffer_horiz_line_5551(0, 320, 180-16-1, BLACK_COL, buffer, 320, 240);
    buffer_horiz_line_5551(0, 320, 180-16, BLACK_COL, buffer, 320, 240);
    buffer_horiz_line_5551(0, 320, 180-16+1, BLACK_COL, buffer, 320, 240);

    buffer_dot_horiz_line_5551(0, 320, 180-16, TOTAL_ALLOC_COL, buffer, 320, 240);

    for (x = 0; x < 315; x++)
    {
        // x == 0
        // first  is (((0  )*2)  ) == 0
        // second is (((0+1)*2)  ) == 2
        // third is  (((0  )*2)+1) == 1
        // fourth is (((0+1)*2)+1) == 3
	first  = (unsigned int)( sound_buffer[((x << 1)    )    ] + 32768) - min_sampleL;
	second = (unsigned int)( sound_buffer[((x + 1) << 1)    ] + 32768) - min_sampleL;
	third  = (unsigned int)( sound_buffer[((x << 1)    ) + 1] + 32768) - min_sampleR;
	fourth = (unsigned int)( sound_buffer[((x + 1) << 1) + 1] + 32768) - min_sampleR;

        // (value / 65535.0) * 255.0
        uint16_t sample1 = ( ((double) first / (double)max_adjusted_sampleL) * (double)250.0);
        uint16_t sample2 = ( ((double)second / (double)max_adjusted_sampleL) * (double)250.0);
        uint16_t sample3 = ( ((double) third / (double)max_adjusted_sampleR) * (double)250.0);
        uint16_t sample4 = ( ((double)fourth / (double)max_adjusted_sampleR) * (double)250.0);

        // ranges 0 - 255, need to scale down to 0 - 32, so divide by 8
        // also clamp final y coords in case they fall outside of screen borders
        int flag_clamp1 = 0;
        int flag_clamp2 = 0;
        int flag_clamp3 = 0;
        int flag_clamp4 = 0;

        uint8_t y1 =  94 - (sample1>>3);
        if (y1 < 94-32) { y1 = 94-32; flag_clamp1 = 1; } if (y1 > 94+32) { y1 = 94+32; flag_clamp1 = 2; }
        uint8_t y2 =  94 - (sample2>>3);
        if (y2 < 94-32) { y2 = 94-32; flag_clamp2 = 1; } if (y2 > 94+32) { y2 = 94+32; flag_clamp2 = 2; }
        uint8_t y3 = 180 - (sample3>>3);
        if (y3 < 180-32) { y3 = 180-32; flag_clamp3 = 1; } if (y3 > 180+32) { y3 = 180+32; flag_clamp3 = 2; }
        uint8_t y4 = 180 - (sample4>>3);
        if (y4 < 180-32) { y4 = 180-32; flag_clamp4 = 1; } if (y4 > 180+32) { y4 = 180+32; flag_clamp4 = 2; }

	uint8_t yma = y1 + ((y2 - y1) / 2);
	uint8_t ymb = y3 + ((y4 - y3) / 2);

        uint32_t col1;
	uint32_t col2;
        uint32_t col3;
	uint32_t col4;
        col1 = colorbar[sample1];
        col2 = colorbar[sample2];
        col3 = colorbar[sample3];
        col4 = colorbar[sample4];
        if(flag_clamp1 == 1) col1 = TOTAL_ALLOC_COL; else if (flag_clamp1 == 2) col1 = MAX_ZONE_USED_COL;
        if(flag_clamp2 == 1) col2 = TOTAL_ALLOC_COL; else if (flag_clamp2 == 2) col2 = MAX_ZONE_USED_COL;
        if(flag_clamp3 == 1) col3 = TOTAL_ALLOC_COL; else if (flag_clamp3 == 2) col3 = MAX_ZONE_USED_COL;
        if(flag_clamp4 == 1) col4 = TOTAL_ALLOC_COL; else if (flag_clamp4 == 2) col4 = MAX_ZONE_USED_COL;

#if 0
        buffer_bresenham_line_5551(1+x, y1,  2+x, yma, col1, buffer, 320, 240);
        buffer_bresenham_line_5551(1+x, yma,  2+x, y2, col2, buffer, 320, 240);
        buffer_bresenham_line_5551(1+x, y3,  2+x, ymb, col3, buffer, 320, 240);
        buffer_bresenham_line_5551(1+x, ymb,  2+x, y4, col4, buffer, 320, 240);
#endif
#if 1
        buffer_fast_line_5551(1+x,  y1, 2+x, yma, col1, buffer, 320, 240);
        buffer_fast_line_5551(1+x, yma, 2+x,  y2, col2, buffer, 320, 240);
        buffer_fast_line_5551(1+x,  y3, 2+x, ymb, col3, buffer, 320, 240);
        buffer_fast_line_5551(1+x, ymb, 2+x,  y4, col4, buffer, 320, 240);
#endif
    }
}
