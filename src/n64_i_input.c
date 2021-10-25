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
//	Doom input handler for Nintendo 64, libdragon
//
//-----------------------------------------------------------------------------


#include <libdragon.h>
#include <stdlib.h>

#include "i_input.h"

#include "doomstat.h"
#include "i_system.h"
#include "v_video.h"
#include "m_argv.h"
#include "d_main.h"

#include "doomdef.h"

extern int detailshift;

extern void I_FinishUpdate(void);
extern void I_ForcePaletteUpdate(void);

void n64_do_cheat(int cheat);

static int GODDED;
extern int PROFILE_MEMORY;
extern int usegamma;

int current_map;
int current_episode;
GameMode_t current_mode;

static int pad_weapon = 1;
static char weapons[8] = { '1', '2', '3', '3', '4', '5', '6', '7' };
static int count = 0;

int controller_mapping = 0;
int last_x = 0;
int last_y = 0;
int center_x = 0;
int center_y = 0;
int shift = 0;

void pressed_key(struct controller_data pressed_data, int player);
void held_key(struct controller_data pressed_data, int player);
void released_key(struct controller_data pressed_data, int player);

int current_player_for_input = 0;

//
// I_GetEvent
// called by I_StartTic, scans player 1 controller for keys
// held_key is only used to scan analog stick to handle "mouse" event
// pressed_key/released_key are used to handle "keyboard" events
//
void I_GetEvent(void)
{
    controller_scan();

    struct controller_data keys_pressed = get_keys_down();
    struct controller_data keys_held = get_keys_held();
    struct controller_data keys_released = get_keys_up();
    // one of these days...
    current_player_for_input = 0;
    pressed_key(keys_pressed,0);
    held_key(keys_held,0);
    released_key(keys_released,0);
}

//
// I_StartTic
// just calls I_GetEvent
//
void I_StartTic(void)
{
    I_GetEvent();
}


//
// held_key
// this function maps analog stick position into mouse movement event
//
void held_key(struct controller_data pressed_data, int player)
{
    struct SI_condat pressed = pressed_data.c[player];
    short mouse_x;
    short mouse_y;

    last_x = pressed.x - center_x;
    last_y = pressed.y - center_y;

    if ( (last_x < -32) || (last_x > 32) || (last_y < -32) || (last_y > 32) )
    {
        mouse_x = last_x << 1;
        mouse_y = last_y << 1;

        if (mouse_x || mouse_y)
        {
            event_t ev;

            ev.type = ev_mouse;
            ev.data1 = 0;
            ev.data2 = mouse_x;
            ev.data3 = mouse_y;

            D_PostEvent(&ev);
        }
    }
}

int shift_times;

