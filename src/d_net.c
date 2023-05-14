#define DATALENGTH 512

#include "m_menu.h"
#include "i_system.h"
#include "i_video.h"
#include "i_net.h"
#include "g_game.h"
#include "doomdef.h"
#include "doomstat.h"

#define	NCMD_EXIT		0x80000000
#define	NCMD_RETRANSMIT		0x40000000
#define	NCMD_SETUP		0x20000000
#define	NCMD_KILL		0x10000000	// kill game
#define	NCMD_CHECKSUM	 	0x0fffffff

doomcom_t*	doomcom;
doomdata_t*	netbuffer;		// points inside doomcom

//
// NETWORKING
//
// gametic is the tic about to (or currently being) run
// maketic is the tick that hasn't had control made for it yet
// nettics[] has the maketics for all players
//
// a gametic cannot be run until nettics[] > gametic for all players
//
#define	RESENDCOUNT	10
#define	PL_DRONE	0x80	// bit flag in doomdata->player

ticcmd_t	localcmds[BACKUPTICS];

ticcmd_t        netcmds[MAXPLAYERS][BACKUPTICS];
int         	nettics[MAXNETNODES];
boolean		nodeingame[MAXNETNODES];		// set false as nodes leave game
boolean		remoteresend[MAXNETNODES];		// set when local needs tics
int		resendto[MAXNETNODES];			// set when remote needs tics
int		resendcount[MAXNETNODES];

int		nodeforplayer[MAXPLAYERS];

int             maketic;
int		lastnettic;
int		skiptics;
int		ticdup;
int		maxsend;	// BACKUPTICS/(2*ticdup)-1


void D_ProcessEvents(void);
void G_BuildTiccmd(ticcmd_t *cmd);
void D_DoAdvanceDemo(void);

boolean		reboundpacket;
doomdata_t		__attribute__((aligned(16))) reboundstore;

//
//
//
int NetbufferSize(void)
{
    return DATALENGTH;
}

//
// Checksum
//
unsigned NetbufferChecksum (void)
{
    unsigned		c;
    c = 0;
    return c & NCMD_CHECKSUM;
}


//
//
//
int ExpandTics (int low)
{
    int	delta;

    delta = low - (maketic&0xff);

    if (delta >= -64 && delta <= 64)
    {
        return (maketic&~0xff) + low;
    }
    if (delta > 64)
    {
        return (maketic&~0xff) - 256 + low;
    }
    if (delta < -64)
    {
    return (maketic&~0xff) + 256 + low;
    }
#ifdef RANGECHECK
    IError("ExpandTics: strange value %i at maketic %i", low, maketic);
#endif
    return 0;
	
}



//
// HSendPacket
//
void HSendPacket(int node, int flags)
{
    netbuffer->checksum = NetbufferChecksum () | flags;

    if (!node)
    {
        reboundstore = *netbuffer;
        reboundpacket = true;
        return;
    }

    if (demoplayback)
    {
    	return;
    }
#ifdef RANGECHECK
    if (!netgame)
    {
        I_Error("HSendPacket: Tried to transmit to another node");
    }
#endif
    doomcom->command = CMD_SEND;
    doomcom->remotenode = node;
    doomcom->datalength = NetbufferSize ();
    I_NetCmd();
}

//
// HGetPacket
// Returns false if no packet is waiting
//
boolean HGetPacket(void)
{
    if (reboundpacket)
    {
		*netbuffer = reboundstore;
		doomcom->remotenode = 0;
		reboundpacket = false;
		return true;
    }

    if (!netgame)
		return false;

    if (demoplayback)
		return false;
		
    doomcom->command = CMD_GET;
    I_NetCmd ();
    
    if (doomcom->remotenode == -1)
		return false;

    if (doomcom->datalength != NetbufferSize ())
    {
		//if (debugfile)
		  //  printf ("bad packet length %i\n",doomcom->datalength);
		return false;
    }
	
    if ( (NetbufferChecksum()&NCMD_CHECKSUM) != (/*netbuffer->checksum&*/NCMD_CHECKSUM) )
    {
		//	if (debugfile)
		//    printf ("bad packet checksum\n");
		return false;
    }

    /*if (1)
    {
	int		realretrans;
//	int	i;
			
	if (netbuffer->checksum & NCMD_SETUP)
	{}    //printf ("setup packet\n");
	else
	{
	    if (netbuffer->checksum & NCMD_RETRANSMIT)
		realretrans = ExpandTics (netbuffer->retransmitfrom);
	    else
		realretrans = -1;
	    
	   printf ("get %i = (%i + %i, R %i)[%i] ",
		     doomcom->remotenode,
		     ExpandTics(netbuffer->starttic),
		     netbuffer->numtics, realretrans, doomcom->datalength);

//	    for (i=0 ; i<doomcom->datalength ; i++)
	//	fprintf (debugfile,"%i ",((byte *)netbuffer)[i]);
	  //  fprintf (debugfile,"\n")
	}
    }*/


    return true;	
}


