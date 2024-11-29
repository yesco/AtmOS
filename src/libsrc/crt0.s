;
; 2023, Rumbledethumps
; 2024, Sodiumlightbaby
;
; crt0.s

.export _init, _exit
.import _main

.export __STARTUP__ : absolute = 1
.import __RAM_START__, __RAM_SIZE__

.import copydata, copyfont, copyaltfont, copyscrn, zerobss, initlib, donelib

.include "loci.inc"
.include "zeropage.inc"

.segment  "STARTUP"

; Essential 6502 startup the CPU doesn't do
_init:
    ldx #$FF
    txs
    cld

; Set cc65 argument stack pointer
    lda #<(__RAM_START__ + __RAM_SIZE__)
    sta sp
    lda #>(__RAM_START__ + __RAM_SIZE__)
    sta sp+1

; Initialize memory storage
    jsr zerobss   ; Clear BSS segment
    jsr copydata  ; Initialize DATA segment
    ;jsr copyfont
    ;jsr copyaltfont
    ;jsr copyscrn
    jsr initlib   ; Run constructors

; Initialize VIA for keyboard and 25Hz interrupt
    sei
    ; Setup DDRA, DDRB and ACR
    lda #%11111111
    sta VIA_DDRA
    lda #%11110111 ; PB0-2 outputs, PB3 input.
    sta VIA_DDRB
    lda #%01000000
    sta VIA_ACR

    ; Setup PCR
    lda #%11011101 ; CB2=low, CA2=low
    sta VIA_PCR

    ; Setup PA, PB default output
    lda #%11111111
    sta VIA_PA2

    lda #%10110000  ; Tape out=high, motor=low, uc=high, Strobe=high, kbdrow=0
    sta VIA_PB

    ; Enable only Timer 1 interrupts
    lda #%00111111
    sta VIA_IER
    lda #%11000000
    sta VIA_IER

    ; Since this is an slow process, we set the VIA timer to 
    ; issue interrupts at 25Hz, instead of 100 Hz. This is 
    ; not necessary -- it depends on your needs
    lda #<40000
    sta VIA_T1LL
    sta VIA_T1CL
    lda #>40000
    sta VIA_T1LH
    sta VIA_T1CH

    cli



; Call main()
    jsr _main

; Back from main() also the _exit entry
; Stack the exit value in case destructors call OS
_exit:
    pha
    txa
    pha
    jsr donelib  ; Run destructors
    pla
    sta MIA_X
    tax
    pla
    sta MIA_A
    lda #$FF     ; exit()
    sta MIA_OP
    brk
