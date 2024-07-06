CC65_TARGET = none

ifndef CC65_HOME
$(warning CC65_HOME not set. Defaulting to /usr/share/cc65)
export CC65_HOME = /usr/share/cc65
endif

SOURCES = main.c tui.c
ASOURCES = display.s keyboard.s irq.s vectors.s

LSOURCES = $(wildcard libsrc/*.c)
LASOURCES = $(wildcard libsrc/*.s)

PROGRAM = loci.rom
LIBRARY = loci.lib

ifneq ($(wildcard $(CC65_HOME)/bin/.*),)
CC      = $(CC65_HOME)/bin/cl65
AR      = $(CC65_HOME)/bin/ar65
else
CC      = cl65
AR      = ar65
endif

CFLAGS  =  -t $(CC65_TARGET) -O --debug-info -I ./include --asm-include-dir ./asminc -I ./asminc
LDFLAGS =  -C cfg/loci.cfg --debug-info -m $(PROGRAM).map
CP      = cp -f

.SUFFIXES:
.PHONY: all clean
all: $(PROGRAM)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

%.o: %.s
	$(CC) -c $(CFLAGS) -o $@ $<

$(LIBRARY): $(LSOURCES:.c=.o) $(LASOURCES:.s=.o)
	$(CP) $(CC65_HOME)/lib/supervision.lib $(LIBRARY)
	$(AR) a $(LIBRARY) $(LSOURCES:.c=.o) $(LASOURCES:.s=.o)

$(PROGRAM): $(ASOURCES:.s=.o) $(SOURCES:.c=.o) $(LIBRARY)
	$(CC) $(LDFLAGS) -o $@ $^

$(PROGRAM).rp6502: $(PROGRAM)
	../tools/rp6502.py -o $(PROGRAM).rp6052 -a 0xC000 create $(PROGRAM)

clean:
	$(RM) $(SOURCES:.c=.o) $(ASOURCES:.s=.o) $(LSOURCES:.c=.o) $(LASOURCES:.s=.o) $(SOURCES:.c=.d) $(PROGRAM) $(PROGRAM).map $(PROGRAM).rp6502 $(LIBRARY)
