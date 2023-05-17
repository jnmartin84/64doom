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
//    Doom input handler for Nintendo 64, libdragon
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

void n64_do_cheat(int cheat);

static int GODDED;

uint32_t current_map;
uint32_t current_episode;
GameMode_t current_mode;

static int pad_weapon = 1;
static char weapons[8] = { '1', '2', '3', '3', '4', '5', '6', '7' };
static int lz_count = 0;

int controller_mapping = 0;
int last_x = 0;
int last_y = 0;
int center_x = 0;
int center_y = 0;
int shift = 0;

void pressed_key(struct controller_data *p_data);//, int player);
void held_key(struct controller_data *h_data);//, int player);
void released_key(struct controller_data *r_data);//, int player);

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
    pressed_key(&keys_pressed);//,0);
    held_key(&keys_held);//,0);
    released_key(&keys_released);//,0);
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
void held_key(struct controller_data *h_data) //, int player)
{
    struct SI_condat held = h_data->c[0];//player];
    short mouse_x;
    short mouse_y;

    last_x = held.x - center_x;
    last_y = held.y - center_y;

    // x,y in (-32,32),(-32,32) "dead zone" in center 
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
// also handles out-of-band input operations like toggling GOD MODE, idclev
//
void pressed_key(struct controller_data *p_data) //, int player)
{
    event_t doom_input_event;
    struct SI_condat pressed = p_data->c[0];//player];

#if 1
    // CHEAT WARP TO NEXT LEVEL
    if (pressed.L && pressed.Z && !pressed.R)
    {
        if ( (lz_count > 0) && (lz_count % 4 == 0) )
        {
            n64_do_cheat(13); // IDCLEV
        }

        lz_count += 1;
    }

    // TOGGLE GOD MODE
    if (pressed.L && pressed.R && !pressed.Z)
    {
        if (!GODDED)
        {
            n64_do_cheat(1);  // IDDQD
            n64_do_cheat(3);  // IDKFA
            n64_do_cheat(10); // IDBEHOLDA
            n64_do_cheat(5);  // IDDT
        }

        GODDED = 1 - GODDED;
    }
#endif

    // RUN ON/OFF
    if (pressed.Z)
    {
        shift = 1 - shift;
    }

    if (shift)
    {
        doom_input_event.data1 = KEY_RSHIFT;
        doom_input_event.type = ev_keydown;
        D_PostEvent(&doom_input_event);
    }
    else
    {
        doom_input_event.data1 = KEY_RSHIFT;
        doom_input_event.type = ev_keyup;
        D_PostEvent(&doom_input_event);
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
    if (pressed.left)
    {
        doom_input_event.data1 = KEY_LEFTARROW;
        doom_input_event.type = ev_keydown;
        D_PostEvent(&doom_input_event);
    }
    if (pressed.right)
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
void released_key(struct controller_data *r_data) //, int player)
{
    event_t doom_input_event;

    struct SI_condat released = r_data->c[0];//player];

    last_x = released.x - center_x;
    last_y = released.y - center_y;

    if (released.A)
    {
        doom_input_event.data1 = KEY_RCTRL;
        doom_input_event.type = ev_keyup;
        D_PostEvent(&doom_input_event);
    }
    if (released.B)
    {
        doom_input_event.data1 = ' ';
        doom_input_event.type = ev_keyup;
        D_PostEvent(&doom_input_event);
    }
    if (released.L)
    {
        doom_input_event.data1 = ',';
        doom_input_event.type = ev_keyup;
        D_PostEvent(&doom_input_event);
    }
    if (released.R)
    {
        doom_input_event.data1 = '.';
        doom_input_event.type = ev_keyup;
        D_PostEvent(&doom_input_event);
    }
    if (released.C_up)
    {
        doom_input_event.data1 = KEY_TAB;
        doom_input_event.type = ev_keyup;
        D_PostEvent(&doom_input_event);
    }
    if (released.C_down)
    {
        doom_input_event.data1 = KEY_ENTER;
        doom_input_event.type = ev_keyup;
        D_PostEvent(&doom_input_event);
    }
    if (released.C_left && !released.C_right)
    {
        doom_input_event.data1 = weapons[pad_weapon];
        doom_input_event.type = ev_keyup;
        D_PostEvent(&doom_input_event);
    }
    if (released.C_right && !released.C_left)
    {
        doom_input_event.data1 = weapons[pad_weapon];
        doom_input_event.type = ev_keyup;
        D_PostEvent(&doom_input_event);
    }
    if (released.up)
    {
        doom_input_event.data1 = KEY_UPARROW;
        doom_input_event.type = ev_keyup;
        D_PostEvent(&doom_input_event);
    }
    if (released.down)
    {
        doom_input_event.data1 = KEY_DOWNARROW;
        doom_input_event.type = ev_keyup;
        D_PostEvent(&doom_input_event);
    }
    if (released.left)
    {
        doom_input_event.data1 = KEY_LEFTARROW;
        doom_input_event.type = ev_keyup;
        D_PostEvent(&doom_input_event);
    }
    if (released.right)
    {
        doom_input_event.data1 = KEY_RIGHTARROW;
        doom_input_event.type = ev_keyup;
        D_PostEvent(&doom_input_event);
    }
    if (released.start)
    {
        doom_input_event.data1 = KEY_ESCAPE;
        doom_input_event.type = ev_keyup;
        D_PostEvent(&doom_input_event);
    }
}

static char __attribute__((aligned(8))) clevstr[9];

extern    char*    get_GAMEID();
extern    char*    doom1wad;
extern    char*    doomwad;
extern    char*    doomuwad;
extern    char*    doom2wad;
extern    char*    plutoniawad;
extern    char*    tntwad;

#define stricmp strcasecmp

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
            const char *gameid = get_GAMEID();

            if (!stricmp(doom2wad,gameid) || !stricmp(tntwad,gameid) || !stricmp(plutoniawad,gameid))
            {
                if (!stricmp(doom2wad,gameid))
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
                else
                {
                    if(current_map < 34) {
                        current_map++;
                    }
                    else
                    {
                        current_map = 1;
                    }
                }

                sprintf(clevstr, "idclev%02ld", current_map);

                str = clevstr;
            }
            else
            {
                // shut up compiler...
                if (current_episode >= 4)
                {
                    I_Error("n64_do_cheat: Invalid current_episode: %ld.", current_episode);
                    return;
                }

                if (current_map < 9)
                {
                    current_map++;
                }
                else
                {
                    current_map = 1;
                }

                sprintf(clevstr, "idclev%1ld%1ld", current_episode, current_map);
                str = clevstr;
            }
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