//
// GetPackets
//
char    exitmsg[80];
#define PACKETS_PER_CALL 10
void GetPackets (void)
{
    int		netconsole;
    int		netnode;
    ticcmd_t	*src, *dest;
    int		realend;
    int		realstart;

    while ( HGetPacket() ) 
    {
		if (netbuffer->checksum & NCMD_SETUP) {
			continue;		// extra setup packet
		}
		netconsole = netbuffer->player & ~PL_DRONE;
		netnode = doomcom->remotenode;
		
		// to save bytes, only the low byte of tic numbers are sent
		// Figure out what the rest of the bytes are
		realstart = ExpandTics (netbuffer->starttic);		
		realend = (realstart+netbuffer->numtics);

		if(realstart == realend) {
			//return;
		}
	
		// check for exiting the game
		if (netbuffer->checksum & NCMD_EXIT)
		{
			if (!nodeingame[netnode])
			continue;
			nodeingame[netnode] = false;
			playeringame[netconsole] = false;
			strcpy (exitmsg, "Player 1 left the game");
			exitmsg[7] += netconsole;
			players[consoleplayer].message = exitmsg;
			if (demorecording)
			G_CheckDemoStatus ();
			continue;
		}
	
		// check for a remote game kill
		if (netbuffer->checksum & NCMD_KILL)
			I_Error ("GetPackets: Killed by network driver");

		nodeforplayer[netconsole] = netnode;
		
		// check for retransmit request
		if ( resendcount[netnode] <= 0 
			 && (netbuffer->checksum & NCMD_RETRANSMIT) )
		{
			resendto[netnode] = ExpandTics(netbuffer->retransmitfrom);
			//if (debugfile)
	//		fprintf (debugfile,"retransmit from %i\n", resendto[netnode]);
			resendcount[netnode] = RESENDCOUNT;
		}
		else
			resendcount[netnode]--;
		
		// check for out of order / duplicated packet		
		if (realend == nettics[netnode])
		{	
	//return;   
		continue;
		}
	
		if (realend < nettics[netnode])
		{
		//    if (debugfile)
	/*		fprintf (debugfile,
				 "out of order packet (%i + %i)\n" ,
				 realstart,netbuffer->numtics);*/
			continue;
		}
		
		// check for a missed packet
		if (realstart > nettics[netnode])
		{
			// stop processing until the other system resends the missed tics
	//	    if (debugfile)
	/*		fprintf (debugfile,
				 "missed tics from %i (%i - %i)\n",
				 netnode, realstart, nettics[netnode]);*/
			remoteresend[netnode] = true;
			continue;
		}

		// update command store from the packet
		{
			int		start;

			remoteresend[netnode] = false;
			
			start = nettics[netnode] - realstart;		
			src = &netbuffer->cmds[start];

			while (nettics[netnode] < realend)
			{
			dest = &netcmds[netconsole][nettics[netnode]%BACKUPTICS];
			nettics[netnode]++;
			*dest = *src;
			src++;
			}
		}	
    }
}


