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
#define DATALENGTH 512
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "doomdef.h"

#include "i_system.h"
#include "d_event.h"
#include "d_net.h"
#include "m_argv.h"

#include "doomstat.h"

#ifdef __GNUG__
#pragma implementation "i_net.h"
#endif
#include "i_net.h"


int get_counter = 0;
int get_flag = 0;
char backup_packet[DATALENGTH];

// For some odd reason...
#define htonl(x) \
        ((unsigned long int)((((unsigned long int)(x) & 0x000000ffU) << 24) | \
                             (((unsigned long int)(x) & 0x0000ff00U) <<  8) | \
                             (((unsigned long int)(x) & 0x00ff0000U) >>  8) | \
                             (((unsigned long int)(x) & 0xff000000U) >> 24)))


#define htons(x) \
        ((unsigned short int)((((unsigned short int)(x) & 0x00ff) << 8) | \
                              (((unsigned short int)(x) & 0xff00) >> 8))) \

	  
#define ntohl(x) x
//ntohl(x)
#define ntohs(x) x
//ntohs(x)

void	NetSend (void);
boolean NetListen (void);

//
// NETWORKING
//

#ifndef IPPORT_USERRESERVED
#define	IPPORT_USERRESERVED	5000
#endif // IPPORT_USERRESERVED

int	DOOMPORT =	(IPPORT_USERRESERVED +0x1d );

int			sendsocket;
int			insocket;

//struct	sockaddr_in	sendaddress[MAXNETNODES];

void	(*netget) (void);
void	(*netsend) (void);


//
// UDPsocket
//
int UDPsocket (void)
{
	return -1;
}

//
// BindToLocalPort
//
void
BindToLocalPort
( int	s,
  int	port )
{
}

//
// PacketSend
//
void PacketSend (void)
{
#if 0
	doomdata_t	__attribute__((aligned(16))) send_sw;
    int		c;

    // byte swap
    send_sw.checksum = htonl(netbuffer->checksum);
    send_sw.player = netbuffer->player;
    send_sw.retransmitfrom = netbuffer->retransmitfrom;
    send_sw.starttic = netbuffer->starttic;
    send_sw.numtics = netbuffer->numtics;
    for (c=0 ; c< netbuffer->numtics ; c++)
    {
		send_sw.cmds[c].forwardmove = netbuffer->cmds[c].forwardmove;
		send_sw.cmds[c].sidemove = netbuffer->cmds[c].sidemove;
		send_sw.cmds[c].angleturn = htons(netbuffer->cmds[c].angleturn);
		send_sw.cmds[c].consistancy = htons(netbuffer->cmds[c].consistancy);
		send_sw.cmds[c].chatchar = netbuffer->cmds[c].chatchar;
		send_sw.cmds[c].buttons = netbuffer->cmds[c].buttons;
    }
#endif	
}

//static int received_any = 0;

//
// PacketGet
//
void PacketGet (void)
{
#if 0
	doomdata_t	__attribute__((aligned(16))) recv_sw;
    int			i;
    int			c;

    // byte swap
    netbuffer->checksum = /*ntohl*/(recv_sw.checksum);
    netbuffer->player = recv_sw.player;
    netbuffer->retransmitfrom = recv_sw.retransmitfrom;
    netbuffer->starttic = recv_sw.starttic;
    netbuffer->numtics = recv_sw.numtics;
	
    for (c=0 ; c< netbuffer->numtics ; c++)
    {
		netbuffer->cmds[c].forwardmove = recv_sw.cmds[c].forwardmove;
		netbuffer->cmds[c].sidemove = recv_sw.cmds[c].sidemove;
		netbuffer->cmds[c].angleturn = /*ntohs*/(recv_sw.cmds[c].angleturn);
		netbuffer->cmds[c].consistancy = /*ntohs*/(recv_sw.cmds[c].consistancy);
		netbuffer->cmds[c].chatchar = recv_sw.cmds[c].chatchar;
		netbuffer->cmds[c].buttons = recv_sw.cmds[c].buttons;
    }	
#endif
}

int GetLocalAddress (void)
{
	return 0;
}

//
// I_InitNetwork
//
void I_InitNetwork (void)
{
/*    boolean		trueval = true;
    int			i;
    int			p;
*/
	
    doomcom = (doomcom_t*)malloc (sizeof (*doomcom));
    memset (doomcom, 0, sizeof(*doomcom));
    
    // set up for network
/*    i = M_CheckParm ("-dup");
    if (i && i< myargc-1)
    {
	doomcom->ticdup = myargv[i+1][0]-'0';
	if (doomcom->ticdup < 1)
	    doomcom->ticdup = 1;
	if (doomcom->ticdup > 9)
2	    doomcom->ticdup = 9;
    }
    else*/
	doomcom-> ticdup = 1;
	
/*    if (M_CheckParm ("-extratic"))
	doomcom-> extratics = 1;
    else*/
	doomcom-> extratics = 0;
	consoleplayer = displayplayer  = doomcom->consoleplayer = 0;//myargv[i+1][0]-'1';
		
/*    p = M_CheckParm ("-port");
    if (p && p<myargc-1)
    {
	DOOMPORT = atoi (myargv[p+1]);
	printf ("using alternate port %i\n",DOOMPORT);
    } */
    
    // parse network game options,
    //  -net <consoleplayer> <host> <host> ...
//    i = M_CheckParm ("-net");
    if (1) //(!i)
    {
		// single player game
		netgame = false;
		doomcom->id = DOOMCOM_ID;
		doomcom->numplayers = doomcom->numnodes = 1;
		doomcom->deathmatch = false;
		doomcom->consoleplayer = 0;
		return;
    }
    else
    {
		netsend = PacketSend;
		netget = PacketGet;
		netgame = true;
    }

  // parse player number and host list
 
    doomcom->numnodes = 2;	// this node for sure

    doomcom->id = DOOMCOM_ID;
    doomcom->numplayers = doomcom->numnodes;
}


void I_NetCmd (void)
{
    if (doomcom->command == CMD_SEND)
    {
		    netsend ();
    }
    else if (doomcom->command == CMD_GET)
    {
		    netget ();
    }
    else
    {
        I_Error("I_NetCmd: Bad net cmd: %i\n", doomcom->command);
    }
}

