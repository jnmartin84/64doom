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

/**********************************************************************/
// On the PC, need swap routines because the MIDI file was made on the Amiga and
// is in big endian format. :)
// The Nintendo 64 is also big endian so these are no-ops here. :D

#define WSWAP(x) x
#define LSWAP(x) x

#define musFixedMul FixedMul

extern volatile struct AI_regs_s *AI_regs;

extern void *__n64_memset_ASM(void *p, int v, size_t n);
extern void *__n64_memset_ZERO_ASM(void *p, int v, size_t n);

extern int rom_close(int fd);
extern int rom_lseek(int fd, off_t offset, int whence);
extern int rom_open(int FILE_START, int size);
extern int rom_read(int fd, void *buf, size_t nbyte);
extern long rom_tell(int fd);

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

// NUM_VOICES = SFX_VOICES + MUS_VOICES
#define SFX_VOICES 16
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

static ULONG NUM_SAMPLES = 316;       // 1260 = 35Hz, 630 = 70Hz, 315 = 140Hz
static ULONG BEATS_PER_PASS = 4;       // 4 = 35Hz, 2 = 70Hz, 1 = 140Hz

static long __attribute__((aligned(8))) accum[316*2];
static short __attribute__((aligned(8))) pcmout1[316 * 2]; // 1260 stereo samples
static short __attribute__((aligned(8))) pcmout2[316 * 2];
static int __attribute__((aligned(8))) pcmflip = 0;
static short __attribute__((aligned(8))) *pcmout[2];// = &pcmout1[0];

short __attribute__((aligned(8))) *pcmbuf;// = &pcmout1[0];

static int changepitch;

static int32_t master_vol =  0x0000C000;//0C000; // 0000.C000

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

uint32_t __attribute__((aligned(8))) bits[8] = {0,0,0,0,0,0,0,0};

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
            n64_free(midiVoice[i].wave);
        }
    }
    __n64_memset_ZERO_ASM(mus_channel, 0, sizeof(struct Channel)*MUS_VOICES);
    __n64_memset_ZERO_ASM(audVoice, 0x00, sizeof(Voice_t)*(NUM_VOICES));
    __n64_memset_ZERO_ASM(midiVoice, 0, sizeof(Voice_t)*256);
    __n64_memset_ZERO_ASM(voice_inuse, 0, MUS_VOICES*sizeof(int));

    // instrument used lookups
    bits[0] = 0;
    bits[1] = 0;
    bits[2] = 0;
    bits[3] = 0;
    bits[4] = 0;
    bits[5] = 0;
    bits[6] = 0;
    bits[7] = 0;

    for (i=0;i<256;i++)
    {
        midiVoice[i].step = 1 << 16;
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
    return (void *)cnvsfx;
}


/////////////////////////////////////////////////////////////////////////////////////////
//Buffer filling
/////////////////////////////////////////////////////////////////////////////////////////



/**********************************************************************/
// Init at program start...
void I_InitSound (void)
{
    int i;

    audio_init(SAMPLERATE, 0);
    pcmout[0] = pcmout1;
    pcmout[1] = pcmout2;
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

    // Finished initialization.
    printf("I_InitSound: Sound module ready.\n");
    
    numChannels = SFX_VOICES;//NUM_VOICES;
}

/**********************************************************************/