//
// pressed_key
// handle pressed buttons that are mapped to keyboard event operations such as
// moving, shooting, opening doors, toggling run mode
// also handles out-of-band input operations like toggling GOD MODE, debug display,
// adjusting gamma correction
//
void pressed_key(struct controller_data pressed_data, int player)
{
    event_t doom_input_event;
    struct SI_condat pressed = pressed_data.c[player];

    // CHEAT WARP TO NEXT LEVEL
    if (pressed.L && pressed.Z)
    {
        if ( (count > 0) && (count % 4 == 0) )
        {
            n64_do_cheat(13); // IDCLEV
        }

        count += 1;
    }

    // TOGGLE GOD MODE
    if (pressed.L && pressed.R)
    {
        if (!GODDED)
        {
            n64_do_cheat(1); // IDDQD
            n64_do_cheat(3); // IDKFA
            n64_do_cheat(10); // IDBEHOLDA
            n64_do_cheat(5); // IDDT
        }

        GODDED = 1 - GODDED;
    }

    // RUN ON/OFf
    if (pressed.Z)
    {
        shift = 1 - shift;
        shift_times = 0;
    }

    if (shift)
    {
        doom_input_event.data1 = KEY_RSHIFT;
        doom_input_event.type = ev_keydown;
        D_PostEvent(&doom_input_event);
        shift_times++;
    }
    else
    {
        if (shift_times == 0)
        {
            doom_input_event.data1 = KEY_RSHIFT;
            doom_input_event.type = ev_keyup;
            D_PostEvent(&doom_input_event);
        }
        shift_times++;
    }

    if (pressed.A)
    {
        doom_input_event.data1 = KEY_RCTRL;
        doom_input_event.type = ev_keydown;
        D_PostEvent(&doom_input_event);
    }
    if (pressed.B)
    {
        doom_input_event.data1 = ' ';
        doom_input_event.type = ev_keydown;
        D_PostEvent(&doom_input_event);
    }
    if (pressed.L && !pressed.R && !pressed.Z)
    {
        doom_input_event.data1 = ',';
        doom_input_event.type = ev_keydown;
        D_PostEvent(&doom_input_event);
    }
    if (pressed.R && !pressed.L && !pressed.Z)
    {
        doom_input_event.data1 = '.';
        doom_input_event.type = ev_keydown;
        D_PostEvent(&doom_input_event);
    }
    if (pressed.C_up)
    {
        doom_input_event.data1 = KEY_TAB;
        doom_input_event.type = ev_keydown;
        D_PostEvent(&doom_input_event);
    }
    if (pressed.C_down)
    {
        doom_input_event.data1 = KEY_ENTER;
        doom_input_event.type = ev_keydown;
        D_PostEvent(&doom_input_event);
    }
    if (pressed.C_left && !pressed.C_right)
    {
        pad_weapon -= 1;

        if (pad_weapon < 0)
        {
            pad_weapon = 7;
        }

        doom_input_event.data1 = weapons[pad_weapon];
        doom_input_event.type = ev_keydown;
        D_PostEvent(&doom_input_event);
    }
    if (pressed.C_right && !pressed.C_left)
    {
        pad_weapon += 1;

        if (pad_weapon > 7)
        {
            pad_weapon = 0;
        }

        doom_input_event.data1 = weapons[pad_weapon];
        doom_input_event.type = ev_keydown;
        D_PostEvent(&doom_input_event);
    }
    if (pressed.up)
    {
        doom_input_event.data1 = KEY_UPARROW;
        doom_input_event.type = ev_keydown;
        D_PostEvent(&doom_input_event);
    }
    if (pressed.down)
    {
        doom_input_event.data1 = KEY_DOWNARROW;
        doom_input_event.type = ev_keydown;
        D_PostEvent(&doom_input_event);
    }
    if (pressed.left && !pressed.right)
    {
        doom_input_event.data1 = KEY_LEFTARROW;
        doom_input_event.type = ev_keydown;
        D_PostEvent(&doom_input_event);
    }
    if (pressed.right && !pressed.left)
    {
        doom_input_event.data1 = KEY_RIGHTARROW;
        doom_input_event.type = ev_keydown;
        D_PostEvent(&doom_input_event);
    }
    if (pressed.start)
    {
        doom_input_event.data1 = KEY_ESCAPE;
        doom_input_event.type = ev_keydown;
        D_PostEvent(&doom_input_event);
    }
}


