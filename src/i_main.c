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
//    Main program, simply calls D_DoomMain high level loop.
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

extern void DoomIsOver();

void check_and_init_mempak(void)
{
    struct controller_data output;
    get_accessories_present(&output);

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
            // I left the below in the code #ifdef'd out for when I needed an easy way to nuke a controller pak
#if 0
            if (format_mempak(0))
            {
                printf("Error formatting mempak!\n");
            }
            else
            {
                printf("Memory card formatted!\n");
            }
#endif
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
                // this will print the details of each entry on the controller pak
#if 0
                for(int j = 0; j < 16; j++)
                {
                    entry_structure_t entry;

                    get_mempak_entry(0, j, &entry);

                    if(entry.valid)
                    {
                        printf("%s - %d blocks\n", entry.name, entry.blocks);
                    }
                    else
                    {
                        printf("(EMPTY)\n");
                    }
                }
#endif
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


extern int center_x, center_y;

int main(int argc, char **argv)
{
//    console_init();
//debug_init_isviewer();

//    console_set_render_mode(RENDER_AUTOMATIC);
    if (dfs_init( DFS_DEFAULT_LOCATION ) != DFS_ESUCCESS)
    {
        printf("Could not initialize filesystem!\n");
        while(1);
    }
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

    int available_memory_size = get_memory_size();
    //printf("Available memory: %d bytes\n", *(int *)(0x80000318));

    if(available_memory_size != 0x800000)
    {
        printf("\n***********************************");
        printf("Expansion Pak not found.\n");
        printf("It is required to run 64Doom.\n");
        printf("Please turn off the Nintendo 64,\ninstall Expansion Pak,\nand try again.\n");
        printf("***********************************\n");
        while(1) {}
    }

    //printf("Expansion Pak found.\n");

    printf("Checking for Mempak:\n");
    check_and_init_mempak();

    D_DoomMain();
    DoomIsOver();

    return 0;
}
