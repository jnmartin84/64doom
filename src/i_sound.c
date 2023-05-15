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
//      System interface for sound.
//
//-----------------------------------------------------------------------------


#include <libdragon.h>
#include "regsinternal.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <unistd.h>

#include "z_zone.h"

#include "i_system.h"
#include "i_sound.h"
#include "m_argv.h"
#include "m_misc.h"
#include "w_wad.h"
#include "m_swap.h"

#include "doomdef.h"
#include "m_fixed.h"

#include <errno.h>

/**********************************************************************/
// On the PC, need swap routines because the MIDI file was made on the Amiga and
// is in big endian format. :)
// The Nintendo 64 is also big endian so these are no-ops here. :D

#define WSWAP(x) x
#define LSWAP(x) x

//extern volatile struct AI_regs_s *AI_regs;
volatile struct AI_regs_s *AI_regs = (struct AI_regs_s *)0xA4500000;


extern const int MIDI_FILE;
extern const int MIDI_FILESIZE;

// Any value of numChannels set
// by the defaults code in M_misc is now clobbered by I_InitSound().
// number of channels available for sound effects
extern int numChannels;

/**********************************************************************/

typedef unsigned int ULONG;
typedef unsigned short UWORD;
typedef unsigned char UBYTE;
typedef char BYTE;

#define SAMPLERATE 11025
//22050

// NUM_VOICES = SFX_VOICES + MUS_VOICES
#define SFX_VOICES 8
#define MUS_VOICES 16
#define NUM_VOICES (SFX_VOICES+MUS_VOICES)


typedef struct MUSheader
{
    // identifier "MUS" 0x1A
    char     ID[4];
    UWORD    scoreLen;
    UWORD    scoreStart;
    // count of primary channels
    UWORD    channels;
    // count of secondary channels
    UWORD    sec_channels;
    UWORD    instrCnt;
    UWORD    dummy;

    // variable-length part starts here
    UWORD    instruments[];
}
MUSheader_t;


struct Channel
{
    int32_t lastnote;
    int32_t pitch;
    int32_t pan;
    int32_t vol;
    int32_t ltvol;
    int32_t rtvol;
    int instrument;
    int map[128];
};


struct midiHdr
{
    ULONG rate;
    ULONG loop;                          // 16.16 fixed pt
    ULONG length;                        // 16.16 fixed pt
    UWORD base;                          // note base of sample
    BYTE sample[8];                      // actual length varies
};


typedef struct Voice
{
    ULONG index;
    ULONG step;
    ULONG loop;
    ULONG length;
    int32_t ltvol;
    int32_t rtvol;
    int chan;
    UWORD base;
    UWORD flags;
    BYTE *wave;
}
Voice_t;

static int __attribute__((aligned(8))) voice_inuse[MUS_VOICES];
static struct Channel __attribute__((aligned(8))) mus_channel[16];

static struct Voice __attribute__((aligned(8))) audVoice[NUM_VOICES];
static struct Voice __attribute__((aligned(8))) midiVoice[256];

// The actual lengths of all sound effects.
static int __attribute__((aligned(8))) lengths[NUMSFX];

static ULONG NUM_SAMPLES = 316;
static ULONG BEATS_PER_PASS = 4;

static short __attribute__((aligned(8))) pcmout1[632*4];
static short __attribute__((aligned(8))) pcmout2[632*4];
static int __attribute__((aligned(8))) pcmflip = 0;
static short __attribute__((aligned(8))) *pcmout[2];

short __attribute__((aligned(8))) *pcmbuf;

static int changepitch;

static int32_t master_vol =  0x0000F000;

static int musicdies  = -1;
static int music_okay =  0;

uint32_t __attribute__((aligned(8))) midi_pointers[182];

static UWORD score_len, score_start, inst_cnt;
static void __attribute__((aligned(8))) *score_data;
static UBYTE __attribute__((aligned(8))) *score_ptr;

static int mus_delay    = 0;
static int mus_looping  = 0;
static int32_t mus_volume = 127;
int mus_playing  = 0;

uint32_t __attribute__((aligned(8))) used_instrument_bits[8] = {0,0,0,0,0,0,0,0};

extern int snd_SfxVolume;

typedef struct mixer_state {
    uint32_t first_voice_to_mix;
    uint32_t last_voice_to_mix;
    uint32_t _fb_index;
    uint32_t _fb_step;
    int32_t _fb_ltvol;
    int32_t _fb_rtvol;
    int32_t _fb_sample;
    int32_t _fb_ix;
    int32_t _fb_iy;
    uint32_t _fb_loop;
    uint32_t _fb_length;
    int32_t _fb_pval, _fb_pval2;
    BYTE *_fb_wvbuff;
} mixer_state_t;

