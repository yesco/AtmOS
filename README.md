# Oric LOCI ROM

> Warning The LOCI ABI is in development. Unmatched ROM and firmware are not guaranteed to work together.

The LOCI ROM is a 6502 application mapped in by LOCI and executed by the attached Oric computer to manage floppy or tape image mounting and other features of LOCI, at the user's request.

## Build instructions
The current setup requires a working install of CC65 and GNU Make
    sudo apt install cc65 make

The environmental variable CC65_HOME must point to the main CC65 SDK directory 
    export CC65_HOME=/usr/share/cc65

Build the ROM from the src/ directory
    cd src
    make

The output ROM file is `loci.rom`

> Warning LOCI currently only accepts RP6502 formatted ROM files