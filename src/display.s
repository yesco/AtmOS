.export _init_display, __txtscreen
.import copyfont, copyaltfont

.include "loci.inc"

.code

.proc _init_display

    lda $bfdf
    ldx #$1a
    stx $bfdf
    ldx #$00
    ldy #$10
@delay:
    dex 
    bne @delay
    dey
    bne @delay
    sta $bfdf
    jsr copyfont
    jsr copyaltfont
    rts

.endproc

.segment    "FONTSTD"
.include    "font_loci.asm"

.segment    "FONTALT"
.include    "font_loci_alt.asm"

.segment    "SCREEN"
__txtscreen = *