//
// NetUpdate
// Builds ticcmds for console player,
// sends out a packet
//
int      gametime;
//extern volatile uint64_t timekeeping;
void NetUpdate (void)
{
    int             nowtime;
    int             newtics;
    int				i,j;
    int				realstart;
    int				gameticdiv;
    
    // check time
    nowtime = I_GetTime ();///ticdup;

    newtics = nowtime - gametime;
    gametime = nowtime;
	
    if (newtics <= 0) 	// nothing new to update
		goto listen; 

    if (skiptics <= newtics)
    {
		newtics -= skiptics;
		skiptics = 0;
    }
    else
    {
		skiptics -= newtics;
		newtics = 0;
    }
	
		
    netbuffer->player = consoleplayer;
    
    // build new ticcmds for console player
    gameticdiv = gametic/ticdup;
    for (i=0 ; i<newtics ; i++)
    {
		I_StartTic ();
		D_ProcessEvents ();
		if (maketic - gameticdiv >= BACKUPTICS/2-1)
			break;          // can't hold any more
		
		//printf ("mk:%i ",maketic);
		G_BuildTiccmd (&localcmds[maketic%BACKUPTICS]);
		maketic++;
    }

    if (singletics)
		return;         // singletic update is syncronous
    
    // send the packet to the other nodes
    for (i=0 ; i<doomcom->numnodes ; i++)
	{
		if (nodeingame[i])
		{
			netbuffer->starttic = realstart = resendto[i];
			netbuffer->numtics = maketic - realstart;
			if (netbuffer->numtics > BACKUPTICS)
			I_Error ("NetUpdate: netbuffer->numtics > BACKUPTICS");

			resendto[i] = maketic - doomcom->extratics;

			for (j=0 ; j< netbuffer->numtics ; j++)
			netbuffer->cmds[j] = 
				localcmds[(realstart+j)%BACKUPTICS];
						
			if (remoteresend[i])
			{
			netbuffer->retransmitfrom = nettics[i];
			HSendPacket (i, NCMD_RETRANSMIT);
			}
			else
			{
			netbuffer->retransmitfrom = 0;
			HSendPacket (i, 0);
			}
		}
    }

    // listen for other packets
listen:
    GetPackets ();
}


//
// CheckAbort
//
void CheckAbort(void)
{
    event_t    *ev;
    int        stoptic;

    stoptic = I_GetTime () + 2;
    while (I_GetTime() < stoptic)
    {
	I_StartTic ();
    }

    I_StartTic ();

//    for ( ; eventtail != eventhead ; eventtail = (++eventtail)&(MAXEVENTS-1) )
    for ( ; eventtail != eventhead ; eventtail = (eventtail+1)&(MAXEVENTS-1) )
    {
        ev = &events[eventtail];

        if (ev->type == ev_keydown && ev->data1 == KEY_ESCAPE)
        {
            I_Error ("CheckAbort: Network game synchronization aborted.");
        }
    }
}


