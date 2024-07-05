CC65_TARGET = none

SOURCES = main.c tui.c
ASOURCES = display.s keyboard.s irq.s vectors.s

LSOURCES = $(wildcard libsrc/*.c)
LASOURCES = $(wildcard libsrc/*.s)

PROGRAM = loci.rom
LIBRARY = loci.lib

CC      = $(CC65_HOME)/bin/cl65
AR      = $(CC65_HOME)/bin/ar65
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

$(LIBRARY): $(CC65_HOME)/lib/$(CC65_TARGET).lib $(LSOURCES:.c=.o) $(LASOURCES:.s=.o)
	$(AR) ar $(LIBRARY) $(LSOURCES:.c=.o) $(LASOURCES:.s=.o)

$(PROGRAM): $(ASOURCES:.s=.o) $(SOURCES:.c=.o) $(LIBRARY)
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	$(RM) $(SOURCES:.c=.o) $(ASOURCES:.s=.o) $(LSOURCES:.c=.o) $(LASOURCES:.s=.o) $(SOURCES:.c=.d) $(PROGRAM) $(PROGRAM).map $(LIBRARY)
