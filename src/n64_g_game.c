// Emacs style mode select   -*- C++ -*- 
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
// DESCRIPTION:  none
//
//-----------------------------------------------------------------------------

#include <libdragon.h>
#include <string.h>
#include <stdlib.h>

#include "m_swap.h"

#include "doomdef.h"
#include "doomstat.h"

#include "z_zone.h"
#include "f_finale.h"
#include "m_argv.h"
#include "m_misc.h"
#include "m_menu.h"
#include "m_random.h"
#include "i_system.h"

#include "p_setup.h"
#include "p_saveg.h"
#include "p_tick.h"

#include "d_main.h"

#include "wi_stuff.h"
#include "hu_stuff.h"
#include "st_stuff.h"
#include "am_map.h"

// Needs access to LFB.
#include "v_video.h"

#include "w_wad.h"

#include "p_local.h"

#include "s_sound.h"

// Data.
#include "dstrings.h"
#include "sounds.h"

// SKY handling - still the wrong place.
#include "r_data.h"
#include "r_sky.h"

#include "g_game.h"

#include "lzfx.h"

#define BYTES_PER_BLOCK 256
#define BYTES_TO_BLOCKS(bytes) ((bytes + (BYTES_PER_BLOCK - 1)) / BYTES_PER_BLOCK)
#define SAVE_SIZE_IN_BLOCKS(bytes) (BYTES_TO_BLOCKS(bytes) + 1)

//extern void *n64_malloc(size_t n);
//extern void n64_free(void *p);
extern void n64_sleep_millis(int count);
extern void I_Error(char *error);
extern void I_Warn(char *str);

extern char *get_GAMEID();
extern void *__n64_memset_ZERO_ASM(void *p, int v, size_t n);
extern void *__n64_memset_ASM(void *p, int v, size_t n);
extern void *__n64_memcpy_ASM(void *d, const void *s, size_t n);

unsigned int last_save_size;

//#define SAVEGAMESIZE	0x2c000
//#define SAVESTRINGSIZE	24

boolean	G_CheckDemoStatus (void); 
void	G_ReadDemoTiccmd (ticcmd_t* cmd); 
void	G_WriteDemoTiccmd (ticcmd_t* cmd); 
void	G_PlayerReborn (int player); 
void	G_InitNew (skill_t skill, int episode, int map); 
 
void	G_DoReborn (int playernum); 
 
void	G_DoLoadLevel (void); 
void	G_DoNewGame (void); 
void	G_DoLoadGame (void); 
void	G_DoPlayDemo (void); 
void	G_DoCompleted (void); 
void	G_DoVictory (void); 
void	G_DoWorldDone (void); 
void	G_DoSaveGame (void); 
 
 
gameaction_t    gameaction; 
gamestate_t     gamestate; 
skill_t         gameskill; 
boolean		respawnmonsters;
int             gameepisode; 
int             gamemap; 
 
boolean         paused; 
boolean         sendpause;             	// send a pause event next tic 
boolean         sendsave;             	// send a save event next tic 
boolean         usergame;               // ok to save / end game 
 
boolean         timingdemo;             // if true, exit with report on completion 
boolean         nodrawers;              // for comparative timing purposes 
boolean         noblit;                 // for comparative timing purposes 
int             starttime;          	// for comparative timing purposes  	 
 
boolean         viewactive; 
 
boolean         deathmatch;           	// only if started as net death 
boolean         netgame;                // only true if packets are broadcast 
boolean         playeringame[MAXPLAYERS]; 
player_t        players[MAXPLAYERS]; 
 
int             consoleplayer;          // player taking events and displaying 
int             displayplayer;          // view being displayed 
int             gametic; 
int             levelstarttic;          // gametic at level start 
int             totalkills, totalitems, totalsecret;    // for intermission 
 
char            demoname[32]; 
boolean         demorecording; 
boolean         demoplayback; 
boolean		netdemo; 
byte*		demobuffer;
byte*		demo_p;
byte*		demoend; 
boolean         singledemo;            	// quit after playing a demo from cmdline 
 
boolean         precache = true;        // if true, load all graphics at start 
 
wbstartstruct_t wminfo;               	// parms for world map / intermission 
 
short		consistancy[MAXPLAYERS][BACKUPTICS]; 
 
byte	__attribute__((aligned(8)))	savebuffer[SAVEGAMESIZE];
 
 
// 
// controls (have defaults) 
// 
int             key_right;
int		key_left;

int		key_up;
int		key_down; 
int             key_strafeleft;
int		key_straferight; 
int             key_fire;
int		key_use;
int		key_strafe;
int		key_speed; 
 
int             mousebfire; 
int             mousebstrafe; 
int             mousebforward; 
 
int             joybfire; 
int             joybstrafe; 
int             joybuse; 
int             joybspeed; 
 
 
 
#define MAXPLMOVE		(forwardmove[1]) 
 
#define TURBOTHRESHOLD	0x32

fixed_t		forwardmove[2] = {0x19, 0x32}; 
fixed_t		sidemove[2] = {0x18, 0x28}; 
fixed_t		angleturn[3] = {640, 1280, 320};	// + slow turn 

#define SLOWTURNTICS	6 
 
#define NUMKEYS		256 

boolean         gamekeydown[NUMKEYS]; 
int             turnheld;				// for accelerative turning 
 
boolean		mousearray[4]; 
boolean*	mousebuttons = &mousearray[1];		// allow [-1]

// mouse values are used once 
int             mousex;
int		mousey;         

int             dclicktime;
int		dclickstate;
int		dclicks; 
int             dclicktime2;
int		dclickstate2;
int		dclicks2;

// joystick values are repeated 
int             joyxmove;
int		joyymove;
boolean         joyarray[5]; 
boolean*	joybuttons = &joyarray[1];		// allow [-1] 
 
int		savegameslot; 
char		savedescription[32]; 
 
 
#define	BODYQUESIZE	32

mobj_t*		bodyque[BODYQUESIZE]; 
int		bodyqueslot; 
 
void*		statcopy;				// for statistics driver
 
 
 
int G_CmdChecksum (ticcmd_t* cmd) 
{ 
    int		i;
    int		sum = 0; 
	 
    for (i=0 ; i< sizeof(*cmd)/4 - 1 ; i++) 
	sum += ((int *)cmd)[i]; 
		 
    return sum; 
} 
 

