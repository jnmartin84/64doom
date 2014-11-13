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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <unistd.h>

#include <float.h>

#include "z_zone.h"

#include "i_system.h"
#include "i_sound.h"
#include "m_argv.h"
#include "m_misc.h"
#include "w_wad.h"
#include "m_swap.h"

#include "doomdef.h"


extern void *n64_memset(void *p, int v, size_t n);

extern int rom_close(int fd);
extern int rom_lseek(int fd, off_t offset, int whence);
extern int rom_open(int FILE_START, int size);
extern int rom_read(int fd, void *buf, size_t nbyte);
extern long rom_tell(int fd);

extern int status_color;

extern const int MIDI_FILE;
extern const int MIDI_FILESIZE;

// Any value of numChannels set
// by the defaults code in M_misc is now clobbered by I_InitSound().
// number of channels available for sound effects
extern int numChannels;

static volatile int should_sound = 0;


/**********************************************************************/

typedef unsigned int ULONG;
typedef unsigned short UWORD;
typedef unsigned char UBYTE;
typedef char BYTE;

#define SAMPLERATE 11025
//#define SAMPLERATE 22050

// NUM_VOICES = SFX_VOICES + MUS_VOICES
#define NUM_VOICES 40
#define SFX_VOICES 16
#define MUS_VOICES 24


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
    float pitch;
    float pan;
    float vol;
    float ltvol;
    float rtvol;
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
    BYTE *wave;
    float index;
    float step;
    ULONG loop;
    ULONG length;
    float ltvol;
    float rtvol;
    UWORD base;
    UWORD flags;
    int chan;
}
Voice_t;


static int __attribute__((aligned(8))) voice_avail[MUS_VOICES];

static struct Channel __attribute__((aligned(8))) mus_channel[16];

static struct Voice __attribute__((aligned(8))) audVoice[NUM_VOICES];
static struct Voice __attribute__((aligned(8))) midiVoice[256];

// The actual lengths of all sound effects.
static int __attribute__((aligned(8))) lengths[NUMSFX];

static ULONG NUM_SAMPLES = 316;       // 1260 = 35Hz, 630 = 70Hz, 315 = 140Hz
static ULONG BEATS_PER_PASS = 4;       // 4 = 35Hz, 2 = 70Hz, 1 = 140Hz

static int sound_status = 0;

#if 0
/*static*/ short __attribute__((aligned(8))) pcmout1[316 * 2]; // 1260 stereo samples
/*static*/ short __attribute__((aligned(8))) pcmout2[316 * 2];
/*static*/ int pcmflip = 0;
#endif

volatile int snd_ticks; // advanced by sound thread

static int changepitch;

/* sampling freq (Hz) for each pitch step when changepitch is TRUE
   calculated from (2^((i-128)/64))*11025
   I'm not sure if this is the right formula */
