#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <malloc.h>
#include <unistd.h>
#include <assert.h>

char *filename;
char *musfile;
char *outputfilename;

#define MIDI_V10

typedef unsigned int ULONG;
typedef unsigned short UWORD;
typedef unsigned char UBYTE;
typedef char BYTE;


static ULONG NUM_SAMPLES = 315;       // 1260 = 35Hz, 630 = 70Hz, 315 = 140Hz
static ULONG BEATS_PER_PASS = 4;       // 4 = 35Hz, 2 = 70Hz, 1 = 140Hz
void fill_buffer(short *buffer, int d);


UWORD WSWAP(UWORD x)
{
    return (UWORD)((x>>8) | (x<<8));
}

ULONG LSWAP(ULONG x)
{
    return
        (x>>24)
        | ((x>>8) & 0xff00)
        | ((x<<8) & 0xff0000)
        | (x<<24);
}

// NUM_VOICES = SFX_VOICES + MUS_VOICES
#define NUM_VOICES 64
#define SFX_VOICES 32
#define MUS_VOICES 32

struct Channel {
        float pitch;
        float pan;
        float vol;
        float ltvol;
        float rtvol;
        int instrument;
        int map[128];
};

#ifdef MIDI_V10
struct midiHdr {
        ULONG rate;
        ULONG loop;                          // 16.16 fixed pt
        ULONG length;                        // 16.16 fixed pt
        UWORD base;                          // note base of sample
        BYTE sample[8];                      // actual length varies
};
#else
struct midiHdr {
        UWORD rate;
        UWORD loop;
        UWORD length;
        UWORD base;                          // note base of sample
        BYTE sample[8];                      // actual length varies
};
#endif

struct Voice {
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
};


int voice_avail[MUS_VOICES];
struct Channel mus_channel[16];
struct Voice audVoice[NUM_VOICES];
struct Voice midiVoice[256];

void *midi_instruments = NULL;

typedef struct MUSheader {
                char    ID[4];          // identifier "MUS" 0x1A
                UWORD    scoreLen;
                UWORD    scoreStart;
                UWORD    channels;	// count of primary channels
                UWORD    sec_channels;	// count of secondary channels
                UWORD    instrCnt;
                UWORD    dummy;
        // variable-length part starts here
                UWORD    instruments[];
} MUSheader_t;



static int sound_status = 0;

static int bufferEmpty;
static int bufferFull;

static short pcmout1[315 * 2]; // 1260 stereo samples
static short pcmout2[315 * 2];
static int pcmflip = 0;
static int pcmchannel = 0;

volatile int snd_ticks; // advanced by sound thread

#define SAMPLERATE 11025


static int changepitch;
/* sampling freq (Hz) for each pitch step when changepitch is TRUE
   calculated from (2^((i-128)/64))*11025
   I'm not sure if this is the right formula */
