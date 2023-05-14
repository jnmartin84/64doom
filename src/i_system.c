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

extern void DebugOutput_String_For_IError(const char *str, int lineNumber, int good);

volatile uint64_t timekeeping;

// give 5 MB to zone
// anything more and music starts to fail to allocate samples
const int kb_used = 4096+512+512;
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

    return kb_used*1024;
}


byte* I_ZoneBase(int* size)
{
    based_zone = 1;

    *size = kb_used*1024;

    byte *ptr = (byte *)malloc(*size);

    return ptr;
}


//
// I_GetTime
// returns time in 1/70th second tics
//
unsigned long I_GetTime(void)
{
    return timekeeping>>2;
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
    new_timer(669643/2, TF_CONTINUOUS, tickercb);

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

void I_Error(const char *fmt, ...)
{   
    char errstr[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(errstr, sizeof(errstr), fmt, args);
    // in case we haven't reached I_InitGraphics yet
    printf("I_Error: %s\n", errstr);
    DebugOutput_String_For_IError(errstr, 0, 0);
    
    D_QuitNetGame();
//    I_ShutdownGraphics();

    while (1)
    {}
}

void I_Warn(char *fmt, ...)
{
    char errstr[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(errstr, sizeof(errstr), fmt, args);
    // in case we haven't reached I_InitGraphics yet
    printf("I_Warn: %s\n", errstr);
    DebugOutput_String_For_IError(errstr, 0, 1);
    
    while (1)
    {}
}
