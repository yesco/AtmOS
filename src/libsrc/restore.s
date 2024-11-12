;
; 2024, Sodiumlightbaby
;
; LOCI save and restore memory functions

.include "loci.inc"
.export mia_save_state, _mia_restore_state, _mia_restore_buffer_ok, _mia_clear_restore_buffer, _mia_get_vmode
.import _init, _mia_call_int_errno

RSTR_PTR  := $0000
MIA_RSTR_XRAM := $0000
REGS_SIZE := 13
RAM_SIZE  := ($2000-$100)
VRAM_SIZE := $2000
VMODE_SIZE := 1
MAGIC_SIZE := 1
RSTR_SIZE := (RAM_SIZE+VRAM_SIZE+VMODE_SIZE+REGS_SIZE+MAGIC_SIZE)
RSTR_LAST := (RSTR_SIZE - 1)
VMODE_ADDR := (RAM_SIZE+VRAM_SIZE)

.code

.proc _mia_restore_buffer_ok
    lda #1
    sta MIA_STEP0
    lda #<RSTR_LAST
    sta MIA_ADDR0+0
    lda #>RSTR_LAST
    sta MIA_ADDR0+1
    lda #'L'
    cmp MIA_RW0
    beq @exit
    lda #0              ; 0 = false, L = true
@exit:
    rts
.endproc

.proc _mia_clear_restore_buffer
    lda #1
    sta MIA_STEP0
    lda #<RSTR_LAST
    sta MIA_ADDR0+0
    lda #>RSTR_LAST
    sta MIA_ADDR0+1
    lda #0
    sta MIA_RW0
    rts
.endproc

.proc _mia_get_vmode
    lda #1
    sta MIA_STEP0
    lda #<VMODE_ADDR
    sta MIA_ADDR0+0
    lda #>VMODE_ADDR
    sta MIA_ADDR0+1
    lda MIA_RW0
    rts
.endproc


.proc mia_save_state
    cld
.endproc
;fall-through
.proc mia_save_registers
;save flags to stack, a,y,x and sp to xstack
    php
    sta MIA_XSTACK          
    sty MIA_XSTACK
    stx MIA_XSTACK
    tsx
    stx MIA_XSTACK
;save VIA select regs to xstack
    lda VIA_T1LH
    sta MIA_XSTACK
    lda VIA_T1LL
    sta MIA_XSTACK
    lda VIA_T2CH
    sta MIA_XSTACK
    lda VIA_T2CL
    sta MIA_XSTACK
    lda VIA_IER
    sta MIA_XSTACK
    lda VIA_PCR
    sta MIA_XSTACK
    lda VIA_ACR
    sta MIA_XSTACK
    lda VIA_DDRA
    sta MIA_XSTACK
    lda VIA_DDRB
    sta MIA_XSTACK
.endproc
;fall-through
.proc mia_save_zp
    lda #1
    sta MIA_STEP0
    lda #<MIA_RSTR_XRAM
    sta MIA_ADDR0+0
    lda #>MIA_RSTR_XRAM
    sta MIA_ADDR0+1
    lda $00
    sta MIA_RW0
    lda #$00
    sta RSTR_PTR
    ldx #1
@loop:
    lda RSTR_PTR,x
    sta MIA_RW0
    inx
    bne @loop
.endproc
;fall-through
.proc mia_save_ram
    ldx #$20
    ldy #1
    sty RSTR_PTR+1
    dey
    sty RSTR_PTR+0
@loop:
    lda (RSTR_PTR),y
    sta MIA_RW0
    iny
    bne @loop
@skip:
    inc RSTR_PTR+1
    lda #$03            ;skip page 3
    cmp RSTR_PTR+1
    beq @skip
    cpx RSTR_PTR+1
    bne @loop
.endproc
;fall-through
.proc mia_save_vram
    lda #$a0
    sta RSTR_PTR+1
    ldx #$c0
@loop:
    lda (RSTR_PTR),y
    sta MIA_RW0
    iny
    bne @loop
    inc RSTR_PTR+1
    cpx RSTR_PTR+1
    bne @loop
.endproc
;fall-through
;ULA mode dependent address jumps
;Use with ULA pattern matching PIO program ($83848586)
;   Text  50Hz: BCA0,BCA1,BBB2,BBB3
;   Hires 50Hz: BCA0,BCA1,A032,A033
;   Text  60Hz: No transition
;   Hires 60Hz: BBB0,BBB1,A032,A033
.proc mia_save_vmode
    lda #$83            ;50Hz setup
    sta $BCA0
    lda #$84
    sta $BCA1
    lda #$85
    sta $BBB2
    sta $A032
    lda #$86
    sta $BBB3
    sta $A033
    lda #$5A            ;Text50  $1A | $40
    sta $BBB4
    lda #$5F            ;Hires50 $1F | $40
    sta $A034
    lda #$86
    sta $03A3
    ldx #$00
    ldy #$10
@delay:
    dex 
    bne @delay
    dey
    bne @delay
    lda $03A3
    beq @60Hz       ;if not found
    and #$BF
    bne @done       ;always taken
