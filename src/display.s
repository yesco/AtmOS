.export _init_display

.include "loci.inc"

.code

.proc _init_display

    pha
    lda $1a
    sta $a000
    pla
    rts

.endproc

.segment    "FONTSTD"
.include    "font_loci.asm"

.segment    "FONTALT"
.include    "font_loci_alt.asm"

.segment    "SCREEN"
__txtscreen = *