static UWORD __attribute__((aligned(8))) freqs[256] =
{
    2756, 2786, 2817, 2847, 2878, 2910, 2941, 2973,
    3006, 3038, 3072, 3105, 3139, 3173, 3208, 3242,
    3278, 3313, 3350, 3386, 3423, 3460, 3498, 3536,
    3574, 3613, 3653, 3692, 3733, 3773, 3814, 3856,
    3898, 3940, 3983, 4027, 4071, 4115, 4160, 4205,
    4251, 4297, 4344, 4391, 4439, 4487, 4536, 4586,
    4635, 4686, 4737, 4789, 4841, 4893, 4947, 5001,
    5055, 5110, 5166, 5222, 5279, 5336, 5394, 5453,
    5513, 5573, 5633, 5695, 5757, 5819, 5883, 5947,
    6011, 6077, 6143, 6210, 6278, 6346, 6415, 6485,
    6556, 6627, 6699, 6772, 6846, 6920, 6996, 7072,
    7149, 7227, 7305, 7385, 7465, 7547, 7629, 7712,
    7796, 7881, 7967, 8053, 8141, 8230, 8319, 8410,
    8501, 8594, 8688, 8782, 8878, 8975, 9072, 9171,
    9271, 9372, 9474, 9577, 9681, 9787, 9893, 10001,
    10110, 10220, 10331, 10444, 10558, 10673, 10789, 10906,
    11025, 11145, 11266, 11389, 11513, 11638, 11765, 11893,
    12023, 12154, 12286, 12420, 12555, 12692, 12830, 12970,
    13111, 13254, 13398, 13544, 13691, 13841, 13991, 14144,
    14298, 14453, 14611, 14770, 14931, 15093, 15258, 15424,
    15592, 15761, 15933, 16107, 16282, 16459, 16639, 16820,
    17003, 17188, 17375, 17564, 17756, 17949, 18144, 18342,
    18542, 18744, 18948, 19154, 19363, 19574, 19787, 20002,
    20220, 20440, 20663, 20888, 21115, 21345, 21578, 21812,
    22050, 22290, 22533, 22778, 23026, 23277, 23530, 23787,
    24046, 24308, 24572, 24840, 25110, 25384, 25660, 25940,
    26222, 26508, 26796, 27088, 27383, 27681, 27983, 28287,
    28595, 28907, 29221, 29540, 29861, 30187, 30515, 30848,
    31183, 31523, 31866, 32213, 32564, 32919, 33277, 33639,
    34006, 34376, 34750, 35129, 35511, 35898, 36289, 36684,
    37084, 37487, 37896, 38308, 38725, 39147, 39573, 40004,
    40440, 40880, 41325, 41775, 42230, 42690, 43155, 43625
};

static int __attribute__((aligned(8))) note_table[128] =
{
    65536/64,69433/64,73562/64,77936/64,82570/64,87480/64,92682/64,98193/64,104032/64,110218/64,116772/64,123715/64,
    65536/32,69433/32,73562/32,77936/32,82570/32,87480/32,92682/32,98193/32,104032/32,110218/32,116772/32,123715/32,
    65536/16,69433/16,73562/16,77936/16,82570/16,87480/16,92682/16,98193/16,104032/16,110218/16,116772/16,123715/16,
    65536/8,69433/8,73562/8,77936/8,82570/8,87480/8,92682/8,98193/8,104032/8,110218/8,116772/8,123715/8,
    65536/4,69433/4,73562/4,77936/4,82570/4,87480/4,92682/4,98193/4,104032/4,110218/4,116772/4,123715/4,
    65536/2,69433/2,73562/2,77936/2,82570/2,87480/2,92682/2,98193/2,104032/2,110218/2,116772/2,123715/2,
    65536,69433,73562,77936,82570,87480,92682,98193,104032,110218,116772,123715,
    65536*2,69433*2,73562*2,77936*2,82570*2,87480*2,92682*2,98193*2,104032*2,110218*2,116772*2,123715*2,
    65536*4,69433*4,73562*4,77936*4,82570*4,87480*4,92682*4,98193*4,104032*4,110218*4,116772*4,123715*4,
    65536*8,69433*8,73562*8,77936*8,82570*8,87480*8,92682*8,98193*8,104032*8,110218*8,116772*8,123715*8,
    65536*16,69433*16,73562*16,77936*16,82570*16,87480*16,92682*16,98193*16
};

static float __attribute__((aligned(8))) pitch_table[256];

static float master_vol =  64.0f;

static int musicdies  = -1;
static int music_okay =  0;

void *midi_pointers = NULL;

static UWORD score_len, score_start, inst_cnt;
static void *score_data;
static UBYTE *score_ptr;

static int mus_delay    = 0;
static int mus_looping  = 0;
static float mus_volume = 1.0f;
static int mus_playing  = 0;


static uint32_t lifetime_max_mus_sample_alloc = 0;
static uint32_t mus_sample_memory_allocated = 0;

static int audio_queued = 1;
static int lastbuf = 0;

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

void __attribute__((aligned(8))) *sample_buffers[256];
int __attribute__((aligned(8))) used_instruments[256];

int used_instrument_count = -1;

