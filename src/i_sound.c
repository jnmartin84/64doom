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

// I haven't re-written this yet to use fillbuffer callbacks so I just bang at the hardware
volatile struct AI_regs_s *AI_regs = (struct AI_regs_s *)0xA4500000;

// Any value of numChannels set
// by the defaults code in M_misc is now clobbered by I_InitSound().
// number of channels available for sound effects
extern int numChannels;

/**********************************************************************/


#define SAMPLERATE 11025

// NUM_VOICES = SFX_VOICES + MUS_VOICES
#define NUM_MIDI_INSTRUMENTS 182
#define SFX_VOICES 8
#define MUS_VOICES 16
#define NUM_VOICES (SFX_VOICES+MUS_VOICES)


typedef struct MUSheader
{
    // identifier "MUS" 0x1A
    char        ID[4];
    uint16_t    scoreLen;
    uint16_t    scoreStart;
    // count of primary channels
    uint16_t    channels;
    // count of secondary channels
    uint16_t    sec_channels;
    uint16_t    instrCnt;
    uint16_t    dummy;

    // variable-length part starts here
    uint16_t    instruments[];
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
    uint32_t rate;
    uint32_t loop;                          // 16.16 fixed pt
    uint32_t length;                        // 16.16 fixed pt
    uint16_t base;                          // note base of sample
    int8_t sample[8];                      // actual length varies
};


typedef struct Voice
{
    uint32_t index;
    uint32_t step;
    uint32_t loop;
    uint32_t length;
    int32_t ltvol;
    int32_t rtvol;
    int chan;
    uint16_t base;
    uint16_t flags;
    int8_t *wave;
}
Voice_t;

static int __attribute__((aligned(8))) voice_inuse[MUS_VOICES];
static struct Channel __attribute__((aligned(8))) mus_channel[MUS_VOICES];

static struct Voice __attribute__((aligned(8))) audVoice[NUM_VOICES];
static struct Voice __attribute__((aligned(8))) midiVoice[NUM_MIDI_INSTRUMENTS];

// The actual lengths of all sound effects.
static int __attribute__((aligned(8))) lengths[NUMSFX];

static uint32_t NUM_SAMPLES = 316;
static uint32_t BEATS_PER_PASS = 4;

static short __attribute__((aligned(8))) pcmout1[632*4];
static short __attribute__((aligned(8))) pcmout2[632*4];
static int __attribute__((aligned(8))) pcmflip = 0;
static short __attribute__((aligned(8))) *pcmout[2];

short __attribute__((aligned(8))) *pcmbuf;

static int changepitch;

static uint32_t master_vol =  0x0001B000; // (0x0000F000 << 1) - 0x3000 

static int musicdies  = -1;
static int music_okay =  0;

uint32_t __attribute__((aligned(8))) midi_pointers[NUM_MIDI_INSTRUMENTS];

static uint16_t score_len, score_start, inst_cnt;
static void __attribute__((aligned(8))) *score_data;
static uint8_t __attribute__((aligned(8))) *score_ptr;

static int mus_delay    = 0;
static int mus_looping  = 0;
static int32_t mus_volume = 127;
int mus_playing  = 0;

uint32_t __attribute__((aligned(8))) used_instrument_bits[8] = {0,0,0,0,0,0,0,0};

extern int snd_SfxVolume;

extern display_context_t _dc;

#define PANMOD 127


/**********************************************************************/

void fill_buffer(short *buffer);