//
// G_BuildTiccmd
// Builds a ticcmd from all of the available inputs
// or reads it from the demo buffer. 
// If recording a demo, write it out 
// 
void G_BuildTiccmd (ticcmd_t* cmd) 
{ 
    int		i; 
    boolean	strafe;
    boolean	bstrafe; 
    int		speed;
    int		tspeed; 
    int		forward;
    int		side;
    
    ticcmd_t*	base;

    base = I_BaseTiccmd ();		// empty, or external driver
    __n64_memcpy_ASM (cmd,base,sizeof(*cmd)); 
	
    cmd->consistancy = 
	consistancy[consoleplayer][maketic%BACKUPTICS]; 

 
    strafe = gamekeydown[key_strafe] || mousebuttons[mousebstrafe] 
	|| joybuttons[joybstrafe]; 
    speed = gamekeydown[key_speed] || joybuttons[joybspeed];
 
    forward = side = 0;
    
    // use two stage accelerative turning
    // on the keyboard and joystick
    if (joyxmove < 0
	|| joyxmove > 0  
	|| gamekeydown[key_right]
	|| gamekeydown[key_left]) 
	turnheld += ticdup; 
    else 
	turnheld = 0; 

    if (turnheld < SLOWTURNTICS) 
	tspeed = 2;             // slow turn 
    else 
	tspeed = speed;
    
    // let movement keys cancel each other out
    if (strafe) 
    { 
	if (gamekeydown[key_right]) 
	{
	    // fprintf(stderr, "strafe right\n");
	    side += sidemove[speed]; 
	}
	if (gamekeydown[key_left]) 
	{
	    //	fprintf(stderr, "strafe left\n");
	    side -= sidemove[speed]; 
	}
	if (joyxmove > 0) 
	    side += sidemove[speed]; 
	if (joyxmove < 0) 
	    side -= sidemove[speed]; 
 
    } 
    else 
    { 
	if (gamekeydown[key_right]) 
	    cmd->angleturn -= angleturn[tspeed]; 
	if (gamekeydown[key_left]) 
	    cmd->angleturn += angleturn[tspeed]; 
	if (joyxmove > 0) 
	    cmd->angleturn -= angleturn[tspeed]; 
	if (joyxmove < 0) 
	    cmd->angleturn += angleturn[tspeed]; 
    } 
 
    if (gamekeydown[key_up]) 
    {
	// fprintf(stderr, "up\n");
	forward += forwardmove[speed]; 
    }
    if (gamekeydown[key_down]) 
    {
	// fprintf(stderr, "down\n");
	forward -= forwardmove[speed]; 
    }
    if (joyymove < 0) 
	forward += forwardmove[speed]; 
    if (joyymove > 0) 
	forward -= forwardmove[speed]; 
    if (gamekeydown[key_straferight]) 
	side += sidemove[speed]; 
    if (gamekeydown[key_strafeleft]) 
	side -= sidemove[speed];
    
    // buttons
    cmd->chatchar = HU_dequeueChatChar(); 
 
    if (gamekeydown[key_fire] || mousebuttons[mousebfire] 
	|| joybuttons[joybfire]) 
	cmd->buttons |= BT_ATTACK; 
 
    if (gamekeydown[key_use] || joybuttons[joybuse] ) 
    { 
	cmd->buttons |= BT_USE;
	// clear double clicks if hit use button 
	dclicks = 0;                   
    } 

    // chainsaw overrides 
    for (i=0 ; i<NUMWEAPONS-1 ; i++)        
	if (gamekeydown['1'+i]) 
	{ 
	    cmd->buttons |= BT_CHANGE; 
	    cmd->buttons |= i<<BT_WEAPONSHIFT; 
	    break; 
	}
    
    // mouse
    if (mousebuttons[mousebforward]) 
	forward += forwardmove[speed];
    
    // forward double click
    if (mousebuttons[mousebforward] != dclickstate && dclicktime > 1 ) 
    { 
	dclickstate = mousebuttons[mousebforward]; 
	if (dclickstate) 
	    dclicks++; 
	if (dclicks == 2) 
	{ 
	    cmd->buttons |= BT_USE; 
	    dclicks = 0; 
	} 
	else 
	    dclicktime = 0; 
    } 
    else 
    { 
	dclicktime += ticdup; 
	if (dclicktime > 20) 
	{ 
	    dclicks = 0; 
	    dclickstate = 0; 
	} 
    }
    
    // strafe double click
    bstrafe =
	mousebuttons[mousebstrafe] 
	|| joybuttons[joybstrafe]; 
    if (bstrafe != dclickstate2 && dclicktime2 > 1 ) 
    { 
	dclickstate2 = bstrafe; 
	if (dclickstate2) 
	    dclicks2++; 
	if (dclicks2 == 2) 
	{ 
	    cmd->buttons |= BT_USE; 
	    dclicks2 = 0; 
	} 
	else 
	    dclicktime2 = 0; 
    } 
    else 
    { 
	dclicktime2 += ticdup; 
	if (dclicktime2 > 20) 
	{ 
	    dclicks2 = 0; 
	    dclickstate2 = 0; 
	} 
    } 
 
    forward += mousey; 
    if (strafe) 
	side += mousex*2; 
    else 
	cmd->angleturn -= mousex*0x8; 

    mousex = mousey = 0; 
	 
    if (forward > MAXPLMOVE) 
	forward = MAXPLMOVE; 
    else if (forward < -MAXPLMOVE) 
	forward = -MAXPLMOVE; 
    if (side > MAXPLMOVE) 
	side = MAXPLMOVE; 
    else if (side < -MAXPLMOVE) 
	side = -MAXPLMOVE; 
 
    cmd->forwardmove += forward; 
    cmd->sidemove += side;
    
    // special buttons
    if (sendpause) 
    { 
	sendpause = false; 
	cmd->buttons = BT_SPECIAL | BTS_PAUSE; 
    } 
 
    if (sendsave) 
    { 
	sendsave = false; 
	cmd->buttons = BT_SPECIAL | BTS_SAVEGAME | (savegameslot<<BTS_SAVESHIFT); 
    } 
} 
 