// Need swap routines because the MIDI file was made on the Amiga and
//   is in big endian format. :)

#define WSWAP(x) x
#define LSWAP(x) x

// Swap 16bit, that is, MSB and LSB byte.
unsigned short SWAPSHORT(unsigned short x)
{
    // No masking with 0xFF should be necessary.
    return (x>>8) | (x<<8);
}

// Swapping 32bit.
unsigned long SWAPLONG( unsigned long x)
{
    return
        (x>>24)
        | ((x>>8) & 0xff00)
        | ((x<<8) & 0xff0000)
        | (x<<24);
}


uint32_t get_max_allocated_mus_memory()
{
    return lifetime_max_mus_sample_alloc;
}


uint32_t get_allocated_mus_memory()
{
    return mus_sample_memory_allocated;
}


void reset_midiVoices(void)
{
    int i;

    for (i=0;i<256;i++)
    {
        if (NULL != (void*)midiVoice[i].wave)
        {
            Z_Free(midiVoice[i].wave);
        }

        midiVoice[i].wave = NULL;
        midiVoice[i].index = 0.0f;
        midiVoice[i].step = 1.0f;
        midiVoice[i].loop = 0;
        midiVoice[i].length = 2000 << 16;
        midiVoice[i].ltvol = 0.0f;
        midiVoice[i].rtvol = 0.0f;
        midiVoice[i].base = 60;
        midiVoice[i].flags = 0x00;

        used_instruments[i] = -1;
        sample_buffers[i] = NULL;
    }

    used_instrument_count = -1;

    mus_sample_memory_allocated = 0;
}


/**********************************************************************/
//
// This function loads the sound data for sfxname from the WAD lump,
// and returns a ptr to the data in fastmem and its length in len.
//
static void *getsfx (char *sfxname, int *len)
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

    // Debug.
    // fprintf( stderr, "." );
    // fprintf( stderr, " -loading  %s (lump %d, %d bytes)\n",
    //         sfxname, sfxlump, size );
    //fflush( stderr );

    sfx = (unsigned char*)W_CacheLumpNum(sfxlump, PU_STATIC);

    // Allocate from zone memory.
    cnvsfx = (unsigned char*)Z_Malloc(size, PU_STATIC, 0);

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
// Audio Callback - next buffer
/////////////////////////////////////////////////////////////////////////////////////////

void sound_callback(void)
{
    short *fillbuf;

    sound_status = 2;

    if (!should_sound)
        return;

    if (audio_queued)
    {
        fillbuf = audio_get_next_buffer(&lastbuf);
        if (!fillbuf)
            return;

        snd_ticks++;
        fill_buffer(fillbuf);
    }

	audio_queued = audio_send_buffer(lastbuf);
}

/**********************************************************************/
// Init at program start...
void I_InitSound (void)
{
    int i;

    audio_init_ex(SAMPLERATE, 2, 1260, sound_callback);

    sound_status = 1;

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

    // fill in pitch wheel table
    for (i=0; i<128; i++)
    {
        pitch_table[i] = 1.0f + (-3678.0f * (float)(128-i) / 64.0f) / 65536.0f;
    }
    for (i=0; i<128; i++)
    {
        pitch_table[i+128] = 1.0f + (3897.0f * (float)i / 64.0f) / 65536.0f;
    }

    // Finished initialization.
    printf("I_InitSound: Sound module ready.\n");

    numChannels = NUM_VOICES;

    if (!should_sound)
    {
        audio_set_num_samples(NUM_SAMPLES);
        should_sound = 1;
        audio_queued = 1;
        disable_interrupts();
        sound_callback();
        enable_interrupts();
        printf("I_InitSound: AI kickstarted.\n");
    }
}

/**********************************************************************/
// ... update sound buffer and audio device at runtime...
void I_UpdateSound (void)
{
}

/**********************************************************************/
// ... update sound buffer and audio device at runtime...
void I_SubmitSound (void)
{
}

