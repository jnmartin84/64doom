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

extern int I_GetHeapSize(void);
extern int Z_FreeMemory(void);

extern int get_allocated_byte_count();
extern int get_null_free_count();
extern int get_unmapped_free_count();
extern uint32_t get_allocated_mus_memory();
extern uint32_t used_instrument_count;

extern void buffer_bresenham_line_5551(int x0, int y0, int x1, int y1, uint32_t color, uint16_t *buffer, int buf_w, int buf_h);
extern void buffer_dot_horiz_line_5551(int x1, int x2, int y, uint32_t fill, uint16_t *buf, int buf_w, int buf_h);
extern void buffer_dot_vert_line_5551(int x, int y1, int y2, uint32_t fill, uint16_t *buf, int buf_w, int buf_h);
extern void buffer_draw_char_5551(char ch, int x, int y, uint16_t *buffer, uint32_t color, int width, int height);
extern void buffer_fast_line_5551(int x, int y, int x2, int y2, uint32_t color, uint16_t *buf, int buf_w, int buf_h);
extern void buffer_horiz_line_5551(int x1, int x2, int y, uint32_t fill, uint16_t *buf, int buf_w, int buf_h);
extern void buffer_rect_5551(int x, int y, int w, int  h, uint32_t fill, uint16_t *buf, int buf_w, int buf_h);
extern void buffer_vert_line_5551(int x, int y1, int y2, uint32_t fill, uint16_t *buf, int buf_w, int buf_h);

extern void n64_free(void *buf);
extern void *n64_malloc(size_t size_to_alloc);
extern void *n64_memcpy(void *d, const void *s, size_t n);
extern void *n64_memmove(void *d, const void *s, size_t n);
extern void *n64_memset(void *p, int v, size_t n);

extern double get_elapsed_seconds();
extern unsigned long get_max_zone_used();
extern unsigned long get_max_allocated_mus_memory();

extern void *__safe_buffer[];

extern int last_save_size;
extern int last_x;
extern int last_y;
extern uint32_t shift;
extern uint32_t ytab[];
extern uint32_t y20tab[];


// function prototypes

void I_SetPalette(byte* palette);

void (*I_FinishUpdate)(void);
void rdp_renderer(void);
void renderer(void);
void debug_renderer(void);

void render_memory_usage_to_dc(display_context_t _dc);
void render_memory_usage(uint16_t *buffer);
void nearest_neighbor(uint8_t *output,uint8_t *input,int w1,int h1,int w2,int h2);
void optimized_bresenham_scale_image(uint8_t *output, uint8_t *input, int w1, int h1, int w2, int h2);


// module-local

static display_context_t _dc;
static int update_count = 0;

static char  __attribute__((aligned(8))) legend_string[256];
static uint32_t __attribute__((aligned(8))) colorbar[256];
static uint32_t __attribute__((aligned(8))) palarray[256];

static uint32_t BLACK_COL;
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

double average_fps = 0.0;
double instant_fps = -1.0;
double last_time;
int PROFILE_MEMORY = 0;
unsigned long last_tics;

int clear_screen = 0;


#define pack_16bit_colors_long_alt(c1,c2)    (c2 | (c1 << 16))


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


void renderer(void)
{
    uint32_t	*screen;
    uint16_t	*video_ptr;
    int		x;
    int		y;
    int		yprime;
    uint32_t	b1234;
    byte	b1,b2,b3,b4;
    uint32_t    *long_ptr;
#if 0
    uint32_t    long_color_12;
    uint32_t    long_color_34;
#endif

    update_count += 1;

    _dc = lockVideo(1);

    if (clear_screen)
    {
        rdp_sync(SYNC_PIPE);
        rdp_set_default_clipping();
        rdp_enable_primitive_fill();
        rdp_attach_display(_dc);
        rdp_sync(SYNC_PIPE);
        rdp_set_primitive_color(BLACK_COL);
        rdp_draw_filled_rectangle(0, 0, 320, 240);
        rdp_detach_display();

        clear_screen--;
    }

    for (y=0;y<200;y++)
    {
        yprime = ytab[y];

        video_ptr = &((uint16_t *)__safe_buffer[(_dc)-1])[y20tab[y]];
        screen = (uint32_t*)&(screens[0][yprime]);

        for (x=0;x<320;x+=4)
        {
            long_ptr = (uint32_t*)(&video_ptr[x]);

            b1234 = screen[x >> 2];
            b1 = (b1234 >> 24) & 0xFF;
            b2 = (b1234 >> 16) & 0xFF;
            b3 = (b1234 >>  8) & 0xFF;
            b4 = (b1234      ) & 0xFF;

#if 0
            long_color_12 = pack_16bit_colors_long_alt((palarray[b1] & 0xffff), (palarray[b2] & 0xffff));
            long_color_34 = pack_16bit_colors_long_alt((palarray[b3] & 0xffff), (palarray[b4] & 0xffff));

            long_ptr[0] = long_color_12;
            long_ptr[1] = long_color_34;
#endif

            long_ptr[0] = pack_16bit_colors_long_alt((palarray[b1] & 0xffff), (palarray[b2] & 0xffff));
            long_ptr[1] = pack_16bit_colors_long_alt((palarray[b3] & 0xffff), (palarray[b4] & 0xffff));
        }
    }

    unlockVideo(_dc);
}


