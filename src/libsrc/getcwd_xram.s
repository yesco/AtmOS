;
; Sodiumlightbaby 2024
;
; LOCI getcwd XRAM version
; Returns the LOCI path of the assumed boot image (ROM,DSK or TAP)
; Does not go directly to XRAM on LOCI side. Saves extra FW function

    .include    "loci.inc"

    .export         _getcwd_xram
    .import         _mia_set_ax, _mia_call_int_errno
    .import         popax
    .importzp       ptr1, ptr2

_getcwd_xram:
    sta     ptr1                    ;max lenght
    stx     ptr1+1
    lda     #255
    ldx     #0
    jsr     _mia_set_ax
    lda     #MIA_OP_GETCWD
    jsr     _mia_call_int_errno
    jsr     popax                   ;xram address
    sta     MIA_ADDR0
    stx     MIA_ADDR0+1
    lda     #1
    sta     MIA_STEP0
    ldy     ptr1                    ;TODO Assuming max 255 chars
@loop:
    lda     MIA_XSTACK
    sta     MIA_RW0
    beq     @done                   ;stop when zero terminated
    dey
    bne     @loop                   ;stop at max length
@done:
    rts
