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


extern void DebugOutput_String(const char *str, int good_or_bad);
extern void *n64_malloc(size_t size_to_alloc);
extern void *n64_memset(void *p, int v, size_t n);


unsigned long change_ticrate = TICRATE;

int	mb_used = 4;
int	based_zone = 0;


void I_Tactile(int on, int off, int total)
{
    // UNUSED.
    on = off = total = 0;
}


ticcmd_t	emptycmd;
ticcmd_t*	I_BaseTiccmd(void)
{
    return &emptycmd;
}


int I_GetHeapSize(void)
{
    if (0 == based_zone)
    {
        return 0;
    }

    return mb_used*1024*1024;
}


byte* I_ZoneBase(int* size)
{
    based_zone = 1;

    //printf("I_ZoneBase: malloc %d (dec) / %x (hex) bytes\n", mb_used*1048576,mb_used*1048576);
    *size = mb_used*1024*1024;

    return (byte *)n64_malloc(*size);
}


static unsigned long t0;


double get_elapsed_seconds()
{
    return (double)(get_ticks() / COUNTS_PER_SECOND);//(double)(get_ticks_ms()) / 1000.0;
}


unsigned long get_doom_ticks(void)
{
    return (get_ticks_ms() * TICRATE) / 1000L;//2294 >> 16;
}


//
// I_GetTime
// returns time in 1/70th second tics
//
unsigned long I_GetTime(void)
{
    return get_doom_ticks();
}


//
// I_Init
//
void I_Init(void)
{
    I_InitSound();
    I_InitMusic();
    t0 = get_doom_ticks();
}


//
// I_Quit
//
void I_Quit(void)
{
    D_QuitNetGame();
    I_ShutdownSound();
    I_ShutdownMusic();
    M_SaveDefaults();
    I_ShutdownGraphics();
    exit(0);
}


void n64_sleep_millis(int count)
{
    unsigned long start = get_ticks_ms();

    while ((get_ticks_ms() - start) < count)
    {
        ;
    }
}


void I_WaitVBL(int count)
{
    n64_sleep_millis(count / change_ticrate);
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

    mem = (byte *)n64_malloc(length);
    n64_memset(mem,0,length);
    return mem;
}


//
// I_Error
//
extern boolean demorecording;


void I_Error(char *error)
{
//    int i;
    char ermac[512];
    sprintf(ermac, "I_Error: %s", error);
    DebugOutput_String(/*"I_Error"*/ermac, 0);

//#if 0
    while(1) //i < 1048576 * 1024)
    {
//        i+=1;
    }
//#endif

    D_QuitNetGame();
    I_ShutdownGraphics();
    exit(-1);
}
