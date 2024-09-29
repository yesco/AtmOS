;
; 2023, Rumbledethumps
; 2024, Sodiumlightbaby
;
; Enables the C IRQ tools

.export initirq, doneirq, _irq_int, _nmi_int
.exportzp _irq_ticks
;.import callirq 
.import _exit, ReadKeyboard

.include "loci.inc"

.segment "ZEROPAGE"
_irq_ticks:
    .res 1

.segment "ONCE"

initirq:
    lda #<handler
    ldx #>handler
    sei
    sta $FFFE
    stx $FFFF
    cli
    rts

.code

doneirq:
    sei
    rts

.segment "LOWCODE"

_irq_int:

handler:
    cld
    sei
    pha
    txa
    pha
    ;tsx
    ;inx
    ;inx
    ;lda $100,X
    ;and #$10
    ;bne break
    tya
    pha
    ;jsr callirq
    inc _irq_ticks
    lda VIA_T1CL        ;clear timer interrupt
    lda #$7F
    sta VIA_IFR         ;cancel any VIA interrupt
    jsr ReadKeyboard
    pla
    tay
    pla
    tax
    pla
    cli
    rti

_nmi_int:
    sei
    pha
    ; Write Status Register Number to PortA 
    lda #$07 
    sta VIA_PA2 

    ; Tell AY this is Register Number 
    lda #$FF 
    sta VIA_PCR 

    ; Clear CB2, as keeping it high hangs on some orics.
    ; Pitty, as all this code could be run only once, otherwise
    lda #$dd 
    sta VIA_PCR 

    lda #$40    ;Enable port output on 8912 

    sta VIA_PA2 
    lda #$fd 

    sta VIA_PCR 
    lda #$dd
    sta VIA_PCR
    pla
    cli
    rti

break:
    lda #$FF
    sta MIA_A
    jmp _exit