void debug_renderer(void)
{
    uint32_t	*screen;
    uint16_t	*video_ptr;
    int		x;
    int		y;
    int		yprime;
    uint32_t	b1234;
    byte	b1,b2,b3,b4;
    uint32_t    *long_ptr;
    uint32_t    long_color_12;
    uint32_t    long_color_34;

    if(last_tics > 0)
    {
        instant_fps = (double)(COUNTS_PER_SECOND / (get_ticks() - last_tics));

        average_fps += instant_fps;
        average_fps /= 2.0;
    }

    last_tics = get_ticks();
    last_time = get_elapsed_seconds();

    update_count += 1;

    _dc = lockVideo(1);

    rdp_sync(SYNC_PIPE);
    rdp_set_default_clipping();
    rdp_enable_primitive_fill();
    rdp_attach_display(_dc);
    rdp_sync(SYNC_PIPE);
    rdp_set_primitive_color(BLACK_COL);
    rdp_draw_filled_rectangle(0, 0, 320, 20);
    rdp_detach_display();

    for (y=0;y<200;y++)
    {
        yprime = ytab[y];

        video_ptr = &((uint16_t *)__safe_buffer[(_dc)-1])[y20tab[y]];
        screen = (uint32_t*)&(screens[0][yprime]);

        for (x=0;x<320;x+=4)
        {
            long_ptr = (uint32_t*)(&video_ptr[x]);

            b1234 = screen[x >> 2];
            b1 = (b1234 >> 24) & 0xFF;
            b2 = (b1234 >> 16) & 0xFF;
            b3 = (b1234 >>  8) & 0xFF;
            b4 = (b1234      ) & 0xFF;

            uint16_t c1 = palarray[b1]; uint16_t c2 = palarray[b2];
            uint16_t c3 = palarray[b3]; uint16_t c4 = palarray[b4];
            long_color_12 = pack_16bit_colors_long_alt(c1,c2);
            long_color_34 = pack_16bit_colors_long_alt(c3,c4);

            long_ptr[0] = long_color_12;
            long_ptr[1] = long_color_34;
        }
    }

    if (PROFILE_MEMORY)
    {
        video_ptr = &((uint16_t *)__safe_buffer[(_dc)-1])[0];
        render_memory_usage(video_ptr);
    }

    unlockVideo(_dc);
}


void render_memory_usage_to_dc(display_context_t _dc)
{
    uint16_t *buffer = ((uint16_t *)__safe_buffer[(_dc)-1]);
    render_memory_usage(buffer);
}


/**
 * TOTAL MEMORY AVAILABLE	(0x800000)
 * TOTAL MEMORY USED		(memory.c:get_allocated_byte_count())
 * TOTAL ZONE ALLOCATED		(0x400000)
 * TOTAL ZONE USED		(i_system.c:I_GetHeapSize() - z_zone.c:Z_FreeMemory())
 */