//
// G_DoLoadLevel 
//
extern  gamestate_t     wipegamestate; 
extern GameMode_t gamemode; 
void G_DoLoadLevel (void) 
{ 
    int             i; 

    // Set the sky map.
    // First thing, we have a dummy sky texture name,
    //  a flat. The data is in the WAD only because
    //  we look for an actual index, instead of simply
    //  setting one.
    skyflatnum = R_FlatNumForName ( SKYFLATNAME );

    // DOOM determines the sky texture to be used
    // depending on the current episode, and the game version.
    if ((gamemode == (GameMode_t)commercial) || (gamemode == (GameMode_t)pack_tnt) || (gamemode == (GameMode_t)pack_plut))
    {
	    skytexture = R_TextureNumForName ("SKY3");
	    if (gamemap < 12)
		{
	        skytexture = R_TextureNumForName ("SKY1");
		}
	    else if (gamemap < 21)
		{
		    skytexture = R_TextureNumForName ("SKY2");
		}
    }

    levelstarttic = gametic;        // for time calculation
    
    if (wipegamestate == GS_LEVEL) 
	{
	    wipegamestate = -1;             // force a wipe 
	}
	
    gamestate = GS_LEVEL; 

    for (i=0 ; i<MAXPLAYERS ; i++) 
    { 
	    if (playeringame[i] && players[i].playerstate == PST_DEAD)
        {			
	        players[i].playerstate = PST_REBORN; 
		}
	    __n64_memset_ZERO_ASM (players[i].frags,0,sizeof(players[i].frags)); 
    } 
		 
    P_SetupLevel (gameepisode, gamemap, 0, gameskill);    
    displayplayer = consoleplayer;		// view the guy you are playing    
    starttime = I_GetTime (); 
    gameaction = ga_nothing; 
    Z_CheckHeap ();
    
    // clear cmd building stuff
    __n64_memset_ZERO_ASM (gamekeydown, 0, sizeof(gamekeydown)); 
    joyxmove = joyymove = 0; 
    mousex = mousey = 0; 
    sendpause = sendsave = paused = false; 
    __n64_memset_ZERO_ASM (mousebuttons, 0, sizeof(mousebuttons)); 
    __n64_memset_ZERO_ASM (joybuttons, 0, sizeof(joybuttons)); 
} 
 
 
//
// G_Responder  
// Get info needed to make ticcmd_ts for the players.
// 
boolean G_Responder (event_t* ev) 
{ 
    // allow spy mode changes even during the demo
    if (gamestate == GS_LEVEL && ev->type == ev_keydown 
	&& ev->data1 == KEY_F12 && (singledemo || !deathmatch) )
    {
	// spy mode 
	do 
	{ 
	    displayplayer++; 
	    if (displayplayer == MAXPLAYERS) 
		displayplayer = 0; 
	} while (!playeringame[displayplayer] && displayplayer != consoleplayer); 
	return true; 
    }
    
    // any other key pops up menu if in demos
    if (gameaction == ga_nothing && !singledemo && 
	(demoplayback || gamestate == GS_DEMOSCREEN) 
	) 
    { 
	if (ev->type == ev_keydown ||  
	    (ev->type == ev_mouse && ev->data1) || 
	    (ev->type == ev_joystick && ev->data1) ) 
	{ 
	    M_StartControlPanel (); 
	    return true; 
	} 
	return false; 
    } 
 
    if (gamestate == GS_LEVEL) 
    { 
#if 0 
	if (devparm && ev->type == ev_keydown && ev->data1 == ';') 
	{ 
	    G_DeathMatchSpawnPlayer (0); 
	    return true; 
	} 
#endif 
	if (HU_Responder (ev)) 
	    return true;	// chat ate the event 
	if (ST_Responder (ev)) 
	    return true;	// status window ate it 
	if (AM_Responder (ev)) 
	    return true;	// automap ate it 
    } 
	 
    if (gamestate == GS_FINALE) 
    { 
	if (F_Responder (ev)) 
	    return true;	// finale ate the event 
    } 
	 
    switch (ev->type) 
    { 
      case ev_keydown: 
	if (ev->data1 == KEY_PAUSE) 
	{ 
	    sendpause = true; 
	    return true; 
	} 
	if (ev->data1 <NUMKEYS) 
	    gamekeydown[ev->data1] = true; 
	return true;    // eat key down events 
 
      case ev_keyup: 
	if (ev->data1 <NUMKEYS) 
	    gamekeydown[ev->data1] = false; 
	return false;   // always let key up events filter down 
		 
      case ev_mouse: 
	mousebuttons[0] = ev->data1 & 1; 
	mousebuttons[1] = ev->data1 & 2; 
	mousebuttons[2] = ev->data1 & 4; 
	mousex = ev->data2*(mouseSensitivity+5)/10; 
	mousey = ev->data3*(mouseSensitivity+5)/10; 
	return true;    // eat events 
 
      case ev_joystick: 
	joybuttons[0] = ev->data1 & 1; 
	joybuttons[1] = ev->data1 & 2; 
	joybuttons[2] = ev->data1 & 4; 
	joybuttons[3] = ev->data1 & 8; 
	joyxmove = ev->data2; 
	joyymove = ev->data3; 
	return true;    // eat events 
 
      default: 
	break; 
    } 
 
    return false; 
} 
 
 
 
//
// G_Ticker
// Make ticcmd_ts for the players.
//
void G_Ticker (void) 
{ 
    int		i;
    int		buf; 
    ticcmd_t*	cmd;
    
    // do player reborns if needed
    for (i=0 ; i<MAXPLAYERS ; i++) 
	if (playeringame[i] && players[i].playerstate == PST_REBORN) 
	    G_DoReborn (i);
    
    // do things to change the game state
    while (gameaction != ga_nothing) 
    { 
	switch (gameaction) 
	{ 
	  case ga_loadlevel: 
	    G_DoLoadLevel (); 
	    break; 
	  case ga_newgame: 
	    G_DoNewGame (); 
	    break; 
	  case ga_loadgame: 
	    G_DoLoadGame (); 
	    break; 
	  case ga_savegame: 
	    G_DoSaveGame (); 
	    break; 
	  case ga_playdemo: 
	    G_DoPlayDemo (); 
	    break; 
	  case ga_completed: 
	    G_DoCompleted (); 
	    break; 
	  case ga_victory: 
	    F_StartFinale (); 
	    break; 
	  case ga_worlddone: 
	    G_DoWorldDone (); 
	    break; 
	  case ga_screenshot: 
	    M_ScreenShot (); 
	    gameaction = ga_nothing; 
	    break; 
	  case ga_nothing: 
	    break; 
	} 
    }
    
    // get commands, check consistancy,
    // and build new consistancy check
    buf = (gametic/ticdup)%BACKUPTICS; 
 
    for (i=0 ; i<MAXPLAYERS ; i++)
    {
	if (playeringame[i]) 
	{ 
	    cmd = &players[i].cmd; 
 
//	    n64_memcpy (cmd, &netcmds[i][buf], sizeof(ticcmd_t)); 
	    __n64_memcpy_ASM (cmd, &netcmds[i][buf], sizeof(ticcmd_t)); 
 
	    if (demoplayback) 
		G_ReadDemoTiccmd (cmd); 
	    if (demorecording) 
		G_WriteDemoTiccmd (cmd);
	    
	    // check for turbo cheats
	    if (cmd->forwardmove > TURBOTHRESHOLD 
		&& !(gametic&31) && ((gametic>>5)&3) == i )
	    {
		static char turbomessage[80];
		extern char *player_names[4];
		sprintf (turbomessage, "%s is turbo!",player_names[i]);
		players[consoleplayer].message = turbomessage;
	    }

	    if (netgame && !netdemo && !(gametic%ticdup))
	    {
		if (gametic > BACKUPTICS && consistancy[i][buf] != cmd->consistancy)
		{
		    char ermac[256];
		    sprintf(ermac, "G_Ticker: consistency failure (%i should be %i)", cmd->consistancy, consistancy[i][buf]);
		    I_Error(ermac);
		}
		if (players[i].mo)
		{
		    consistancy[i][buf] = players[i].mo->x;
		}
		else
		{
		    consistancy[i][buf] = rndindex;
		}
	    }
	}
    }

    // check for special buttons
    for (i=0 ; i<MAXPLAYERS ; i++)
    {
	if (playeringame[i]) 
	{ 
	    if (players[i].cmd.buttons & BT_SPECIAL) 
	    { 
		switch (players[i].cmd.buttons & BT_SPECIALMASK) 
		{ 
		  case BTS_PAUSE: 
		    paused ^= 1; 
		    if (paused) 
			S_PauseSound (); 
		    else 
			S_ResumeSound (); 
		    break; 
					 
		  case BTS_SAVEGAME: 
		    if (!savedescription[0]) 
			strcpy (savedescription, "NET GAME"); 
		    savegameslot =  
			(players[i].cmd.buttons & BTS_SAVEMASK)>>BTS_SAVESHIFT; 
		    gameaction = ga_savegame; 
		    break; 
		} 
	    } 
	}
    }
    
    // do main actions
    switch (gamestate) 
    { 
      case GS_LEVEL: 
	P_Ticker (); 
	ST_Ticker (); 
	AM_Ticker (); 
	HU_Ticker ();            
	break; 
	 
      case GS_INTERMISSION: 
	WI_Ticker (); 
	break; 
			 
      case GS_FINALE: 
	F_Ticker (); 
	break; 
 
      case GS_DEMOSCREEN: 
	D_PageTicker (); 
	break; 
    }        
} 
 
 
//
// PLAYER STRUCTURE FUNCTIONS
// also see P_SpawnPlayer in P_Things
//

