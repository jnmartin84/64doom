#include <libdragon.h>
#include <stdint.h>
#include "z_zone.h"
#include "i_system.h"

#include "doomdef.h"
#include "w_wad.h"

extern void unlockVideo(surface_t* _dc);
extern surface_t* lockVideo(int i);
extern surface_t* _dc;

extern int mus_playing;
extern void *mainzone;

static uint32_t __attribute__((aligned(8))) cga_pal[16];
static uint8_t ENDOOM_BYTES[2*25*80];

void DoomIsOver(void)
{
    set_AI_interrupt(0);
    mus_playing = -1;
    audio_close();

    cga_pal[ 0] = graphics_make_color(0x00,0x00,0x00,0xFF);
    cga_pal[ 1] = graphics_make_color(0x00,0x00,0xAA-20,0xFF);
    cga_pal[ 2] = graphics_make_color(0x00,0xAA-20,0x00,0xFF);
    cga_pal[ 3] = graphics_make_color(0x00,0xAA-20,0xAA-20,0xFF);
    cga_pal[ 4] = graphics_make_color(0xAA-20,0x00,0x00,0xFF);
    cga_pal[ 5] = graphics_make_color(0xAA-20,0x00,0xAA-20,0xFF);
    cga_pal[ 6] = graphics_make_color(0xAA-20,0x55-20,0x00,0xFF);
    cga_pal[ 7] = graphics_make_color(0xAA-20,0xAA-20,0xAA-20,0xFF);
    cga_pal[ 8] = graphics_make_color(0x55-20,0x55-20,0x55-20,0xFF);
    cga_pal[ 9] = graphics_make_color(0x55-20,0x55-20,0xFF-20,0xFF);
    cga_pal[ 9] = graphics_make_color(0x55-20,0x55-20,0xFF-20,0xFF);
    cga_pal[10] = graphics_make_color(0x55-20,0xFF-20,0x55-20,0xFF);
    cga_pal[11] = graphics_make_color(0x55-20,0xFF-20,0xFF-20,0xFF);
    cga_pal[12] = graphics_make_color(0xFF-20,0x55-20,0x55-20,0xFF);
    cga_pal[13] = graphics_make_color(0xFF-20,0x55-20,0xFF-20,0xFF);
    cga_pal[14] = graphics_make_color(0xFF-20,0xFF-20,0x55-20,0xFF);
    cga_pal[15] = graphics_make_color(0xFF-20,0xFF-20,0xFF-20,0xFF);

    int ENDOOM_NUM = W_GetNumForName("ENDOOM");
    if(-1 == ENDOOM_NUM)
    {
        I_Error("DoomIsOver: Could not load ENDOOM lump.\n");
    }

	display_close();

    // last thing we do with any of the Doom engine, get the text data lump
    W_ReadLump(ENDOOM_NUM, ENDOOM_BYTES);
    // done with using Doom subsystems here
	// need to free the entire zone memory block that got allocated during init
	free((void*)((uintptr_t)mainzone & (uintptr_t)0x8FFFFFFF));
	// otherwise there isn't enough memory to reallocate the frame buffers
	// and nothing else works
	display_init(RESOLUTION_640x480, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE);

    for (int i=0;i<2;i++)
    {
        char indexstr[2] = {0};
        uint8_t attr;
        uint32_t color0;
        uint32_t color1;
        int y; int x;
        int n = 0;

        _dc = lockVideo(1);
        graphics_fill_screen(_dc, cga_pal[4]);
        for (y=0;y<25;y++)
        {
            for (x=0;x<80;x++)
            {
                indexstr[0] = ENDOOM_BYTES[n];
                attr = ENDOOM_BYTES[n+1];
                color0 = cga_pal[attr >> 4];
                color1 = cga_pal[attr & 0x0F];
                graphics_set_color(color1, color0);
                graphics_draw_text(_dc, (x*8), (y*16)+40, indexstr);
                n += 2;
            }
        }

        unlockVideo(_dc);
    }

    while (1)
    {
        // show's over, go home
    }
}