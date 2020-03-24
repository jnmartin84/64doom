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

extern display_context_t _dc;
extern void unlockVideo(display_context_t _dc);
extern display_context_t lockVideo(int i);
extern void DebugOutput_String(const char *str, int good_or_bad);
extern void DebugOutput_String_On_Line(const char *str, int lineNumber, int good);
extern void DebugOutput_String_For_IError(const char *str, int lineNumber, int good);
extern void *__n64_memset_ASM(void *p, int v, size_t n);
extern void *__n64_memset_ZERO_ASM(void *p, int v, size_t n);


unsigned long change_ticrate = TICRATE;

int	mb_used = 4;
int	based_zone = 0;


void I_Tactile(int on, int off, int total)
{
    // UNUSED.
    on = off = total = 0;
}


ticcmd_t	emptycmd;
inline ticcmd_t*	I_BaseTiccmd(void)
{
    return &emptycmd;
}

extern unsigned int Z_ZoneSize(void);

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

    *size = mb_used*1024*1024;

    return (byte *)n64_malloc(*size);
}


static unsigned long t0;


double get_elapsed_seconds(void)
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
    return (get_ticks_ms() * TICRATE) / 1000L; //get_doom_ticks();
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

int return_from_D_DoomMain = 0;
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
//    exit(0);
return_from_D_DoomMain = 1;
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
    __n64_memset_ZERO_ASM(mem,0,length);
    return mem;
}


//
// I_Error
//
extern boolean demorecording;


void I_Error(char *error)
{
//    printf("%s\n", error);
//	printf("...\n");
    DebugOutput_String_For_IError(error, 0, 0);
		unlockVideo(_dc);
		_dc = lockVideo(1);
    DebugOutput_String_For_IError(error, 0, 0);
		unlockVideo(_dc);
		
    while(1)
    {}

#if 0
    D_QuitNetGame();
    I_ShutdownGraphics();
    exit(-1);
#endif
}

void I_Warn(char *error)
{
#if 1
//    char ermac[512] = {'\0'};
//    sprintf(ermac, "%s", error);

    DebugOutput_String_For_IError(error, 0, 1);
		unlockVideo(_dc);
		_dc = lockVideo(1);
    DebugOutput_String_For_IError(error, 0, 1);
		unlockVideo(_dc);

    while(1)
    {}
    //n64_sleep_millis(250*1);

//    D_QuitNetGame();
//    I_ShutdownGraphics();
//    exit(-1);
#endif
}
