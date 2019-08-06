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

//#define SCALE 1
#define SCREENW 320

// externs

extern int detailshift;

extern void dma_and_copy(void *buf, int count, int ROM_base_address, int current_ROM_seek);

extern int I_GetHeapSize(void);
extern int Z_FreeMemory(void);

extern int get_allocated_byte_count();
extern int get_null_free_count();
extern int get_unmapped_free_count();
extern uint32_t get_allocated_mus_memory();
extern uint32_t used_instrument_count;

extern void *__n64_memcpy_ASM(void *d, const void *s, size_t n);
extern void *__n64_memset_ASM(void *p, int v, size_t n);
extern void *__n64_memset_ZERO_ASM(void *p, int v, size_t n);

extern double get_elapsed_seconds();
extern unsigned long get_max_allocated_mus_memory();

extern void *__safe_buffer[];

extern float min_sample;
extern float max_sample;
extern int last_save_size;
extern int last_x;
extern int last_y;
extern int pcmflip;
extern uint32_t shift;
extern short pcmout1[];
extern short pcmout2[];
extern uint32_t ytab[];
extern uint32_t y20tab[];

// function prototypes

uint32_t color_from_sample(double h);

void I_SetPalette(byte* palette);

void I_FinishUpdate(void);

void nearest_neighbor(uint8_t *output,uint8_t *input,int w1,int h1,int w2,int h2);
void optimized_bresenham_scale_image(uint8_t *output, uint8_t *input, int w1, int h1, int w2, int h2);

// module-local

static int update_count = 0;

static char  __attribute__((aligned(8))) legend_string[256];
static uint8_t __attribute__((aligned(8))) tmpscreen[SCREENW*240];
static uint32_t __attribute__((aligned(8))) colorbar[256];

static uint32_t BLACK_COL;
static uint32_t WHITE_COL;
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

// globals
uint32_t __attribute__((aligned(8))) palarray[256];
uint32_t RED_COL;
uint32_t GREEN_COL;
uint32_t BLUE_COL;

display_context_t _dc;

double instant_fps = -1.0;
double average_fps = 0.0;
int PROFILE_MEMORY = 0;


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
    printf("asdfg");
}


void I_UpdateNoBlit(void)
{
}

#define pack_16bit_colors_long(c1,c2) ( (c1 << 16) | (c2 & 0x0000FFFF) )

#define pack_16bit_colors_long_alt(c1,c2)    (c2 | (c1 << 16))

unsigned long last_tics;
double last_time;


//
// I_FinishUpdate
//
static int dri;
static uint16_t *dri_video_ptr;

void I_FinishUpdate(void)
{
#ifdef SCALE
	dri_video_ptr = &((uint16_t *)__safe_buffer[(_dc)-1])[0]; // start at 0 320-pixel lines into the framebuffer
#endif
#ifndef SCALE
	dri_video_ptr = &((uint16_t *)__safe_buffer[(_dc)-1])[SCREENW*20]; // start at 20 320-pixel lines into the framebuffer
#endif

#ifdef SCALE
//	nearest_neighbor(tmpscreen, screens[0], SCREENW, 200, SCREENW, 240);
	optimized_bresenham_scale_image(tmpscreen, screens[0], SCREENW, 200, SCREENW, 240);
#endif

#ifdef SCALE
    for(dri=0;dri<SCREENW*240;dri++)
    {
        dri_video_ptr[dri] = palarray[tmpscreen[dri]];
    }
#endif
#ifndef SCALE
    for(dri=0;dri<SCREENW*200;dri++)
    {
        dri_video_ptr[dri] = palarray[screens[0][dri]];
    }
#endif
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


extern byte* big_pal;


void I_ForcePaletteUpdate(void)
{
    if (0 != big_pal)
    {
        I_SetPalette(big_pal);
    }
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
        r = gammaptr[*palette++];// & 0x000000FF;
        g = gammaptr[*palette++];// & 0x000000FF;
        b = gammaptr[*palette++];// & 0x000000FF;

////        for grayscale
//        int d = sqrt((r*r + g*g + b*b) / 9);
//        palarray[i] = graphics_make_color(d,d,d,0xFF);
////        or 16-bit color, just do
        palarray[i] = graphics_make_color(r,g,b,0xFF);
    }
}


void I_InitGraphics(void)
{
    int i;

    __n64_memset_ZERO_ASM(palarray, 0, 256*sizeof(uint32_t));

    BLACK_COL         = graphics_make_color(  0,   0,   0,   0);
    // duplicate of TOTAL_ALLOC_COL ...
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

    for(i=0;i<256;i++)
    {
        char rgb[3];
        HEADER_PIXEL(dataPointer,rgb);
        colorbar[i] = graphics_make_color(rgb[0],rgb[1],rgb[2],0xFF);
    }
}