//
// G_InitPlayer 
// Called at the start.
// Called by the game initialization functions.
//
void G_InitPlayer (int player) 
{ 
	player_t*	p; 
 
    // set up the saved info         
    p = &players[player]; 
	 
    // clear everything else to defaults 
    G_PlayerReborn (player); 
	 
} 
 
 

//
// G_PlayerFinishLevel
// Can when a player completes a level.
//
void G_PlayerFinishLevel (int player) 
{ 
    player_t*	p; 
	 
    p = &players[player]; 
	 
    __n64_memset_ZERO_ASM (p->powers, 0, sizeof (p->powers)); 
    __n64_memset_ZERO_ASM (p->cards, 0, sizeof (p->cards)); 
    p->mo->flags &= ~MF_SHADOW;		// cancel invisibility 
    p->extralight = 0;			// cancel gun flashes 
    p->fixedcolormap = 0;		// cancel ir gogles 
    p->damagecount = 0;			// no palette changes 
    p->bonuscount = 0; 
} 
 

//
// G_PlayerReborn
// Called after a player dies 
// almost everything is cleared and initialized 
//
void G_PlayerReborn (int player) 
{ 
    player_t*	p; 
    int		i; 
    int		frags[MAXPLAYERS]; 
    int		killcount;
    int		itemcount;
    int		secretcount; 
	 
    __n64_memcpy_ASM (frags,players[player].frags,sizeof(frags)); 
    killcount = players[player].killcount; 
    itemcount = players[player].itemcount; 
    secretcount = players[player].secretcount; 
	 
    p = &players[player]; 
    __n64_memset_ZERO_ASM (p, 0, sizeof(*p)); 
 
    __n64_memcpy_ASM (players[player].frags, frags, sizeof(players[player].frags)); 
    players[player].killcount = killcount; 
    players[player].itemcount = itemcount; 
    players[player].secretcount = secretcount; 
 
    p->usedown = p->attackdown = true;	// don't do anything immediately 
    p->playerstate = PST_LIVE;       
    p->health = MAXHEALTH; 
    p->readyweapon = p->pendingweapon = wp_pistol; 
    p->weaponowned[wp_fist] = true; 
    p->weaponowned[wp_pistol] = true; 
    p->ammo[am_clip] = 50; 
	 
    for (i=0 ; i<NUMAMMO ; i++) 
	p->maxammo[i] = maxammo[i]; 
		 
}

//
// G_CheckSpot  
// Returns false if the player cannot be respawned
// at the given mapthing_t spot  
// because something is occupying it 
//
void P_SpawnPlayer (mapthing_t* mthing); 
 
boolean
G_CheckSpot
( int		playernum,
  mapthing_t*	mthing ) 
{ 
    fixed_t		x;
    fixed_t		y; 
    subsector_t*	ss; 
    unsigned		an; 
    mobj_t*		mo; 
    size_t			i;
	
    if (!players[playernum].mo)
    {
	// first spawn of level, before corpses
	for (i=0 ; i<playernum ; i++)
	    if (players[i].mo->x == mthing->x << FRACBITS
		&& players[i].mo->y == mthing->y << FRACBITS)
		return false;	
	return true;
    }
		
    x = mthing->x << FRACBITS; 
    y = mthing->y << FRACBITS; 
	 
    if (!P_CheckPosition (players[playernum].mo, x, y) ) 
	return false; 
 
    // flush an old corpse if needed 
    if (bodyqueslot >= BODYQUESIZE) 
	P_RemoveMobj (bodyque[bodyqueslot%BODYQUESIZE]); 
    bodyque[bodyqueslot%BODYQUESIZE] = players[playernum].mo; 
    bodyqueslot++; 
	
    // spawn a teleport fog 
    ss = R_PointInSubsector (x,y); 
    an = ( ANG45 * (mthing->angle/45) ) >> ANGLETOFINESHIFT; 
 
    mo = P_SpawnMobj (x+20*finecosine[an], y+20*finesine[an] 
		      , ss->sector->floorheight 
		      , MT_TFOG); 
	 
    if (players[consoleplayer].viewz != 1) 
	S_StartSound (mo, sfx_telept);	// don't start sound on first frame 
 
    return true; 
} 


//
// G_DeathMatchSpawnPlayer
// Spawns a player at one of the random death match spots
// called at level load and each death
//
void G_DeathMatchSpawnPlayer(int playernum)
{
    int i;
    int j;
    int selections;

    selections = deathmatch_p - deathmatchstarts;
    if (selections < 4)
    {
	char ermac[256];
	sprintf(ermac, "G_DeathMatchSpawnPlayer: Only %i deathmatch spots, 4 required", selections);
	I_Error(ermac);
    }
 
    for (j=0 ; j<20 ; j++) 
    { 
	i = P_Random() % selections; 
	if (G_CheckSpot (playernum, &deathmatchstarts[i]) ) 
	{ 
	    deathmatchstarts[i].type = playernum+1; 
	    P_SpawnPlayer (&deathmatchstarts[i]); 
	    return; 
	} 
    } 
 
    // no good spot, so the player will probably get stuck 
    P_SpawnPlayer (&playerstarts[playernum]); 
} 

//
// G_DoReborn 
// 
void G_DoReborn (int playernum) 
{ 
    int                             i; 
	 
    if (!netgame)
    {
	// reload the level from scratch
	gameaction = ga_loadlevel;  
    }
    else 
    {
	// respawn at the start

	// first dissasociate the corpse 
	players[playernum].mo->player = NULL;   
		 
	// spawn at random spot if in death match 
	if (deathmatch) 
	{ 
	    G_DeathMatchSpawnPlayer (playernum); 
	    return; 
	} 
		 
	if (G_CheckSpot (playernum, &playerstarts[playernum]) ) 
	{ 
	    P_SpawnPlayer (&playerstarts[playernum]); 
	    return; 
	}
	
	// try to spawn at one of the other players spots 
	for (i=0 ; i<MAXPLAYERS ; i++)
	{
	    if (G_CheckSpot (playernum, &playerstarts[i]) ) 
	    { 
		playerstarts[i].type = playernum+1;	// fake as other player 
		P_SpawnPlayer (&playerstarts[i]); 
		playerstarts[i].type = i+1;		// restore 
		return; 
	    }	    
	    // he's going to be inside something.  Too bad.
	}
	P_SpawnPlayer (&playerstarts[playernum]); 
    } 
} 
 
 
void G_ScreenShot (void) 
{ 
    gameaction = ga_screenshot; 
} 
 


