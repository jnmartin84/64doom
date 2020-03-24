#include <stdio.h>
#include <libdragon.h>
extern void *__safe_buffer[];
#define VI_ORIGIN 0xA4400004
#define VI_WIDTH 0xA4400008
#define PHYS_TO_K0(x)   ((uint32_t)(x)|0x80000000)      /* physical to kseg0 */
#define PHYS_TO_K1(x)   ((uint32_t)(x)|0xA0000000)      /* physical to kseg1 */

#define IO_READ(addr)           (*(volatile uint32_t *)PHYS_TO_K1(addr))
#define IO_WRITE(addr,data)     (*(volatile uint32_t *)PHYS_TO_K1(addr)=(uint32_t)(data))

struct regdump_s {
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
	uint32_t r10;
	uint32_t r11;
	uint32_t r12;
	uint32_t r13;
	uint32_t r14;
	uint32_t r15;
	uint32_t r16;
	uint32_t r17;
	uint32_t r18;
	uint32_t r19;
	uint32_t r20;
	uint32_t r21;
	uint32_t r22;
	uint32_t r23;
	uint32_t r24;
	uint32_t r25;
	uint32_t r26;
	uint32_t r27;
	uint32_t r28;
	uint32_t r29;
	uint32_t r30;
	uint32_t r31;
	uint32_t hi;
	uint32_t lo;
	uint32_t cop00;
	uint32_t cop01;
	uint32_t cop02;
	uint32_t cop03;
	uint32_t cop04;
	uint32_t cop05;
	uint32_t cop06;
	uint32_t cop07;
	uint32_t cop08;
	uint32_t cop09;
	uint32_t cop010;
	uint32_t cop011;
	uint32_t cop012;
	uint32_t cop013;
	uint32_t cop014;
	uint32_t cop015;
	uint32_t cop016;
	uint32_t cop017;
	uint32_t cop018;
	uint32_t cop019;
	uint32_t cop020;
	uint32_t cop021;
	uint32_t cop022;
	uint32_t cop023;
	uint32_t cop024;
	uint32_t cop025;
	uint32_t cop026;
	uint32_t cop027;
	uint32_t cop028;
	uint32_t cop029;
	uint32_t cop030;
	uint32_t cop031;
};

struct regdump_s __attribute__((aligned(8))) reg_dump;

extern void graphics_buffer_draw_text( void* disp, int x, int y, int dw, int dh, const char * const msg );

static char errstr[256];
static uint32_t blue;
void register_dump() {
	int i;
	int vpi;
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE);
    uint16_t *video_ptr_1 = &((uint16_t *)__safe_buffer[0])[0];
    uint16_t *video_ptr_2 = &((uint16_t *)__safe_buffer[1])[0];
	uint16_t *video_ptr[2] = {video_ptr_1, video_ptr_2};
	disable_interrupts();