void DebugOutput_Hex(const int number)
{
//    display_context_t _dc;
    char error_string[256];
    sprintf(error_string, "DEBUG: %08X\n", number);

//    _dc = lockVideo(1);
    graphics_set_color(graphics_make_color(0xFF,0x00,0xFF,0x00), graphics_make_color(0x00,0x00,0x00,0x00));
    graphics_draw_box(_dc, 0, 16, 320, 24, graphics_make_color(0xFF,0x00,0xFF,0x00));
    graphics_draw_text(_dc, 0, 16, error_string);
//    unlockVideo(_dc);
}


void DoNothing (void)
{
}


void DebugOutput_String(const char *str, int good)
{
//    display_context_t _dc;

//    _dc = lockVideo(1);
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
//    unlockVideo(_dc);
}


void DebugOutput_String_For_IError(const char *str, int lineNumber, int good)
{
#define ERROR_LINE_LEN 32
    int error_string_length = strlen(str);
    int error_string_line_count = (error_string_length / ERROR_LINE_LEN) + 1;

//    display_context_t _dc;

//    _dc = lockVideo(1);
    graphics_set_color(graphics_make_color(0xFF,0xFF,0xFF,0x00), graphics_make_color(0x00,0x00,0x00,0x00));

    for(int i=0;i<=error_string_line_count;i++)
    {
        if(!good) {
            graphics_draw_box(_dc, 18, 12+((lineNumber+i)*8), 284, 16/*(16+8)+((lineNumber+i)*8)*/, graphics_make_color(0xFF,0x00,0x00,0x00));
        }
        else {
            graphics_draw_box(_dc, 18, 12+((lineNumber+i)*8), 284, 16/*(16+8)+((lineNumber+i)*8)*/, graphics_make_color(0x00,0x00,0xFF,0x00));
        }
    }

    for(int i=0;i<=error_string_line_count;i++)
    {
//        if(
        char copied_line[ERROR_LINE_LEN + 1] = {'\0'};
        if(0 == i)
        {
            strncpy(copied_line, "I_Error:", ERROR_LINE_LEN);
        }
        else
        {
            strncpy(copied_line, str + ((i-1)*ERROR_LINE_LEN), ERROR_LINE_LEN);
        }
/*        for(int abc=0;abc<ERROR_LINE_LEN;abc++)
        {
            copied_line[abc] = str[abc+(i*ERROR_LINE_LEN)];
        }*/

//        if(good)
//        {
//            graphics_draw_box(_dc, 18, 12+((lineNumber+i)*8), 284, 12/*(16+8)+((lineNumber+i)*8)*/, graphics_make_color(0x00,0x00,0xFF,0x00));
//        }
//        else
//        {
//            graphics_draw_box(_dc, 18, 12+((lineNumber+i)*8), 284, 12/*(16+8)+((lineNumber+i)*8)*/, graphics_make_color(0xFF,0x00,0x00,0x00));
//        }

        graphics_draw_text(_dc, 20, 16+((lineNumber+i)*8), copied_line);
    }

//    unlockVideo(_dc);
}


// algorithm from: http://tech-algorithm.com/articles/nearest-neighbor-image-scaling/
void nearest_neighbor(uint8_t *output, uint8_t *input, int w1, int h1, int w2, int h2)
{
    byte *inptr;
    byte *outptr;

    uint32_t *woutptr;

    int x_ratio = (int)((w1<<16)/w2) + 1;
    int y_ratio = (int)((h1<<16)/h2) + 1;

    int x2;
    int y2;

    int rat;

    byte b1,b2,b3,b4;

    for (int i=0;i<h2;i++)
    {
        rat = 0;

        outptr = output + (i*w2);
        woutptr = (uint32_t*)outptr;

        y2 = ((i*y_ratio)>>16);

	inptr = input + (y2*w1);


        for (int j=0;j<w2;j+=4)
        {
            x2 = (rat >> 16);
            b1 = inptr[x2];
            rat += x_ratio;

            x2 = (rat >> 16);
            b2 = inptr[x2];
            rat += x_ratio;

            x2 = (rat >> 16);
            b3 = inptr[x2];
            rat += x_ratio;

            x2 = (rat >> 16);
            b4 = inptr[x2];
            rat += x_ratio;

            *woutptr++ = (b1 << 24) | (b2 << 16) | (b3 << 8) | b4;
        }
    }
}


// algorithm from: http://willperone.net/Code/codescaling.php
void optimized_bresenham_scale_image(uint8_t *output, uint8_t *input, int w1, int h1, int w2, int h2)
{
    int YD = (h1 / h2) * w1 - w1;
    int YR = h1 % h2;
    int XD = w1 / w2;
    int XR = w1 % w2;

    int outOffset	= 0;
    int inOffset	= 0;

    int y;
    int x;
    int XE;
    int YE;

    for (y = h2, YE = 0; y > 0; y--)
    {
        for (x = w2, XE = 0; x > 0; x--)
        {
            output[outOffset++] = input[inOffset];
            inOffset += XD;
            XE += XR;

            if (XE >= w2)
            {
                XE -= w2;
                inOffset++;
            }
        }

        inOffset += YD;

        YE += YR;

        if (YE >= h2)
        {
            YE -= h2;
            inOffset += w1;
        }
    }
}