// DOOM Par Times
int pars[4][10] = 
{ 
    {0}, 
    {0,30,75,120,90,165,180,180,30,165}, 
    {0,90,90,90,120,90,360,240,30,170}, 
    {0,90,45,90,150,90,90,165,30,135} 
}; 

// DOOM II Par Times
int cpars[32] =
{
    30,90,120,120,90,150,120,120,270,90,	//  1-10
    210,150,150,150,210,150,420,150,210,150,	// 11-20
    240,150,180,150,150,300,330,420,300,180,	// 21-30
    120,30					// 31-32
};
 

//
// G_DoCompleted 
//
boolean		secretexit; 
extern char*	pagename; 
 
void G_ExitLevel (void) 
{ 
    secretexit = false; 
    gameaction = ga_completed; 
} 

// Here's for the german edition.
void G_SecretExitLevel (void) 
{ 
    // IF NO WOLF3D LEVELS, NO SECRET EXIT!
    if ( (gamemode == commercial)
      && (W_CheckNumForName("map31")<0))
	secretexit = false;
    else
	secretexit = true; 
    gameaction = ga_completed; 
} 
 
void G_DoCompleted (void) 
{ 
    int             i; 
	 
    gameaction = ga_nothing; 
 
    for (i=0 ; i<MAXPLAYERS ; i++) 
	if (playeringame[i]) 
	    G_PlayerFinishLevel (i);        // take away cards and stuff 
	 
    if (automapactive) 
	AM_Stop (); 
	
    if ( gamemode != commercial)
	switch(gamemap)
	{
	  case 8:
	    gameaction = ga_victory;
	    return;
	  case 9: 
	    for (i=0 ; i<MAXPLAYERS ; i++) 
		players[i].didsecret = true; 
	    break;
	}
		
#if 0  
Hmmm - why?
    if ( (gamemap == 8)
	 && (gamemode != commercial) ) 
    {
	// victory 
	gameaction = ga_victory; 
	return; 
    } 
	 
    if ( (gamemap == 9)
	 && (gamemode != commercial) ) 
    {
	// exit secret level 
	for (i=0 ; i<MAXPLAYERS ; i++) 
	    players[i].didsecret = true; 
    } 
#endif
    
	 
    wminfo.didsecret = players[consoleplayer].didsecret; 
    wminfo.epsd = gameepisode -1; 
    wminfo.last = gamemap -1;
    
    // wminfo.next is 0 biased, unlike gamemap
    if ( gamemode == commercial)
    {
	if (secretexit)
	    switch(gamemap)
	    {
	      case 15: wminfo.next = 30; break;
	      case 31: wminfo.next = 31; break;
	    }
	else
	    switch(gamemap)
	    {
	      case 31:
	      case 32: wminfo.next = 15; break;
	      default: wminfo.next = gamemap;
	    }
    }
    else
    {
	if (secretexit) 
	    wminfo.next = 8; 	// go to secret level 
	else if (gamemap == 9) 
	{
	    // returning from secret level 
	    switch (gameepisode) 
	    { 
	      case 1: 
		wminfo.next = 3; 
		break; 
	      case 2: 
		wminfo.next = 5; 
		break; 
	      case 3: 
		wminfo.next = 6; 
		break; 
	      case 4:
		wminfo.next = 2;
		break;
	    }                
	} 
	else 
	    wminfo.next = gamemap;          // go to next level 
    }
		 
    wminfo.maxkills = totalkills; 
    wminfo.maxitems = totalitems; 
    wminfo.maxsecret = totalsecret; 
    wminfo.maxfrags = 0; 
    if ( gamemode == commercial )
	wminfo.partime = 35*cpars[gamemap-1]; 
    else
	wminfo.partime = 35*pars[gameepisode][gamemap]; 
    wminfo.pnum = consoleplayer; 
 
    for (i=0 ; i<MAXPLAYERS ; i++) 
    { 
	wminfo.plyr[i].in = playeringame[i]; 
	wminfo.plyr[i].skills = players[i].killcount; 
	wminfo.plyr[i].sitems = players[i].itemcount; 
	wminfo.plyr[i].ssecret = players[i].secretcount; 
	wminfo.plyr[i].stime = leveltime; 
	__n64_memcpy_ASM (wminfo.plyr[i].frags, players[i].frags 
		, sizeof(wminfo.plyr[i].frags)); 
    } 
 
    gamestate = GS_INTERMISSION; 
    viewactive = false; 
    automapactive = false; 
 
    if (statcopy)
	__n64_memcpy_ASM (statcopy, &wminfo, sizeof(wminfo));
	
    WI_Start (&wminfo); 
} 


//
// G_WorldDone 
//
void G_WorldDone (void) 
{ 
    gameaction = ga_worlddone; 

    if (secretexit) 
	players[consoleplayer].didsecret = true; 

    if ( gamemode == commercial )
    {
	switch (gamemap)
	{
	  case 15:
	  case 31:
	    if (!secretexit)
		break;
	  case 6:
	  case 11:
	  case 20:
	  case 30:
	    F_StartFinale ();
	    break;
	}
    }
} 
 
void G_DoWorldDone (void) 
{        
    gamestate = GS_LEVEL; 
    gamemap = wminfo.next+1; 
    G_DoLoadLevel (); 
    gameaction = ga_nothing; 
    viewactive = true; 
} 
 
byte array1[256*128];	
byte array2[256*128];
	


//
// G_InitFromSavegame
// Can be called by the startup code or the menu task. 
//
extern boolean setsizeneeded;
void R_ExecuteSetViewSize (void);

extern void DebugOutput_String(const char *str, int good_or_bad);

char	savename[256];

void G_LoadGame (char* name) 
{ 
    strcpy (savename, name); 
    gameaction = ga_loadgame; 
} 
 
#define VERSIONSIZE		16 

// a block of memory that is the full size of one mempak
byte __attribute__((aligned(8))) readbuffer[MEMPAK_BLOCK_SIZE * 128];

