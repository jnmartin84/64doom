ROOTDIR = $(N64_INST)
CFLAGS = -std=gnu99 -Os -G0 -Wall -Werror -mtune=vr4300 -march=vr4300 -I$(CURDIR)/include -I$(ROOTDIR)/include -I$(ROOTDIR)/mips64-elf/include
ASFLAGS = -mtune=vr4300 -march=vr4300
N64PREFIX = $(N64_INST)/bin/mips64-elf-
INSTALLDIR = $(N64_INST)
CC = $(N64PREFIX)gcc
AS = $(N64PREFIX)as
LD = $(N64PREFIX)ld
AR = $(N64PREFIX)ar

all: libn64memory

libn64memory: libn64memory.a

include files.in

libn64memory.a: $(OFILES_LD)
	$(AR) -rcs -o libn64memory.a $(OFILES_LD)

install: libn64memory.a
	install -D --mode=644 libn64memory.a $(INSTALLDIR)/lib/libn64memory.a
	install -D --mode=644 include/n64memory.h $(INSTALLDIR)/include/n64memory.h

clean:
	rm -f *.o *.a
	rm -rf $(CURDIR)/build

clobber: clean

.PHONY : clobber clean