extern display_context_t _dc;

#define PANMOD 127

typedef struct event_loop_state_s
{
    UBYTE event;
    UBYTE note;
    UBYTE time;
    UBYTE ctrl;
    UBYTE value;
    UBYTE a,b,c;
    ULONG pitch;
    int next_time;
    int channel;
    int voice;
    int inst;
    int volume;
    int pan;
}
event_loop_state_t;

static mixer_state_t doom_mixer;


/**********************************************************************/

void fill_buffer(short *buffer);

void Sfx_Start(char *wave, int cnum, int step, int vol, int sep, int length);
void Sfx_Update(int cnum, int step, int vol, int sep);
void Sfx_Stop(int cnum);
int Sfx_Done(int cnum);

void Mus_SetVol(int vol);
int Mus_Register(void *musdata);
void Mus_Unregister(int handle);
void Mus_Play(int handle, int looping);
void Mus_Stop(int handle);
void Mus_Pause(int handle);
void Mus_Resume(int handle);

int used_instrument_count = -1;

void reset_midiVoices(void)
{
    int i;

    used_instrument_count = -1;

    for (i=0;i<256;i++)
    {
        if (midiVoice[i].wave)
        {
            free((void*)((uintptr_t)midiVoice[i].wave & 0x8FFFFFFF));
        }
    }
    D_memset(mus_channel, 0, sizeof(struct Channel)*MUS_VOICES);
    D_memset(audVoice, 0x00, sizeof(Voice_t)*(NUM_VOICES));
    D_memset(midiVoice, 0, sizeof(Voice_t)*256);
    D_memset(voice_inuse, 0, MUS_VOICES*sizeof(int));

    // instrument used lookups
    used_instrument_bits[0] = 0;
    used_instrument_bits[1] = 0;
    used_instrument_bits[2] = 0;
    used_instrument_bits[3] = 0;
    used_instrument_bits[4] = 0;
    used_instrument_bits[5] = 0;
    used_instrument_bits[6] = 0;
    used_instrument_bits[7] = 0;

    for (i=0;i<256;i++)
    {
        midiVoice[i].step = (1 << 16);
        midiVoice[i].length = 2000 << 16;
        midiVoice[i].base = 60;
    }
}


/**********************************************************************/
//
// This function loads the sound data for sfxname from the WAD lump,
// and returns a ptr to the data in fastmem and its length in len.
//
void *getsfx (char *sfxname, int *len)
{
    unsigned char *sfx;
    unsigned char *cnvsfx;
    int i;
    int size;
    char name[32];
    int sfxlump;

    // Get the sound data from the WAD, allocate lump
    //  in zone memory.
    sprintf(name, "ds%s", sfxname);

    // Now, there is a severe problem with the
    //  sound handling, in it is not (yet/anymore)
    //  gamemode aware. That means, sounds from
    //  DOOM II will be requested even with DOOM
    //  shareware.
    // The sound list is wired into sounds.c,
    //  which sets the external variable.
    // I do not do runtime patches to that
    //  variable. Instead, we will use a
    //  default sound for replacement.
    if (W_CheckNumForName(name) == -1)
    {
        sfxlump = W_GetNumForName("dspistol");
    }
    else
    {
        sfxlump = W_GetNumForName(name);
    }

    size = W_LumpLength(sfxlump);
    sfx = (unsigned char*)W_CacheLumpNum(sfxlump, PU_STATIC);

    // Allocate from zone memory.
    cnvsfx = (unsigned char*)Z_Malloc(size, PU_SOUND, 0);
    // Now copy and convert offset to signed.
    for (i = 0; i < size; i++)
    {
        cnvsfx[i] = sfx[i] ^ 0x80;
    }

    // Remove the cached lump.
    Z_Free(sfx);

    // return length.
    *len = size;

    // Return allocated converted data.
    return (void *)((uintptr_t)cnvsfx|0xA0000000);
}


/////////////////////////////////////////////////////////////////////////////////////////
//Buffer filling
/////////////////////////////////////////////////////////////////////////////////////////


void sound_callback(void)
{
    // Sound mixing for the buffer is synchronous.
    // Synchronous sound output is explicitly called.
    I_UpdateSound();
    // Update sound output.
    I_SubmitSound();
}