void G_DoLoadGame (void)
{
    char *gameid = get_GAMEID();
    char *msg = "Loaded game from mempak.";
    int i;
    int j;
    int lmn;
    int a,b,c;
    char vcheck[VERSIONSIZE];

    gameaction = ga_nothing;

    int err = 0;
    entry_structure_t entry;

    // flag set when mempak entry name matches the current game identifier
    int entry_found = 1;
    int any_entries = 0;

	// maximum 16 mempak entries
	for (j=0;j<16;j++)
	{
        // reset this flag on each iteration! -- bug-fixed 2014/09/11 23:45
        entry_found = 1;
		
		get_mempak_entry( 0, j, &entry );
		
		if( entry.valid )
		{
			any_entries = 1;

			// using value of get_GAMEID() without the ".WAD" suffix as identifier for entry
			// clears "entry_found" if strings don't match
			// breaks when a "." is found in the name (filename)
			for(lmn = 0; lmn < strlen(gameid)-1; lmn++)
 			{
 				if(gameid[lmn] == '.') break;
				if(entry.name[lmn] != gameid[lmn]) entry_found = 0;
			}
			
			// this means that one of the mempak entries had a name matching current game version
			if(entry_found)
			{
				break;
			}
		}
		// invalid entry
		else
		{
			entry_found = 0;
		}
	}

    // bug-fix 2017-10-06 12:29 modified get_GAMEID to use a static char[16]
#if 0
    // get_GAMEID uses n64_malloc to create the buffer it returns
    n64_free(gameid);
#endif
	
    // not finding an entry is NOT an error case that should case the game to hang
    // suggest to Doom that it print a message about it
    if (!entry_found || !any_entries)
    {
        msg = "Must save a game to load!";
        goto the_end_of_loading;
    }
	
    // found an entry, but the "valid" flag is not set, this is probably an error case
    // I am going to make this just print a message and return, instead of I_Error-ing
    if (any_entries && entry_found && (!entry.valid))
    {
        msg = "Save game invalid. Not loading.";
        goto the_end_of_loading;
    }	

    // read the mempak blocks referenced by the entry into readbuf for later processing
    err |= read_mempak_entry_data(0, &entry, readbuffer);
    if (err)
    {
        I_Error("G_DoLoadGame: Error loading data!\nread_mempak_entry_data failed.");
    }
	
    // the first block of the entry is used to store the exact size in bytes of the savebuffer
    // this size is the original un-compressed size
    // wasting 256 bytes to store a 4 byte int... oh well
    int *size_block_ptr = (int *)readbuffer;
    int dec_size = *size_block_ptr;
	
    // allocate another block of memory for the LZW-decoded savebuffer
    byte *to_decode = array2;//n64_malloc(dec_size);
    // if we try to lzw_decode the savebuffer directly from the blocks we read into readbuffer,
    // it will fail due to all of the extra bytes stuck on the end, even if they are empty
    // sequence doesn't end with correct symbol, some kind of error like that
    for (i=0;i<dec_size;i++)
    {
        to_decode[i] = readbuffer[MEMPAK_BLOCK_SIZE + i];
    }

    // dec ends up with the original, un-compressed savebuffer
	unsigned int unencsize = 0x2c000;
	byte *dec = array1;//(byte*)n64_malloc(dec_size * 4);
    lzfx_decompress(to_decode, dec_size, dec, &unencsize);
	
    // copy the data from "dec" into "savebuffer" so the rest of the original G_DoLoadGame code can do its thing like before
    __n64_memcpy_ASM(savebuffer, dec, unencsize);
	
//    n64_free(dec);
 //   n64_free(to_decode);

	save_p = savebuffer + SAVESTRINGSIZE;

    // skip the description field
    __n64_memset_ZERO_ASM(vcheck,0,sizeof(vcheck));
    sprintf(vcheck,"version %i",VERSION);
    if (strcmp ((char *)save_p, vcheck))
    {
        // bad version
        msg = "Save game failed version check.";
        // done
        goto the_end_of_loading;
    }

    save_p += VERSIONSIZE;

    gameskill = *save_p++; 
    gameepisode = *save_p++; 
    gamemap = *save_p++; 
    for (i=0 ; i<MAXPLAYERS ; i++) 
	playeringame[i] = *save_p++; 

    // load a base level 
    G_InitNew (gameskill, gameepisode, gamemap); 
    // get the times 
    a = *save_p++; 
    b = *save_p++; 
    c = *save_p++; 
    leveltime = (a<<16) + (b<<8) + c; 
	 
    // dearchive all the modifications
    P_UnArchivePlayers (); 
    P_UnArchiveWorld (); 
    P_UnArchiveThinkers (); 
    P_UnArchiveSpecials (); 
 
    if (*save_p != 0x1d)
    {
	    I_Error ("G_DoLoadGame: Bad savegame");
    }
	
    // done
the_end_of_loading:
    __n64_memset_ZERO_ASM(savebuffer, 0x00, SAVEGAMESIZE);

    players[consoleplayer].message = msg;

    if (setsizeneeded)
    {
	    R_ExecuteSetViewSize();
    }

    // draw the pattern into the back screen
    R_FillBackScreen();
} 
 

//
// G_SaveGame
// Called by the menu task.
// Description is a 24 byte text string 
//
void
G_SaveGame
( int	slot,
  char*	description ) 
{ 
    savegameslot = slot; 
    strcpy (savedescription, description); 
    sendsave = true; 
} 
 
byte __attribute__((aligned(8))) size_block[MEMPAK_BLOCK_SIZE] = {0};
byte __attribute__((aligned(8))) final_block[MEMPAK_BLOCK_SIZE] = {0};
	