#define AI_STATUS_FULL  ( 1 << 31 )
/**********************************************************************/
// ... update sound buffer and audio device at runtime...
void I_SubmitSound (void)
{
    if (!(AI_regs->status & AI_STATUS_FULL))
    {
        AI_regs->address = (uint32_t *)pcmbuf;
        AI_regs->length = 316*2*2;
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
    Sfx_Start (S_sfx[id].data, cnum, changepitch ? 
	//freqs[pitch]
	2765 + (((pitch<<2) + pitch)<<5) //	(160*pitch)
	: 11025, vol, sep, lengths[id]);
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
    Sfx_Update(handle, changepitch ? 
//	freqs[pitch]
	2765 + (((pitch<<2) + pitch)<<5) //	(160*pitch)
	: 11025, vol, sep);
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

    reset_midiVoices();
    
    music_okay = 0;
    
    mphnd = rom_open(MIDI_FILE, MIDI_FILESIZE);
    rom_read(mphnd, midi_pointers, sizeof(ULONG)*182);
    rom_close(mphnd);

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
    int32_t vol = (volume<<3)+7;//vol_table[volume];
    int32_t sep = seperation;

    audVoice[cnum].wave = wave + 8;
    audVoice[cnum].index = 0;
    audVoice[cnum].step = (ULONG)((step<<16) / SAMPLERATE);
    audVoice[cnum].loop = 0 << 16;
    audVoice[cnum].length = (length - 8) << 16;
    audVoice[cnum].ltvol = musFixedMul(((127 - ((127*sep*sep) / 65536))<<16),(vol<<7));
    sep -= 256;
    audVoice[cnum].rtvol = musFixedMul(((127 - ((127*sep*sep) / 65536))<<16),(vol<<7));
    audVoice[cnum].flags = 0x81;
}

void Sfx_Update(int cnum, int step, int volume, int seperation)
{
    int32_t vol = (volume<<3)+7;//vol_table[volume];
    int32_t sep = seperation;

    audVoice[cnum].step = (ULONG)((step<<16) / SAMPLERATE);
    audVoice[cnum].ltvol = musFixedMul(((127 - (127*sep*sep) / 65536) << 16),(vol<<7));
    sep -= 256;
    audVoice[cnum].rtvol = musFixedMul(((127 - (127*sep*sep) / 65536) << 16),(vol<<7));
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
        int ix;
        mus_volume = 0;
//        mus_playing = -1;
        /*for (ix=SFX_VOICES; ix<NUM_VOICES; ix++)
        {
            // disable voice
            audVoice[ix].flags = 0x00;
        }*/
    }
    else
    {
        if (!mus_volume)
        {
  //          mus_playing = 1;
        }
        mus_volume = (vol<<3)+7;//vol_table[vol];
    }
}