/**********************************************************************/
// Init at program start...
void I_InitSound (void)
{
    int i;

    audio_init(SAMPLERATE, 0);
    // need to use uncached address of these buffers when rendering audio
    pcmout[0] = (int16_t *)((uintptr_t)pcmout1 | 0xA0000000);
    pcmout[1] = (int16_t *)((uintptr_t)pcmout2 | 0xA0000000);
    pcmbuf = pcmout[pcmflip];

    changepitch = M_CheckParm("-changepitch");

    // Initialize external data (all sounds) at start, keep static.
    for (i = 1; i < NUMSFX; i++)
    {
        // Alias? Example is the chaingun sound linked to pistol.
        if (!S_sfx[i].link)
        {
            // Load data from WAD file.
            S_sfx[i].data = getsfx(S_sfx[i].name, &lengths[i]);
        }
        else
        {
            // Previously loaded already?
            S_sfx[i].data = S_sfx[i].link->data;
            lengths[i] = lengths[(S_sfx[i].link - S_sfx)/sizeof(sfxinfo_t)];
        }
    }

    printf ("I_InitSound: Pre-cached all sound data.\n");

    // install sound callback
    register_AI_handler(sound_callback);
    printf("I_InitSound: Audio Interface callback registered.\n");
    set_AI_interrupt(1);
    printf("I_InitSound: Audio Interface interrupt enabled.\n");
    sound_callback();

    // Finished initialization.
    printf("I_InitSound: Sound module ready.\n");

    numChannels = SFX_VOICES;
}

/**********************************************************************/

#define AI_STATUS_FULL  ( 1 << 31 )
/**********************************************************************/
// ... update sound buffer and audio device at runtime...
void I_SubmitSound (void)
{
    if (!(AI_regs->status & AI_STATUS_FULL))
    {
        AI_regs->address = (uint32_t *)((uintptr_t)pcmbuf);
        AI_regs->length = NUM_SAMPLES*2*2;
        AI_regs->control = 1;
        pcmflip ^= 1;
        pcmbuf = pcmout[pcmflip];
    }
}

/**********************************************************************/
// ... shut down and relase at program termination.
void I_ShutdownSound (void)
{
}

/**********************************************************************/
/**********************************************************************/
//
//  SFX I/O
//

/**********************************************************************/
// Initialize number of channels
void I_SetChannels (void)
{
}

/**********************************************************************/
// Get raw data lump index for sound descriptor.
int I_GetSfxLumpNum (sfxinfo_t *sfx)
{
    char namebuf[9];
    sprintf(namebuf, "ds%s", sfx->name);
    return W_GetNumForName(namebuf);
}

/**********************************************************************/
// Starts a sound in a particular sound channel.
int I_StartSound (
  int id,
  int cnum,
  int vol,
  int sep,
  int pitch,
  int priority )
{
    I_StopSound(cnum);
    Sfx_Start (S_sfx[id].data, cnum, changepitch ? 2765 + (((pitch<<2) + pitch)<<5) : 11025, vol, sep, lengths[id]);
    return cnum;
}

/**********************************************************************/
// Stops a sound channel.
void I_StopSound(int handle)
{
    Sfx_Stop(handle);
}

/**********************************************************************/
// Called by S_*() functions
//  to see if a channel is still playing.
// Returns 0 if no longer playing, 1 if playing.
int I_SoundIsPlaying(int handle)
{
    return Sfx_Done(handle) ? 1 : 0;
}

/**********************************************************************/
// Updates the volume, separation,
//  and pitch of a sound channel.
void
I_UpdateSoundParams
( int        handle,
  int        vol,
  int        sep,
  int        pitch )
{
    Sfx_Update(handle, changepitch ? 2765 + (((pitch<<2) + pitch)<<5) : 11025, vol, sep);
}

/**********************************************************************/
/**********************************************************************/
//
// MUSIC API.
//
/**********************************************************************/
//
//  MUSIC I/O
//
void I_InitMusic(void)
{
    int mphnd;
    size_t rc;

    reset_midiVoices();

    music_okay = 0;
    mphnd = dfs_open("MIDI_Instruments");
    if (-1 == mphnd)
    {
        printf("I_InitMusic: Could not open MIDI_Instruments file (%s)\n", strerror(errno));
        return;
    }

    rc = dfs_read(midi_pointers, sizeof(ULONG)*182, 1, mphnd);
    dfs_close(mphnd);
    if ((sizeof(ULONG)*182) != rc)
    {
        printf("I_InitMusic: Could not read MUS instrument headers (%s)\n", strerror(errno));
        return;
    }

    printf("I_InitMusic: Pre-cached all MUS instrument headers.\n");
    music_okay = 1;
}

/**********************************************************************/
void I_ShutdownMusic(void)
{
}

/**********************************************************************/
// Volume.
void I_SetMusicVolume(int volume)
{
    snd_MusicVolume = volume;

    if (music_okay)
    {
        Mus_SetVol(volume);
    }
}

/**********************************************************************/
// PAUSE game handling.
void I_PauseSong(int handle)
{
    if (music_okay)
    {
        Mus_Pause(handle);
    }
}

