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
//
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdarg.h>
#include <sys/time.h>
#include <unistd.h>

#include "doomdef.h"
#include "m_misc.h"
#include "i_video.h"
#include "i_sound.h"

#include "d_net.h"
#include "g_game.h"

#ifdef __GNUG__
#pragma implementation "i_system.h"
#endif
#include "i_system.h"

extern surface_t* _dc;
extern void unlockVideo(surface_t* _dc);
extern surface_t* lockVideo(int i);

void DebugOutput_String_For_IError(const char *str, int lineNumber, int good);

volatile uint32_t timekeeping;

// freed up enough space through gc-sections to give 5.5 MB to zone
// this also allowed me to get screen wipe working again
#define MB_FOR_ZONE 5
#define BYTES_PER_MB 1048576
const size_t zone_size = MB_FOR_ZONE * BYTES_PER_MB;
int based_zone = 0;

void I_Tactile(int on, int off, int total)
{
    // UNUSED.
    on = off = total = 0;
}


ticcmd_t emptycmd;
inline ticcmd_t* I_BaseTiccmd(void)
{
    return &emptycmd;
}


int I_GetHeapSize(void)
{
    if (0 == based_zone)
    {
        return 0;
    }

    return zone_size;
}


byte* I_ZoneBase(int* size)
{
    based_zone = 1;

    *size = zone_size;

    byte *ptr = (byte *)malloc(*size);

    return ptr;
}


//
// I_GetTime
// returns time in 1/70th second tics
//
unsigned long I_GetTime(void)
{
    return timekeeping;
}


void tickercb(int ovfl) {
    timekeeping++;
}


//
// I_Init
//
void I_Init(void)
{
    I_InitSound();
    I_InitMusic();

    timer_init();
    timekeeping = 0;

    // 70 times per second
    // 93750000 / 70
    new_timer(1339286, TF_CONTINUOUS, tickercb);

    I_InitGraphics();
}

int return_from_D_DoomMain = 0;

//
// I_Quit
//
void I_Quit(void)
{
    timer_close();
    D_QuitNetGame();
    I_ShutdownSound();
    I_ShutdownMusic();
    M_SaveDefaults();
    I_ShutdownGraphics();
    return_from_D_DoomMain = 1;
}

void I_WaitVBL(int count)
{
    volatile uint32_t start = I_GetTime();
    while(((volatile uint32_t)I_GetTime() - start) < ((uint32_t)count)) {}
}


void I_BeginRead(void)
{
}


void I_EndRead(void)
{
}


byte* I_AllocLow(int length)
{
    byte* mem;

    mem = (byte *)malloc(length);

    return mem;
}


//
// I_Error
//
extern boolean demorecording;


void DebugOutput_String_For_IError(const char *str, int lineNumber, int good)
{
    #define ERROR_LINE_LEN 36
    int error_string_length = strlen(str);
    int error_string_line_count = (error_string_length / ERROR_LINE_LEN) + 1;

    graphics_set_color(graphics_make_color(0xFF,0xFF,0xFF,0x00), graphics_make_color(0x00,0x00,0x00,0x00));

    for (int i=0;i<=error_string_line_count;i++)
    {
        if (!good)
        {
            graphics_draw_box(_dc, 16, 12+((lineNumber+i)*8), display_get_width()-32, 16, graphics_make_color(0xFF,0x00,0x00,0x00));
        }
        else
        {
            graphics_draw_box(_dc, 16, 12+((lineNumber+i)*8), display_get_width()-32, 16, graphics_make_color(0x00,0x00,0xFF,0x00));
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
        graphics_draw_text(_dc, 16, 16+((lineNumber+i)*8), copied_line);
    }
}


void I_Error(const char *fmt, ...)
{   
    char errstr[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(errstr, sizeof(errstr), fmt, args);
    // in case we haven't reached I_InitGraphics yet
    printf("I_Error: %s\n", errstr);
#if 1
    unlockVideo(_dc);
    for(int i=0;i<2;i++)
    {
        _dc = lockVideo(1);
        DebugOutput_String_For_IError(errstr, 0, 0);
        unlockVideo(_dc);
    }    
#endif
    D_QuitNetGame();
    I_ShutdownMusic();
    I_ShutdownSound();
    I_ShutdownGraphics();

    while (1)
    {}
}