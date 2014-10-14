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
// DESCRIPTION:
//	Handles WAD file header, directory, lump I/O.
//
//-----------------------------------------------------------------------------


#include <libdragon.h>

#include <ctype.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <alloca.h>

#include "doomdef.h"

#include "doomtype.h"
#include "m_swap.h"
#include "i_system.h"
#include "z_zone.h"

#ifdef __GNUG__
#pragma implementation "w_wad.h"
#endif
#include "w_wad.h"

#include "hash.h"


extern void n64_free(void *buf);
extern void *n64_malloc(size_t size_to_alloc);
extern void *n64_memcpy(void *d, const void *s, size_t n);
extern void *n64_memset(void *p, int v, size_t n);
extern long rom_tell(int fd);
extern int rom_lseek(int fd, off_t offset, int whence);
extern int rom_open(int FILE_START, int size);
extern int rom_close(int fd);
extern int rom_read(int fd, void *buf, size_t nbyte);

extern uint32_t final_screen[];
extern void master_blaster();

static char error[256];
static hashtable_t ht;

extern const int WAD_FILE;
extern const int WAD_FILESIZE;


void wadupr(char *s)
{
    int i;

    for (i=0;i<8;i++)
    {
        if (s[i] == '\0')
        {
            return;
        }

        s[i] = toupper(s[i]);
    }
}


//
// GLOBALS
//

// Location of each lump on disk.
lumpinfo_t*		lumpinfo;
int			numlumps;

void**			lumpcache;


#define strcmpi	strcasecmp


void ExtractFileBase(char* path, char* dest)
{
    char*	src;
    int		length;

    src = path + strlen(path) - 1;

    // back up until a \ or the start
    while ((src != path) && (*(src-1) != '\\') && (*(src-1) != '/'))
    {
	src--;
    }

    // copy up to eight characters
    n64_memset(dest, 0, 8);
    length = 0;

    while ((*src) && (*src != '.'))
    {
	if (++length == 9)
	{
            sprintf(error, "Filename base of %s >8 chars", path);
            I_Error(error);
	}

	*dest++ = toupper((int)*src++);
    }
}


//
// LUMP BASED ROUTINES.
//

//
// W_AddFile
// All files are optional, but at least one file must be
//  found (PWAD, if all required lumps are present).
// Files with a .wad extension are wadlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
//
// If filename starts with a tilde, the file is handled
//  specially to allow map reloads.
// But: the reload feature is a fragile hack...


int			reloadlump;
char*			reloadname;


// Hash table for fast lookups
int comp_keys(void *el1, void *el2)
{
    register char *t1 = ((lumpinfo_t *)el1)->name;
    register char *t2 = ((lumpinfo_t *)el2)->name;
    int i;

    for (i=0;i<8;i++)
    {
        if(t1[i] != t2[i])
        return 1;
    }

    return 0;
}


unsigned long int W_LumpNameHash(char *s)
{
    // This is the djb2 string hash function, modded to work on strings
    // that have a maximum length of 8.

    unsigned long int result = 5381;
    unsigned long int i;

    for (i=0; i < 8 && s[i] != '\0'; ++i)
    {
        result = ((result << 5) ^ result ) ^ toupper(s[i]);
    }

    return result;
}


unsigned long int hash(void *element, void *params)
{
    wadupr(((lumpinfo_t *)element)->name);
    return W_LumpNameHash(((lumpinfo_t *)element)->name) % 256;
}


void W_Init()
{
    hashtable_init(&ht, 256, comp_keys, hash, 0);
}