/**********************************************************************/
void I_ResumeSong(int handle)
{
    if (music_okay)
    {
        Mus_Resume(handle);
    }
}

/**********************************************************************/
// Registers a song handle to song data.
int I_RegisterSong(void *data)
{
    if (music_okay)
    {
        return Mus_Register(data);
    }

    return 0;
}

/**********************************************************************/
// Called by anything that wishes to start music.
//  plays a song, and when the song is done,
//  starts playing it again in an endless loop.
// Horrible thing to do, considering.
void
I_PlaySong
( int        handle,
  int        looping )
{
    if (music_okay)
    {
        Mus_Play(handle, looping);
    }

    musicdies = gametic + TICRATE*30;
}

/**********************************************************************/
// Stops a song over 3 seconds.
void I_StopSong(int handle)
{
    if (music_okay)
    {
        Mus_Stop(handle);
    }

    musicdies = 0;
}

/**********************************************************************/
// See above (register), then think backwards
void I_UnRegisterSong(int handle)
{
    if (music_okay)
    {
        Mus_Unregister(handle);
    }
}

/**********************************************************************/

void Sfx_Start(char *wave, int cnum, int step, int volume, int seperation, int length)
{
    int32_t vol = (volume<<3);
    int32_t sep = seperation;

    audVoice[cnum].wave = wave + 8;
    audVoice[cnum].index = 0;
    audVoice[cnum].step = (ULONG)((step<<16) / SAMPLERATE);
    audVoice[cnum].loop = 0 << 16;
    audVoice[cnum].length = (length - 8) << 16;
    audVoice[cnum].ltvol = FixedMul(((127 - ((127*sep*sep) / 65536))<<16), (vol<<7));
    sep -= 256;
    audVoice[cnum].rtvol = FixedMul(((127 - ((127*sep*sep) / 65536))<<16), (vol<<7));
    audVoice[cnum].flags = 0x81;
}

void Sfx_Update(int cnum, int step, int volume, int seperation)
{
    int32_t vol = (volume<<3);
    int32_t sep = seperation;

    audVoice[cnum].step = (ULONG)((step<<16) / SAMPLERATE);
    audVoice[cnum].ltvol = FixedMul(((127 - (127*sep*sep) / 65536) << 16), (vol<<7));
    sep -= 256;
    audVoice[cnum].rtvol = FixedMul(((127 - (127*sep*sep) / 65536) << 16), (vol<<7));
}

void Sfx_Stop(int cnum)
{
    audVoice[cnum].flags &= 0xFE;
}

int Sfx_Done(int cnum)
{
    int done;
    done = (audVoice[cnum].flags & 0x01) ? (audVoice[cnum].index >> 16) + 1 : 0;
    return done;
}

/**********************************************************************/

void Mus_SetVol(int vol)
{
    if (!vol)
    {
        mus_volume = 0;
    }
    else
    {
        mus_volume = (vol<<3)+7;
    }
}

#define instrument_used(i) (used_instrument_bits[((i) >> 5)]  & (1 << ((i) & 31)))

