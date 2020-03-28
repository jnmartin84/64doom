#include <libdragon.h>
#include <stdint.h>

#include "doomdef.h"
#include "w_wad.h"

extern void buffer_draw_char_tall_5551(char ch, int x, int y, uint16_t *buffer, uint32_t color, int width, int height);
extern void buffer_rect_5551(int x, int y, int w, int  h, uint32_t fill, uint16_t *buf, int buf_w, int buf_h);

static uint32_t __attribute__((aligned(8))) cga_pal[16];

extern void unlockVideo(display_context_t _dc);
extern display_context_t lockVideo(int i);
extern void *__n64_memset_ASM(void *p, int v, size_t n);
extern void *__n64_memset_ZERO_ASM(void *p, int v, size_t n);
extern int mus_playing;
extern short pcmout1[];
extern short pcmout2[];
extern void *__safe_buffer[];
extern display_context_t _dc;
uint8_t ENDOOM_BYTES[50*80];
void DoomIsOver(void)
{
	mus_playing = -1;
    //display_init(RESOLUTION_640x480, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE);
unlockVideo(_dc);
	__n64_memset_ZERO_ASM(pcmout1,0,316*4);
	__n64_memset_ZERO_ASM(pcmout2,0,316*4);
	audio_close();
	__n64_memset_ZERO_ASM(__safe_buffer[0],0,640*420*2);
	__n64_memset_ZERO_ASM(__safe_buffer[1],0,640*420*2);

	//    console_init();
//    console_set_render_mode(RENDER_AUTOMATIC);
        cga_pal[ 0] = graphics_make_color(0x00,0x00,0x00,0);
        cga_pal[ 1] = graphics_make_color(0x00,0x00,0xAA,0);
        cga_pal[ 2] = graphics_make_color(0x00,0xAA,0x00,0);
        cga_pal[ 3] = graphics_make_color(0x00,0xAA,0xAA,0);
        cga_pal[ 4] = graphics_make_color(0xAA,0x00,0x00,0);
        cga_pal[ 5] = graphics_make_color(0xAA,0x00,0xAA,0);
        cga_pal[ 6] = graphics_make_color(0xAA,0x55,0x00,0);
        cga_pal[ 7] = graphics_make_color(0xAA,0xAA,0xAA,0);
        cga_pal[ 8] = graphics_make_color(0x55,0x55,0x55,0);
        cga_pal[ 9] = graphics_make_color(0x55,0x55,0xFF,0);
        cga_pal[ 9] = graphics_make_color(0x55,0x55,0xFF,0);
        cga_pal[10] = graphics_make_color(0x55,0xFF,0x55,0);
        cga_pal[11] = graphics_make_color(0x55,0xFF,0xFF,0);
        cga_pal[12] = graphics_make_color(0xFF,0x55,0x55,0);
        cga_pal[13] = graphics_make_color(0xFF,0x55,0xFF,0);
        cga_pal[14] = graphics_make_color(0xFF,0xFF,0x55,0);
        cga_pal[15] = graphics_make_color(0xFF,0xFF,0xFF,0);

    _dc = lockVideo(1);
    uint16_t *buffer = &((uint16_t *)__safe_buffer[_dc - 1])[0];

        int ENDOOM_NUM = W_GetNumForName("ENDOOM");

        if(-1 == ENDOOM_NUM)
        {
                goto doom_end;
        }

/*        uint8_t *ENDOOM_BYTES = malloc(lump_size);

        if(0 == ENDOOM_BYTES)
        {
                goto doom_end;
        }
*/
        W_ReadLump(ENDOOM_NUM, ENDOOM_BYTES);

        uint8_t index;
        uint8_t attr;
        uint32_t color0;
        uint32_t color1;
        int y; int x;
        int n = 0;
        for(y=0;y<25;y++)
        {
        for(x=0;x<80;x++)
        {
                index = ENDOOM_BYTES[n];
                attr = ENDOOM_BYTES[n+1];
                color0 = cga_pal[attr >> 4];
                color1 = cga_pal[attr & 0x0F];
                buffer_rect_5551((x*8), (y*16), 8, 16, color0, buffer, 640, 480);
                buffer_draw_char_tall_5551(index, (x*8),(y*16), buffer, color1, 640, 480);
                n+= 2;
        }
        }
unlockVideo(_dc);
while(1){}
//return;

doom_end:
    printf("C:\\_\n");
}