void Sfx_Start(int8_t *wave, int cnum, int step, int vol, int sep, int length);
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

    for (i=0;i<NUM_MIDI_INSTRUMENTS;i++)
    {
        if (midiVoice[i].wave)
        {
            free((void*)((uintptr_t)midiVoice[i].wave & 0x8FFFFFFF));
        }
    }
    D_memset(mus_channel, 0, sizeof(mus_channel));
    D_memset(audVoice,    0, sizeof(audVoice));
    D_memset(midiVoice,   0, sizeof(midiVoice));
    D_memset(voice_inuse, 0, sizeof(voice_inuse));

    // instrument used lookups
    D_memset(used_instrument_bits, 0, sizeof(used_instrument_bits));

    for (i=0;i<NUM_MIDI_INSTRUMENTS;i++)
    {
        midiVoice[i].step = 0x10000;
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
    D_memset((void *)pcmbuf, 0, (NUM_SAMPLES << 2));
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

    rc = dfs_read(midi_pointers, sizeof(uint32_t)*NUM_MIDI_INSTRUMENTS, 1, mphnd);
    dfs_close(mphnd);
    if ((sizeof(uint32_t)*NUM_MIDI_INSTRUMENTS) != rc)
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

void Sfx_Start(int8_t *wave, int cnum, int step, int volume, int seperation, int length)
{
    int32_t vol = (volume<<3);
    int32_t sep = seperation;

    audVoice[cnum].wave = wave + 8;
    audVoice[cnum].index = 0;
    audVoice[cnum].step = (uint32_t)((step<<16) / SAMPLERATE);
    audVoice[cnum].loop = 0 ;
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

    audVoice[cnum].step = (uint32_t)((step<<16) / SAMPLERATE);
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

    uint32_t *lptr = (uint32_t *)musdata;
    uint16_t *wptr = (uint16_t *)musdata;

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
    D_memset(used_instrument_bits,0,sizeof(used_instrument_bits));
    used_instrument_count = inst_cnt;
    for (i=0;i<inst_cnt;i++)
    {
        uint8_t instrument = (uint8_t)SHORT(musheader->instruments[i]);
        
        // fix TNT crash with one of the 10s levels
        uint32_t ptr = midi_pointers[instrument];
        if (!ptr)
            continue;
        used_instrument_bits[(instrument >> 5)] |= (1 << (instrument & 31));
    }

    // iterating over all instruments
    for (i=0;i<NUM_MIDI_INSTRUMENTS;i++)
    {
        // current instrument is used
        if (instrument_used(i))
        {
            // get the pointer into the instrument data struct
            uint32_t ptr = midi_pointers[i];

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

                uint32_t length = mhdr->length >> 16;

                void *sample = (void *)((uintptr_t)malloc(length));

                if (sample)
                {
                    dfs_seek(hnd,ptr + 4 + 4 + 4 + 2,SEEK_SET);
                    dfs_read(sample,length,1,hnd);
                    // it doesn't hurt to access sound data from a non-cached address
                    // it gets mixed to uncached buffer anyway
                    midiVoice[i].wave   = (int8_t*)((uintptr_t)sample | 0xA0000000);
                    midiVoice[i].index  = 0;
                    midiVoice[i].step   = 0x10000;
                    midiVoice[i].loop   = mhdr->loop;
                    midiVoice[i].length = mhdr->length;
                    midiVoice[i].ltvol  = 0;
                    midiVoice[i].rtvol  = 0;
                    midiVoice[i].base   = mhdr->base;
                    midiVoice[i].flags  = 0;

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

void I_MixSound (void)
{
    for (size_t iy=0; iy < (NUM_SAMPLES << 1); iy+=2)
    {
        uint32_t next_mixed_sample = 0;
        // mix enabled voices
        for (size_t ix=0; ix<NUM_VOICES; ix++)
        {
            uint32_t index = audVoice[ix].index;
            if (!(audVoice[ix].flags & 0x01))
                continue;

            uint32_t step = audVoice[ix].step;;
            uint32_t ltvol = audVoice[ix].ltvol;
            uint32_t rtvol = audVoice[ix].rtvol;
            uint32_t loop = audVoice[ix].loop;
            uint32_t length = audVoice[ix].length;
            int8_t*  wvbuff = audVoice[ix].wave;
#ifdef RANGECHECK
            // Doom 2 has issues without this, or at least my 1.666 data files do
            if (!((uintptr_t)wvbuff & 0x80000000))
            {
                continue;
            }
#endif
            if (!(audVoice[ix].flags & 0x80))
            {
                // special handling for instrument
                if ((audVoice[ix].flags & 0x02) && iy == 0)
                {
                    // releasing
                    ltvol = ((ltvol << 3)-ltvol)>>3;
                    rtvol = ((rtvol << 3)-rtvol)>>3;
                    audVoice[ix].ltvol = ltvol;
                    audVoice[ix].rtvol = rtvol;

                    // this was originally derived from some floating point value in the original code
                    // changed to fixed_t when I went to a fixed-point mixer
                    // and further adjusted by ear until settling on this value
                    #define cmpvol (665835 - 150000)
                    if (((uint32_t)ltvol <= cmpvol) && ((uint32_t)rtvol <= cmpvol))
                    {
                        // disable voice
                        audVoice[ix].flags = 0;
                        // next voice
                        continue;
                    }
                }
                step = FixedMul(step, mus_channel[audVoice[ix].chan & 15].pitch);
                ltvol = FixedMul(ltvol, mus_volume) << 7;//(((uint64_t)ltvol * (uint64_t)(mus_volume)) >> 9);
                rtvol = FixedMul(rtvol, mus_volume) << 7;//(((uint64_t)rtvol * (uint64_t)(mus_volume)) >> 9);
            }

            if (index >= length)
            {
                if (audVoice[ix].flags & 0x80)
                {
                    // disable voice
                    audVoice[ix].flags = 0;
                    // exit sample loop
                    continue;
                }
                else
                {
                    // check if instrument has loop
                    if (loop)
                    {
                        index -= length;
                        index += loop;
                    }
                    else
                    {
                        // disable voice
                        audVoice[ix].flags = 0;
                        // exit sample loop
                        continue;
                    }
                }
            }

            uint32_t sample = FixedMul(wvbuff[index >> 16],master_vol);

            uint32_t ssmp1 = FixedMul(sample, ltvol)<<16;
            uint32_t ssmp2 = FixedMul(sample, rtvol)&0xFFFF;

            next_mixed_sample += (ssmp1 | ssmp2);

            index += step;

            audVoice[ix].index = index;
        }

        *((uint32_t *)&pcmbuf[iy]) = next_mixed_sample;
    }
}

/**********************************************************************/

void I_UpdateSound (void)
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
            score_ptr = (uint8_t *)((uint32_t)score_data + (uint32_t)score_start);
        }

        // 1=140Hz, 2=70Hz, 4=35Hz
        mus_delay -= BEATS_PER_PASS;
        if (mus_delay <= 0)
        {
           uint32_t pitch;
           uint8_t event;
           uint8_t note;
           uint8_t time;
           uint8_t ctrl;
           uint8_t value;
           int32_t next_time;
           int32_t channel;
           int32_t voice;
           int32_t inst;
           int32_t volume;
           int32_t pan;
nextEvent:
            do
            {
                event = *score_ptr++;
                switch ((event >> 4) & 7)
                {
                    // Release
                    case 0:
                    {
                        note = *score_ptr++;
                        note &= 0x7f;
                        channel = (int)(event & 15);
                        voice = mus_channel[channel].map[(uint32_t)note] - 1;
                        mus_channel[channel].lastnote = -1;
                        if (voice >= 0)
                        {
                            // clear mapping
                            mus_channel[channel].map[(uint32_t)note] = 0;
                            // voice available
                            voice_inuse[voice] = 0;
                            // voice releasing
                            audVoice[voice + SFX_VOICES].flags |= 0x02;
                        }
                        break;
                    }
                    // Play note
                    case 1:
                    {
                        note = *score_ptr++;
                        channel = (int)(event & 15);
                        volume = 0;//-1;
                        if (note & 0x80)
                        {
                            // set volume as well
                            note &= 0x7f;
                            // score_ptr++ 3
                            volume = (int32_t)(*score_ptr++);
                        }
                        for (voice=0; voice<MUS_VOICES+1; voice++)
                        {
                            if(voice < MUS_VOICES && !voice_inuse[voice])
                            {
                                // found free voice
                                break;
                            }
                        }
                        if (voice < MUS_VOICES)
                        {
                            // in use
                            voice_inuse[voice] = 1;
                            mus_channel[channel].lastnote = note;
                            mus_channel[channel].map[(uint32_t)note] = voice + 1;
                            if (volume > 0)
                            {
                                mus_channel[channel].vol = volume;
                                pan = mus_channel[channel].pan;
                                mus_channel[channel].ltvol = (((127 - pan) * volume)) << 9;
                                mus_channel[channel].rtvol = (((127 - (127 - pan)) * volume)) << 9;
                            }

                            audVoice[voice + SFX_VOICES].ltvol = mus_channel[channel].ltvol;
                            audVoice[voice + SFX_VOICES].rtvol = mus_channel[channel].rtvol;

                            // enable voice
                            audVoice[voice + SFX_VOICES].flags = 0x01;

                            if (channel != 15)
                            {
                                inst = mus_channel[channel].instrument;
                                struct Voice *mvc = &midiVoice[inst];
                                // back link for pitch wheel
                                audVoice[voice + SFX_VOICES].chan = channel;
                                audVoice[voice + SFX_VOICES].wave = mvc->wave;

                                // I am sorry I don't have documentation on what I did here
                                // but the highlights are
                                // I took the tables for frequency and whatnot and did some curve fitting
                                // with an online tool and then finessed the equations until it sounded good

                                // all of this was to reduce memory accesses when mixing music

                                // empirically derived formula for note_table lookup
                                uint32_t x = ((72 - mvc->base + (uint32_t)note) & 0x7f);

                                // why 12? i dont know any more, but it was something
                                // modern GCC at -O3 compiles this without generating a div, makes me happy
                                uint32_t xi  = x % 12;
                                uint32_t xs  = x / 12;
                                // xi squared
                                uint32_t xi2 = xi * xi;

                                audVoice[voice + SFX_VOICES].step = ( (0x10000 + (((xi2 << 7) + (xi2 << 4) + (xi2<<3))+((xi<<12)-(xi<<9))) ) << xs ) >> 6;
                                //note_table[(72 - midiVoice[inst].base + (uint32_t)note) & 0x7f];
                                audVoice[voice + SFX_VOICES].loop = mvc->loop;
                                audVoice[voice + SFX_VOICES].length = mvc->length;
                            }
                            else
                            {
                                // percussion channel - note is percussion instrument
                                inst = (uint32_t)note + 100;
                                struct Voice *mvc = &midiVoice[inst];
                                // back link for pitch wheel
                                audVoice[voice + SFX_VOICES].chan = channel;
                                audVoice[voice + SFX_VOICES].wave = mvc->wave;
                                audVoice[voice + SFX_VOICES].step = 0x10000;
                                audVoice[voice + SFX_VOICES].loop = mvc->loop;
                                audVoice[voice + SFX_VOICES].length = mvc->length;
                            }
                            audVoice[voice + SFX_VOICES].index = 0;
                        }
                        break;
                    }
                    // Pitch
                    case 2:
                    {
                        // score_ptr++ 2
                        pitch = ((uint32_t)*score_ptr++)&0xFF;
                        channel = (int)(event & 15);
                        // pitch*64 - pitch*4 == pitch*60
                        mus_channel[channel].pitch = ((pitch << 6) - (pitch<<2)) + 58180;
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
                        ctrl = *score_ptr++;
                        // score_ptr++ 3
                        value = *score_ptr++;
                        channel = (int)(event & 15);
                        switch (ctrl)
                        {
                            case 0:
                            {
                                // set channel instrument
                                mus_channel[channel].instrument = (uint32_t)value;
                                mus_channel[channel].pitch = 0x10000;
                                break;
                            }
                            case 3:
                            {
                                // set channel volume
                                mus_channel[channel].vol = volume = value;
                                pan = mus_channel[channel].pan;
                                mus_channel[channel].ltvol = (((127 - pan) * volume)) << 9;
                                mus_channel[channel].rtvol = (((127 - (127 - pan)) * volume)) << 9;
                                break;
                            }
                            case 4:
                            {
                                // set channel pan
                                mus_channel[channel].pan = pan = value;
                                volume = mus_channel[channel].vol;
                                mus_channel[channel].ltvol = (((127 - pan) * volume)) << 9;
                                mus_channel[channel].rtvol = (((127 - (127 - pan)) * volume)) << 9;
                                break;
                            }
                        }
                        break;
                    }
                    // End score
                    case 6:
                    {
                        for (voice=0; voice<MUS_VOICES; voice++)
                        {
                            audVoice[voice + SFX_VOICES].flags = 0;
                            voice_inuse[voice] = 0;
                        }

                        if (mus_looping)
                        {
                            score_ptr = (uint8_t *)((uint32_t)score_data + (uint32_t)score_start);
                        }
                        else
                        {
                            mus_delay = 0;
                            mus_playing = 0;
                            goto mix;
                        }
                        break;
                    }
                }
            }
            // not last event
            while (!(event & 0x80));

            // now get the time until the next event(s)
            next_time = 0;
            time = *score_ptr++;

            while (time & 0x80)
            {
                // while msb set, accumulate 7 bits
                next_time |= (uint32_t)(time & 0x7f);
                next_time <<= 7;
                time = *score_ptr++;
            }

            next_time |= (uint32_t)time;
            mus_delay += next_time;

            if (mus_delay <= 0)
            {
                goto nextEvent;
            }
        }
    }

mix:
    I_MixSound();

mixout:
    return;
}

/**********************************************************************/
