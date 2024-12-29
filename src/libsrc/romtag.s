; ---------------------------------------------------------------------------
; romtag.s
; ---------------------------------------------------------------------------
;
; A ROM tag to go first/early in ROM address space to identify any mapping problems

.export check_romtag
.import __txtscreen
.importzp ptr1, ptr2

.segment  "ROMTAG"
romtag:
.byte   'r'

.segment "STARTUP"
check_romtag:
    lda romtag
    cmp #'r'
    beq @tag_ok
    lda #<__txtscreen
    ldx #>__txtscreen
    sta ptr1+0               
    stx ptr1+1
    lda #<@tag_msg
    ldx #>@tag_msg
    sta ptr2+0               
    stx ptr2+1
@forever:
    ldy #0
@loop:
    lda (ptr2),y
    beq @forever
    sta (ptr1),y
    iny
    bne @loop
@tag_ok:
    rts
@tag_msg:
    .byte $1a, $01, $0c, "DUAL ROM SYSTEM?", $08, $00


