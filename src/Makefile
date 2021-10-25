ROOTDIR			= $(N64_INST)
DOOMDIR			= /home/jason/n64dev/gold_dooms/64doom/src

MIDIPATH		= $(DOOMDIR)/ADoom_Ins
MIDIFILE		= MIDI_Instruments

TNT			= 0
PLUTONIA		= 0
DOOM2			= 0
DOOMSHAREWARE		= 0
ULTIMATE		= 1

IDPATH			= $(DOOMDIR)/identifiers
IDFILE_TNT		= identifier_TNT
IDFILE_PLUTONIA		= identifier_PLUTONIA
IDFILE_DOOM2		= identifier_DOOM2
IDFILE_DOOMSHAREWARE	= identifier_DOOMSHAREWARE
IDFILE_ULTIMATE 	= identifier_ULTIMATEDOOM
IDFILE			= $(IDFILE_ULTIMATE)
#IDFILE			= $(IDFILE_DOOM2)
#IDFILE			= $(IDFILE_PLUTONIA)
#IDFILE			= $(IDFILE_TNT)

WADDIR			= $(DOOMDIR)/filesystems
WADPATH_TNT		= $(WADDIR)/filesystem_tnt
WADPATH_PLUTONIA	= $(WADDIR)/filesystem_plutonia
WADPATH_DOOM2		= $(WADDIR)/filesystem_doom2
WADPATH_DOOMSHAREWARE	= $(WADDIR)/filesystem_doomshareware
WADPATH_ULTIMATE	= $(WADDIR)/filesystem_ultimatedoom
WADPATH		= $(WADPATH_ULTIMATE)
#WADPATH			= $(WADPATH_DOOM2)
#WADPATH		= $(WADPATH_PLUTONIA)
#WADPATH		= $(WADPATH_TNT)

WADFILE_TNT		= TNT.WAD
WADFILE_PLUTONIA	= PLUTONIA.WAD
WADFILE_DOOM2		= DOOM2.WAD
WADFILE_DOOMSHAREWARE	= DOOM1.WAD
WADFILE_ULTIMATE        = DOOMU.WAD
WADFILE		= $(WADFILE_ULTIMATE)
#WADFILE			= $(WADFILE_DOOM2)
#WADFILE		= $(WADFILE_PLUTONIA)
#WADFILE		= $(WADFILE_TNT)

MIDI_ROM_OFFSET		= 1048576B
ID_ROM_OFFSET		= 5242880B
WAD_ROM_OFFSET		= 5242896B


GCCN64PREFIX = $(ROOTDIR)/bin/mips64-elf-
CHKSUM64PATH = $(ROOTDIR)/bin/chksum64
MKDFSPATH = $(ROOTDIR)/bin/mkdfs
MKSPRITEPATH = $(ROOTDIR)/bin/mksprite
HEADERPATH = $(ROOTDIR)/lib
N64TOOL = $(ROOTDIR)/bin/n64tool
HEADERNAME = header
LIBS = -ldragon -lc -lm -ldragonsys
LINK_FLAGS = -G4 -L$(ROOTDIR)/lib -L$(ROOTDIR)/mips64-elf/lib $(LIBS) -Tn64.ld
PROG_NAME = doom
OPTFLAG = -O3


# changed all to pedantic
CFLAGS = -mno-shared -mno-abicalls -DTNT=$(TNT) -DPLUTONIA=$(PLUTONIA) -DDOOMSHAREWARE=$(DOOMSHAREWARE) -DULTIMATE=$(ULTIMATE) -DDOOM2=$(DOOM2) -mno-branch-likely -mno-llsc -mno-check-zero-division -std=gnu99 -mabi=o64 -march=vr4300 -mtune=vr4300 -mips3 -G4 $(OPTFLAG) -I$(ROOTDIR)/include -I$(ROOTDIR)/mips64-elf/include
CFLAGS_ASM = -mno-shared -mno-abicalls -DTNT=$(TNT) -DPLUTONIA=$(PLUTONIA) -DDOOMSHAREWARE=$(DOOMSHAREWARE) -DULTIMATE=$(ULTIMATE) -DDOOM2=$(DOOM2) -mno-branch-likely -mno-llsc -mno-check-zero-division -std=gnu99 -mabi=o64 -march=vr4300 -mtune=vr4300 -mips3 -G4 $(OPTFLAG) -I$(ROOTDIR)/include -I$(ROOTDIR)/mips64-elf/include
#CFLAGS_ASM = -mno-check-zero-division -std=gnu99 -mabi=o64 -march=vr4300 -mtune=vr4300 -Wall -mips3 -G4 $(OPTFLAG) -I$(ROOTDIR)/include -I$(ROOTDIR)/mips64-elf/include