/**********************************************************************/
// ... shut down and relase at program termination.
void I_ShutdownSound (void)
{
    should_sound = 0; // stop audio playback
    audio_close();
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

//    fprintf (stderr, "I_GetSfxLumpNum()\n");

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
//  fprintf (stderr, "I_StartSound(%d,%d,%d,%d,%d,%d)\n", id, cnum, vol, sep, pitch, priority);

//  if (sound_status == 2) {
    I_StopSound(cnum);
    Sfx_Start (S_sfx[id].data, cnum, changepitch ? freqs[pitch] : 11025,
               vol, sep, lengths[id]);
//  }
  return cnum;
}

/**********************************************************************/
// Stops a sound channel.
void I_StopSound(int handle)
{
//  fprintf (stderr, "I_StopSound(%d)\n", handle);

//  if (sound_status == 2)
    Sfx_Stop(handle);
}

/**********************************************************************/
// Called by S_*() functions
//  to see if a channel is still playing.
// Returns 0 if no longer playing, 1 if playing.
int I_SoundIsPlaying(int handle)
{
//  fprintf (stderr, "I_SoundIsPlaying(%d)\n", handle);

//  if (sound_status == 2)
    return Sfx_Done(handle) ? 1 : 0;

//  return 0;
}

/**********************************************************************/
// Updates the volume, separation,
//  and pitch of a sound channel.
void
I_UpdateSoundParams
( int		handle,
  int		vol,
  int		sep,
  int		pitch )
{
//  fprintf (stderr, "I_UpdateSoundParams(%d,%d,%d,%d)\n", handle, vol, sep, pitch);

//  if (sound_status == 2)
    Sfx_Update(handle, changepitch ? freqs[pitch] : 11025, vol, sep);
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
	int i;

	for(i=0;i<256;i++)
	{
		midiVoice[i].wave = NULL;
		midiVoice[i].index = 0.0f;
		midiVoice[i].step = 1.0f;
		midiVoice[i].loop = 0;
		midiVoice[i].length = 2000 << 16;
		midiVoice[i].ltvol = 0.0f;
		midiVoice[i].rtvol = 0.0f;
		midiVoice[i].base = 60;
		midiVoice[i].flags = 0x00;
	}

        if(!midi_pointers)
        {
                music_okay = 0;

                midi_pointers = (void*)Z_Malloc(sizeof(ULONG)*182, PU_STATIC, 0);
                int mphnd = rom_open(MIDI_FILE,MIDI_FILESIZE);

                rom_read(mphnd, midi_pointers, sizeof(ULONG)*182);

                rom_close(mphnd);

		printf("I_InitMusic: Pre-cached all MUS instrument headers.\n");

                music_okay = 1;
        }

	int f;
	for (f = 0; f < 256; f++)
	{
		sample_buffers[f] = NULL;
	}
}

/**********************************************************************/
void I_ShutdownMusic(void)
{
//  fprintf (stderr, "I_ShutdownMusic()\n");
}

/**********************************************************************/
// Volume.
void I_SetMusicVolume(int volume)
{
//  fprintf (stderr, "I_SetMusicVolume(%d)\n", volume);

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
//  fprintf (stderr, "I_PauseSong(%d)\n", handle);

	if (music_okay)
	{
		Mus_Pause(handle);
	}
}

/**********************************************************************/
void I_ResumeSong(int handle)
{
//  fprintf (stderr, "I_ResumeSong(%d)\n", handle);

	if (music_okay)
	{
		Mus_Resume(handle);
	}
}