void G_DoSaveGame (void) 
{ 
    char	name[100]; 
    char	name2[VERSIONSIZE]; 
    char*	description; 
    char    *msg = "Saved game to mempak.";
	int     available_free_blocks;
    int		length; 
    int		i; 
	int     j;
	int     k;
	
    if (M_CheckParm("-cdrom"))
    {
	    sprintf(name,"c:\\doomdata\\"SAVEGAMENAME"%d.dsg",savegameslot);
    }
    else
    {
	    sprintf(name,SAVEGAMENAME"%d.dsg",savegameslot);
    }

    description = savedescription; 
	 
    save_p = savebuffer;// = screens[1]+0x4000;
	 
    __n64_memcpy_ASM (save_p, description, SAVESTRINGSIZE);
    save_p += SAVESTRINGSIZE;
    __n64_memset_ZERO_ASM (name2,0,sizeof(name2));
    sprintf (name2,"version %i",VERSION);
    __n64_memcpy_ASM (save_p, name2, VERSIONSIZE);
    save_p += VERSIONSIZE;
 	 
    *save_p++ = gameskill; 
    *save_p++ = gameepisode; 
    *save_p++ = gamemap; 
    for (i=0 ; i<MAXPLAYERS ; i++)
	{
	    *save_p++ = playeringame[i]; 
	}
    *save_p++ = leveltime>>16; 
    *save_p++ = leveltime>>8; 
    *save_p++ = leveltime; 
 
    P_ArchivePlayers (); 
    P_ArchiveWorld (); 
    P_ArchiveThinkers (); 
    P_ArchiveSpecials (); 
	 
    *save_p++ = 0x1d;		// consistancy marker 
	 
    length = save_p - savebuffer; 
    if (length > SAVEGAMESIZE)
    {
	    I_Error("G_DoSaveGame: Savegame buffer overrun");
    } 
	
    // CHECK AVAILABLE FREE SPACE ON THE MEMPAK IN FIRST CONTROLLER
    // VALUE IS IN BLOCKS (256 byte chunks)
    available_free_blocks = get_mempak_free_space(0);
	
	if (0 == available_free_blocks)
	{
		msg = "Mempak is full. Not saving.";
		goto the_end_of_saving;
	}
    last_save_size = length*4;
	byte *enc = array1;//(byte *)n64_malloc(length*4);
	int rv = lzfx_compress(savebuffer, length, enc, &last_save_size);
	if(0 > rv) {
		//	char errbufmac[256];
	//sprintf(errbufmac, "lzfx_compress failed %08X %d", length, rv);
	//I_Error(errbufmac);
		msg = "cant malloc for compress";
		goto the_end_of_saving;
		//I_Error("lzfx_compress failed");
	}
/*	char errbufmac[256];
	sprintf(errbufmac, "%08X", last_save_size);
	I_Error(errbufmac);*/
	
	int save_size_in_blocks = SAVE_SIZE_IN_BLOCKS(last_save_size);
	
	int err = 0;
	
    int num_blocks = last_save_size / MEMPAK_BLOCK_SIZE;
    int last_block_size = last_save_size % MEMPAK_BLOCK_SIZE;
	
	__n64_memset_ZERO_ASM (size_block, 0, MEMPAK_BLOCK_SIZE);
	__n64_memset_ZERO_ASM (final_block, 0, MEMPAK_BLOCK_SIZE);
	
	int *size_block_ptr = (int *)size_block;
	*size_block_ptr = last_save_size;
	
	int total_blocks = 1 + num_blocks + 1;
	byte *tmp_blocks = array2;//(byte *)n64_malloc(MEMPAK_BLOCK_SIZE * total_blocks);
	
    if (NULL == tmp_blocks)
    {
        msg = "No memory to malloc save data.";
        err = -1;
        goto the_end_of_saving;
    }
	
    for (j=0; j<last_block_size;j++)
    {
        final_block[j] = enc[(num_blocks * MEMPAK_BLOCK_SIZE) + j];
    }

    for (k=0;k<MEMPAK_BLOCK_SIZE;k++)
    {
        tmp_blocks[k] = size_block[k];
    }

    for (j=0;j<num_blocks;j++)
    {
        for (k=0;k<MEMPAK_BLOCK_SIZE;k++)
        {
            tmp_blocks[((j+1)*MEMPAK_BLOCK_SIZE)+k] = enc[(j*MEMPAK_BLOCK_SIZE)+k];
        }
    }
	
    for (k=0;k<last_block_size;k++)
    {
        tmp_blocks[((num_blocks+1)*MEMPAK_BLOCK_SIZE)+k] = final_block[k];
    }
	
    int old_entry_existed = 0;
    int old_entry_size = -1;
	
    char *gameid = get_GAMEID();
	
    for (int j = 0; j < 16; j++)
    {
        entry_structure_t entry;
        get_mempak_entry(0, j, &entry);

        int matches = 1;

        if (entry.valid)
        {
            for(int lmn = 0; lmn < strlen(gameid)-1; lmn++)
            {
                if(gameid[lmn] == '.') break;
                if(entry.name[lmn] != gameid[lmn]) matches = 0;
            }

            if(matches)
            {
                old_entry_existed = 1;
                old_entry_size = entry.blocks;

                if (save_size_in_blocks > (available_free_blocks + old_entry_size))
                {
                    msg = "Not enough mempak space for new save.";
  //                I_Error(msg);
  //                  err = -1;
                    goto the_end_of_saving;
                }

                err |= delete_mempak_entry(0, &entry);
                if( err )
                {
                      msg = "Can't remove old data, can't save.";
//                      I_Error(msg);
                      goto the_end_of_saving;
                }

                break;
            }
        }
    }

    entry_structure_t doom_save_entry;
    doom_save_entry.blocks = total_blocks;
    doom_save_entry.region = 'A';
	
    for (i=0;i<strlen(gameid)-1;i++)
    {
        if (gameid[i] == '.') break;
        doom_save_entry.name[i] = gameid[i];
    }

    // bug-fix 2017-10-06 12:31 changed get_GAMEID to use static char[16]	
#if 0
    // get_GAMEID uses n64_malloc to create the buffer it returns
    n64_free(gameid);
#endif

    err = 0;
    err |= write_mempak_entry_data(0, &doom_save_entry, tmp_blocks);
    if (err)
    {
        msg = "Error saving, all mempak data lost.";
        goto the_end_of_saving;
    }
	
 //   n64_free(tmp_blocks);
 //   n64_free(enc);
	
the_end_of_saving:
    gameaction = ga_nothing;
    savedescription[0] = 0;

    players[consoleplayer].message = msg;

    if (setsizeneeded)
    {
	R_ExecuteSetViewSize();
    }

    // draw the pattern into the back screen
    R_FillBackScreen ();	
} 
 

//
// G_InitNew
// Can be called by the startup code or the menu task,
// consoleplayer, displayplayer, playeringame[] should be set. 
//
skill_t	d_skill; 
int     d_episode; 
int     d_map; 
 
void
G_DeferedInitNew
( skill_t	skill,
  int		episode,
  int		map) 
{ 
    d_skill = skill; 
    d_episode = episode; 
    d_map = map; 
    gameaction = ga_newgame; 
} 


void G_DoNewGame (void) 
{
    demoplayback = false; 
    netdemo = false;
    netgame = false;
    deathmatch = false;
    playeringame[1] = playeringame[2] = playeringame[3] = 0;
    respawnparm = false;
    fastparm = false;
    nomonsters = false;
    consoleplayer = 0;
    G_InitNew (d_skill, d_episode, d_map); 
    gameaction = ga_nothing; 
} 

// The sky texture to be used instead of the F_SKY1 dummy.
extern  int	skytexture; 


void
G_InitNew
( skill_t	skill,
  int		episode,
  int		map ) 
{ 
    int             i; 
	 
    if (paused) 
    { 
	paused = false; 
	S_ResumeSound (); 
    } 
	

    if (skill > sk_nightmare) 
	skill = sk_nightmare;


    // This was quite messy with SPECIAL and commented parts.
    // Supposedly hacks to make the latest edition work.
    // It might not work properly.
    if (episode < 1)
      episode = 1; 

    if ( gamemode == retail )
    {
      if (episode > 4)
	episode = 4;
    }
    else if ( gamemode == shareware )
    {
      if (episode > 1) 
	   episode = 1;	// only start episode 1 on shareware
    }  
    else
    {
      if (episode > 3)
	episode = 3;
    }
    

  
    if (map < 1) 
	map = 1;
    
    if ( (map > 9)
	 && ( gamemode != commercial) )
      map = 9; 
		 
    M_ClearRandom (); 
	 
    if (skill == sk_nightmare || respawnparm )
	respawnmonsters = true;
    else
	respawnmonsters = false;
		
    if (fastparm || (skill == sk_nightmare && gameskill != sk_nightmare) )
    { 
	for (i=S_SARG_RUN1 ; i<=S_SARG_PAIN2 ; i++) 
	    states[i].tics >>= 1; 
	mobjinfo[MT_BRUISERSHOT].speed = 20*FRACUNIT; 
	mobjinfo[MT_HEADSHOT].speed = 20*FRACUNIT; 
	mobjinfo[MT_TROOPSHOT].speed = 20*FRACUNIT; 
    } 
    else if (skill != sk_nightmare && gameskill == sk_nightmare) 
    { 
	for (i=S_SARG_RUN1 ; i<=S_SARG_PAIN2 ; i++) 
	    states[i].tics <<= 1; 
	mobjinfo[MT_BRUISERSHOT].speed = 15*FRACUNIT; 
	mobjinfo[MT_HEADSHOT].speed = 10*FRACUNIT; 
	mobjinfo[MT_TROOPSHOT].speed = 10*FRACUNIT; 
    } 
	 
			 
    // force players to be initialized upon first level load         
    for (i=0 ; i<MAXPLAYERS ; i++) 
	players[i].playerstate = PST_REBORN; 
 
    usergame = true;                // will be set false if a demo 
    paused = false; 
    demoplayback = false; 
    automapactive = false; 
    viewactive = true; 
    gameepisode = episode; 
    gamemap = map; 
    gameskill = skill; 
 
    viewactive = true;
    
    // set the sky map for the episode
    if ( gamemode == commercial)
    {
	skytexture = R_TextureNumForName ("SKY3");
	if (gamemap < 12)
	    skytexture = R_TextureNumForName ("SKY1");
	else
	    if (gamemap < 21)
		skytexture = R_TextureNumForName ("SKY2");
    }
    else
	switch (episode) 
	{ 
	  case 1: 
	    skytexture = R_TextureNumForName ("SKY1"); 
	    break; 
	  case 2: 
	    skytexture = R_TextureNumForName ("SKY2"); 
	    break; 
	  case 3: 
	    skytexture = R_TextureNumForName ("SKY3"); 
	    break; 
	  case 4:	// Special Edition sky
	    skytexture = R_TextureNumForName ("SKY4");
	    break;
	} 
    G_DoLoadLevel (); 
} 
 