void W_AddFile (char *filename)
{
    wadinfo_t		header;
    lumpinfo_t*		lump_p;
    unsigned		i;
    int			handle = -1;
    int			length;
    int			startlump;
    filelump_t*		fileinfo;
    int			storehandle;

    int old_numlumps = numlumps;

    // open the file and add to directory

    // handle reload indicator.
    if (filename[0] == '~')
    {
	filename++;
	reloadname = filename;
	reloadlump = numlumps;
    }

    handle = rom_open(WAD_FILE, WAD_FILESIZE);

    startlump = numlumps;

    // WAD file
    rom_read(handle, &header, sizeof(header));

    if (strncmp(header.identification,"IWAD",4))
    {
        // Homebrew levels?
        if (strncmp(header.identification,"PWAD",4))
        {
            sprintf(error,"%s: Wad file %s doesn't have IWAD or PWAD id\n", header.identification, filename);
            I_Error(error);
        }

        // ???modifiedgame = true;
    }

    header.numlumps = LONG(header.numlumps);
    header.infotableofs = LONG(header.infotableofs);
    length = header.numlumps*sizeof(filelump_t);
    fileinfo = alloca(length);
    rom_lseek(handle, header.infotableofs, SEEK_SET);
    rom_read(handle, fileinfo, length);
    numlumps += header.numlumps;

    // Fill in lumpinfo
    if (numlumps > 0)
    {
        lumpinfo_t *tmp_new_lumpinfo = (lumpinfo_t *)n64_malloc(numlumps * sizeof(lumpinfo_t));
	// clear new lumpinfo memory
	n64_memset(tmp_new_lumpinfo, 0, numlumps * sizeof(lumpinfo_t));
        // copy old lumpinfo into new one
	n64_memcpy(tmp_new_lumpinfo, lumpinfo, old_numlumps * sizeof(lumpinfo_t));

	// swap the lumpinfos and free the old one
	lumpinfo_t *tmp_old_lumpinfo = lumpinfo;
	lumpinfo = tmp_new_lumpinfo;
	n64_free(tmp_old_lumpinfo);
    }
    else
    {
        lumpinfo = (lumpinfo_t *)n64_malloc(numlumps * sizeof(lumpinfo_t));
        n64_memset(lumpinfo, 0, numlumps * sizeof(lumpinfo_t));
    }


    if (!lumpinfo)
    {
	I_Error("Couldn't realloc lumpinfo");
    }

    lump_p = &lumpinfo[startlump];

    storehandle = reloadname ? -1 : handle;


    // hash the lumps
    for (i=startlump ; i<numlumps ; i++,lump_p++, fileinfo++)
    {
	lump_p->handle = storehandle;
	lump_p->position = LONG(fileinfo->filepos);
	lump_p->size = LONG(fileinfo->size);
	strncpy(lump_p->name, fileinfo->name, 8);

	hashtable_insert(&ht, (void*)lump_p, -1);
    }

    if (reloadname)
    {
        rom_close(handle);
    }
}


//
// W_Reload
// Flushes any of the reloadable lumps in memory
//  and reloads the directory.
//
void W_Reload (void)
{
    wadinfo_t		header;
    int			lumpcount;
    lumpinfo_t*		lump_p;
    unsigned		i;
    int			handle;
    int			length;
    filelump_t*		fileinfo;

    if (!reloadname)
    {
        return;
    }

    if ( (handle = rom_open(WAD_FILE, WAD_FILESIZE)) == -1 )
    {
        char ermac[256];
        sprintf(ermac, "W_Reload: couldn't open %d,%d", WAD_FILE, WAD_FILESIZE);
	I_Error(ermac);
    }

    rom_lseek(handle, 0, SEEK_SET);

    rom_read(handle, &header, sizeof(header));

    lumpcount = LONG(header.numlumps);
    header.infotableofs = LONG(header.infotableofs);
    length = lumpcount*sizeof(filelump_t);
    fileinfo = alloca(length);

    rom_lseek(handle, header.infotableofs, SEEK_SET);
    rom_read(handle, fileinfo, length);

    // Fill in lumpinfo
    lump_p = &lumpinfo[reloadlump];

    for (i=reloadlump ; i<reloadlump+lumpcount; i++,lump_p++,fileinfo++)
    {
        if (lumpcache[i])
        {
            Z_Free(lumpcache[i]);
        }

        lump_p->position = LONG(fileinfo->filepos);
        lump_p->size = LONG(fileinfo->size);
    }
}