CC = $(GCCN64PREFIX)gcc
AS = $(GCCN64PREFIX)as
LD = $(GCCN64PREFIX)ld
OBJCOPY = $(GCCN64PREFIX)objcopy
OBJDUMP = $(GCCN64PREFIX)objdump

O=obj

R_DRAW=r_draw
OBJS=						\
		$(O)/n64_memcpy.o		\
		$(O)/n64_memset.o		\
		$(O)/$(R_DRAW).o		\
		$(O)/n64_rom_fs.o		\
		$(O)/n64_i_sound.o		\
		$(O)/endoom.o			\
		$(O)/exception.o		\
		$(O)/dll.o			\
		$(O)/hash.o			\
		$(O)/lzfx.o			\
		$(O)/n64_i_video.o		\
		$(O)/buf_draw.o			\
		$(O)/n64_i_input.o		\
		$(O)/doomdef.o			\
		$(O)/doomstat.o			\
		$(O)/dstrings.o			\
		$(O)/i_system.o			\
		$(O)/i_net.o			\
		$(O)/tables.o			\
		$(O)/n64_r_draw_16bpp.o		\
		$(O)/n64_v_video_16bit.o	\
		$(O)/f_finale.o			\
		$(O)/f_wipe.o 			\
		$(O)/d_main.o			\
		$(O)/d_net.o			\
		$(O)/d_items.o			\
		$(O)/n64_g_game.o		\
		$(O)/p_saveg.o			\
		$(O)/m_swap.o			\
		$(O)/m_menu.o			\
		$(O)/m_misc.o			\
		$(O)/m_argv.o  			\
		$(O)/m_bbox.o			\
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
		$(O)/p_user.o			\
		$(O)/r_bsp.o			\
		$(O)/r_data.o			\
		$(O)/r_main.o			\
		$(O)/r_plane.o			\
		$(O)/r_segs.o			\
		$(O)/r_sky.o			\
		$(O)/r_things.o			\
		$(O)/w_wad.o			\
		$(O)/wi_stuff.o			\
		$(O)/z_zone.o			\
		$(O)/st_lib.o			\
		$(O)/st_stuff.o			\
		$(O)/hu_stuff.o			\
		$(O)/hu_lib.o			\
		$(O)/s_sound.o			\
		$(O)/info.o			\
		$(O)/sounds.o			\
		$(O)/i_main.o

$(PROG_NAME).z64: $(PROG_NAME).elf
	$(OBJCOPY) $(PROG_NAME).elf $(PROG_NAME).bin -O binary
	rm -f $(PROG_NAME).z64
	./newtool -l 32M	\
		-h $(HEADERPATH)/$(HEADERNAME)			\
		-o $(PROG_NAME).z64	\
		-t DOOM	$(PROG_NAME).bin		\
		-v	\
		-s $(MIDI_ROM_OFFSET) $(MIDIPATH)/$(MIDIFILE)		\
		-s $(ID_ROM_OFFSET) $(IDPATH)/$(IDFILE)		\
		-s $(WAD_ROM_OFFSET) $(WADPATH)/$(WADFILE)
	chmod ugo=rw doom.z64
	$(CHKSUM64PATH) $(PROG_NAME).z64

$(PROG_NAME).elf : $(OBJS)
	$(LD) -o $(PROG_NAME).elf $(OBJS) $(LINK_FLAGS)
# I used these for debugging in the past
#	$(OBJDUMP) -t $(PROG_NAME).elf > $(PROG_NAME)_symbols.txt
#	cat $(PROG_NAME)_symbols.txt | grep 'F .text' > $(PROG_NAME)_functions.txt

copy: $(PROG_NAME).z64
	cp $(PROG_NAME).z64 ~/

all: $(PROG_NAME).z64

clean:
	rm -f *.z64 *.elf *.o *.bin *.dfs *.txt
	rm -f $(O)/*

$(O)/m_swap.o: m_swap.S
	$(CC) $(CFLAGS_ASM) -c $< -o $@
$(O)/n64_memcpy.o: n64_memcpy.S
	$(CC) $(CFLAGS_ASM) -c $< -o $@
$(O)/n64_memset.o: n64_memset.S
	$(CC) $(CFLAGS_ASM) -c $< -o $@
$(O)/$(R_DRAW).o: $(R_DRAW).S
	$(CC) $(CFLAGS_ASM) -c $< -o $@

$(O)/%.o:	%.c
	$(CC) $(CFLAGS) -c $< -o $@

#############################################################
#
#############################################################
