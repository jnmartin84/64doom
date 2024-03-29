ifeq ($(IWAD_DIRECTORY),)
  $(error IWAD_DIRECTORY is not set)
endif
ifeq ($(IWAD_PREFIX),)
  $(error IWAD_PREFIX is not set)
endif

ROOTDIR			= $(N64_INST)
GCCN64PREFIX = $(ROOTDIR)/bin/mips64-elf-
CHKSUM64PATH = $(ROOTDIR)/bin/chksum64
MKDFSPATH = $(ROOTDIR)/bin/mkdfs
MKSPRITEPATH = $(ROOTDIR)/bin/mksprite
HEADERPATH = $(ROOTDIR)/mips64-elf/lib
N64TOOL = $(ROOTDIR)/bin/n64tool
N64_SYM = $(ROOTDIR)/bin/n64sym
N64_DSO = $(ROOTDIR)/bin/n64dso
N64_DSOEXTERN = $(ROOTDIR)/bin/n64dso-extern
N64_DSOMSYM = $(ROOTDIR)/bin/n64dso-msym
HEADERNAME = header
LIBS = -ldragon -lc -lm -ldragonsys
PROG_NAME = $(IWAD_PREFIX)
OPTFLAG = -O3

# function/data sections + gc-sections saves about ~200kb from output binary
CFLAGS = -ffunction-sections -fdata-sections -falign-functions=32 -ffile-prefix-map=$(CURDIR)= -DN64 -fdiagnostics-color=always -Wno-error=switch -Wall -g -mabi=o64 -mno-shared -mno-abicalls -mno-branch-likely -mno-llsc -mno-check-zero-division -mfix4300 -std=gnu99 -march=vr4300 -mtune=vr4300 -mips3 -G4 $(OPTFLAG) -I$(ROOTDIR)/mips64-elf/include
#-DRANGECHECK
LINK_FLAGS = --gc-sections -G4 -g -L$(ROOTDIR)/mips64-elf/lib $(LIBS) -Tn64.ld

CC = $(GCCN64PREFIX)gcc
AS = $(GCCN64PREFIX)as
LD = $(GCCN64PREFIX)ld
OBJCOPY = $(GCCN64PREFIX)objcopy
OBJDUMP = $(GCCN64PREFIX)objdump



O=obj

OBJS=						\
		$(O)/i_sound.o			\
		$(O)/endoom.o			\
		$(O)/dll.o			\
		$(O)/hash.o			\
		$(O)/lzfx.o			\
		$(O)/i_video.o			\
		$(O)/i_input.o			\
		$(O)/doomdef.o			\
		$(O)/doomstat.o			\
		$(O)/dstrings.o			\
		$(O)/i_system.o			\
		$(O)/i_net.o			\
		$(O)/r_draw.o			\
		$(O)/v_video.o			\
		$(O)/f_finale.o			\
		$(O)/f_wipe.o 			\
		$(O)/d_main.o			\
		$(O)/d_net.o			\
		$(O)/d_items.o			\
		$(O)/g_game.o			\
		$(O)/p_saveg.o			\
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
		$(O)/nodereorder.o		\
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
	$(N64_SYM) $(PROG_NAME).elf $(PROG_NAME).sym
	$(N64_DSOMSYM) $(PROG_NAME).elf $(PROG_NAME).msym
	rm -f $(PROG_NAME).z64
	rm -f rom.dfs
	cp $(IWAD_DIRECTORY)/$(IWAD_PREFIX).WAD filesystem
	echo $(IWAD_PREFIX).WAD > filesystem/identifier
	$(MKDFSPATH) rom.dfs ./filesystem/
	$(N64TOOL) \
        --header $(HEADERPATH)/$(HEADERNAME) --title $(PROG_NAME) --toc --output $(PROG_NAME).z64 $(PROG_NAME).bin --align 8 $(PROG_NAME).sym --align 8 $(PROG_NAME).msym --align 16 rom.dfs
	chmod ugo=rw $(PROG_NAME).z64
	$(CHKSUM64PATH) $(PROG_NAME).z64

$(PROG_NAME).elf : $(OBJS)
	$(LD) -o $(PROG_NAME).elf $(OBJS) $(LINK_FLAGS)

copy: $(PROG_NAME).z64
	cp $(PROG_NAME).z64 ~/

all: $(PROG_NAME).z64

clean:
	rm -f *.z64 *.elf *.msym *.sym *.bin *.dfs
	rm -f $(O)/*

$(O)/%.o:	%.c
	$(CC) $(CFLAGS) -c $< -o $@

#############################################################
#
#############################################################