//
// D_ArbitrateNetStart
//
void D_ArbitrateNetStart (void)
{
#if 0
	char __attribute__((aligned(16))) packet[DATALENGTH];
	int			oldsec;
	int			localstage, remotestage;
	char		str[20];
	char		idstr[7];
	char		remoteidstr[7];
	int			i;		int idnum = 0;
	idstr[0] = '0' + idnum/ 100000l;
	idnum -= (idstr[0]-'0')*100000l;
	idstr[1] = '0' + idnum/ 10000l;
	idnum -= (idstr[1]-'0')*10000l;
	idstr[2] = '0' + idnum/ 1000l;
	idnum -= (idstr[2]-'0')*1000l;
	idstr[3] = '0' + idnum/ 100l;
	idnum -= (idstr[3]-'0')*100l;
	idstr[4] = '0' + idnum/ 10l;
	idnum -= (idstr[4]-'0')*10l;
	idstr[5] = '0' + idnum;
	idstr[6] = 0;
	//
// sit in a loop until things are worked out
//
// the packet is:  ID000000_0
// the first field is the idnum, the second is the acknowledge stage
// ack stage starts out 0, is bumped to 1 after the other computer's id
// is known, and is bumped to 2 after the other computer has raised to 1
//
	oldsec = -1;
	localstage = remotestage = 0;

	do {
		if (!bi_usb_can_rd()) {
			//doomcom->remotenode = -1;
			//return;
			continue; // ?
		}
		
		int c = bi_usb_rd(packet, DATALENGTH);
		// successful read
		if(0 == c)
		{
			if(packet[0] != 10)
			{
				continue;
			}		
			if (strncmp(&packet[0+4],"ID",2) )
				continue;
			strncpy (remoteidstr,&packet[2+4],6);
			//printf("got %s\n", remoteidstr);
			remotestage = packet[9+4] - '0';
			localstage = remotestage+1;
			oldsec = -1;			
		}
		
		int time = I_GetTime();
		if(time != oldsec)
		{
			oldsec = time;
sprintf (str,"ID%s_%i",idstr,localstage);
		//printf("sent %s\n", str);
		packet[0] = strlen(str);
		packet[1] = 0; packet[2] = 0; packet[3] = 0;
		D_memcpy(&packet[4], str, strlen(str));
		while(!bi_usb_can_wr()) {}
		//WritePacket (str,strlen(str))
			bi_usb_wr(packet,DATALENGTH);
		}
	} while (localstage < 2);
	
		deathmatch = 1;
		nomonsters = 1;
		startskill = sk_medium;
    startepisode = 2;
    startmap = 1;

return;
#endif
#if 1
    int		i;
    boolean	gotinfo[MAXNETNODES];

    autostart = true;
    D_memset (gotinfo,0,sizeof(gotinfo));
	
    if (doomcom->consoleplayer)
    {
	// listen for setup info from key player
	printf ("listening for network start info...\n");
	while (1)
	{
	    CheckAbort ();
	    if (!HGetPacket ())
		continue;
	    if (netbuffer->checksum & NCMD_SETUP)
	    {
		if (netbuffer->player != VERSION)
		    I_Error ("D_ArbitrateNetStart: Different DOOM versions cannot play a net game!");
		startskill = netbuffer->retransmitfrom & 15;
		deathmatch = (netbuffer->retransmitfrom & 0xc0) >> 6;
		nomonsters = (netbuffer->retransmitfrom & 0x20) > 0;
		respawnparm = (netbuffer->retransmitfrom & 0x10) > 0;
		startmap = netbuffer->starttic & 0x3f;
		startepisode = netbuffer->starttic >> 6;
		return;
	    }
	}
    }
    else
    {
		/*startskill = 0;
		deathmatch = 1;
		nomonsters = 1;
		startepisode = 1;
		startmap = 1;*/
		deathmatch = 1;
		nomonsters = 1;
		startskill = sk_medium;
    startepisode = 1;
    startmap = 1;
	// key player, send the setup info
	printf ("sending network start info...\n");
//	do
	{
		//printf("next loop\n");
	    CheckAbort ();
	    for (i=0 ; i<doomcom->numnodes ; i++)
	    {
		netbuffer->retransmitfrom = startskill;
		if (deathmatch)
		    netbuffer->retransmitfrom |= (deathmatch<<6);
		if (nomonsters)
		    netbuffer->retransmitfrom |= 0x20;
		if (respawnparm)
		    netbuffer->retransmitfrom |= 0x10;
		netbuffer->starttic = startepisode * 64 + startmap;
		netbuffer->player = VERSION;
		netbuffer->numtics = 0;
		HSendPacket (i, NCMD_SETUP);
	    }
/*
#if 1
	    for(i = 10 ; i  &&  HGetPacket(); --i)
	    {
			if((netbuffer->player&0x7f) < MAXNETNODES)
			{	
				//printf("gotinfo %d == true\n", netbuffer->player&0x7f);
				gotinfo[netbuffer->player&0x7f] = true;
			}
		}
#else
	    while (HGetPacket ())
	    {
		gotinfo[netbuffer->player&0x7f] = true;
	    }
#endif

	    for (i=1 ; i<doomcom->numnodes ; i++)
		if (!gotinfo[i])
		    break;
*/
gotinfo[0] = true;
gotinfo[1] = true;
//i=2;
		}
//		while (i < doomcom->numnodes); 
    }
	  //  nodeingame[0] = 1;
	//nodeingame[1] = 1;
#endif	
}

//
// D_CheckNetGame
// Works out player numbers among the net participants
//
extern	int			viewangleoffset;

void D_CheckNetGame (void)
{
    int             i;
	
    for (i=0 ; i<MAXNETNODES ; i++)
    {
	nodeingame[i] = false;
       	nettics[i] = 0;
	remoteresend[i] = false;	// set when local needs tics
	resendto[i] = 0;		// which tic to start sending
    }
	
    // I_InitNetwork sets doomcom and netgame
    I_InitNetwork ();
//    printf("I_InitNetwork\n");
    if (doomcom->id != DOOMCOM_ID)
	I_Error ("D_CheckNetGame: Doomcom buffer invalid!");
    
    netbuffer = &doomcom->data;
    consoleplayer = displayplayer = doomcom->consoleplayer;
    
	//n64_sleep_millis(1000);
	
	if (netgame)
	D_ArbitrateNetStart ();
//			deathmatch = 1;
//		nomonsters = 1;
//		startskill = sk_medium;
 //   startepisode = 2;
  //  startmap = 1;


//   printf ("startskill %i  deathmatch: %i  startmap: %i  startepisode: %i\n",
	//    startskill, deathmatch, startmap, startepisode);
	
    // read values out of doomcom
    ticdup = doomcom->ticdup;
    maxsend = BACKUPTICS/(2*ticdup)-1;
    if (maxsend<1)
	maxsend = 1;
			
    for (i=0 ; i<doomcom->numplayers ; i++)
	playeringame[i] = true;
    for (i=0 ; i<doomcom->numnodes ; i++)
	nodeingame[i] = true;
	
//    printf ("player %i of %i (%i nodes)\n",
	//    consoleplayer+1, doomcom->numplayers, doomcom->numnodes);

}