void render_memory_usage(uint16_t *buffer)
{
    if(PROFILE_MEMORY)
    {
        int i;

        double eightmeg_double = 1048576.0 * 8.0;
        double total_allocated_scalar = (double)get_allocated_byte_count() / eightmeg_double;
        int total_allocated_len = 320 * total_allocated_scalar;

        double zone_allocated_scalar = (double)I_GetHeapSize() / eightmeg_double;
        int zone_allocated_len = 320 * zone_allocated_scalar;

        double zone_used_scalar = ((double)I_GetHeapSize() - (double)Z_FreeMemory()) / eightmeg_double;
        int zone_used_len = 320 * zone_used_scalar;

        double mus_used_scalar = (double)get_allocated_mus_memory() / eightmeg_double;
        int mus_used_len = 320 * mus_used_scalar;

        if (shift)
        {
            rdp_sync(SYNC_PIPE);
            rdp_set_default_clipping();
            rdp_enable_primitive_fill();
            rdp_attach_display(_dc);
            rdp_sync(SYNC_PIPE);
            rdp_set_primitive_color(TOTAL_AVAIL_COL);
            rdp_draw_filled_rectangle(0, 0, 320, 16);
            rdp_set_primitive_color(TOTAL_ALLOC_COL);
            rdp_draw_filled_rectangle(0, 1, total_allocated_len, 1 + 14);
            rdp_set_primitive_color(ZONE_ALLOC_COL);
            rdp_draw_filled_rectangle(0, 2, zone_allocated_len, 2 + 12);
            rdp_set_primitive_color(ZONE_USED_COL);
            rdp_draw_filled_rectangle(0, 3, zone_used_len, 3 + 10);
            rdp_set_primitive_color(MUS_USED_COL);
            rdp_draw_filled_rectangle(0, 4, mus_used_len, 4 + 8);
            rdp_detach_display();
        }
        else
        {
            buffer_rect_5551(0, 0, 320,                 16, TOTAL_AVAIL_COL, buffer, 320, 240);
            buffer_rect_5551(0, 1, total_allocated_len, 14, TOTAL_ALLOC_COL, buffer, 320, 240);
            buffer_rect_5551(0, 2, zone_allocated_len,  12, ZONE_ALLOC_COL,  buffer, 320, 240);
            buffer_rect_5551(0, 3, zone_used_len,       10, ZONE_USED_COL,   buffer, 320, 240);
            buffer_rect_5551(0, 4, mus_used_len,         8, MUS_USED_COL,    buffer, 320, 240);
        }

        for (i=0;i<320;i+=10)
        {
            if (i % 40 == 0)
            {
                buffer_vert_line_5551    (i, 0, 16, BLACK_COL, buffer, 320, 240);
            }
            else
            {
                buffer_dot_vert_line_5551(i, 0, 16, BLACK_COL, buffer, 320, 240);
            }
        }

        buffer_horiz_line_5551(0, 320,  0, BLACK_COL, buffer, 320, 240);
        buffer_horiz_line_5551(0, 320, 16, BLACK_COL, buffer, 320, 240);

        graphics_set_color(TOTAL_AVAIL_COL, BLACK_COL);
        sprintf(legend_string, "%s:%06lX", "RDRAM", (uint32_t)(1048576*8));
        graphics_draw_text(_dc,   0,  20, legend_string);

        graphics_set_color(TOTAL_ALLOC_COL, BLACK_COL);
        sprintf(legend_string, "%s:%06lX", "Alloc", (uint32_t)get_allocated_byte_count());
        graphics_draw_text(_dc,  98,  20, legend_string);

        graphics_set_color(MUS_USED_COL, BLACK_COL);
        sprintf(legend_string, "%s:%06lX", "MusUsed", (uint32_t)get_allocated_mus_memory());
        graphics_draw_text(_dc, 196,  20, legend_string);

        graphics_set_color(ZONE_ALLOC_COL, BLACK_COL);
        sprintf(legend_string, "%s:%06lX", "ZAlloc", (uint32_t)I_GetHeapSize());
        graphics_draw_text(_dc,   0,  30, legend_string);

        graphics_set_color(ZONE_USED_COL, BLACK_COL);
        sprintf(legend_string, "%s:%06lX", "ZUsed", (uint32_t)(I_GetHeapSize() - Z_FreeMemory()));
        graphics_draw_text(_dc, 106,  30, legend_string);

        graphics_set_color(MAX_ZONE_USED_COL, BLACK_COL);
        sprintf(legend_string, "%s:%06lX", "MAX_ZUsed", get_max_zone_used());
        graphics_draw_text(_dc, 106,  40, legend_string);

        graphics_set_color(MAX_MUS_USED_COL, BLACK_COL);
        sprintf(legend_string, "%s:%06lX", "MAX_MusUsed", get_max_allocated_mus_memory());
        graphics_draw_text(_dc, 106,  50, legend_string);

        graphics_set_color(UNMAP_FREE_COL, BLACK_COL);
        sprintf(legend_string, "%s:%06lX", "Unmapped_Free", (uint32_t)get_unmapped_free_count());
        graphics_draw_text(_dc, 106,  60, legend_string);

        graphics_set_color(USED_INSTR_COL, BLACK_COL);
        sprintf(legend_string, "%s:%06lX", "Used_Instruments", used_instrument_count);
        graphics_draw_text(_dc, 106,  70, legend_string);

        graphics_set_color(NULL_FREE_COL, BLACK_COL);
        sprintf(legend_string, "%s:%06lX", "NULL_Free", (uint32_t)get_null_free_count());
        graphics_draw_text(_dc, 106,  80, legend_string);

        graphics_set_color(NULL_FREE_COL, BLACK_COL);
        sprintf(legend_string, "%s:%06lX", "SHIFT", shift);
        graphics_draw_text(_dc,   0,  80, legend_string);

        graphics_set_color(NULL_FREE_COL, BLACK_COL);
        sprintf(legend_string, "%s:%.0f", "FPS", instant_fps);
        graphics_draw_text(_dc,   0,  100, legend_string);
        graphics_set_color(NULL_FREE_COL, BLACK_COL);
        sprintf(legend_string, "%s:%.0f", "FPS (avg)", average_fps);
        graphics_draw_text(_dc, 100,  100, legend_string);
    }
}