int Mus_Register(void *musdata)
{
    int i;
    int hnd;

    struct midiHdr *mhdr;

    ULONG *lptr = (ULONG *)musdata;
    UWORD *wptr = (UWORD *)musdata;

    Mus_Unregister(1);
    reset_midiVoices();
    music_okay = 0;

    // music won't start playing until mus_playing set at this point

    if (lptr[0] != 0x4d55531a)
    {
        return 0; // "MUS",26 always starts a vaild MUS file
    }

    score_len = SHORT(wptr[2]); // score length
    if (!score_len)
    {
        return 0; // illegal score length
    }

    score_start = SHORT(wptr[3]); // score start
    if (score_start < 18)
    {
        return 0; // illegal score start offset
    }

    inst_cnt = SHORT(wptr[6]); // instrument count
    if (!inst_cnt)
    {
        return 0; // illegal instrument count
    }

    // okay, MUS data seems to check out

    score_data = musdata;
    MUSheader_t *musheader = (MUSheader_t*)score_data;
    // instrument used lookups
    used_instrument_bits[0] = 0;
    used_instrument_bits[1] = 0;
    used_instrument_bits[2] = 0;
    used_instrument_bits[3] = 0;
    used_instrument_bits[4] = 0;
    used_instrument_bits[5] = 0;
    used_instrument_bits[6] = 0;
    used_instrument_bits[7] = 0;
    used_instrument_count = inst_cnt;
    for (i=0;i<inst_cnt;i++)
    {
        uint8_t instrument = (uint8_t)SHORT(musheader->instruments[i]);
        
        // fix TNT crash with one of the 10s levels
        ULONG ptr = LSWAP(midi_pointers[instrument]);
        if (!ptr)
            continue;
        used_instrument_bits[(instrument >> 5)] |= (1 << (instrument & 31));
    }

    // iterating over all instruments
    for (i=0;i<182;i++)
    {
        // current instrument is used
        if (instrument_used(i))
        {
            // get the pointer into the instrument data struct
            ULONG ptr = LSWAP(midi_pointers[i]);

#ifdef RANGECHECK
            if (!ptr)
            {
                I_Error("Mus_Register: instrument %d used but MUS instrument pointer is NULL.\n", i);
            }

            // make sure it doesn't point to NULL
            if (ptr)
#endif
            {
                // allocate some space for the header
                mhdr = (struct midiHdr*)malloc(sizeof(struct midiHdr));

#ifdef RANGECHECK
                if (!mhdr)
                {
                    I_Error("MUS_Register: could not allocate memory for MIDI instrument header.\n");
                }
#else
                if (!mhdr)
                {
                    return 0;
                }
#endif
                // open MIDI Instrument Set file from ROM
                hnd = dfs_open("MIDI_Instruments");
                if (hnd < 0)
                {
#ifdef RANGECHECK
                    I_Error("Mus_Register: Could not open MIDI_Instruments file (%s)\n", strerror(errno));
#else
                    // clean up malloc for header
                    free(mhdr);

                    //continue;
                    return 0;
#endif
                }

                // documentation for this makes it sound like it is pointless to test the return value
                dfs_seek(hnd,ptr,SEEK_SET);

                if (sizeof(struct midiHdr) != dfs_read((void*)mhdr,sizeof(struct midiHdr),1,hnd))
                {
#ifdef RANGECHECK
                    I_Error("Mus_Register: Could not read header for instrument %d from MIDI_Instruments file.\n", i);
#else
                    dfs_close(hnd);
                    free(mhdr);
#endif
                }

                ULONG length = LSWAP(mhdr->length) >> 16;

                void *sample = (void *)((uintptr_t)malloc(length));

                if (sample)
                {
                    dfs_seek(hnd,ptr + 4 + 4 + 4 + 2,SEEK_SET);
                    dfs_read(sample,length,1,hnd);
                    // it doesn't hurt to access sound data from a non-cached address
                    // it gets mixed to uncached buffer anyway
                    midiVoice[i].wave   = (BYTE*)((uintptr_t)sample | 0xA0000000);
                    midiVoice[i].index  = 0;
                    midiVoice[i].step   = 1 << 16;
                    midiVoice[i].loop   = LSWAP(mhdr->loop);
                    midiVoice[i].length = LSWAP(mhdr->length);
                    midiVoice[i].ltvol  = 0;
                    midiVoice[i].rtvol  = 0;
                    midiVoice[i].base   = WSWAP(mhdr->base);
                    midiVoice[i].flags  = 0x00;

                    free(mhdr);
                    dfs_close(hnd);
                }
#ifdef RANGECHECK
                else
                {
                    I_Error("MUS_Register: could not allocate memory for sample.\n");
                }
#endif
            }
        }
    }

    music_okay = 1;
    return 1;
}


void Mus_Unregister(int handle)
{
    Mus_Stop(handle);
    // music won't start playing until mus_playing set at this point

    score_data = 0;
    score_len = 0;
    score_start = 0;
    inst_cnt = 0;
}


void Mus_Play(int handle, int looping)
{
    if (!handle)
    {
        return;
    }

    mus_looping = looping;
    mus_playing = 2;                     // 2 = play from start
}


void Mus_Stop(int handle)
{
    int ix;

    if (mus_playing)
    {
        mus_playing = -1;                  // stop playing
    }

    // disable instruments in score (just disable them all)
    for (ix=SFX_VOICES; ix<NUM_VOICES; ix++)
    {
        audVoice[ix].flags = 0x00;        // disable voice
    }

    mus_looping = 0;
    mus_delay = 0;
}


void Mus_Pause(int handle)
{
    mus_playing = 0;                     // 0 = not playing
}


void Mus_Resume(int handle)
{
    mus_playing = 1;                     // 1 = play from current position
}


