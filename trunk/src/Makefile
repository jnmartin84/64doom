ROOTDIR			=$(N64_INST)

GCCN64PREFIX = $(ROOTDIR)/bin/mips64-elf-
CHKSUM64PATH = $(ROOTDIR)/bin/chksum64
MKDFSPATH = $(ROOTDIR)/bin/mkdfs
MKSPRITEPATH = $(ROOTDIR)/bin/mksprite
HEADERPATH = $(ROOTDIR)/lib
N64TOOL = $(ROOTDIR)/bin/n64tool
HEADERNAME = header

LIBS = -ldragon -lc -lm -ldragonsys -ln64memory
LINK_FLAGS = -G0 -L$(ROOTDIR)/lib -L$(ROOTDIR)/mips64-elf/lib $(LIBS) -Tn64ld.x
PROG_NAME = doom
CFLAGS = -std=gnu99 -march=vr4300 -mtune=vr4300 -fno-strict-aliasing -Wall -G0 -O3 -I$(ROOTDIR)/include -I$(ROOTDIR)/mips64-elf/include
ASFLAGS = -mtune=vr4300 -march=vr4300

CC = $(GCCN64PREFIX)gcc
AS = $(GCCN64PREFIX)as
LD = $(GCCN64PREFIX)ld
OBJCOPY = $(GCCN64PREFIX)objcopy

O=obj

OBJS=						\
		$(O)/dll.o			\
		$(O)/hash.o			\
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
		$(O)/info.o			\
		$(O)/sounds.o			\
		$(O)/i_main.o


$(PROG_NAME).bin: $(PROG_NAME).elf 
	$(OBJCOPY) $(PROG_NAME).elf $(PROG_NAME).bin -O binary
	rm -f $(PROG_NAME).elf

$(PROG_NAME).elf : $(OBJS)
	$(LD) -o $(PROG_NAME).elf $(OBJS) $(LINK_FLAGS)

all: $(PROG_NAME).bin

clean:
	rm -f *.elf *.bin
	rm -f $(O)/*.o

$(O)/%.o:	%.c
	$(CC) $(CFLAGS) -c $< -o $@

#############################################################
#
#############################################################
