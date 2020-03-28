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

#include <float.h>

#include "z_zone.h"

#include "i_system.h"
#include "i_sound.h"
#include "m_argv.h"
#include "m_misc.h"
#include "w_wad.h"
#include "m_swap.h"

#include "doomdef.h"

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
    BYTE *wave;
    ULONG index;
    ULONG step;
    ULONG loop;
    ULONG length;
    int32_t ltvol;
    int32_t rtvol;
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

short __attribute__((aligned(8))) pcmout1[316 * 2]; // 1260 stereo samples
short __attribute__((aligned(8))) pcmout2[316 * 2];
static int pcmflip = 0;
static short *pcmout[2];// = &pcmout1[0];

static short *pcmbuf;// = &pcmout1[0];

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

static unsigned int __attribute__((aligned(8))) note_table[128] =
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

//static int __attribute__((aligned(8))) pitch_table[256];
static unsigned int __attribute__((aligned(8))) pitch_table[256] = {
58180, 58237, 58294, 58352, 58409, 58467, 58524, 58582, 58639, 
58697, 58754, 58812, 58869, 58927, 58984, 59042, 59099, 59156,
59214, 59271, 59329, 59386, 59444, 59501, 59559, 59616, 59674,
59731, 59789, 59846, 59904, 59961, 60019, 60076, 60133, 60191,
60248, 60306, 60363, 60421, 60478, 60536, 60593, 60651, 60708,
60766, 60823, 60881, 60938, 60995, 61053, 61110, 61168, 61225,
61283, 61340, 61398, 61455, 61513, 61570, 61628, 61685, 61743,
61800, 61858, 61915, 61972, 62030, 62087, 62145, 62202, 62260,
62317, 62375, 62432, 62490, 62547, 62605, 62662, 62720, 62777,
62834, 62892, 62949, 63007, 63064, 63122, 63179, 63237, 63294,
63352, 63409, 63467, 63524, 63582, 63639, 63697, 63754, 63811,
63869, 63926, 63984, 64041, 64099, 64156, 64214, 64271, 64329,
64386, 64444, 64501, 64559, 64616, 64673, 64731, 64788, 64846,
64903, 64961, 65018, 65076, 65133, 65191, 65248, 65306, 65363,
65421, 65478, 65536, 65596, 65657, 65718, 65779, 65840, 65901,
65962, 66023, 66084, 66144, 66205, 66266, 66327, 66388, 66449,
66510, 66571, 66632, 66692, 66753, 66814, 66875, 66936, 66997,
67058, 67119, 67180, 67240, 67301, 67362, 67423, 67484, 67545,
67606, 67667, 67728, 67788, 67849, 67910, 67971, 68032, 68093,
68154, 68215, 68276, 68336, 68397, 68458, 68519, 68580, 68641,
68702, 68763, 68824, 68884, 68945, 69006, 69067, 69128, 69189,
69250, 69311, 69372, 69433, 69493, 69554, 69615, 69676, 69737,
69798, 69859, 69920, 69981, 70041, 70102, 70163, 70224, 70285,
70346, 70407, 70468, 70529, 70589, 70650, 70711, 70772, 70833,
70894, 70955, 71016, 71077, 71137, 71198, 71259, 71320, 71381,
71442, 71503, 71564, 71625, 71685, 71746, 71807, 71868, 71929,
71990, 72051, 72112, 72173, 72233, 72294, 72355, 72416, 72477,
72538, 72599, 72660, 72721, 72781, 72842, 72903, 72964, 73025, 
73086, 73147, 73208, 73269
};

static int __attribute__((aligned(8))) pan_table[256];

static int32_t master_vol =  0x19000; // 0000.0000

static int musicdies  = -1;
static int music_okay =  0;

//void *midi_pointers = NULL;

uint32_t midi_pointers[182];

static UWORD score_len, score_start, inst_cnt;
static void *score_data;
static UBYTE *score_ptr;

