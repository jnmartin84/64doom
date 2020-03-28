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
//	Main program, simply calls D_DoomMain high level loop.
//
//-----------------------------------------------------------------------------
#include <limits.h>

#include <stdlib.h>

#include <inttypes.h>
#include <libdragon.h>

#include "doomdef.h"
#include "m_argv.h"
#include "d_main.h"
#include "g_game.h"

extern void *__n64_memset_ZERO_ASM(void *ptr, int value, size_t num);
extern void *__n64_memset_ASM(void *ptr, int value, size_t num);

extern void DoomIsOver();

extern double get_elapsed_seconds(void);
extern void register_dump(exception_t *exception);
extern void I_InitGraphics(void);
extern void I_FinishUpdate(void);

#if 1
extern uint32_t ytab[];
//extern uint32_t y10tab[];
//extern uint32_t y20tab[];
#endif

#define SCREENW 640

void check_and_init_mempak(void)
{
//#ifdef newld
    get_accessories_present(0);
//#endif
//#ifndef newld
//	get_accessories_present();
//#endif
    switch (identify_accessory(0))
    {
        case ACCESSORY_NONE:
        {
            printf("No accessory inserted!\n");
            break;
        }
        case ACCESSORY_MEMPAK:
        {
            int err;
            if ((err = validate_mempak(0)))
            {
                if (err == -3)
                {
                    printf("Mempak is not formatted! Formatting it automatically.\n");

                    if (format_mempak(0))
                    {
                        printf("Error formatting mempak!\n");
                    }
                    else
                    {
                        printf("Memory card formatted!\n");
                    }
                }
                else
                {
                    printf("Mempak bad or removed during read!\n");
                }
            }
            else
            {
                printf("\nFree space: %d blocks\n", get_mempak_free_space(0));
            }

            break;
        }
        case ACCESSORY_RUMBLEPAK:
        {
            printf("Cannot read data off of rumblepak!\n");
            break;
        }
    }
}
//#define fmultest
extern byte* savebuffer;
extern int center_x, center_y;
extern void DoNothing (void);
#ifdef fmultest
uint32_t testa, testb;
uint32_t testc;

uint32_t C_FixedDiv(uint32_t a, uint32_t b)
{
    if ((abs(a) >> 14) >= abs(b))
    {
         return (a^b) < 0 ? INT_MIN : INT_MAX;
    }
    else
    {
        int64_t result;

        result = ((int64_t) a << 16) / b;

        return (uint32_t) result;
    }
}

uint32_t
C_FixedMul
( uint32_t	a,
  uint32_t	b )
{
    return ((int64_t) a * (int64_t) b) >> 16;
}
double starts;
double ends;
#endif
int main(int argc, char **argv)
{
    int j;
#if 1
    for (j=0;j</*240*/480;j++)
    {
        ytab[j]   = ((j+40   )*640);
//        y10tab[j] = ((j+10)*SCREENW);
//	y20tab[j] = ((j+20)*SCREENW);
    }

#endif

    init_interrupts();

    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE);
	register_exception_handler(register_dump);
//    rdp_init();

    console_init();
    console_set_render_mode(RENDER_AUTOMATIC);
#ifdef fmultest
	starts = get_elapsed_seconds();
	for(testa=1234567;testa<1234567+0x800;testa++){
	for(testb=9876543;testb<9876543+0x800;testb++){
			testc = FixedDiv(testb+0x10000,testa+0x10000);
	}
	}
	ends = get_elapsed_seconds();
	printf("%08X %f\n", testc, (ends-starts));
/*
	starts = get_elapsed_seconds();
		for(testa=0x00000000;testa<0x1000;testa++){
	for(testb=0x00000000;testb<0x1000;testb++){
			testc = C_FixedDiv(testa+0x10000,testb+0x10000);
	}
	}
	ends = get_elapsed_seconds();
	printf("%08X %f\n", testc, (ends-starts));*/
	while(1){}	
	#endif
#if 1
    controller_init();

    // center joystick...
    controller_scan();
    struct controller_data keys_pressed = get_keys_down();
    struct SI_condat pressed = keys_pressed.c[0];
    center_x = pressed.x;
    center_y = pressed.y;

    printf("64Doom by jnmartin84\n");
    printf("github.com/jnmartin84/64doom/\n");
    printf("built %s %s\n", __DATE__, __TIME__);

    int available_memory_size = *(int *)(0x80000318);

    if(available_memory_size != 0x800000)
    {
        printf("\n***********************************");
        printf("Expansion Pak not found.\n");
        printf("It is required to run 64Doom.\n");
        printf("Please turn off the Nintendo 64,\ninstall Expansion Pak,\nand try again.\n");
        printf("***********************************\n");
        //return -1;
    }

    printf("Expansion Pak found.\n");
    printf("Available memory: %d bytes\n", *(int *)(0x80000318));

    printf("Checking for Mempak:\n");
    check_and_init_mempak();

//    savebuffer = (byte *)n64_malloc(SAVEGAMESIZE);
    display_close();
    I_InitGraphics();
//    I_FinishUpdate = &different_renderer;
    D_DoomMain();
	DoomIsOver();
	timer_close();
#endif	
    return 0;
}