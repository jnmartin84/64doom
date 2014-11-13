ifdef $(N64_INST)
ROOTDIR = $(N64_INST)
else
ROOTDIR = /opt/toolchains/n64
endif

CFLAGS = -std=gnu99 -O0 -G0 -Wall -Werror -mtune=vr4300 -march=vr4300 -I$(CURDIR)/include -I$(ROOTDIR)/include -I$(ROOTDIR)/mips64-elf/include
ASFLAGS = -mtune=vr4300 -march=vr4300
N64PREFIX = $(ROOTDIR)/mips64-elf/bin/mips64-elf-
INSTALLDIR = $(ROOTDIR)/mips64-elf
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