@60Hz:
    lda #$83
    sta $BBB0
    sta $BBB2       ;disable 50Hz test (anything but $85)
    lda #$84
    sta $BBB1
    lda #$5C        ;Hires60 $1C | $40
    sta $A034
    lda #$86
    sta $03A3
    ldx #$00
    ldy #$10
@delay60:
    dex 
    bne @delay60
    dey
    bne @delay60
    lda $03A3
    and #$BF
    bne @done       ;Hires60
    lda #$18        ;Text60
@done:
    sta MIA_RW0
.endproc
;fall-through
.proc mia_save_xstack
    ldx #REGS_SIZE
@loop:
    lda MIA_XSTACK
    sta MIA_RW0
    dex
    bne @loop
.endproc
;fall-through
.proc mia_save_exit
    lda #'L'
    sta MIA_RW0
    jmp _init
.endproc

.proc _mia_restore_state
    lda #255        ;-1
    sta MIA_STEP0
    lda #<RSTR_LAST
    sta MIA_ADDR0+0
    lda #>RSTR_LAST
    sta MIA_ADDR0+1
    lda #'L'
    cmp MIA_RW0
    beq mia_restore_xstack
    lda #MIA_OP_BOOT            ;Fallback if bad restore state
    jmp _mia_call_int_errno
.endproc
;fall-through
.proc mia_restore_xstack
    sei
    ldx #REGS_SIZE
@loop:
    lda MIA_RW0
    sta MIA_XSTACK
    dex
    bne @loop
.endproc
;fall-through
.proc mia_restore_clear_magic
    lda #<RSTR_LAST
    sta MIA_ADDR0+0
    lda #>RSTR_LAST
    sta MIA_ADDR0+1
    lda #0
    sta MIA_RW0
.endproc
;fall-through
.proc mia_restore_ram
    ldx #$20
    lda #1
    sta MIA_STEP0
    lda #$01
    sta RSTR_PTR+1
    sta MIA_ADDR0+1
    lda #$00
    tay
    sta RSTR_PTR+0
    sta MIA_ADDR0+0
@loop:
    lda MIA_RW0
    sta (RSTR_PTR),y
    iny
    bne @loop
@skip:
    inc RSTR_PTR+1
    lda #$03            ;skip page 3
    cmp RSTR_PTR+1
    beq @skip
    cpx RSTR_PTR+1
    bne @loop
.endproc
;fall-through
.proc mia_restore_vram
    lda #$a0
    sta RSTR_PTR+1
    ldx #$c0
@loop:
    lda MIA_RW0
    sta (RSTR_PTR),y
    iny
    bne @loop
    inc RSTR_PTR+1
    cpx RSTR_PTR+1
    bne @loop
.endproc
;fall-through
.proc mia_restore_vmode
    lda $bfdf
    ldx MIA_RW0
    stx $bfdf
    ldx #$00
    ldy #$10
@delay:
    dex 
    bne @delay
    dey
    bne @delay
    sta $bfdf
.endproc
;fall-through
.proc mia_restore_zp
    lda #1
    sta MIA_STEP0
    lda #<MIA_RSTR_XRAM
    sta MIA_ADDR0+0
    lda #>MIA_RSTR_XRAM
    sta MIA_ADDR0+1
    lda #0
    sta RSTR_PTR
    ldy MIA_RW0         ;Stash $00 value in y
    ldx #1
@loop:
    lda MIA_RW0
    sta RSTR_PTR,x
    inx
    bne @loop
    sty $00             ;Done with zeropage RSTR_PTR
.endproc
;fall-through
.proc mia_restore_registers
    lda MIA_XSTACK
    sta VIA_DDRB
    lda MIA_XSTACK
    sta VIA_DDRA
    lda MIA_XSTACK
    sta VIA_ACR
    lda MIA_XSTACK
    sta VIA_PCR
    lda #$7F            ;Clear all first
    sta VIA_IER
    lda MIA_XSTACK
    sta VIA_IER         ;Restore only sets bits
    lda MIA_XSTACK
    sta VIA_T2CL
    lda MIA_XSTACK
    sta VIA_T2CH
    lda MIA_XSTACK
    sta VIA_T1LL
    lda MIA_XSTACK
    sta VIA_T1LH
    sta VIA_T1CH
    bit VIA_T1CL        ;Clear T1 IRQ flag
    bit VIA_IER         ;Check T1 enabled
    bvc @skip
@waitirq:               ;Wait for real IRQ if T1 enabled
    bit VIA_IFR
    bvc @waitirq
@skip:
    ldx MIA_XSTACK
    txs 
    ldx MIA_XSTACK
    ldy MIA_XSTACK
    ;lda MIA_XSTACK after we call MIA_OP
.endproc
;fall-through
.proc call_loci_boot
    lda #$40        ;flag resume for boot op
    ora MIA_A
    sta MIA_A
    lda #MIA_OP_BOOT
    sta MIA_OP
    lda MIA_XSTACK
    plp
    jmp MIA_SPIN
.endproc