/**********************************************************************/
// Registers a song handle to song data.
int I_RegisterSong(void *data)
{
//  fprintf (stderr, "I_RegisterSong(%08x)\n", data);

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
( int		handle,
  int		looping )
{
//  fprintf (stderr, "I_PlaySong(%d,%d)\n", handle, looping);

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
//  fprintf (stderr, "I_StopSong(%d)\n", handle);

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
//  fprintf (stderr, "I_UnRegisterSong(%d)\n", handle);

	if (music_okay)
	{
		Mus_Unregister(handle);
	}
}

/**********************************************************************/

void _STDaudio_cleanup (void)
{
	I_ShutdownSound ();
	I_ShutdownMusic ();
}

/**********************************************************************/

void Sfx_Start(char *wave, int cnum, int step, int volume, int seperation, int length)
{
  float vol = (float)volume;
  float sep = (float)seperation;

  vol = (volume > 15) ? 127.0f : vol * 8.0f + 7.0f;

  //Forbid();
  audVoice[cnum].wave = wave + 8;
  audVoice[cnum].index = 0.0f;
  audVoice[cnum].step = (float)step / 11025.0f;//44100.0f;
  audVoice[cnum].loop = 0;
  audVoice[cnum].length = length - 8;
  audVoice[cnum].ltvol = (vol - (vol * sep * sep) / 65536.0f) / 127.0f;
  sep -= 256.0f;
  audVoice[cnum].rtvol = (vol - (vol * sep * sep) / 65536.0f) / 127.0f;
  audVoice[cnum].flags = 0x81;
  //Permit();
}

void Sfx_Update(int cnum, int step, int volume, int seperation)
{
  float vol = (float)volume;
  float sep = (float)seperation;

  vol = (volume > 15) ? 127.0f : vol * 8.0f + 7.0f;

  //Forbid();
  audVoice[cnum].step = (float)step / 11025.0f;//44100.0f;
  audVoice[cnum].ltvol = (vol - (vol * sep * sep) / 65536.0f) / 127.0f;
  sep -= 256.0f;
  audVoice[cnum].rtvol = (vol - (vol * sep * sep) / 65536.0f) / 127.0f;
  //Permit();
}

void Sfx_Stop(int cnum)
{
  //Forbid();
  audVoice[cnum].flags &= 0xFE;
  //Permit();
}

int Sfx_Done(int cnum)
{
  int done;

  //Forbid();
  done = (audVoice[cnum].flags & 0x01) ? (int)audVoice[cnum].index + 1 : 0;
  //Permit();
  return done;
}

/**********************************************************************/

void Mus_SetVol(int vol)
{
    if (0 == vol)
    {
        mus_volume = 0;
    }
    else
    {
        mus_volume = (vol > 15) ? 1.0f : ((float)vol * 8.0f + 7.0f) / 127.0f;
    }
}


int instrument_used(int instrument)
{
    int i;
    for (i=0;i<256;i++)
    {
        if(used_instruments[i] == instrument)
        {
            return 1;
        }
    }

    return 0;
}


int Mus_Register(void *musdata)
{
    int i;
    int uic = 0;
    int hnd;

    ULONG *miptr;

    struct midiHdr *mhdr;


    ULONG *lptr = (ULONG *)musdata;
    UWORD *wptr = (UWORD *)musdata;

    Mus_Unregister(1);
    reset_midiVoices();
    // music won't start playing until mus_playing set at this point

    if (lptr[0] != SWAPLONG(0x1a53554d))  // 0x4d55531a
    {
        return 0;                          // "MUS",26 always starts a vaild MUS file
    }

    score_len = SWAPSHORT(wptr[2]);      // score length
    if (!score_len)
    {
        return 0;                          // illegal score length
    }

    score_start = SWAPSHORT(wptr[3]);    // score start
    if (score_start < 18)
    {
        return 0;                          // illegal score start offset
    }

    inst_cnt = SWAPSHORT(wptr[6]);       // instrument count
    if (!inst_cnt)
    {
        return 0;                          // illegal instrument count
    }

    // okay, MUS data seems to check out

    // if the instrument pointers haven't been set up previously, return here, can't do anything
    if (!midi_pointers)
    {
        return 0;
    }

    music_okay = 0;

    score_data = musdata;
    MUSheader_t *musheader = (MUSheader_t*)score_data;

    used_instrument_count = inst_cnt;
    for (i = 0; i<inst_cnt;i++)
    {
        used_instruments[i] = SWAPSHORT(musheader->instruments[i]);
    }

    miptr = (ULONG *)midi_pointers;

    // iterating over all instruments
    for (i=0;i<182;i++)
    {
        // current instrument is used
        if (instrument_used(i))
        {
            // get the pointer into the instrument data struct
            ULONG ptr = LSWAP(miptr[i]);

            // make sure it doesn't point to NULL
            if (ptr != 0)
            {
                uic += 1;

                // allocate some space for the header out of Doom's heap
                mhdr = (struct midiHdr*)Z_Malloc(sizeof(struct midiHdr), PU_STATIC, 0);
                if (!mhdr)
                {
                    continue;
                }

                // open MIDI Instrument Set file from ROM
                hnd = rom_open(MIDI_FILE,MIDI_FILESIZE);
                if (hnd < 0)
                {
                    continue;
                }

                rom_lseek(hnd,ptr,SEEK_SET);
                rom_read(hnd,(void*)mhdr,sizeof(struct midiHdr));

                ULONG length = LSWAP(mhdr->length) >> 16;

                void *sample = (void *)Z_Malloc(length, PU_STATIC, 0);
                if (!sample)
                {
                    Z_Free(mhdr);
                    rom_close(hnd);
                    continue;
                }
                else
                {
                    sample_buffers[i] = sample;
                    mus_sample_memory_allocated += length;
                }

                rom_lseek(hnd,ptr + 4 + 4 + 4 + 2,SEEK_SET);
                rom_read(hnd, sample, length);

                midiVoice[i].wave   = (BYTE*)sample;
                midiVoice[i].index  = 0.0f;
                midiVoice[i].step   = 1.0f;
                midiVoice[i].loop   = LSWAP(mhdr->loop);
                midiVoice[i].length = LSWAP(mhdr->length);
                midiVoice[i].ltvol  = 0.0f;
                midiVoice[i].rtvol  = 0.0f;
                midiVoice[i].base   = WSWAP(mhdr->base);
                midiVoice[i].flags  = 0x00;

                Z_Free(mhdr);
                rom_close(hnd);
            }
        }
    }

    if (mus_sample_memory_allocated > lifetime_max_mus_sample_alloc)
    {
        lifetime_max_mus_sample_alloc = mus_sample_memory_allocated;
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


float min_sample = FLT_MAX;
float max_sample = FLT_MIN;


float adjust_sample(float in)
{
    float out;

    if (in > 0.0f)
    {
        if (in >= 127.0f)
        {
            in = 127.0f;
        }

        if (in >= 96.0f)
        {
            out = (0.4f/*39f*/ * in) + 60.0f/*58.3*/;
        }

        else
        {
            out = in;
        }
    }
    else
    {
        if(in < -128.0f)
        {
            in = -128.0f;
        }

        if(in < -96.0f)
        {
            out = (/*0.39*/0.4f * in) - /*58.3*/60.0f;
        }
        else
        {
            out = in;
        }
    }

    return out;
}


void fill_buffer(short *buffer)
{
    float index;
    float step;
    float ltvol;
    float rtvol;
    float sample;
    int ix;
    int iy;
    ULONG loop;
    ULONG length;

    BYTE *wvbuff;
    short *smpbuff = buffer;

    // process music if playing
    if (mus_playing)
    {

        if (mus_playing < 0)
        {
            // music now off
            mus_playing = 0;
        }
        else
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
                UBYTE event;
                UBYTE note;
                UBYTE time;
                UBYTE ctrl;
                UBYTE value;
                int next_time;
                int channel;
                int voice;
                int inst;
                float volume;
                float pan;

nextEvent:      // next event
                do
                {
                    event = *score_ptr++;

                    switch((event >> 4) & 7)
                    {
                        // Release
                        case 0:
                        {
                            channel = (int)(event & 15);
                            note = *score_ptr++;
                            note &= 0x7f;
                            voice = mus_channel[channel].map[(ULONG)note] - 1;
                            if (voice >= 0)
                            {
                                // clear mapping
                                mus_channel[channel].map[(ULONG)note] = 0;
                                // voice available
                                voice_avail[voice] = 0;
                                // voice releasing
                                audVoice[voice + SFX_VOICES].flags |= 0x02;
                            }
                            break;
                        }
                        // Play note
                        case 1:
                        {
                            channel = (int)(event & 15);
                            note = *score_ptr++;
                            volume = -1.0f;
                            if (note & 0x80)
                            {
                                // set volume as well
                                note &= 0x7f;
                                volume = (float)*score_ptr++;
                            }
                            for (voice=0; voice<MUS_VOICES; voice++)
                            {
                                if (!voice_avail[voice])
                                {
                                    // found free voice
                                    break;
                                }
                            }
                            if (voice < MUS_VOICES)
                            {
                                // in use
                                voice_avail[voice] = 1;
                                mus_channel[channel].map[(ULONG)note] = voice + 1;
                                if (volume >= 0.0f)
                                {
                                    mus_channel[channel].vol = volume;
                                    pan = mus_channel[channel].pan;
                                    mus_channel[channel].ltvol = (volume - (volume * pan * pan) / 65536.0f) / 127.0f;
                                    pan -= 256.0f;
                                    mus_channel[channel].rtvol = (volume - (volume * pan * pan) / 65536.0f) / 127.0f;
                                }
                                audVoice[voice + SFX_VOICES].ltvol = mus_channel[channel].ltvol;
                                audVoice[voice + SFX_VOICES].rtvol = mus_channel[channel].rtvol;

                                if (channel != 15)
                                {
                                    inst = mus_channel[channel].instrument;
                                    // back link for pitch wheel
                                    audVoice[voice + SFX_VOICES].chan = channel;
                                    audVoice[voice + SFX_VOICES].wave = midiVoice[inst].wave;
                                    audVoice[voice + SFX_VOICES].index = 0.0f;
                                    audVoice[voice + SFX_VOICES].step = (float)note_table[(72 - midiVoice[inst].base + (ULONG)note) & 0x7f] / 65536.0f;
                                    audVoice[voice + SFX_VOICES].loop = midiVoice[inst].loop >> 16;
                                    audVoice[voice + SFX_VOICES].length = midiVoice[inst].length >> 16;
                                    // enable voice
                                    audVoice[voice + SFX_VOICES].flags = 0x01;
                                }
                                else
                                {
                                    // percussion channel - note is percussion instrument
                                    inst = (ULONG)note + 100;
                                    // back link for pitch wheel
                                    audVoice[voice + SFX_VOICES].chan = channel;
                                    audVoice[voice + SFX_VOICES].wave = midiVoice[inst].wave;
                                    audVoice[voice + SFX_VOICES].index = 0.0f;
                                    audVoice[voice + SFX_VOICES].step = 1.0f;
                                    audVoice[voice + SFX_VOICES].loop = midiVoice[inst].loop >> 16;
                                    audVoice[voice + SFX_VOICES].length = midiVoice[inst].length >> 16;
                                    // enable voice
                                    audVoice[voice + SFX_VOICES].flags = 0x01;
                                }
                            }
                            break;
                        }
                        // Pitch
                        case 2:
                        {
                            channel = (int)(event & 15);
                            mus_channel[channel].pitch = pitch_table[(ULONG)*score_ptr++ & 0xff];
                            break;
                        }
                        // Tempo
                        case 3:
                        {
                            // skip value - not supported
                            score_ptr++;
                            break;
                        }
                        // Change control
                        case 4:
                        {
                            channel = (int)(event & 15);
                            ctrl = *score_ptr++;
                            value = *score_ptr++;
                            switch(ctrl)
                            {
                                case 0:
                                {
                                    // set channel instrument
                                    mus_channel[channel].instrument = (ULONG)value;
                                    mus_channel[channel].pitch = 1.0f;
                                    break;
                                }
                                case 3:
                                {
                                    // set channel volume
                                    mus_channel[channel].vol = volume = (float)value;
                                    pan = mus_channel[channel].pan;
                                    mus_channel[channel].ltvol = (volume - (volume * pan * pan) / 65536.0f) / 127.0f;
                                    pan -= 256.0f;
                                    mus_channel[channel].rtvol = (volume - (volume * pan * pan) / 65536.0f) / 127.0f;
                                    break;
                                }
                                case 4:
                                {
                                    // set channel pan
                                    mus_channel[channel].pan = pan = (float)value;
                                    volume = mus_channel[channel].vol;
                                    mus_channel[channel].ltvol = (volume - (volume * pan * pan) / 65536.0f) / 127.0f;
                                    pan -= 256.0f;
                                    mus_channel[channel].rtvol = (volume - (volume * pan * pan) / 65536.0f) / 127.0f;
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
                                score_ptr = (UBYTE *)((ULONG)score_data + (ULONG)score_start);
                            }
                            else
                            {
                                for (voice=0; voice<MUS_VOICES; voice++)
                                {
                                    audVoice[voice + SFX_VOICES].flags = 0;
                                    voice_avail[voice] = 0;
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
                while (!(event & 0x80));

                // now get the time until the next event(s)
                next_time = 0;
                time = *score_ptr++;

                while (time & 0x80)
                {
                    // while msb set, accumulate 7 bits
                    next_time |= (ULONG)(time & 0x7f);
                    next_time <<= 7;
                    time = *score_ptr++;
                }

                next_time |= (ULONG)time;
                mus_delay += next_time;

                if (mus_delay <= 0)
                {
                    goto nextEvent;
                }
            }
        }
    } // endif musplaying

mix:
    // clear buffer
    n64_memset((void *)buffer, 0, (NUM_SAMPLES << 2));

    // mix enabled voices
    for (ix=0; ix<NUM_VOICES; ix++)
    {
        if (audVoice[ix].flags & 0x01)
        {
            // process enabled voice
            if (!(audVoice[ix].flags & 0x80) && !(mus_playing))
            {
                // skip instrument if music not playing
                continue;
            }

            index  = audVoice[ix].index;
            step   = audVoice[ix].step;
            loop   = audVoice[ix].loop;
            length = audVoice[ix].length;
            ltvol  = audVoice[ix].ltvol;
            rtvol  = audVoice[ix].rtvol;
            wvbuff = audVoice[ix].wave;

            if (!(audVoice[ix].flags & 0x80))
            {
                // special handling for instrument
                if (audVoice[ix].flags & 0x02)
                {
                    // releasing
                    ltvol *= 0.90f;
                    rtvol *= 0.90f;
                    audVoice[ix].ltvol = ltvol;
                    audVoice[ix].rtvol = rtvol;

                    if (ltvol <= 0.02f && rtvol <= 0.02f)
                    {
                        // disable voice
                        audVoice[ix].flags = 0;
                        // next voice
                        continue;
                    }
                }
                step *= mus_channel[audVoice[ix].chan & 15].pitch;
                ltvol *= mus_volume;
                rtvol *= mus_volume;
            }

            for (iy=0; iy < (NUM_SAMPLES << 1); iy+=2)
            {
                if ((ULONG)index >= length)
                {
                    if (!(audVoice[ix].flags & 0x80))
                    {
                        // check if instrument has loop
                        if (loop)
                        {
                            index -= (float)length;
                            index += (float)loop;
                        }
                        else
                        {
                            // disable voice
                            audVoice[ix].flags = 0;
                            // exit sample loop
                            break;
                        }
                    }
                    else
                    {
                        // disable voice
                        audVoice[ix].flags = 0;
                        // exit sample loop
                        break;
                    }
                }

                // for safety
                sample = (wvbuff) ? (float)wvbuff[(int)index] : 0.0f;

                sample = adjust_sample(sample);

                if(sample < min_sample) min_sample = sample;
                if(sample > max_sample) max_sample = sample;

                smpbuff[iy    ] += (short)(sample * ltvol * master_vol);
                smpbuff[iy + 1] += (short)(sample * rtvol * master_vol);

                index += step;
            }

            audVoice[ix].index = index;
        }
    }
}

/**********************************************************************/
