ifdef $(N64_INST)
ROOTDIR = $(N64_INST)
else
ROOTDIR = /opt/toolchains/n64
endif

HEADERDIR = $(ROOTDIR)/headers
HEADER = $(HEADERDIR)/header

TOOLDIR = $(ROOTDIR)/bin
CHKSUM64 = $(TOOLDIR)/chksum64
MKDFS = $(TOOLDIR)/mkdfs
N64TOOL = $(TOOLDIR)/n64tool

LDSCRIPTSDIR = $(ROOTDIR)/ldscripts
LINK_FLAGS = -T$(LDSCRIPTSDIR)/n64.ld -Map map.txt -nostdlib

LIBPATH = -L$(ROOTDIR)/mips64-elf/lib -L$(ROOTDIR)/mips64-elf/lib/gcc/mips64-elf/4.8.3 -L$(ROOTDIR)/mips64-elf/mips64-elf/lib
INCPATH = -I$(CURDIR)/include -I$(ROOTDIR)/mips64-elf/include -I$(ROOTDIR)/mips64-elf/mips64-elf/include
CCFLAGS = -std=gnu99 -march=vr4300 -mtune=vr4300 -G 0 -Ofast -Wall -c $(INCPATH)
CCFLAGS += -DSTBI_NO_HDR
HWCCFLAGS = -std=gnu99 -march=vr4300 -mtune=vr4300 -O1 -G 0 -Wall $(INCPATH)
ASFLAGS = -mtune=vr4300 -march=vr4300

N64PREFIX = $(ROOTDIR)/mips64-elf/bin/mips64-elf-
CC = $(N64PREFIX)gcc
AS = $(N64PREFIX)as
LD = $(N64PREFIX)ld
OBJCOPY = $(N64PREFIX)objcopy

PROG_NAME = doom
LIBS = -ln64memory -ldragon -lc -lm -ldragonsys -lnosys
O=obj
OBJS=							\
		$(O)/dll.o				\
		$(O)/hash.o				\
		$(O)/n64_i_video.o		\
		$(O)/n64_rom_fs.o		\
		$(O)/buf_draw.o			\
		$(O)/n64_i_input.o		\
		$(O)/doomdef.o			\
		$(O)/doomstat.o			\
		$(O)/dstrings.o			\
		$(O)/i_system.o			\
		$(O)/n64_i_sound.o		\
		$(O)/i_net.o			\
		$(O)/tables.o			\
		$(O)/f_finale.o			\
		$(O)/f_wipe.o 			\
		$(O)/d_main.o			\
		$(O)/d_net.o			\
		$(O)/d_items.o			\
		$(O)/g_game.o			\
		$(O)/m_menu.o			\
		$(O)/m_misc.o			\
		$(O)/m_argv.o  			\
		$(O)/m_bbox.o			\
		$(O)/m_fixed.o			\
		$(O)/m_swap.o			\
		$(O)/m_cheat.o			\
		$(O)/m_random.o			\
		$(O)/am_map.o			\
		$(O)/p_ceilng.o			\
		$(O)/p_doors.o			\
		$(O)/p_enemy.o			\
		$(O)/p_floor.o			\
		$(O)/p_inter.o			\
		$(O)/p_lights.o			\
		$(O)/p_map.o			\
		$(O)/p_maputl.o			\
		$(O)/p_plats.o			\
		$(O)/p_pspr.o			\
		$(O)/p_setup.o			\
		$(O)/p_sight.o			\
		$(O)/p_spec.o			\
		$(O)/p_switch.o			\
		$(O)/p_mobj.o			\
		$(O)/p_telept.o			\
		$(O)/p_tick.o			\
		$(O)/p_saveg.o			\
		$(O)/p_user.o			\
		$(O)/r_bsp.o			\
		$(O)/r_data.o			\
		$(O)/r_draw.o			\
		$(O)/r_main.o			\
		$(O)/r_plane.o			\
		$(O)/r_segs.o			\
		$(O)/r_sky.o			\
		$(O)/r_things.o			\
		$(O)/w_wad.o			\
		$(O)/wi_stuff.o			\
		$(O)/v_video.o			\
		$(O)/z_zone.o			\
		$(O)/st_lib.o			\
		$(O)/st_stuff.o			\
		$(O)/hu_stuff.o			\
		$(O)/hu_lib.o			\
		$(O)/s_sound.o			\
		$(O)/info.o				\
		$(O)/sounds.o			\
		$(O)/i_main.o

$(PROG_NAME).bin: $(PROG_NAME).elf
	$(OBJCOPY) $(PROG_NAME).elf $(PROG_NAME).bin -O binary
	rm -f $(PROG_NAME).elf

$(PROG_NAME).elf : $(OBJS)
	$(LD) -o $(PROG_NAME).elf $(OBJS) $(LINK_FLAGS) $(LIBPATH) $(LIBS)

all: $(PROG_NAME).bin

clean:
	rm -f *.elf *.bin
	rm -f $(O)/*.o

$(O)/%.o: %.c
	$(CC) $(CCFLAGS) $< -o $@

$(O)/%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@