/**********************************************************************/
//static uint32_t lsmp[4];
void I_UpdateSound (void)
{
    D_memset((void *)pcmbuf, 0, (NUM_SAMPLES << 3));

//if(music_okay) 
{
    if (mus_playing < 0)
    {
        // music now off
        mus_playing = 0;
        if (!snd_SfxVolume)
        {
            goto mixout;
        }
        else
        {
            goto mix;
        }
    }

    if (mus_playing)
    {
        if (mus_playing > 1)
        {
            mus_playing = 1;
            // start music from beginning
            score_ptr = (UBYTE *)((ULONG)score_data + (ULONG)score_start);
        }

        // 1=140Hz, 2=70Hz, 4=35Hz
        mus_delay -= BEATS_PER_PASS;
        if (mus_delay <= 0)
        {
            event_loop_state_t loop_state;

nextEvent:
            do
            {
                // score_ptr++ 1
                loop_state.event = *score_ptr++;
                switch ((loop_state.event >> 4) & 7)
                {
                    // Release
                    case 0:
                    {
                           // score_ptr++ 2
                        loop_state.note = *score_ptr++;
                        loop_state.note &= 0x7f;
                        loop_state.channel = (int)(loop_state.event & 15);
                        loop_state.voice = mus_channel[loop_state.channel].map[(ULONG)loop_state.note] - 1;
                        mus_channel[loop_state.channel].lastnote = -1;
                        if (loop_state.voice >= 0)
                        {
                            // clear mapping
                            mus_channel[loop_state.channel].map[(ULONG)loop_state.note] = 0;
                            // voice available
                            voice_inuse[loop_state.voice] = 0;
                            // voice releasing
                            audVoice[loop_state.voice + SFX_VOICES].flags |= 0x02;
                        }
                        break;
                    }
                    // Play note
                    case 1:
                    {
                        // score_ptr++ 2
                        loop_state.note = *score_ptr++;
                        loop_state.channel = (int)(loop_state.event & 15);
                        loop_state.volume = 0;//-1;
                        if (loop_state.note & 0x80)
                        {
                            // set volume as well
                            loop_state.note &= 0x7f;
                            // score_ptr++ 3
                            loop_state.volume = (int32_t)(*score_ptr++);
                        }
                        for (loop_state.voice=0; loop_state.voice<MUS_VOICES+1; loop_state.voice++)
                        {
                            if(loop_state.voice < MUS_VOICES && !voice_inuse[loop_state.voice])
                            {
                                // found free voice
                                break;
                            }
                        }
                        if (loop_state.voice < MUS_VOICES)
                        {
                            // in use
                            voice_inuse[loop_state.voice] = 1;
                            mus_channel[loop_state.channel].lastnote = loop_state.note;
                            mus_channel[loop_state.channel].map[(ULONG)loop_state.note] = loop_state.voice + 1;
                            if (loop_state.volume > 0)
                            {
                                mus_channel[loop_state.channel].vol = loop_state.volume;
                                loop_state.pan = mus_channel[loop_state.channel].pan;
                                mus_channel[loop_state.channel].ltvol = (((127 - loop_state.pan) * loop_state.volume)) << 9;
                                mus_channel[loop_state.channel].rtvol = (((127 - (127 - loop_state.pan)) * loop_state.volume)) << 9;
                            }

                            audVoice[loop_state.voice + SFX_VOICES].ltvol = mus_channel[loop_state.channel].ltvol;
                            audVoice[loop_state.voice + SFX_VOICES].rtvol = mus_channel[loop_state.channel].rtvol;

                            // enable voice
                            audVoice[loop_state.voice + SFX_VOICES].flags = 0x01;

                            if (loop_state.channel != 15)
                            {
                                loop_state.inst = mus_channel[loop_state.channel].instrument;
                                struct Voice *mvc = &midiVoice[loop_state.inst];
                                // back link for pitch wheel
                                audVoice[loop_state.voice + SFX_VOICES].chan = loop_state.channel;
                                audVoice[loop_state.voice + SFX_VOICES].wave = mvc->wave;

                                // I am sorry I don't have documentation on what I did here
                                // but the highlights are
                                // I took the tables for frequency and whatnot and did some curve fitting
                                // with an online tool and then finessed the equations until it sounded good

                                // all of this was to reduce memory accesses when mixing music

                                // empirically derived formula for note_table lookup
                                uint32_t x = ((72 - mvc->base + (ULONG)loop_state.note) & 0x7f);

                                // the pair of % and / hopefully compiles down to a single div instruction
                                // with a mflo mfhi to get each part... hopefully
                                // why 12? i dont know any more, but it was something
                                uint32_t xi = x % 12;
                                uint32_t xs = x / 12;
                                uint32_t xi2=0;
                                // xi squared
                                for (int m=0;m<xi;m++)
                                {
                                    xi2 += xi;
                                }
                                xi2 <<= 3;
                                xi <<= 9;

                                audVoice[loop_state.voice + SFX_VOICES].step = ( (0x10000 + (((xi2 << 4) + (xi2 << 1) + (xi2))+((xi<<3)-(xi))) ) << xs ) >> 6;
                                //note_table[(72 - midiVoice[inst].base + (ULONG)note) & 0x7f];
                                audVoice[loop_state.voice + SFX_VOICES].loop = mvc->loop;
                                audVoice[loop_state.voice + SFX_VOICES].length = mvc->length;
                            }
                            else
                            {
                                // percussion channel - note is percussion instrument
                                loop_state.inst = (ULONG)loop_state.note + 100;
                                struct Voice *mvc = &midiVoice[loop_state.inst];
                                // back link for pitch wheel
                                audVoice[loop_state.voice + SFX_VOICES].chan = loop_state.channel;
                                audVoice[loop_state.voice + SFX_VOICES].wave = mvc->wave;
                                audVoice[loop_state.voice + SFX_VOICES].step = 0x10000;
                                audVoice[loop_state.voice + SFX_VOICES].loop = mvc->loop;
                                audVoice[loop_state.voice + SFX_VOICES].length = mvc->length;
                            }
                            audVoice[loop_state.voice + SFX_VOICES].index = 0;
                        }
                        break;
                    }
                    // Pitch
                    case 2:
                    {
                        // score_ptr++ 2
                        loop_state.pitch = ((ULONG)*score_ptr++)&0xFF;
                        loop_state.channel = (int)(loop_state.event & 15);
                        // pitch*64 - pitch*4 == pitch*60
                        mus_channel[loop_state.channel].pitch = ((loop_state.pitch << 6) - (loop_state.pitch<<2)) + 58180;
                        break;
                    }
                    // Tempo
                    case 3:
                    {
                        // score_ptr++ 2
                        // skip value - not supported
                        score_ptr++;
                        break;
                    }
                    // Change control
                    case 4:
                    {
                        // score_ptr++ 2
                        loop_state.ctrl = *score_ptr++;
                        // score_ptr++ 3
                        loop_state.value = *score_ptr++;
                        loop_state.channel = (int)(loop_state.event & 15);
                        switch (loop_state.ctrl)
                        {
                            case 0:
                            {
                                // set channel instrument
                                mus_channel[loop_state.channel].instrument = (ULONG)loop_state.value;
                                mus_channel[loop_state.channel].pitch = 0x10000;
                                break;
                            }
                            case 3:
                            {
                                // set channel volume
                                mus_channel[loop_state.channel].vol = loop_state.volume = loop_state.value;
                                loop_state.pan = mus_channel[loop_state.channel].pan;
                                mus_channel[loop_state.channel].ltvol = (((127 - loop_state.pan) * loop_state.volume)) << 9;
                                mus_channel[loop_state.channel].rtvol = (((127 - (127 - loop_state.pan)) * loop_state.volume)) << 9;
                                break;
                            }
                            case 4:
                            {
                                // set channel pan
                                mus_channel[loop_state.channel].pan = loop_state.pan = loop_state.value;
                                loop_state.volume = mus_channel[loop_state.channel].vol;
                                mus_channel[loop_state.channel].ltvol = (((127 - loop_state.pan) * loop_state.volume)) << 9;
                                mus_channel[loop_state.channel].rtvol = (((127 - (127 - loop_state.pan)) * loop_state.volume)) << 9;
                                break;
                            }
                        }
                        break;
                    }
                    // End score
                    case 6:
                    {
                        if (mus_looping)
                        {
                            for (loop_state.voice=0; loop_state.voice<MUS_VOICES; loop_state.voice++)
                            {
                                audVoice[loop_state.voice + SFX_VOICES].flags = 0;
                                voice_inuse[loop_state.voice] = 0;
                            }
                            score_ptr = (UBYTE *)((ULONG)score_data + (ULONG)score_start);
                        }
                        else
                        {
                            for (loop_state.voice=0; loop_state.voice<MUS_VOICES; loop_state.voice++)
                            {
                                audVoice[loop_state.voice + SFX_VOICES].flags = 0;
                                voice_inuse[loop_state.voice] = 0;
                            }
                            mus_delay = 0;
                            mus_playing = 0;
                            goto mix;
                        }
                        break;
                    }
                }
            }
            // not last event
            while (!(loop_state.event & 0x80));

            // now get the time until the next event(s)
            loop_state.next_time = 0;
            loop_state.time = *score_ptr++;

            while (loop_state.time & 0x80)
            {
                // while msb set, accumulate 7 bits
                loop_state.next_time |= (ULONG)(loop_state.time & 0x7f);
                loop_state.next_time <<= 7;
                loop_state.time = *score_ptr++;
            }

            loop_state.next_time |= (ULONG)loop_state.time;
            mus_delay += loop_state.next_time;

            if (mus_delay <= 0)
            {
                goto nextEvent;
            }
        }
    }

}

mix:
    doom_mixer.first_voice_to_mix = snd_SfxVolume ? 0 : SFX_VOICES;
    doom_mixer.last_voice_to_mix = mus_playing ? NUM_VOICES : (snd_SfxVolume ? SFX_VOICES : 0);
    // mix enabled voices
    for (
        doom_mixer._fb_ix=doom_mixer.first_voice_to_mix;
        doom_mixer._fb_ix<doom_mixer.last_voice_to_mix;
        doom_mixer._fb_ix++
        )
    {
        if (audVoice[doom_mixer._fb_ix].flags & 0x01)
        {
            doom_mixer._fb_index = audVoice[doom_mixer._fb_ix].index;
            doom_mixer._fb_step = audVoice[doom_mixer._fb_ix].step;
            doom_mixer._fb_loop = audVoice[doom_mixer._fb_ix].loop;
            doom_mixer._fb_length = audVoice[doom_mixer._fb_ix].length;
            doom_mixer._fb_ltvol = audVoice[doom_mixer._fb_ix].ltvol;
            doom_mixer._fb_rtvol = audVoice[doom_mixer._fb_ix].rtvol;
            doom_mixer._fb_wvbuff = audVoice[doom_mixer._fb_ix].wave;

            if (!(audVoice[doom_mixer._fb_ix].flags & 0x80))
            {
                // special handling for instrument
                if (audVoice[doom_mixer._fb_ix].flags & 0x02)
                {
                    // releasing
                    doom_mixer._fb_ltvol = ((doom_mixer._fb_ltvol << 3)-doom_mixer._fb_ltvol)>>3;
                    doom_mixer._fb_rtvol = ((doom_mixer._fb_rtvol << 3)-doom_mixer._fb_rtvol)>>3;
                    audVoice[doom_mixer._fb_ix].ltvol = doom_mixer._fb_ltvol;
                    audVoice[doom_mixer._fb_ix].rtvol = doom_mixer._fb_rtvol;

                    // this was originally derived from some floating point value in the original code
                    // changed to fixed_t when I went to a fixed-point mixer
                    // and further adjusted by ear until settling on this value
                    #define cmpvol (665835 - 150000)
                    if (((uint32_t)doom_mixer._fb_ltvol <= cmpvol) && ((uint32_t)doom_mixer._fb_rtvol <= cmpvol))
                    {
                        // disable voice
                        audVoice[doom_mixer._fb_ix].flags = 0;
                        // next voice
                        continue;
                    }
                }

                doom_mixer._fb_step = (((int64_t)doom_mixer._fb_step * (int64_t)(mus_channel[audVoice[doom_mixer._fb_ix].chan & 15].pitch)) >> 16);
                doom_mixer._fb_ltvol = (((int64_t)doom_mixer._fb_ltvol * (int64_t)(mus_volume)) >> 9);
                doom_mixer._fb_rtvol = (((int64_t)doom_mixer._fb_rtvol * (int64_t)(mus_volume)) >> 9);
            }

            for (doom_mixer._fb_iy=0; doom_mixer._fb_iy < (NUM_SAMPLES << 1); doom_mixer._fb_iy+=2)
            {
                if (doom_mixer._fb_index >= doom_mixer._fb_length)
                {
                    if (audVoice[doom_mixer._fb_ix].flags & 0x80)
                    {
                        // disable voice
                        audVoice[doom_mixer._fb_ix].flags = 0;
                        // exit sample loop
                        break;
                    }
                    else
                    {
                        // check if instrument has loop
                        if (doom_mixer._fb_loop)
                        {
                            doom_mixer._fb_index -= doom_mixer._fb_length;
                            doom_mixer._fb_index += doom_mixer._fb_loop;
                        }
                        else
                        {
                            // disable voice
                            audVoice[doom_mixer._fb_ix].flags = 0;
                            // exit sample loop
                            break;
                        }
                    }
                }

                doom_mixer._fb_sample = 0;

                // Doom 2 has issues without this, or at least my 1.666 data files do
                if ((uint32_t)doom_mixer._fb_wvbuff & 0x80000000)
                {
                    int index = doom_mixer._fb_index >> 16;
                    doom_mixer._fb_sample = (doom_mixer._fb_wvbuff[index]);
                }
                doom_mixer._fb_sample = FixedMul(doom_mixer._fb_sample,master_vol);

                int ssmp1 = FixedMul(doom_mixer._fb_sample, doom_mixer._fb_ltvol);
                int ssmp2 = FixedMul(doom_mixer._fb_sample, doom_mixer._fb_rtvol);
                pcmbuf[doom_mixer._fb_iy    ] += ssmp1<<1;
                pcmbuf[doom_mixer._fb_iy + 1] += ssmp2<<1;
                doom_mixer._fb_index += doom_mixer._fb_step;
            }
            audVoice[doom_mixer._fb_ix].index = doom_mixer._fb_index;
        }
    }

mixout:
    return;
}

/**********************************************************************/