static int mus_delay    = 0;
static int mus_looping  = 0;
static int32_t mus_volume = 127;
int mus_playing  = 0;

extern struct AI_regs_s *AI_regs;


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

int __attribute__((aligned(8))) used_instruments[256];
int used_instrument_count = -1;

int32_t  __attribute__((aligned(8))) vol_table[128];


void reset_midiVoices(void)
{
    int i;

    used_instrument_count = -1;

    for (i=0;i<256;i++)
    {
        if (NULL != (void*)midiVoice[i].wave)
        {
            n64_free(midiVoice[i].wave);
        }
    }
	
	__n64_memset_ZERO_ASM((void *)(&audVoice[SFX_VOICES]), 0x00, sizeof(Voice_t)*MUS_VOICES);
	__n64_memset_ZERO_ASM(midiVoice, 0, sizeof(Voice_t)*256);
	__n64_memset_ZERO_ASM(voice_avail, 0, 64);
	__n64_memset_ASM(used_instruments, 0xff, 1024);

	for (i=0;i<256;i++) {
        midiVoice[i].step = 1 << 16;
        midiVoice[i].length = 2000 << 16;
        midiVoice[i].base = 60;
    }
/*
	for(i=SFX_VOICES;i<NUM_VOICES;i++) {
		if(audVoice[i].wave > 0) {
			n64_free(audVoice[i].wave);
		}
	}
*/	
	
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
// On the PC, need swap routines because the MIDI file was made on the Amiga and
// is in big endian format. :)
// The Nintendo 64 is also big endian so these are no-ops here. :D

#define WSWAP(x) x
#define LSWAP(x) x

#if 0
void n64_fillBuffer()
{
	int fillbuf;
    fillbuf = pcmflip ? 1 : 0;
    fill_buffer(pcmout[fillbuf]);
}

/////////////////////////////////////////////////////////////////////////////////////////
//Audio output
/////////////////////////////////////////////////////////////////////////////////////////

int audioOutput()
{
    int playbuf;

    playbuf = pcmflip ? 0 : 1;
    pcmflip ^= 1;

    if (audio_can_write())
    {
        audio_write(pcmout[playbuf]);
    }

    return 0;
}
#endif 

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

	for (i=0;i<255;i++)
		pan_table[i] = i*i;
	//for (i=128;i<255;i++)
		//pan_table[i] = i*i;
	
	for (i=0;i<128;i++) {
		if(i < 15)
		vol_table[i] = ((i * 8) + 7);
		else
			vol_table[i] = (127);
	}
	
	vol_table[0] = 0;
	
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

	// precomputed now
#if 0
    // fill in pitch wheel table
    for (i=0; i<128; i++)
    {
        pitch_table[i] = 1.0f + (-3678.0f * (float)(128-i) / 64.0f) / 65536.0f;
    }
    for (i=0; i<128; i++)
    {
        pitch_table[i+128] =  1.0f + (3897.0f * (float)i / 64.0f)  / 65536.0f;
    }
#endif

    // Finished initialization.
    printf("I_InitSound: Sound module ready.\n");
	
    numChannels = NUM_VOICES;
}

/**********************************************************************/
// ... update sound buffer and audio device at runtime...
#if 0
void I_UpdateSound (void)
{
//    n64_fillBuffer();
	int fillbuf;
    fillbuf = pcmflip ? 1 : 0;
    fill_buffer(pcmout[fillbuf]);
}
#endif 
/*static*/ extern volatile int buf_full;
/*static*/ extern int _num_buf;
/*static*/ extern volatile int now_playing;
/*static*/ extern volatile int now_writing;