//
// I_ReadScreen
//
void I_ReadScreen(byte* scr)
{
    n64_memcpy(scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
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

    int r,g,b;
    int i;

    for (i = 0; i < 256; i++)
    {
        r = gammaptr[*palette++] & 0x000000FF;
        g = gammaptr[*palette++] & 0x000000FF;
        b = gammaptr[*palette++] & 0x000000FF;

        palarray[i] = graphics_make_color(r,g,b,0xFF);
    }
}


void I_InitGraphics(void)
{
    int i;

    n64_memset(palarray, 0, 256*sizeof(uint32_t));

    BLACK_COL         = graphics_make_color(  0,   0,   0,   0);
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

    for (i=0;i<256;i++)
    {
        char rgb[3];
        HEADER_PIXEL(dataPointer,rgb);
        colorbar[i] = graphics_make_color(rgb[0],rgb[1],rgb[2],0xFF);
    }
}


void DebugOutput_Hex(const int number)
{
    display_context_t _dc;
    char error_string[256];
    sprintf(error_string, "DEBUG: %08X\n", number);

    _dc = lockVideo(1);
    graphics_set_color(graphics_make_color(0xFF,0x00,0xFF,0x00), graphics_make_color(0x00,0x00,0x00,0x00));
    graphics_draw_box(_dc, 0, 16, 320, 24, graphics_make_color(0xFF,0x00,0xFF,0x00));
    graphics_draw_text(_dc, 0, 16, error_string);
    unlockVideo(_dc);
}


void DebugOutput_String(const char *str, int good)
{
    display_context_t _dc;

    _dc = lockVideo(1);
    graphics_set_color(graphics_make_color(0xFF,0xFF,0xFF,0x00), graphics_make_color(0x00,0x00,0x00,0x00));
    if (good)
    {
        graphics_draw_box(_dc, 0, 16, 320, 24, graphics_make_color(0x00,0x00,0xFF,0x00));
    }
    else
    {
        graphics_draw_box(_dc, 0, 16, 320, 24, graphics_make_color(0xFF,0x00,0x00,0x00));
    }
    graphics_draw_text(_dc, 0, 16, str);
    unlockVideo(_dc);
}


// algorithm from: http://tech-algorithm.com/articles/nearest-neighbor-image-scaling/
void nearest_neighbor(uint8_t *output, uint8_t *input, int w1, int h1, int w2, int h2)
{
    int i;
    int j;

    byte *inptr;
    byte *outptr;

    uint32_t *woutptr;

    int x_ratio = (int)((w1<<16)/w2) + 1;
    int y_ratio = (int)((h1<<16)/h2) + 1;

    int x2;
    int y2;

    int rat;

    byte b1,b2,b3,b4;

    for (i=0;i<h2;i++)
    {
        rat = 0;

        outptr = output + (i*w2);
        woutptr = (uint32_t*)outptr;

        y2 = ((i*y_ratio)>>16);

        inptr = input + (y2*w1);


        for (j=0;j<w2;j+=4)
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