//
// released_key
// handle released buttons that are mapped to keyboard event operations such as
// moving, shooting, opening doors, etc
//
void released_key(struct controller_data pressed_data, int player)
{
    event_t doom_input_event;

    struct SI_condat pressed = pressed_data.c[player];

    last_x = pressed.x - center_x;
    last_y = pressed.y - center_y;

    if (pressed.A)
    {
        doom_input_event.data1 = KEY_RCTRL;
        doom_input_event.type = ev_keyup;
        D_PostEvent(&doom_input_event);
    }
    if (pressed.B)
    {
        doom_input_event.data1 = ' ';
        doom_input_event.type = ev_keyup;
        D_PostEvent(&doom_input_event);
    }
    if (pressed.L && !pressed.R && !pressed.Z)
    {
        doom_input_event.data1 = ',';
        doom_input_event.type = ev_keyup;
        D_PostEvent(&doom_input_event);
    }
    if (pressed.R && !pressed.L && !pressed.Z)
    {
        doom_input_event.data1 = '.';
        doom_input_event.type = ev_keyup;
        D_PostEvent(&doom_input_event);
    }
    if (pressed.C_up)
    {
        doom_input_event.data1 = KEY_TAB;
        doom_input_event.type = ev_keyup;
        D_PostEvent(&doom_input_event);
    }
    if (pressed.C_down)
    {
        doom_input_event.data1 = KEY_ENTER;
        doom_input_event.type = ev_keyup;
        D_PostEvent(&doom_input_event);
    }
    if (pressed.C_left && !pressed.C_right)
    {
        doom_input_event.data1 = weapons[pad_weapon];
        doom_input_event.type = ev_keyup;
        D_PostEvent(&doom_input_event);
    }
    if (pressed.C_right && !pressed.C_left)
    {
        doom_input_event.data1 = weapons[pad_weapon];
        doom_input_event.type = ev_keyup;
        D_PostEvent(&doom_input_event);
    }
    if (pressed.up)
    {
        doom_input_event.data1 = KEY_UPARROW;
        doom_input_event.type = ev_keyup;
        D_PostEvent(&doom_input_event);
    }
    if (pressed.down)
    {
        doom_input_event.data1 = KEY_DOWNARROW;
        doom_input_event.type = ev_keyup;
        D_PostEvent(&doom_input_event);
    }
    if (pressed.left && !pressed.right)
    {
        doom_input_event.data1 = KEY_LEFTARROW;
        doom_input_event.type = ev_keyup;
        D_PostEvent(&doom_input_event);
    }
    if (pressed.right && !pressed.left)
    {
        doom_input_event.data1 = KEY_RIGHTARROW;
        doom_input_event.type = ev_keyup;
        D_PostEvent(&doom_input_event);
    }
    if (pressed.start)
    {
        doom_input_event.data1 = KEY_ESCAPE;
        doom_input_event.type = ev_keyup;
        D_PostEvent(&doom_input_event);
    }
}

char __attribute__((aligned(8))) clevstr[256];

void n64_do_cheat(int cheat)
{
    char       *str;
    int        i;
    event_t    event;

    switch (cheat)
    {
        case 1: // God Mode
        {
            str = "iddqd";
            break;
        }
        case 2: // Fucking Arsenal
        {
            str = "idfa";
            break;
        }
        case 3: // Key Full Ammo
        {
            str = "idkfa";
            break;
        }
        case 4: // No Clipping
        {
            str = "idclip";
            break;
        }
        case 5: // Toggle Map
        {
            str = "iddt";
            break;
        }
        case 6: // Invincible with Chainsaw
        {
            str = "idchoppers";
            break;
        }
        case 7: // Berserker Strength Power-up
        {
            str = "idbeholds";
            break;
        }
        case 8: // Invincibility Power-up
        {
            str = "idbeholdv";
            break;
        }
        case 9: // Invisibility Power-Up
        {
            str = "idbeholdi";
            break;
        }
        case 10: // Automap Power-up
        {
            str = "idbeholda";
            break;
        }
        case 11: // Anti-Radiation Suit Power-up
        {
            str = "idbeholdr";
            break;
        }
        case 12: // Light-Amplification Visor Power-up
        {
            str = "idbeholdl";
            break;
        }
        case 13: // change level
        {
#if 0
            if ((current_mode == (GameMode_t)commercial) || (current_mode == (GameMode_t)pack_tnt) || (current_mode == (GameMode_t)pack_plut))
            {
                if (current_mode == (GameMode_t)commercial)
                {
                    if(current_map < 34) {
                        current_map++;
                    }
                    else
                    {
                        current_map = 1;
                    }
                }
                else
                {
                    if (current_map < 32)
                    {
                        current_map++;
                    }
                    else
                    {
                        current_map = 1;
                    }
                }

                if(current_map < 10)
                    sprintf(clevstr, "idclev%02d", current_map);
                else
                    sprintf(clevstr, "idclev%d", current_map);

                str = clevstr;
            }
#endif
#if 0
            else
#endif
#if 1
            {
                //current_map = gamemap;
                if (current_map < 9)
                {
                    current_map++;
                }
                else
                {
                    current_map = 1;
                }

                sprintf(clevstr, "idclev%1d%1d", current_episode, current_map);
                str = clevstr;
            }
#endif
            break;
        }
        default:
        {
            return;
        }
    }

    for (i=0; i<strlen(str); i++)
    {
        event.type = ev_keydown;
        event.data1 = str[i];
        D_PostEvent (&event);
        event.type = ev_keyup;
        event.data1 = str[i];
        D_PostEvent (&event);
    }
}