//extern int __full();
#define AI_STATUS_FULL  ( 1 << 31 )
/**********************************************************************/
// ... update sound buffer and audio device at runtime...
void I_SubmitSound (void)
{
#if 0
    if (audio_can_write())
    {
        audio_write(pcmbuf);
    }
    pcmflip ^= 1;
	pcmbuf = pcmout[pcmflip];
#endif
#if 1
	if(!(AI_regs->status & AI_STATUS_FULL)) {
		//disable_interrupts();
		AI_regs->address = (uint32_t *)((uint32_t)pcmbuf | (uint32_t)0xA0000000);
		AI_regs->length = 316*2*2;
		AI_regs->control = 1;
		//enable_interrupts();
		pcmflip ^= 1;
		pcmbuf = pcmout[pcmflip];//pcmflip ? pcmout2 : pcmout1;
	}
#endif
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
    Sfx_Start (S_sfx[id].data, cnum, changepitch ? freqs[pitch] : 11025,
               vol, sep, lengths[id]);
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
( int		handle,
  int		vol,
  int		sep,
  int		pitch )
{
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
( int		handle,
  int		looping )
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
if(!volume) return;
	int32_t vol = vol_table[volume]; // it wasn't loud enough
	int32_t sep1 = /*pan_table[seperation];//*/(seperation*seperation);
	int32_t sep2 = /*pan_table[255-seperation];//*/((seperation-256)*(seperation-256));

  audVoice[cnum].wave = wave + 8;
  audVoice[cnum].index = 0;
  audVoice[cnum].step = (ULONG)((step<<16) / SAMPLERATE);
  audVoice[cnum].loop = 0 << 16;
  audVoice[cnum].length = (length - 8) << 16;
  audVoice[cnum].ltvol = (vol - (vol*sep1));
  audVoice[cnum].rtvol = (vol - (vol*sep2));
  audVoice[cnum].flags = 0x81;
}

void Sfx_Update(int cnum, int step, int volume, int seperation)
{
if(!volume) return;
int32_t vol = vol_table[volume]; // it wasn't loud enough
int32_t sep1 = /*pan_table[seperation];//*/(seperation*seperation);
	int32_t sep2 = /*pan_table[255-seperation];//*/((seperation-256)*(seperation-256));

  audVoice[cnum].step = (ULONG)((step<<16) / SAMPLERATE);
audVoice[cnum].ltvol = (vol - (vol*sep1));
audVoice[cnum].rtvol = (vol - (vol*sep2));
}

void Sfx_Stop(int cnum)
{
  audVoice[cnum].flags &= 0xFE;
}

int Sfx_Done(int cnum)
{
  int done;
  done = (audVoice[cnum].flags & 0x01) ? (audVoice[cnum].index >> 16) + 1 : 0;//(int)audVoice[cnum].index + 1 : 0;
  return done;
}

/**********************************************************************/

void Mus_SetVol(int vol)
{
    if (0 == vol)
    {
		int ix;
        mus_volume = 0;
		mus_playing = -1;
			for (ix=SFX_VOICES; ix<NUM_VOICES; ix++)
			{
				audVoice[ix].flags = 0x00;        // disable voice
			}
		}
    else
    {
		if(mus_volume == 0) {
			mus_playing = 1;
		}
        mus_volume = vol_table[vol];
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
    int hnd;

    //ULONG *miptr;

    struct midiHdr *mhdr;


    ULONG *lptr = (ULONG *)musdata;
    UWORD *wptr = (UWORD *)musdata;

    Mus_Unregister(1);
    reset_midiVoices();
    // music won't start playing until mus_playing set at this point

    if (lptr[0] != LONG(0x1a53554d))  // 0x4d55531a
    {
        return 0;                          // "MUS",26 always starts a vaild MUS file
    }

    score_len = SHORT(wptr[2]);      // score length
    if (!score_len)
    {
        return 0;                          // illegal score length
    }

    score_start = SHORT(wptr[3]);    // score start
    if (score_start < 18)
    {
        return 0;                          // illegal score start offset
    }

    inst_cnt = SHORT(wptr[6]);       // instrument count
    if (!inst_cnt)
    {
        return 0;                          // illegal instrument count
    }

    // okay, MUS data seems to check out

    // if the instrument pointers haven't been set up previously, return here, can't do anything
    //if (!midi_pointers)
    //{
    //    return 0;
    //}

    music_okay = 0;

    score_data = musdata;
    MUSheader_t *musheader = (MUSheader_t*)score_data;

    used_instrument_count = inst_cnt;
    for (i = 0; i<inst_cnt;i++)
    {
        used_instruments[i] = SHORT(musheader->instruments[i]);
    }

    //miptr = (ULONG *)midi_pointers;

    // iterating over all instruments
    for (i=0;i<182;i++)
    {
        // current instrument is used
        if (instrument_used(i))
        {
            // get the pointer into the instrument data struct
            ULONG ptr = LSWAP(midi_pointers[i]);

            // make sure it doesn't point to NULL
            if (ptr != 0)
            {
                // allocate some space for the header
                mhdr = (struct midiHdr*)n64_malloc(sizeof(struct midiHdr));
                if (!mhdr)
                {
					I_Error("no hdr malloc");
                }

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

				if(sample > 0)
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
				else {
					I_Error("no sample malloc");
				}
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
    //short *_fb_smpbuff = pcmbuf;
extern int snd_SfxVolume;
//void fill_buffer(short *buffer)
//int first = 1;

typedef struct mixer_state {
	/*static*/ uint32_t first_voice_to_mix;
/*static*/ uint32_t last_voice_to_mix;
/*static*/    ULONG _fb_index;
/*static*/    ULONG _fb_step;
/*static*/    uint32_t _fb_ltvol;
/*static*/  uint32_t _fb_rtvol;
/*static*/ int _fb_sample;
/*static*/  int _fb_ix;
/*static*/   int _fb_iy;
/*static*/   ULONG _fb_loop;
/*static*/   ULONG _fb_length;
/*static*/	uint32_t _fb_pval, _fb_pval2;
/*static*/    BYTE *_fb_wvbuff;
} mixer_state_t;

mixer_state_t doom_mixer;

//int16_t minL=32767,maxL=-32768,minR=32767,maxR=-32768;
void I_UpdateSound (void)
{   
size_t first;

///*static*/ uint32_t first_voice_to_mix;
///*static*/ uint32_t last_voice_to_mix;
///*static*/    ULONG _fb_index;
///*static*/    ULONG _fb_step;
///*static*/    uint32_t _fb_ltvol;
///*static*/  uint32_t _fb_rtvol;
///*static*/ int _fb_sample;
///*static*/  int _fb_ix;
///*static*/   int _fb_iy;
///*static*/   ULONG _fb_loop;
///*static*/   ULONG _fb_length;
///*static*/	uint32_t _fb_pval, _fb_pval2;
///*static*/    BYTE *_fb_wvbuff;

    // clear buffer
    __n64_memset_ZERO_ASM((void *)pcmbuf, 0, (NUM_SAMPLES << 2));

	for(first=0;first<158;first++) {
		*((uint64_t *)&pcmbuf[first*4]) = 0;
		//*(((uint64_t *)(pcmbuf + (first<<3))))=0;
	}
	
	if (mus_playing < 0)
    {
            // music now off
            mus_playing = 0;
			goto mix;
    }

    if (mus_playing)
    {
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
                int volume;
                int pan;

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
                            volume = -1;
                            if (note & 0x80)
                            {
                                // set volume as well
                                note &= 0x7f;
                                volume = (int32_t)(*score_ptr++);
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
                                if (volume >= 0)
                                {
                                    mus_channel[channel].vol = volume;
                                    pan = mus_channel[channel].pan;
									doom_mixer._fb_pval = (pan*pan);
									doom_mixer._fb_pval2 = ((pan-256)*(pan-256));
                                    mus_channel[channel].ltvol = (volume - (volume * doom_mixer._fb_pval));
                                    mus_channel[channel].rtvol = (volume - (volume * doom_mixer._fb_pval2));

								}
                                audVoice[voice + SFX_VOICES].ltvol = mus_channel[channel].ltvol;
                                audVoice[voice + SFX_VOICES].rtvol = mus_channel[channel].rtvol;

                                if (channel != 15)
                                {
                                    inst = mus_channel[channel].instrument;
                                    // back link for pitch wheel
                                    audVoice[voice + SFX_VOICES].chan = channel;
                                    audVoice[voice + SFX_VOICES].wave = midiVoice[inst].wave;
                                    audVoice[voice + SFX_VOICES].index = 0;
                                    audVoice[voice + SFX_VOICES].step = note_table[(72 - midiVoice[inst].base + (ULONG)note) & 0x7f];
                                    audVoice[voice + SFX_VOICES].loop = midiVoice[inst].loop;
                                    audVoice[voice + SFX_VOICES].length = midiVoice[inst].length;
                                    // enable voice
                                    audVoice[voice + SFX_VOICES].flags = 0x01;
                                }
                                else
                                {
//									mus_channel[channel].vol >>= 1;
                                    // percussion channel - note is percussion instrument
                                    inst = (ULONG)note + 100;
                                    // back link for pitch wheel
                                    audVoice[voice + SFX_VOICES].chan = channel;
                                    audVoice[voice + SFX_VOICES].wave = midiVoice[inst].wave;
                                    audVoice[voice + SFX_VOICES].index = 0;
                                    audVoice[voice + SFX_VOICES].step = 0x10000;
                                    audVoice[voice + SFX_VOICES].loop = midiVoice[inst].loop;
                                    audVoice[voice + SFX_VOICES].length = midiVoice[inst].length;
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
                                    mus_channel[channel].pitch = 0x10000;
                                    break;
                                }
                                case 3:
                                {
                                    // set channel volume
                                    mus_channel[channel].vol = volume = value;
                                    pan = mus_channel[channel].pan;
									doom_mixer._fb_pval = (pan*pan);
									doom_mixer._fb_pval2 = ((pan-256)*(pan-256));
                                    mus_channel[channel].ltvol = (volume - (volume * doom_mixer._fb_pval));
                                    mus_channel[channel].rtvol = (volume - (volume * doom_mixer._fb_pval2));
                                    break;
                                }
                                case 4:
                                {
                                    // set channel pan
                                    mus_channel[channel].pan = pan = value;
									doom_mixer._fb_pval = (pan*pan);
									doom_mixer._fb_pval2 = ((pan-256)*(pan-256));
                                    volume = mus_channel[channel].vol;
                                    mus_channel[channel].ltvol = (volume - (volume * doom_mixer._fb_pval));
                                    mus_channel[channel].rtvol = (volume - (volume * doom_mixer._fb_pval2));
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
	if(!snd_SfxVolume && !mus_playing) {
	return;
	}

	doom_mixer.first_voice_to_mix = snd_SfxVolume ? 0 : SFX_VOICES;
 	doom_mixer.last_voice_to_mix = NUM_VOICES >> (1 - mus_playing);
    // mix enabled voices
    for (doom_mixer._fb_ix=doom_mixer.first_voice_to_mix; doom_mixer._fb_ix<doom_mixer.last_voice_to_mix; doom_mixer._fb_ix++)
    {
        if (audVoice[doom_mixer._fb_ix].flags & 0x01)
        {
            doom_mixer._fb_index  = audVoice[doom_mixer._fb_ix].index;
            doom_mixer._fb_step   = audVoice[doom_mixer._fb_ix].step;
            doom_mixer._fb_loop   = audVoice[doom_mixer._fb_ix].loop;
            doom_mixer._fb_length = audVoice[doom_mixer._fb_ix].length;
            doom_mixer._fb_ltvol  = audVoice[doom_mixer._fb_ix].ltvol;
            doom_mixer._fb_rtvol  = audVoice[doom_mixer._fb_ix].rtvol;
            doom_mixer._fb_wvbuff = audVoice[doom_mixer._fb_ix].wave;

            if (!(audVoice[doom_mixer._fb_ix].flags & 0x80))
            {
                // special handling for instrument
                if (audVoice[doom_mixer._fb_ix].flags & 0x02)
                {
                    // releasing
                    doom_mixer._fb_ltvol >>= 1; // = ((_fb_ltvol << 3)-_fb_ltvol)>>3;//>> 1;//(_fb_ltvol * 7) >> 3;
					doom_mixer._fb_rtvol >>= 1; // = ((_fb_rtvol << 3)-_fb_rtvol)>>3;//_fb_rtvol //>> 1;//(_fb_rtvol * 7) >> 3;
					audVoice[doom_mixer._fb_ix].ltvol = doom_mixer._fb_ltvol;
                    audVoice[doom_mixer._fb_ix].rtvol = doom_mixer._fb_rtvol;

                    if (doom_mixer._fb_ltvol <= 0x800 && doom_mixer._fb_rtvol <= 0x800)
                    {
                        // disable voice
                        audVoice[doom_mixer._fb_ix].flags = 0;
                        // next voice
                        continue;
                    }
                }

                doom_mixer._fb_step = (((int64_t)doom_mixer._fb_step * (int64_t)(mus_channel[audVoice[doom_mixer._fb_ix].chan & 15].pitch))>>16);
				
                doom_mixer._fb_ltvol = (((int64_t)doom_mixer._fb_ltvol * (int64_t)(mus_volume))>>16)>>3;
                doom_mixer._fb_rtvol = (((int64_t)doom_mixer._fb_rtvol * (int64_t)(mus_volume))>>16)>>3;
            }

            for (doom_mixer._fb_iy=0; doom_mixer._fb_iy < (NUM_SAMPLES << 1); doom_mixer._fb_iy+=2)
            {
                if ((ULONG)(doom_mixer._fb_index) >= (ULONG)(doom_mixer._fb_length))
                {
                    if (!(audVoice[doom_mixer._fb_ix].flags & 0x80))
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
                    else
                    {
                        // disable voice
                        audVoice[doom_mixer._fb_ix].flags = 0;
                        // exit sample loop
                        break;
                    }
                }

	            doom_mixer._fb_sample = ((((int64_t)doom_mixer._fb_wvbuff[doom_mixer._fb_index>>16]) * (int64_t)master_vol) >> 16);
				int16_t ssmp1 = (int16_t)(((int64_t)doom_mixer._fb_sample * (int64_t)doom_mixer._fb_ltvol)>>16);
				int16_t ssmp2 = (int16_t)(((int64_t)doom_mixer._fb_sample * (int64_t)doom_mixer._fb_rtvol)>>16);
				uint32_t lsmp = (ssmp1 << 16) | (ssmp2);
				*((uint32_t *)(&pcmbuf[doom_mixer._fb_iy])) += lsmp;

	//			if(ssmp1 < minL) minL = ssmp1;
	//			if(ssmp1 > maxL) maxL = ssmp1;
	//			if(ssmp2 < minR) minR = ssmp2;
	//			if(ssmp2 > maxR) maxR = ssmp2;
				
                //pcmbuf[_fb_iy    ] += ssmp;//(short)(((int64_t)_fb_sample * (int64_t)_fb_ltvol)>>16);
				//pcmbuf[_fb_iy  + 1  ] += ssmp;//(short)(((int64_t)_fb_sample * (int64_t)_fb_ltvol)>>16);
                //pcmbuf[_fb_iy + 1] += (short)(((int64_t)_fb_sample * (int64_t)_fb_rtvol)>>16);

                doom_mixer._fb_index += doom_mixer._fb_step;
            }

            audVoice[doom_mixer._fb_ix].index = doom_mixer._fb_index;
        }
    }
}

/**********************************************************************/		