; ---------------------------------------------------------------------------
; tui_asm.s
; ---------------------------------------------------------------------------

.export _tui_screen_xy, _tui_cls, _tui_fill
.import popa, popax

.define TUI_SCREEN $BB80

.zeropage
tui_ptr:    .res 2
tui_tmp:    .res 1

.data

tui_row_offset:
    .byte  0,  5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65
    .byte 70, 75, 80, 85, 90, 95,100,105,110,115,120,125,130,135

.code

; Calculate memory address of x,y position of screen
.proc _tui_screen_xy
    tax
    jsr popa
    ;OR instead of adc as x is <= 40 and <TUI_SCREEN is 0x80
    ora #<TUI_SCREEN
    sta tui_ptr+0
    lda #>TUI_SCREEN
    sta tui_ptr+1
    lda #0
    sta tui_tmp
    clc
    lda tui_row_offset,x
    rol A
    rol tui_tmp
    rol A
    rol tui_tmp
    rol A
    rol tui_tmp
    ;c is zero
    adc tui_ptr+0
    tay ;sta tui_ptr+0
    lda tui_tmp
    adc tui_ptr+1
    tax
    tya ;lda tui_ptr+0
    rts
.endproc

.proc _tui_cls
    sta tui_tmp
    lda #<TUI_SCREEN
    sta tui_ptr+0
    lda #>TUI_SCREEN
    sta tui_ptr+1
    ldx #28
@looph:
    lda #' '
    ldy #39
@loopw:
    sta (tui_ptr),y
    dey
    bne @loopw
    lda tui_tmp
    sta (tui_ptr),y
    lda tui_ptr+0
    clc 
    adc #40
    sta tui_ptr+0
    lda tui_ptr+1
    adc #0
    sta tui_ptr+1
    dex
    bne @looph
    rts
.endproc

.proc _tui_fill
    sta tui_ptr+0
    stx tui_ptr+1
    jsr popa
    pha
    jsr popa
    tay
    pla
@loop:
    dey
    sta (tui_ptr),y
    bne @loop
    rts
.endproc