static UWORD freqs[256] = {
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

static int note_table[128] = {
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

static float pitch_table[256];

static float master_vol =  64.0f;

static int musicdies=-1;
static int music_okay = 0;


static UWORD score_len, score_start, inst_cnt;
static void *score_data;
static UBYTE *score_ptr;

static int mus_delay = 0;
static int mus_playing = 0;
static int mus_looping = 0;
static float mus_volume = 1.0f;


ULONG *midi_pointers;


int used_instruments[256];

int used_instrument_count = -1;

int instrument_used(int instrument)
{
	int i;
	for(i=0;i<256;i++)
	{
		if(used_instruments[i] == instrument)
			return 1;
	}

	return 0;
}


void reset_midiVoices()
{
  int i;

  for(i=0;i<256;i++)
  {
          if(midiVoice[i].wave != 0)
          {
            free(midiVoice[i].wave);
          }
          midiVoice[i].wave = 0;
          midiVoice[i].index = 0.0f;
          midiVoice[i].step = 1.0f;
          midiVoice[i].loop = 0;
          midiVoice[i].length = 2000 << 16;
          midiVoice[i].ltvol = 0.0f;
          midiVoice[i].rtvol = 0.0f;
          midiVoice[i].base = 60;
          midiVoice[i].flags = 0x00;
  }

  for(i=0;i<256;i++) used_instruments[i] = -1;
  used_instrument_count = -1;
}


short *thelastbuf;

int audioOutput(int d)
{
        short *playbuf;

        snd_ticks += 1;
        playbuf = pcmflip ? pcmout1 : pcmout2;
        pcmflip ^= 1;

	if(d)
	printf("audioOutput: pcmflip = %d\n", pcmflip);

/*        if (audio_can_write())
        {
                audio_write(playbuf);
        }*/

        return 0;
}


static int fill_buffer_count = 0;


void psp_fillBuffer(int d)
{
  int numSamples;
  int fillbuf;

if(d)
  printf("psp_fillBuffer: fill_buffer_count = %d\n", fill_buffer_count);
  fill_buffer_count += 1;

if(!midi_pointers)
{
	if(d)
	printf("psp_fillBuffer: building midiPointers\n");
        music_okay = 0;

        midi_pointers = (ULONG *)malloc(sizeof(ULONG)*182);
        int mphnd = open(filename,0);

        read(mphnd, midi_pointers, sizeof(ULONG)*182);

        close(mphnd);
/*int i;
  for(i=0;i<256;i++)
  {
          if(midiVoice[i].wave != 0)
          {
            free(midiVoice[i].wave);
          }
          midiVoice[i].wave = 0;
          midiVoice[i].index = 0.0f;
          midiVoice[i].step = 1.0f;//11025.0f / 44100.0f;
          midiVoice[i].loop = 0;
          midiVoice[i].length = 2000 << 16;
          midiVoice[i].ltvol = 0.0f;
          midiVoice[i].rtvol = 0.0f;
          midiVoice[i].base = 60;
          midiVoice[i].flags = 0x00;
  }

  for(i=0;i<256;i++) used_instruments[i] = -1;
  used_instrument_count = -1;*/
	reset_midiVoices();

        music_okay = 1;
}



        sound_status = 2;

        numSamples = 0;

        fillbuf = pcmflip ? (int)pcmout2 : (int)pcmout1;

	if(d)	
	printf("psp_fillBuffer: fillbuf = %d\n", fillbuf);

        while (numSamples < 315)
        {
                fill_buffer((short *)(fillbuf + (numSamples<<2)), d);
                numSamples += NUM_SAMPLES;
        }
}

void Mus_Stop(int handle);

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
    return;

  mus_looping = looping;
  mus_playing = 2;                     // 2 = play from start
}

void Mus_Stop(int handle)
{
  int ix;

  if(mus_playing) {
    mus_playing = -1;                  // stop playing
  }

  // disable instruments in score (just disable them all)
  for (ix=SFX_VOICES; ix<NUM_VOICES; ix++)
    audVoice[ix].flags = 0x00;        // disable voice

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


#define SWAPLONG(x) (x)
#define SWAPSHORT(x) (x)


int Mus_Register(void *musdata)
{
  ULONG *lptr = (ULONG *)musdata;
  UWORD *wptr = (UWORD *)musdata;

  Mus_Unregister(1);
  reset_midiVoices();
  // music won't start playing until mus_playing set at this point
  int i;

  if(lptr[0] != SWAPLONG(0x1a53554d))  // 0x4d55531a
    return 0;                          // "MUS",26 always starts a vaild MUS file
  score_len = SWAPSHORT(wptr[2]);      // score length
  if(!score_len)
    return 0;                          // illegal score length
  score_start = SWAPSHORT(wptr[3]);    // score start
  if(score_start < 18)
    return 0;                          // illegal score start offset

  inst_cnt = SWAPSHORT(wptr[6]);       // instrument count
  if(!inst_cnt)
    return 0;                          // illegal instrument count

  // okay, MUS data seems to check out

  score_data = musdata;

  MUSheader_t *musheader = (MUSheader_t*)score_data;

  used_instrument_count = inst_cnt;
  for(i = 0; i<inst_cnt;i++)
  {
     used_instruments[i] = SWAPSHORT(musheader->instruments[i]);
  }

  int hnd;
  ULONG *miptr;
  struct midiHdr *mhdr;

  if(!midi_pointers)
  {
    return 0;
  }

  miptr = (ULONG *)midi_pointers;

        int uic = 0;

        for(i=0;i<182;i++)
        {
                if(instrument_used(i))
                {
                  ULONG ptr = LSWAP(miptr[i]);
                  if(ptr != 0)
                  {
                    uic += 1;
                    mhdr = (struct midiHdr *)malloc(sizeof(struct midiHdr));
                    if(!mhdr) return 0;
                    hnd = open(filename,0);
                    lseek(hnd,ptr,SEEK_SET);
                    read(hnd,(void*)mhdr,sizeof(struct midiHdr));
                    ULONG length = LSWAP(mhdr->length) >> 16;

                    void *sample = (void*)malloc(length);
                    if(!sample)
		    {
			free(mhdr);
			close(hnd);
			return 0;
		    }
                    lseek(hnd,ptr + 4 + 4 + 4 + 2,SEEK_SET);
                    read(hnd, sample, length);

                    midiVoice[i].wave = sample;
                    if(midiVoice[i].wave != 0)
                    {
		        if(uic == used_instrument_count)
		        {
				;//some kind of status output
		        }
                    }
                    midiVoice[i].index = 0.0f;
                    midiVoice[i].step = 11025.0f / 11025.0f;
#ifdef MIDI_V10
                    midiVoice[i].loop = LSWAP(mhdr->loop);
                    midiVoice[i].length = LSWAP(mhdr->length);
#else
                    midiVoice[i].loop = WSWAP(mhdr->loop)<<16;
                    midiVoice[i].length = WSWAP(mhdr->length)<<16;
#endif
                    midiVoice[i].ltvol = 0.0f;
                    midiVoice[i].rtvol = 0.0f;
                    midiVoice[i].base = WSWAP(mhdr->base);
                    midiVoice[i].flags = 0x00;

		    printf("Instrument %d:\n", i);
		    printf("\tindex = %f\n", midiVoice[i].index);
		    printf("\tstep = %f\n", midiVoice[i].step);
		    printf("\tloop = %f\n", midiVoice[i].loop / 65536.0f);
		    printf("\tlength = %f\n", midiVoice[i].length / 65536.0f);
		    printf("\tltvol = %f\n", midiVoice[i].ltvol);
		    printf("\trtvol = %f\n", midiVoice[i].rtvol);
		    printf("\tbase = %f\n", midiVoice[i].base);
		    printf("\tflags = %f\n", midiVoice[i].flags);

                    close(hnd);
                    free(mhdr);
                  }
              }
        }

        music_okay = 1;

  return 1;
}



void fill_buffer(short *buffer, int d)
{
  float index;
  float step;
  ULONG loop;
  ULONG length;
  float ltvol;
  float rtvol;
  BYTE *wvbuff;
  short *smpbuff = buffer;
  float sample;
  int ix;
  int iy;

  // process music if playing
  if (mus_playing) {

    if (mus_playing < 0)
    {
      mus_playing = 0;                 // music now off
    }
    else
    {
      if (mus_playing > 1) {
        mus_playing = 1;
        // start music from beginning
        score_ptr = (UBYTE *)((ULONG)score_data + (ULONG)score_start);
      }

      mus_delay -= BEATS_PER_PASS;     // 1=140Hz, 2=70Hz, 4=35Hz
      if (mus_delay <= 0) {
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

nextEvent:        // next event
        do {
          event = *score_ptr++;
          switch((event >> 4) & 7) {
            case 0:                    // Release
              channel = (int)(event & 15);
              note = *score_ptr++;
              note &= 0x7f;
              voice = mus_channel[channel].map[(ULONG)note] - 1;
              if (voice >= 0) {
                mus_channel[channel].map[(ULONG)note] = 0;       // clear mapping
                voice_avail[voice] = 0;                        // voice available
                audVoice[voice + SFX_VOICES].flags |= 0x02;    // voice releasing
              }
              break;
            case 1:                    // Play note
              channel = (int)(event & 15);
              note = *score_ptr++;
              volume = -1.0f;
              if (note & 0x80) {       // set volume as well
                note &= 0x7f;
                volume = (float)*score_ptr++;
              }
              for (voice=0; voice<MUS_VOICES; voice++) {
                if (!voice_avail[voice])
                  break;               // found free voice
              }
              if (voice < MUS_VOICES) {
                voice_avail[voice] = 1;        // in use
                mus_channel[channel].map[(ULONG)note] = voice + 1;
                if (volume >= 0.0f) {
                  mus_channel[channel].vol = volume;
                  pan = mus_channel[channel].pan;
                  mus_channel[channel].ltvol = (volume - (volume * pan * pan) / 65536.0f) / 127.0f;
                  pan -= 256.0f;
                  mus_channel[channel].rtvol = (volume - (volume * pan * pan) / 65536.0f) / 127.0f;
                }
                audVoice[voice + SFX_VOICES].ltvol = mus_channel[channel].ltvol;
                audVoice[voice + SFX_VOICES].rtvol = mus_channel[channel].rtvol;
                if (channel != 15) {
                  inst = mus_channel[channel].instrument;
                  audVoice[voice + SFX_VOICES].chan = channel; // back link for pitch wheel
                  audVoice[voice + SFX_VOICES].wave = midiVoice[inst].wave;
                  audVoice[voice + SFX_VOICES].index = 0.0f;
                  audVoice[voice + SFX_VOICES].step = (float)note_table[(72 - midiVoice[inst].base + (ULONG)note) & 0x7f] / 65536.0f;
                  audVoice[voice + SFX_VOICES].loop = midiVoice[inst].loop >> 16;
                  audVoice[voice + SFX_VOICES].length = midiVoice[inst].length >> 16;
                  audVoice[voice + SFX_VOICES].flags = 0x01; // enable voice
                } else {
                  // percussion channel - note is percussion instrument
                  inst = (ULONG)note + 100;
                  audVoice[voice + SFX_VOICES].chan = channel; // back link for pitch wheel
                  audVoice[voice + SFX_VOICES].wave = midiVoice[inst].wave;
                  audVoice[voice + SFX_VOICES].index = 0.0f;
                  audVoice[voice + SFX_VOICES].step = 1.0f;//11025.0f / 44100.0f;
                  audVoice[voice + SFX_VOICES].loop = midiVoice[inst].loop >> 16;
                  audVoice[voice + SFX_VOICES].length = midiVoice[inst].length >> 16;
                  audVoice[voice + SFX_VOICES].flags = 0x01; // enable voice
                }
              }
              break;
            case 2:                    // Pitch
              channel = (int)(event & 15);
              mus_channel[channel].pitch = pitch_table[(ULONG)*score_ptr++ & 0xff];
              break;
            case 3:                    // Tempo
              score_ptr++;             // skip value - not supported
              break;
            case 4:                    // Change control
              channel = (int)(event & 15);
              ctrl = *score_ptr++;
              value = *score_ptr++;
              switch(ctrl) {
                case 0:
                  // set channel instrument
                  mus_channel[channel].instrument = (ULONG)value;
                  mus_channel[channel].pitch = 1.0f;
                  break;
                case 3:
                  // set channel volume
                  mus_channel[channel].vol = volume = (float)value;
                  pan = mus_channel[channel].pan;
                  mus_channel[channel].ltvol = (volume - (volume * pan * pan) / 65536.0f) / 127.0f;
                  pan -= 256.0f;
                  mus_channel[channel].rtvol = (volume - (volume * pan * pan) / 65536.0f) / 127.0f;
                  break;
                case 4:
                  // set channel pan
                  mus_channel[channel].pan = pan = (float)value;
                  volume = mus_channel[channel].vol;
                  mus_channel[channel].ltvol = (volume - (volume * pan * pan) / 65536.0f) / 127.0f;
                  pan -= 256.0f;
                  mus_channel[channel].rtvol = (volume - (volume * pan * pan) / 65536.0f) / 127.0f;
                  break;
              }
              break;
            case 6:                    // End score
              if (mus_looping)
                score_ptr = (UBYTE *)((ULONG)score_data + (ULONG)score_start);
             else {
                for (voice=0; voice<MUS_VOICES; voice++) {
                  audVoice[voice + SFX_VOICES].flags = 0;
                  voice_avail[voice] = 0;
                }
                mus_delay = 0;
                mus_playing = 0;
                goto mix;
              }
              break;
          }
        } while (!(event & 0x80));     // not last event

        // now get the time until the next event(s)
        next_time = 0;
        time = *score_ptr++;
        while (time & 0x80) {
          // while msb set, accumulate 7 bits
          next_time |= (ULONG)(time & 0x7f);
          next_time <<= 7;
          time = *score_ptr++;
        }
        next_time |= (ULONG)time;
        mus_delay += next_time;
        if (mus_delay <= 0)
          goto nextEvent;
      }
    }
  } // endif musplaying

mix:
  // clear buffer
  memset((void *)buffer, 0, NUM_SAMPLES<<2);

  // mix enabled voices
  for (ix=0; ix<NUM_VOICES; ix++) {
    if (audVoice[ix].flags & 0x01) {
      // process enabled voice
      if (!(audVoice[ix].flags & 0x80) && !(mus_playing))
        continue;                      // skip instrument if music not playing

      index = audVoice[ix].index;
      step = audVoice[ix].step;
      loop = audVoice[ix].loop;
      length = audVoice[ix].length;
      ltvol = audVoice[ix].ltvol;
      rtvol = audVoice[ix].rtvol;
      wvbuff = audVoice[ix].wave;

      if (!(audVoice[ix].flags & 0x80)) {
        // special handling for instrument
        if (audVoice[ix].flags & 0x02) {
          // releasing
          ltvol *= 0.90f;
          rtvol *= 0.90f;
          audVoice[ix].ltvol = ltvol;
          audVoice[ix].rtvol = rtvol;
          if (ltvol <= 0.02f && rtvol <= 0.02f) {
            audVoice[ix].flags = 0;      // disable voice
            continue;                    // next voice
          }
        }
        step *= mus_channel[audVoice[ix].chan & 15].pitch;
        ltvol *= mus_volume;
        rtvol *= mus_volume;
      }

      for (iy=0; iy<(NUM_SAMPLES<<1); iy+=2) {

        if ((ULONG)index >= length) 
	{
          if (!(audVoice[ix].flags & 0x80)) 
	  {
            // check if instrument has loop
            if (loop)
	    {
	      printf("loop -- index -= %f ; index += %f\n", (float)length, (float)loop);
              index -= (float)length;
              index += (float)loop;
            }
	    else
	    {
              audVoice[ix].flags = 0;  // disable voice
              break;                   // exit sample loop
            }
          }
          else
          {
            audVoice[ix].flags = 0;  // disable voice
            break;                   // exit sample loop
          }
        }

	if(d)
	printf("index = %d ; step = %f ;\n", (int)index, step);


        sample = (wvbuff) ? (float)wvbuff[(int)index] : 0.0f;  // for safety

        smpbuff[iy] += (short)(sample * ltvol * master_vol);
        smpbuff[iy + 1] += (short)(sample * rtvol * master_vol);

        index += step;
      }
      audVoice[ix].index = index;
    }
  }
}




int main(int argc, char **argv)
{
	int fill_buffer_iterator = 0;
	char *mus_file;
	char *inst_file;
	void *musdata;
	int mussize;
	FILE *mushnd;
	FILE *write_handle;
	short *pcmbuf;

	if(argc != 4)
	{
		printf("not enough args; usage:\n");
		printf("%s mus_filename midi_instruments_filename raw_output_filename\n", argv[0]);
		return -1;
	}

	mus_file = argv[1];
	inst_file = argv[2];
	outputfilename = argv[3];

	if(strlen(mus_file) < 1)
	{
		printf("invalid MUS filename: %s\n", mus_file);
		printf("exiting now\n");
		return -1;
	}

	if(strlen(inst_file) < 1)
	{
		printf("invalid instrument set filename: %s\n", inst_file);
		printf("exiting now\n");
		return -1;
	}

	filename = inst_file;

	reset_midiVoices();

	mushnd = fopen(mus_file,"rb");
	if(mushnd)
	{
		fseek(mushnd,0,SEEK_END);
		mussize = ftell(mushnd);
		fseek(mushnd,0,SEEK_SET);
		musdata = (void *)malloc(mussize);

		fread((void *)musdata, 1, mussize, mushnd);
		MUSheader_t *musheader = (MUSheader_t *)musdata;

		printf("ID == %c%c%c%c\n", musheader->ID[0], musheader->ID[1], musheader->ID[2], musheader->ID[3]);
		printf("scoreLen == %d\n", musheader->scoreLen);
		printf("scoreStart == %d\n", musheader->scoreStart);
		printf("channels == %d\n", musheader->channels);
		printf("sec_channels == %d\n", musheader->sec_channels);
		printf("instrCnt == %d\n", musheader->instrCnt);
		used_instrument_count = musheader->instrCnt;
		int i;
		for(i = 0; i<musheader->instrCnt;i++)
		{
			printf("instruments[%d] == %d\n", i, musheader->instruments[i]);
			used_instruments[i] = musheader->instruments[i];
		}
	}

	psp_fillBuffer(0);

        Mus_Play(Mus_Register(musdata),1);

	write_handle = fopen(outputfilename, "wb");

	for(fill_buffer_iterator=0;fill_buffer_iterator<4096;fill_buffer_iterator++)
	{
		psp_fillBuffer(0);
		pcmbuf = pcmflip ? pcmout2 : pcmout1;

		fwrite(pcmbuf, sizeof(short), 315*2, write_handle);

		audioOutput(0);

/*		printf("\n");
		for(c = 0; c<16;c++)
		{
			if(c > 0 && c < 16)
				printf(",");
			printf("%X", pcmbuf[c]);
		}
		printf("\n");*/
	}

	fclose(write_handle);

	return 0;
}




void write_little_endian(unsigned int word, int num_bytes, FILE *wav_file)
{
    unsigned char buf;
    while(num_bytes>0)
    {   buf = word & 0xff;
        fwrite(&buf, 1,1, wav_file);
        num_bytes--;
    word >>= 8;
    }
}
 
/* information about the WAV file format from
 
http://ccrma.stanford.edu/courses/422/projects/WaveFormat/
 
 */
 
void write_wav(char * filename, unsigned long num_samples, short int * data, int s_rate)
{
    FILE* wav_file;
    unsigned int sample_rate;
    unsigned int num_channels;
    unsigned int bytes_per_sample;
    unsigned int byte_rate;
    unsigned long i;    /* counter for samples */
 
    num_channels = 2;   /* monoaural */
    bytes_per_sample = 2;
 
    if (s_rate<=0) sample_rate = 44100;
    else sample_rate = (unsigned int) s_rate;
 
    byte_rate = sample_rate*num_channels*bytes_per_sample;
 
    wav_file = fopen(filename, "wb");
    assert(wav_file);   /* make sure it opened */
 
    /* write RIFF header */
    fwrite("RIFF", 1, 4, wav_file);
    write_little_endian(36 + bytes_per_sample* num_samples*num_channels, 4, wav_file);
    fwrite("WAVE", 1, 4, wav_file);
 
    /* write fmt  subchunk */
    fwrite("fmt ", 1, 4, wav_file);
    write_little_endian(16, 4, wav_file);   /* SubChunk1Size is 16 */
    write_little_endian(1, 2, wav_file);    /* PCM is format 1 */
    write_little_endian(num_channels, 2, wav_file);
    write_little_endian(sample_rate, 4, wav_file);
    write_little_endian(byte_rate, 4, wav_file);
    write_little_endian(num_channels*bytes_per_sample, 2, wav_file);  /* block align */
    write_little_endian(8*bytes_per_sample, 2, wav_file);  /* bits/sample */
 
    /* write data subchunk */
    fwrite("data", 1, 4, wav_file);
    write_little_endian(bytes_per_sample* num_samples, 4, wav_file);
    for (i=0; i< num_samples; i++)
    {   write_little_endian((unsigned int)(data[i]),bytes_per_sample, wav_file);
    }
 
    fclose(wav_file);
}