//
// DEMO RECORDING 
// 
#define DEMOMARKER		0x80


void G_ReadDemoTiccmd (ticcmd_t* cmd) 
{ 
    if (*demo_p == DEMOMARKER) 
    {
	// end of demo data stream 
	G_CheckDemoStatus (); 
	return; 
    } 
    cmd->forwardmove = ((signed char)*demo_p++); 
    cmd->sidemove = ((signed char)*demo_p++); 
    cmd->angleturn = ((unsigned char)*demo_p++)<<8; 
    cmd->buttons = (unsigned char)*demo_p++; 
} 


void G_WriteDemoTiccmd (ticcmd_t* cmd) 
{ 
    if (gamekeydown['q'])           // press q to end demo recording 
	G_CheckDemoStatus (); 
    *demo_p++ = cmd->forwardmove; 
    *demo_p++ = cmd->sidemove; 
    *demo_p++ = (cmd->angleturn+128)>>8; 
    *demo_p++ = cmd->buttons; 
    demo_p -= 4; 
    if (demo_p > demoend - 16)
    {
	// no more space 
	G_CheckDemoStatus (); 
	return; 
    } 
	
    G_ReadDemoTiccmd (cmd);         // make SURE it is exactly the same 
} 
 
 
 
//
// G_RecordDemo 
// 
void G_RecordDemo (char* name) 
{ 
    int             i; 
    int				maxsize;
	
    usergame = false; 
    strcpy (demoname, name); 
    strcat (demoname, ".lmp"); 
    maxsize = 0x20000;
    i = M_CheckParm ("-maxdemo");
    if (i && i<myargc-1)
	maxsize = atoi(myargv[i+1])*1024;
    demobuffer = Z_Malloc (maxsize,PU_STATIC,NULL); 
    demoend = demobuffer + maxsize;
	
    demorecording = true; 
} 
 
 
void G_BeginRecording (void) 
{ 
    int             i; 
		
    demo_p = demobuffer;
	
    *demo_p++ = VERSION;
    *demo_p++ = gameskill; 
    *demo_p++ = gameepisode; 
    *demo_p++ = gamemap; 
    *demo_p++ = deathmatch; 
    *demo_p++ = respawnparm;
    *demo_p++ = fastparm;
    *demo_p++ = nomonsters;
    *demo_p++ = consoleplayer;
	 
    for (i=0 ; i<MAXPLAYERS ; i++) 
	*demo_p++ = playeringame[i]; 		 
} 
 

//
// G_PlayDemo 
//

char*	defdemoname; 
 
void G_DeferedPlayDemo (char* name) 
{ 
    defdemoname = name; 
    gameaction = ga_playdemo; 
} 
 
void G_DoPlayDemo (void) 
{ 
    skill_t skill; 
    int             i, episode, map; 
	 
    gameaction = ga_nothing; 
    demobuffer = demo_p = W_CacheLumpName (defdemoname, PU_STATIC); 
    if ( LONG(*demo_p++) != VERSION)
    {
//      fprintf( stderr, "Demo is from a different game version!\n");
//      /*I_Error*/printf("Demo is from a different game version!\n");
      gameaction = ga_nothing;
      return;
    }
    
    skill = *demo_p++; 
    episode = *demo_p++; 
    map = *demo_p++; 
    deathmatch = *demo_p++;
    respawnparm = *demo_p++;
    fastparm = *demo_p++;
    nomonsters = *demo_p++;
    consoleplayer = *demo_p++;
	
    for (i=0 ; i<MAXPLAYERS ; i++) 
	playeringame[i] = *demo_p++; 
    if (playeringame[1]) 
    { 
	netgame = true; 
	netdemo = true; 
    }

    // don't spend a lot of time in loadlevel 
    precache = false;
//    printf("Begin G_InitNew(skill, episode, map);\n");
    G_InitNew (skill, episode, map); 
//    printf("End G_InitNew(skill, episode, map);\n");
    precache = true; 

    usergame = false; 
    demoplayback = true; 
} 

//
// G_TimeDemo 
//
void G_TimeDemo (char* name) 
{ 	 
    nodrawers = M_CheckParm ("-nodraw"); 
    noblit = M_CheckParm ("-noblit"); 
    timingdemo = true; 
    singletics = true; 

    defdemoname = name; 
    gameaction = ga_playdemo; 
} 
 
 
/* 
=================== 
= 
= G_CheckDemoStatus 
= 
= Called after a death or level completion to allow demos to be cleaned up 
= Returns true if a new demo loop action will take place 
=================== 
*/

boolean G_CheckDemoStatus(void)
{
    int             endtime;

    if (timingdemo)
    {
	endtime = I_GetTime ();
	char ermac[256];
	sprintf(ermac, "G_CheckDemoStatus: timed %i gametics in %i realtics", gametic, endtime-starttime);
	I_Error(ermac);
    } 
	 
    if (demoplayback) 
    { 
	if (singledemo) 
	    I_Quit (); 
			 
	Z_ChangeTag (demobuffer, PU_CACHE); 
	demoplayback = false; 
	netdemo = false;
	netgame = false;
	deathmatch = false;
	playeringame[1] = playeringame[2] = playeringame[3] = 0;
	respawnparm = false;
	fastparm = false;
	nomonsters = false;
	consoleplayer = 0;
	D_AdvanceDemo (); 
	return true; 
    } 
 
    if (demorecording) 
    { 
	*demo_p++ = DEMOMARKER; 
	M_WriteFile (demoname, demobuffer, demo_p - demobuffer); 
	Z_Free (demobuffer); 
	demorecording = false;
	char ermac[256];
	sprintf(ermac, "G_CheckDemoStatus: Demo %s recorded",demoname);
	I_Error(ermac);
    } 
	 
    return false; 
}