//	IO_WRITE(VI_WIDTH, 320);
//	IO_WRITE(VI_ORIGIN, video_ptr[0]);
//    set_VI_interrupt(0,0);
//    set_VI_interrupt(1,0);

	
	blue = graphics_make_color(0,0,255,0);
	
	for(vpi=0;vpi<2;vpi++) {
		for(i=0;i<320*240;i++) {
			*(video_ptr[vpi]) = blue;
		}

        sprintf(errstr, "r0 %08lX at %08lX v0 %08lX\n", reg_dump.r0,reg_dump.r1,reg_dump.r2);
        graphics_buffer_draw_text(video_ptr[vpi], 16, 16, 320, 240, errstr);
		sprintf(errstr, "v1 %08lX\n", reg_dump.r3);
        graphics_buffer_draw_text(video_ptr[vpi], 16, 24, 320, 240, errstr);

        sprintf(errstr, "a0 %08lX a1 %08lX a2 %08lX\n", reg_dump.r4,reg_dump.r5,reg_dump.r6);
        graphics_buffer_draw_text(video_ptr[vpi], 16, 32, 320, 240, errstr);
		sprintf(errstr, "a3 %08lX\n", reg_dump.r7);
        graphics_buffer_draw_text(video_ptr[vpi], 16, 40, 320, 240, errstr);

        sprintf(errstr, "t0 %08lX t1 %08lX t2 %08lX\n", reg_dump.r8, reg_dump.r9,reg_dump.r10);
        graphics_buffer_draw_text(video_ptr[vpi], 16, 48, 320, 240, errstr);
		sprintf(errstr, "t3 %08lX\n", reg_dump.r11);
        graphics_buffer_draw_text(video_ptr[vpi], 16, 56, 320, 240, errstr);

        sprintf(errstr, "t4 %08lX t5 %08lX t6 %08lX\n", reg_dump.r12,reg_dump.r13,reg_dump.r14);
        graphics_buffer_draw_text(video_ptr[vpi], 16, 64, 320, 240, errstr);
		sprintf(errstr, "t7 %08lX\n", reg_dump.r15);
        graphics_buffer_draw_text(video_ptr[vpi], 16, 72, 320, 240, errstr);

        sprintf(errstr, "s0 %08lX s1 %08lX s2 %08lX\n", reg_dump.r16,reg_dump.r17,reg_dump.r18);
        graphics_buffer_draw_text(video_ptr[vpi], 16, 80, 320, 240, errstr);
		sprintf(errstr, "s3 %08lX\n", reg_dump.r19);
        graphics_buffer_draw_text(video_ptr[vpi], 16, 88, 320, 240, errstr);

        sprintf(errstr, "s4 %08lX s5 %08lX s6 %08lX\n", reg_dump.r20,reg_dump.r21,reg_dump.r22);
        graphics_buffer_draw_text(video_ptr[vpi], 16, 96, 320, 240, errstr);
		sprintf(errstr, "s7 %08lX\n", reg_dump.r23);
        graphics_buffer_draw_text(video_ptr[vpi], 16, 104, 320, 240, errstr);

        sprintf(errstr, "t8 %08lX t9 %08lX k0 %08lX\n", reg_dump.r24,reg_dump.r25,reg_dump.r26);
        graphics_buffer_draw_text(video_ptr[vpi], 16, 112, 320, 240, errstr);
		sprintf(errstr, "k1 %08lX\n", reg_dump.r27);
        graphics_buffer_draw_text(video_ptr[vpi], 16, 120, 320, 240, errstr);

        sprintf(errstr, "gp %08lX sp %08lX fp %08lX\n", reg_dump.r28,reg_dump.r29,reg_dump.r30);
        graphics_buffer_draw_text(video_ptr[vpi], 16, 128, 320, 240, errstr);
		sprintf(errstr, "ra %08lX\n", reg_dump.r31);
        graphics_buffer_draw_text(video_ptr[vpi], 16, 136, 320, 240, errstr);
		
		sprintf(errstr, "epc %08lX", reg_dump.cop014); 
		graphics_buffer_draw_text(video_ptr[vpi], 16, 150, 320, 240, errstr);		
		sprintf(errstr, "cause %08lX", reg_dump.cop013);
		graphics_buffer_draw_text(video_ptr[vpi], 16, 158, 320, 240, errstr);		
		sprintf(errstr, "vaddr %08lX", reg_dump.cop08);
		graphics_buffer_draw_text(video_ptr[vpi], 16, 166, 320, 240, errstr);		
        /*
        sprintf(errstr, "hi %08lX lo %08lX\n", e->regs->hi, e->regs->lo);
        graphics_buffer_draw_text(video_ptr[vpi], 16, 80+4, 320, 240, errstr);

        sprintf(errstr, "sr %08lX epc %08lX\n", e->regs->sr,e->regs->epc);
        graphics_buffer_draw_text(video_ptr[vpi], 16, 88+8, 320, 240, errstr);
*/
	}
	enable_interrupts();
	while(1) {}
}