//
// D_QuitNetGame
// Called before quitting to leave a net game
// without hanging the other players
//
void D_QuitNetGame (void)
{
    int             i, j;
	
/*    if (debugfile)
	fclose (debugfile);*/
		
    if (!netgame || !usergame || consoleplayer == -1 || demoplayback)
	return;
	
    // send a bunch of packets for security
    netbuffer->player = consoleplayer;
    netbuffer->numtics = 0;
    for (i=0 ; i<4 ; i++)
    {
	for (j=1 ; j<doomcom->numnodes ; j++)
	    if (nodeingame[j])
		HSendPacket (j, NCMD_EXIT);
	I_WaitVBL (1);
    }
}



//
// TryRunTics
//
int	frametics[4];
int	frameon;
int	frameskip[4];
int	oldnettics;

extern	boolean	advancedemo;
    static int	oldentertics=0;
void TryRunTics (void)
{
    int		i;
    int		lowtic;
    int		entertic;

    int		realtics;
    int		availabletics;
    int		counts;
    int		numplaying;
    
    // get real tics		
    entertic = I_GetTime ();//(timekeeping >> 2);// /ticdup;
    realtics = entertic - oldentertics;
    oldentertics = entertic;
    
    // get available tics
    NetUpdate ();
	
    lowtic = MAXINT;
    numplaying = 0;
    for (i=0 ; i<doomcom->numnodes ; i++)
    {
	if (nodeingame[i])
	{
	    numplaying++;
	    if (nettics[i] < lowtic)
		lowtic = nettics[i];
	}
    }
    availabletics = lowtic - gametic;///ticdup;
    
    // decide how many tics to run
    if (realtics < availabletics-1)
	counts = realtics+1;
    else if (realtics < availabletics)
	counts = realtics;
    else
	counts = availabletics;
    
    if (counts < 1)
	counts = 1;
		
    frameon++;

/*    if (debugfile)
	fprintf (debugfile,
		 "=======real: %i  avail: %i  game: %i\n",
		 realtics, availabletics,counts);*/

    if (!demoplayback)
    {	
		// ideally nettics[0] should be 1 - 3 tics above lowtic
		// if we are consistantly slower, speed up time
		for (i=0 ; i<MAXPLAYERS ; i++)
		{
			if (playeringame[i])
			{
				break;
			}
		}
	//}
	
	if (consoleplayer == i)
	{
	    // the key player does not adapt
	}
	else
	{
	    if (nettics[0] <= nettics[nodeforplayer[i]])
	    {
			gametime--;
			// printf ("-");
	    }
	    frameskip[frameon&3] = (oldnettics > nettics[nodeforplayer[i]]);
	    oldnettics = nettics[0];
	    if (frameskip[0] && frameskip[1] && frameskip[2] && frameskip[3])
	    {
			skiptics = 1;
			// printf ("+");
	    }
	}
    }// demoplayback
	
    // wait for new tics if needed
    while (lowtic < gametic/*/ticdup*/ + counts)	
    {
	NetUpdate ();   
	lowtic = MAXINT;
	
	for (i=0 ; i<doomcom->numnodes ; i++)
	    if (nodeingame[i] && nettics[i] < lowtic)
		lowtic = nettics[i];
	
	if (lowtic < gametic/*/ticdup*/)
	    I_Error ("TryRunTics: lowtic < gametic");
				
	// don't stay in here forever -- give the menu a chance to work
	if (I_GetTime ()/*/ticdup*//*(timekeeping >> 2)*/ - entertic >= 20)
	{
	    M_Ticker ();
	    return;
	} 
    }
    
    // run the count * ticdup dics
    while (counts--)
    {
	for (i=0 ; i<ticdup ; i++)
	{
	    if (gametic/*/ticdup*/ > lowtic)
		I_Error ("TryRunTics: gametic > lowtic");
	    if (advancedemo)
		D_DoAdvanceDemo ();
	    M_Ticker ();
	    G_Ticker ();
	    gametic++;
	    
	    // modify command for duplicated tics
	    if (i != ticdup-1)
	    {
		ticcmd_t	*cmd;
		int			buf;
		int			j;
				
		buf = (gametic/*/ticdup*/)%BACKUPTICS; 
		for (j=0 ; j<MAXPLAYERS ; j++)
		{
		    cmd = &netcmds[j][buf];
		    cmd->chatchar = 0;
		    if (cmd->buttons & BT_SPECIAL)
			cmd->buttons = 0;
		}
	    }
	}
	NetUpdate ();	// check for new console commands
    }
}
