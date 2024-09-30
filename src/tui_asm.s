; ---------------------------------------------------------------------------
; tui_asm.s
; ---------------------------------------------------------------------------

.export _tui_org_list, _tui_current
.export _tui_screen_xy, _tui_cls, _tui_fill, _tui_hit, _tui_toggle_highlight
.export _tui_clear_txt, _tui_set_current, _tui_get_current, _tui_draw_txt
.import popa, popax

.define TUI_SCREEN $BB80

.zeropage
_tui_org_list: .res 2
tui_ptr:    .res 2
tui_ptr2:   .res 2
tui_tmp:    .res 1

.bss
_tui_current: .res 1
tui_x:      .res 1
tui_y:      .res 1
tui_len:    .res 1
tui_widget: .res 6

.data

tui_row_offset:
    .byte  0,  5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65
    .byte 70, 75, 80, 85, 90, 95,100,105,110,115,120,125,130,135

.code

; Calculate memory address of x,y position of screen
; y in reg a, x on c-stack
.proc _tui_screen_xy
    tax
    jsr popa
.endproc

; x in reg a, y in reg x
.proc tui_screen_xy
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
    pha ;sta tui_ptr+0
    lda tui_tmp
    adc tui_ptr+1
    tax
    pla ;lda tui_ptr+0
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

.proc _tui_hit
    sec
    ldy #2
    sbc (_tui_org_list),y     ;y-org_y
    sta tui_y
    jsr popa
    dey
    sec
    sbc (_tui_org_list),y     ;x-org_x
    sta tui_x
    lda #0
    sta tui_tmp
    lda _tui_org_list
    sta tui_ptr
    lda _tui_org_list+1
    sta tui_ptr+1
@loop:
    inc tui_tmp
    clc
    lda #6
    adc tui_ptr
    sta tui_ptr
    lda #0
    tay
    adc tui_ptr+1
    sta tui_ptr+1
    lda (tui_ptr),y
    beq @miss               ;TUI_END
    bpl @loop               ;Not Active
    ldy #2
    lda (tui_ptr),y         ;Get widget Y
    cmp tui_y
    bne @loop               ;Not same Y
    dey
    lda (tui_ptr),y         ;Get widget X
    cmp tui_x
    beq @pass
    bpl @loop               ;Left of widget
@pass:
    ldy #3
    clc
    adc (tui_ptr),y         ;Add widget len
    cmp tui_x
    beq @loop
    bmi @loop               ;Right of widget
    bpl @hit                ;Left of widget end = HIT
@miss:
    lda #0
    rts
@hit:
    lda tui_tmp
    rts
.endproc

;widget idx in A
;returns addr of widget in tui_ptr
.proc tui_idx_to_addr
    ldy #0
    sty tui_ptr+0
    sty tui_ptr+1
    sta tui_tmp            ;widget idx in A
    clc                    ;calc *6 
    rol A                   ; idx<<1 = *2
    ;sta tui_ptr+0
    rol tui_ptr+1           ; c->hibyte
    adc tui_tmp             ; *2+idx = *3
    sta tui_ptr+0           ; store lobyte
    lda #0
    adc tui_ptr+1           ; c->hibyte
    sta tui_ptr+1
    rol tui_ptr+0           ; *3<<1 = *6
    rol tui_ptr+1
    clc
    lda _tui_org_list+0     ; add orig_list to idx*6 
    adc tui_ptr+0
    sta tui_ptr+0
    lda _tui_org_list+1
    adc tui_ptr+1
    sta tui_ptr+1
    rts
.endproc

;copy current widget to tui_widget
.proc tui_cpy_widget
    ldy #5
@loop:
    lda (tui_ptr),y         ; get widget copy
    sta tui_widget,y
    dey
    bpl @loop
    rts
.endproc

.proc _tui_toggle_highlight
    jsr tui_idx_to_addr
    jsr tui_cpy_widget
    clc
    ldy #2
    lda (_tui_org_list),y   ; orig Y
    adc tui_widget+2        ; add widget Y
    tax
    clc
    dey
    lda (_tui_org_list),y   ; orig X
    adc tui_widget+1        ; add widget X
    jsr tui_screen_xy       ; get screen address of widget 
    clc
    sta tui_ptr+0
    stx tui_ptr+1
    lda tui_widget+0        ; get type
    bpl @exit               ; exit if not "active"
    cmp #130                ; TUI_INP
    beq @tui_inp
    ldy tui_widget+3        ; get widget len        
    dey
@inv:
    lda #$80
    eor (tui_ptr),y 
    sta (tui_ptr),y 
    dey
    bpl @inv
    bmi @exit               ; always

@tui_inp:
    lda tui_widget+4
    sta tui_ptr2+0
    lda tui_widget+5
    sta tui_ptr2+1
    ldy #$ff                  
@strlen:
    iny
    lda (tui_ptr2),Y        ; widget->data[y]
    bne @strlen
    lda #' '
    cmp (tui_ptr),y
    bne @cursor_off
    lda #$80
    eor (tui_ptr),y
@cursor_off:
    sta (tui_ptr),y
@exit:
    rts
.endproc


.proc _tui_clear_txt
    jsr tui_idx_to_addr
    ldy #3
    lda (tui_ptr),y         ;get widget->len
    sta tui_len
    dey
    clc
    lda (tui_ptr),y         ;get widget->y
    adc (_tui_org_list),y   ;add org y
    tax
    dey
    clc
    lda (tui_ptr),y         ;get widget->x
    adc (_tui_org_list),y   ;add org x
    jsr tui_screen_xy
    sta tui_ptr+0
    stx tui_ptr+1
    ldy tui_len
    dey
    lda #' '
@loop:
    sta (tui_ptr),y
    dey
    bpl @loop
    rts
.endproc

.proc _tui_set_current
    pha
    lda _tui_current
    jsr _tui_toggle_highlight
    pla
    sta _tui_current
    jsr _tui_toggle_highlight
    rts
.endproc

.proc _tui_get_current
    lda _tui_current
    rts
.endproc

.proc _tui_draw_txt
    jsr tui_idx_to_addr
    jsr tui_cpy_widget
    lda tui_widget+2        ;calc widget->x + org->x
    clc
    ldy #2
    adc (_tui_org_list),y
    tax
    lda tui_widget+1        ;calc widget->y + org->y
    dey
    clc
    adc (_tui_org_list),y   ;screen widget pointer
    jsr tui_screen_xy
    sta tui_ptr+0           
    stx tui_ptr+1
    lda tui_widget+4        ;widget data pointer
    sta tui_ptr2+0
    lda tui_widget+5
    sta tui_ptr2+1
    ldy #0                  ;positive direction loop
@loop:
    cpy tui_widget+3        ;compare to widget->len
    beq @exit
    lda (tui_ptr2),y
    beq @exit
    sta (tui_ptr),y 
    iny
    bne @loop               ;branch always
@exit:
    rts
.endproc