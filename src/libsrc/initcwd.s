;
; Sodiumlightbaby 2024
;
; LOCI __cwd init
; Returns the LOCI path of the assumed boot image (ROM,DSK or TAP)

    .include    "loci.inc"

    .export         initcwd
    .import         __cwd, __cwd_buf_size, _mia_set_ax, _mia_call_int_errno
    .importzp       ptr2

initcwd:
    lda     #<__cwd
    ldx     #>__cwd
    sta     ptr2
    stx     ptr2+1
    ldy     __cwd_buf_size         ;buffer size - 1
    dey
    tya
    ldx     #0
    jsr     _mia_set_ax
    lda     #MIA_OP_GETCWD
    jsr     _mia_call_int_errno

    ldy     #0
@loop:
    lda     MIA_XSTACK
    sta     (ptr2),y
    beq     @done                   ;stop when zero terminated
    iny
    bne     @loop                   ;stop at max 255
@done:
    rts