#define instrument_used(i) (bits[((i) >> 5)]  & (1 << ((i) & 31)))

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

    if (lptr[0] != LONG(0x1a53554d)) // 4D 55 53 1A
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

    used_instrument_count = inst_cnt;
    for (i=0;i<inst_cnt;i++)
    {
        uint8_t instrument = (uint8_t)SHORT(musheader->instruments[i]);
        bits[(instrument >> 5)] |= (1 << (instrument & 31));
    }

    // iterating over all instruments
    for (i=0;i<182;i++)
    {
        // current instrument is used
        if (instrument_used(i))
        {
            // get the pointer into the instrument data struct
            ULONG ptr = LSWAP(midi_pointers[i]);

            // make sure it doesn't point to NULL
            if (ptr)
            {
                // allocate some space for the header
                mhdr = (struct midiHdr*)n64_malloc(sizeof(struct midiHdr));
#ifdef RANGECHECK
                if (!mhdr)
                {
                    I_Error("no hdr malloc");
                }
#endif
                // open MIDI Instrument Set file from ROM
                hnd = rom_open(MIDI_FILE, MIDI_FILESIZE);
                if (hnd < 0)
                {
                    n64_free(mhdr);
                    continue;
                }

                rom_lseek(hnd,ptr,SEEK_SET);
                rom_read(hnd,(void*)mhdr,sizeof(struct midiHdr));

                ULONG length = LSWAP(mhdr->length) >> 16;

                void *sample = (void *)n64_malloc(length);

                if (sample)
                {
                    rom_lseek(hnd,ptr + 4 + 4 + 4 + 2,SEEK_SET);
                    rom_read(hnd, sample, length);
                                    
                    midiVoice[i].wave   = (BYTE*)sample;
                    midiVoice[i].index  = 0;
                    midiVoice[i].step   = 1 << 16;
                    midiVoice[i].loop   = LSWAP(mhdr->loop);
                    midiVoice[i].length = LSWAP(mhdr->length);
                    midiVoice[i].ltvol  = 0;
                    midiVoice[i].rtvol  = 0;
                    midiVoice[i].base   = WSWAP(mhdr->base);
                    midiVoice[i].flags  = 0x00;
                    
                    n64_free(mhdr);
                    rom_close(hnd);
                }
#ifdef RANGECHECK
                else
                {
                    I_Error("no sample malloc");
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

static int some_sample;

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
} event_loop_state_t;
static mixer_state_t doom_mixer;
//static event_loop_state_t loop_state;


/***
lw      t0,%gp_rel(mus_playing)(gp)
lui	a0, %hi(accum)
addu a1, zero, zero
jal	__n64_memset_ASM
addiu a2, zero, (NUM_SAMPLES<<3)

blt	t0,zero, music_off
lui     t1,%hi(loop_state)

// t0 is mus_playing
// t1 is: pointer to loop state
addiu t1, t1, %lo(loop_state)

addi t2, zero, 1
ble  t0, t2, not_restart
sw t2, %gp_rel(mus_playing)(gp)
lw      t3,%gp_rel(score_data)(gp)
lhu     t4,%gp_rel(score_start)(gp)
// t4 is now score pointer
addu t4, t3, t4

not_restart:
  


mix:

lui     t1,%hi(doom_mixer)
addiu	t1,t1,%lo(doom_mixer)

b mixout
nop

mixout:
lui     t0,%hi(accum)
addiu	t0,t0,%lo(accum)
lw      t1,%gp_rel(pcmbuf)(gp)
sra		t2, a2, 1

mixout_loop:
	addu t3, t0, t2

	// accumulate samples
	lhu  t4, 0(t3)
	lhu  t5, 2(t3)
	lhu  t6, 4(t3)
	lhu  t7, 6(t3)

	// y<<16 | (y+1)
	sll	t4, t4, 16
	or t5, t5, t4
	// y<<16 | (y+1)
	sll t6, t6, 16
	or t7, t7, t6
	
	// pcmbuf + index
	addu t3, t1, t2

	sw t5, 0(t3)
	sw t7, 4(t3)
	blt t2, a2, mixout_loop
	// increment index by 2 samples
	addiu	t2, t2, 8

	jr ra
	nop

music_off:
lw t0,%gp_rel(snd_SfxVolume)($28)
bgt	t0, zero, mixout
sw zero,%gp_rel(mus_playing)(gp)
b mix
nop

    uint32_t lsmp = ((accum[doom_mixer._fb_iy])<<16) | ((accum[doom_mixer._fb_iy+1])&0x0000FFFF);
        *((uint32_t *)(&pcmbuf[doom_mixer._fb_iy])) = lsmp;



// t1 is &doom_mixer
// 0(t0) is first voice to mix
// 4(t0) is last voice to mix
    8(t0) _fb_index;
    12(t0) _fb_step;
    16(t0) _fb_ltvol;
    20(t0) _fb_rtvol;
    24(t0) _fb_sample;
    28(t0) _fb_ix;
    32(t0) _fb_iy;
    36(t0) _fb_loop;
    40(t0) _fb_length;
    44(t0) _fb_pval;
	48(t0) _fb_pval2;
    52(t0) BYTE *_fb_wvbuff;
*/




void I_UpdateSound (void)
{
	event_loop_state_t loop_state;
    __n64_memset_ZERO_ASM((void *)accum, 0, (NUM_SAMPLES << 3));
	// clear buffer
	// find out if this is faster or slower than memset
/*    for(int i=0;i<NUM_SAMPLES*2;i+=8) {
        *(uint64_t *)(&accum[i]) = 0;
        *(uint64_t *)(&accum[i+2]) = 0;
        *(uint64_t *)(&accum[i+4]) = 0;
        *(uint64_t *)(&accum[i+6]) = 0;
    }*/

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
			__n64_memset_ZERO_ASM((void*)&loop_state, 0, sizeof(event_loop_state_t));

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

							audVoice[loop_state.voice + SFX_VOICES].index = 5;					
                            audVoice[loop_state.voice + SFX_VOICES].flags = 0x01;

                            if (loop_state.channel != 15)
                            {
								loop_state.inst = mus_channel[loop_state.channel].instrument;
                                // back link for pitch wheel
                                audVoice[loop_state.voice + SFX_VOICES].chan = loop_state.channel;
                                audVoice[loop_state.voice + SFX_VOICES].wave = midiVoice[loop_state.inst].wave;
                                
								
								// need to make a table that says which instruments should not have their index reset
								// when voice changes, it fixes some of the music issues
								// test for weird bass
								
								// empirically derived formula for note_table lookup
								uint32_t x = ((72 - midiVoice[loop_state.inst].base + (ULONG)loop_state.note) & 0x7f);
								// the pair of % and / hopefully compiles down to a single div instruction
								// with a mflo mfhi to get each part... hopefully
								uint32_t xi = x%12;
								uint32_t xs = x/12;
								uint32_t xi2 = xi*xi;
								// (xi2 << 7) + (xi2 << 4)128+16
                                audVoice[loop_state.voice + SFX_VOICES].step = (	((65536)+(((xi2 << 7) + (xi2 << 4) + (xi2 << 3))+((xi<<12)-(xi<<9))) ) << xs ) >> 6; //note_table[(72 - midiVoice[inst].base + (ULONG)note) & 0x7f];
                                audVoice[loop_state.voice + SFX_VOICES].loop = midiVoice[loop_state.inst].loop;
                                audVoice[loop_state.voice + SFX_VOICES].length = midiVoice[loop_state.inst].length;
                                // enable voice
                            }
                            else
                            {
                                // percussion channel - note is percussion instrument
                                loop_state.inst = (ULONG)loop_state.note + 100;
                                // back link for pitch wheel
                                audVoice[loop_state.voice + SFX_VOICES].chan = loop_state.channel;
                                audVoice[loop_state.voice + SFX_VOICES].wave = midiVoice[loop_state.inst].wave;
//                                audVoice[loop_state.voice + SFX_VOICES].index = 0;
                                audVoice[loop_state.voice + SFX_VOICES].step = 0x10000;
                                audVoice[loop_state.voice + SFX_VOICES].loop = midiVoice[loop_state.inst].loop;
                                audVoice[loop_state.voice + SFX_VOICES].length = midiVoice[loop_state.inst].length;
                                // enable voice
//                                audVoice[loop_state.voice + SFX_VOICES].flags = 0x01;
                            }
                        }
                        break;
                    }
                    // Pitch
                    case 2:
                    {
                        // score_ptr++ 2
                        loop_state.pitch = ((ULONG)*score_ptr++)&0xFF;
                        loop_state.channel = (int)(loop_state.event & 15);
						mus_channel[loop_state.channel].pitch = (/*60*(pitch&0xff)*/(loop_state.pitch << 6) - (loop_state.pitch<<2))+58180;//pitch_table[pitch & 0xff];
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

mix:
    doom_mixer.first_voice_to_mix = snd_SfxVolume ? 0 : SFX_VOICES;
    doom_mixer.last_voice_to_mix = mus_playing ? NUM_VOICES : (snd_SfxVolume ? SFX_VOICES : 0); //NUM_VOICES >> (1 - mus_playing);
    //uint32_t numvoices = doom_mixer.last_voice_to_mix + doom_mixer.first_voice_to_mix;
    // mix enabled voices
    for (doom_mixer._fb_ix=doom_mixer.first_voice_to_mix; doom_mixer._fb_ix<doom_mixer.last_voice_to_mix; doom_mixer._fb_ix++)
//    for (doom_mixer._fb_ix=0; doom_mixer._fb_ix<NUM_VOICES; doom_mixer._fb_ix++)
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
//					uint32_t ltv,rtv;
//					ltv = doom_mixer._fb_ltvol;
//					rtv = doom_mixer._fb_rtvol;
                    // releasing
					// supposed to be * 7/8 but * 1/2 works and is cheaper
                    doom_mixer._fb_ltvol = ((doom_mixer._fb_ltvol << 3)-doom_mixer._fb_ltvol)>>3;
                    doom_mixer._fb_rtvol = ((doom_mixer._fb_rtvol << 3)-doom_mixer._fb_rtvol)>>3;
                    audVoice[doom_mixer._fb_ix].ltvol = doom_mixer._fb_ltvol;
                    audVoice[doom_mixer._fb_ix].rtvol = doom_mixer._fb_rtvol;

					// 0.02*127 / 2 as 16.16 fixed point
					// 0x19664D>>1
					uint32_t cmpvol = 0x19664D>>1;//(uint32_t)((mus_volume<<13)+((mus_volume<<2) - mus_volume));//+(mus_volume<<14)); // 12,11
                    //if (((uint32_t)doom_mixer._fb_ltvol <= (uint32_t)((0x000CB326*mus_volume) >> 7)) && ((uint32_t)doom_mixer._fb_rtvol <= (uint32_t)((0x000CB326*mus_volume)>>7))) 
                    if (((uint32_t)doom_mixer._fb_ltvol <= cmpvol) && ((uint32_t)doom_mixer._fb_rtvol <= cmpvol))
					{
                        // disable voice
                        audVoice[doom_mixer._fb_ix].flags = 0;
                        // next voice
                        continue;
                    }
                }

                doom_mixer._fb_step = (((int64_t)doom_mixer._fb_step * (int64_t)(mus_channel[audVoice[doom_mixer._fb_ix].chan & 15].pitch))>>16);
//				doom_mixer._fb_ltvol = (((int64_t)doom_mixer._fb_ltvol * (int64_t)(mus_volume<<7))>>16);
//                doom_mixer._fb_rtvol = (((int64_t)doom_mixer._fb_rtvol * (int64_t)(mus_volume<<7))>>16);	
				doom_mixer._fb_ltvol = (((int64_t)doom_mixer._fb_ltvol * (int64_t)(mus_volume))>>9);
                doom_mixer._fb_rtvol = (((int64_t)doom_mixer._fb_rtvol * (int64_t)(mus_volume))>>9);	
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
				
				some_sample = 0;
				if((uint32_t)doom_mixer._fb_wvbuff & 0x80000000)
				{
//if(doom_mixer._fb_index == 0 || doom_mixer._fb_index == doom_mixer._fb_length-1) {
			some_sample = doom_mixer._fb_wvbuff[doom_mixer._fb_index>>16];
//}
//else {
//			some_sample = (doom_mixer._fb_wvbuff[(doom_mixer._fb_index>>16)-1] + doom_mixer._fb_wvbuff[doom_mixer._fb_index>>16]
//			+
//			doom_mixer._fb_wvbuff[(doom_mixer._fb_index>>16)+1]) / 3;
//			
//}
				}
                doom_mixer._fb_sample = musFixedMul(some_sample,master_vol);
				int ssmp1 = musFixedMul(doom_mixer._fb_sample, doom_mixer._fb_ltvol);
                int ssmp2 = musFixedMul(doom_mixer._fb_sample, doom_mixer._fb_rtvol);
                accum[doom_mixer._fb_iy    ] += ssmp1<<1;
                accum[doom_mixer._fb_iy + 1] += ssmp2<<1;
                doom_mixer._fb_index += doom_mixer._fb_step;
            }
            audVoice[doom_mixer._fb_ix].index = doom_mixer._fb_index;
        }
//		break;
    }

mixout:
    for (doom_mixer._fb_iy=0; doom_mixer._fb_iy < (NUM_SAMPLES << 1); doom_mixer._fb_iy+=4)
    {
        uint32_t lsmp = ((accum[doom_mixer._fb_iy])<<16) | ((accum[doom_mixer._fb_iy+1])&0x0000FFFF);
        uint32_t lsmp2 = ((accum[doom_mixer._fb_iy+2])<<16) | ((accum[doom_mixer._fb_iy+3])&0x0000FFFF);
        *((uint32_t *)(&pcmbuf[doom_mixer._fb_iy])) = lsmp;
        *((uint32_t *)(&pcmbuf[doom_mixer._fb_iy+2])) = lsmp2;
    }
}

/**********************************************************************/