//
// W_InitMultipleFiles
// Pass a null terminated list of files to use.
// All files are optional, but at least one file
//  must be found.
// Files with a .wad extension are idlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
// Lump names can appear multiple times.
// The name searcher looks backwards, so a later file
//  does override all earlier ones.
//
void W_InitMultipleFiles (char** filenames)
{
    int		size;

    W_Init();

    // open all the files, load headers, and count lumps
    numlumps = 0;

    // will be realloced as lumps are added
    lumpinfo = (lumpinfo_t *)n64_malloc(1);

    for ( ; *filenames ; filenames++)
    {
	W_AddFile(*filenames);
    }

    if (!numlumps)
    {
	I_Error("W_InitFiles: no files found");
    }

    // set up caching
    size = numlumps * sizeof(*lumpcache);
    lumpcache = (void **)n64_malloc (size);

    if (!lumpcache)
    {
        I_Error("Couldn't allocate lumpcache");
    }

    n64_memset(lumpcache,0, size);
}


//
// W_InitFile
// Just initialize from a single file.
//
void W_InitFile(char* filename)
{
    char*	names[2];

    names[0] = filename;
    names[1] = NULL;
    W_InitMultipleFiles(names);
}


//
// W_NumLumps
//
int W_NumLumps(void)
{
    return numlumps;
}


//
// W_CheckNumForName
// Returns -1 if name not found.
//
int W_CheckNumForName(char* name)
{
    lumpinfo_t *testlump = (lumpinfo_t *)alloca(sizeof(lumpinfo_t));
    strncpy(testlump->name, name, 8);
    wadupr(testlump->name);

    void *ret_node;
    lumpinfo_t *retlump;
    retlump = (lumpinfo_t *)is_in_hashtable( &ht, testlump, &ret_node );
    if (!retlump)
    {
        return -1;
    }

    return retlump - lumpinfo;
}


//
// W_GetNumForName
// Calls W_CheckNumForName, but bombs out if not found.
//
int W_GetNumForName(char* name)
{
    int	i;

    i = W_CheckNumForName (name);

    if (i == -1)
    {
        sprintf(error,"W_GetNumForName: %s not found!", name);
        I_Error(error);
    }

    return i;
}


//
// W_LumpLength
// Returns the buffer size needed to load the given lump.
//
int W_LumpLength(int lump)
{
    if (lump >= numlumps)
    {
        sprintf(error, "W_LumpLength: %i >= numlumps", lump);
        I_Error(error);
    }

    return lumpinfo[lump].size;
}


//
// W_ReadLump
// Loads the lump into the given buffer,
//  which must be >= W_LumpLength().
//
void W_ReadLump(int lump, void* dest)
{
    int		c;
    lumpinfo_t*	l;
    int		handle;

    if (lump >= numlumps)
    {
        sprintf(error, "W_ReadLump: %i >= numlumps", lump);
        I_Error(error);
    }

    l = lumpinfo+lump;

    if (l->handle == -1)
    {
        // reloadable file, so use open / read / close
        if ( (handle = rom_open(WAD_FILE, WAD_FILESIZE)) == -1 )
        {
            sprintf(error, "W_ReadLump: couldn't open %d,%d", WAD_FILE, WAD_FILESIZE);
            I_Error(error);
        }
    }
    else
    {
        handle = l->handle;
    }

    // ??? I_BeginRead ();

    rom_lseek(handle, l->position, SEEK_SET);
    c = rom_read(handle, dest, l->size);

    if (c < l->size)
    {
        sprintf(error, "W_ReadLump: only read %i of %i on lump %i", c, l->size, lump);
        I_Error(error);
    }

    if(l->handle == -1)
    {
        rom_close(handle);
    }
}


//
// W_CacheLumpNum
//
void* W_CacheLumpNum(int lump, int tag)
{
    byte*	ptr;

    if ((unsigned)lump >= numlumps)
    {
        sprintf(error, "W_CacheLumpNum: %i >= numlumps", lump);
        I_Error(error);
    }
    if (!lumpcache[lump])
    {
	// read the lump in

        ptr = Z_Malloc(W_LumpLength(lump), tag, &lumpcache[lump]);

        if ((ptr == NULL) || (ptr != lumpcache[lump]))
        {
            sprintf(error, "cache allocation error on lump %i\n", lump);
            I_Error(error);
	}

        W_ReadLump(lump, lumpcache[lump]);
    }
    else
    {
        Z_ChangeTag(lumpcache[lump],tag);
    }

    return lumpcache[lump];
}


//
// W_CacheLumpName
//
void* W_CacheLumpName(char* name, int tag)
{
    return W_CacheLumpNum(W_GetNumForName(name), tag);
}
