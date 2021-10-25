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
//  Sky rendering. The DOOM sky is a texture map like any
//  wall, wrapping around. A 1024 columns equal 360 degrees.
//  The default sky map is 256 columns and repeats 4 times
//  on a 320 screen?
//  
//
//-----------------------------------------------------------------------------

// Needed for FRACUNIT.
#include "m_fixed.h"

// Needed for Flat retrieval.
#include "r_data.h"


#ifdef __GNUG__
#pragma implementation "r_sky.h"
#endif
#include "r_sky.h"

//
// sky mapping
//
int			skyflatnum;
int			skytexture;
int			skytexturemid;

sprite_t *sky_sprite;// = malloc(sizeof(sprite_t) + (256*128));
sprite_t *sky_sprite2;// = malloc(sizeof(sprite_t) + (256*128));

//
// R_InitSkyMap
// Called whenever the view size changes.
//
void R_InitSkyMap (void)
{
  // skyflatnum = R_FlatNumForName ( SKYFLATNAME );
    skytexturemid = 100*FRACUNIT;
#if 0
	sky_sprite = malloc(sizeof(sprite_t) + (32*32));	
	sky_sprite->width = 32;
	sky_sprite->height = 32;
	sky_sprite->bitdepth = 1;
	sky_sprite->vslices = 1;
	sky_sprite->hslices = 1;
	__n64_memset_ASM(sky_sprite->data, 0, 32*32);

	sky_sprite2 = malloc(sizeof(sprite_t) + (32*32));	
	sky_sprite2->width = 32;
	sky_sprite2->height = 32;
	sky_sprite2->bitdepth = 1;
	sky_sprite2->vslices = 1;
	sky_sprite2->hslices = 1;
	__n64_memset_ASM(sky_sprite2->data, 0, 32*32);
#endif
}

