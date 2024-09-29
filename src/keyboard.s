
.include "loci.inc"

.importzp sp, sreg

.export _InitKeyboard
.export _KeyMatrix, _KeyRowArrows, _KeyAsciiUpper, _KeyAsciiLower, _KeyCapsLock, _ReadKey, _ReadKeyNoBounce
.export ReadKeyboard

;Values before that one are the modifier keys
.define KEY_FIRST_ASCII 6      

; Usually it is a good idea to keep 0 all the unused
; entries, as it speeds up things. Z=1 means no key
; pressed and there is no need to look in tables later on. 
; This keys don't have an ASCII code assigned, so we will
; use consecutive values outside the usual alphanumeric 
; space.

.define KEY_LCTRL       1
.define KEY_RCTRL       2
.define KEY_LSHIFT      3
.define KEY_RSHIFT      4
.define KEY_FUNCT       5

.define KEY_LEFT        8
.define KEY_RIGHT       9
.define KEY_DOWN        10
.define KEY_UP			11

.define KEY_RETURN      13
.define KEY_ESCAPE      27
.define KEY_SPACE       32
.define KEY_DELETE      127


; Using these defines, you can easily check the content of KeyRowArrows
.define KEY_MASK_SPACE        1
.define KEY_MASK_LESS_THAN    2
.define KEY_MASK_GREATER_THAN 4
.define KEY_MASK_UP_ARROW     8
.define KEY_MASK_LEFT_SHIFT   16
.define KEY_MASK_LEFT_ARROW   32
.define KEY_MASK_DOWN_ARROW   64
.define KEY_MASK_RIGHT_ARROW  128

; Control key (left one on PC)
.define VKEY_LEFT_CONTROL     20        
; Left Shift key
.define VKEY_LEFT_SHIFT       36     
; Right Shift key
.define VKEY_RIGHT_SHIFT      60        
; Function key (right Control key on PC)
.define VKEY_FUNCTION          4        

.zeropage

zpTemp01:			.res 1
zpTemp02:			.res 1
tmprow:				.res 1

.bss

_KeyMatrix:            .res 4     ; The virtual Key Matrix (top half)
_KeyRowArrows:         .res 4     ; The virtual Key Matrix (bottom half starting on the row with the arrows and the space bar)

.data
_KeyCapsLock:          .byt 1     ; By default we use CAPS letter (only acceptable values are 0 and 1)

; Regarding SHIFT and CAPS LOCK:
; - SHIFT does impact all the keys (letters, symbols, numbers)
; - CAPS LOCK only impacts the actual letters

; Some more routines, not actualy needed, but quite useful
; for reading a single key (get the first active bit in 
; the virtual matrix) and returning his ASCII value.
; Should serve as an example about how to handle the keymap.
; Both _ReadKey and _ReadKeyNoBounce can be used directly from 
; C, declared as:

.segment "RODATA"

_KeyAsciiUpper:
    .byt "7","N","5","V",KEY_RCTRL,"1","X","3"
    .byt "J","T","R","F",0,KEY_ESCAPE,"Q","D"
    .byt "M","6","B","4",KEY_LCTRL,"Z","2","C"
    .byt "K","9",59,"-",0,0,92,39
    .byt " ",",",".",KEY_UP,KEY_LSHIFT,KEY_LEFT,KEY_DOWN,KEY_RIGHT
    .byt "U","I","O","P",KEY_FUNCT,KEY_DELETE,"]","["
    .byt "Y","H","G","E",0,"A","S","W"
    .byt "8","L","0","\",KEY_RSHIFT,KEY_RETURN,0,"="

_KeyAsciiLower:
    .byt "&","n","%","v",KEY_RCTRL,"!","x","#"
    .byt "j","t","r","f",0,KEY_ESCAPE,"q","d"
    .byt "m","^","b","$",KEY_LCTRL,"z","@","c"
    .byt "k","(",58,95,0,0,124,34
    .byt " ","<",">",KEY_UP,KEY_LSHIFT,KEY_LEFT,KEY_DOWN,KEY_RIGHT
    .byt "u","i","o","p",KEY_FUNCT,KEY_DELETE,"}","{"
    .byt "y","h","g","e",0,"a","s","w"
    .byt "*","l",")","?",KEY_RSHIFT,KEY_RETURN,0,"+"

.code 

.proc _InitKeyboard
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
    rts
.endproc

.proc ReadKeyboard

    ; Write Column Register Number to PortA 
    lda #$0E 
    sta VIA_PA2 

    ; Tell AY this is Register Number 
    lda #$FF 
    sta VIA_PCR

    ; Clear CB2, as keeping it high hangs on some orics.
    ; Pitty, as all this code could be run only once, otherwise
    ldy #$dd
    sty VIA_PCR 

    ldx #7 

loop_row:   ;Clear relevant bank 
    lda #00 
    sta _KeyMatrix,x 

    ; Write 0 to Column Register 

    sta VIA_PA2 
    lda #$fd 
    sta VIA_PCR 
    lda #$dd
    sta VIA_PCR


    lda VIA_PB 
    and #%11111000
    stx zpTemp02
    ora zpTemp02 
    sta VIA_PB 


    ; Wait 10 cycles for circuit to settle on new row 
    ; Use time to load inner loop counter and load Bit 

    ; CHEMA: Fabrice Broche uses 4 cycles (lda #8:inx) plus
    ; the four cycles of the and absolute. That is 8 cycles.
    ; So I guess that I could do the same here (ldy,lda)

    ldy #$80
    lda #8 

    ; Sense Row activity 
    and VIA_PB
    beq skip2 

    ; Store Column 
    tya
loop_column:   
    eor #$FF 

    sta VIA_PA2 
    lda #$fd 
    sta VIA_PCR
    lda #$dd
    sta VIA_PCR

    lda VIA_PB 
    and #%11111000
    stx zpTemp02
    ora zpTemp02 
    sta VIA_PB 


    ; Use delay(10 cycles) for setting up bit in _KeyMatrix and loading Bit 
    tya 
    ora _KeyMatrix,x 
    sta zpTemp01 
    lda #8 

    ; Sense key activity 
    and VIA_PB
    beq skip1 

    ; Store key 
    lda zpTemp01 
    sta _KeyMatrix,x 

skip1:   ;Proceed to next column 
    tya 
    lsr 
    tay 
    bcc loop_column 

skip2:   ;Proceed to next row 
    dex 
    bpl loop_row 

    rts 
.endproc

;Self-modifying code. Need to be in data segment 
.data 

; Read a single key, same as before but no repeating.
.proc _ReadKeyNoBounce

    jsr _ReadKey
__auto_raw_current_key_code = *+1     ; For the debouncing: Current raw value (shift ignored) of the pressed key
    lda #0
__auto_raw_previous_key_code = *+1     ; For the debouncing: Raw value of the key from the previous call
    cmp #0
    beq retz
    sta __auto_raw_previous_key_code   ; The key is different
    ldx #0
    rts
retz:
    lda #0                             ; The same key is still being pressed
    tax
    rts
.endproc


; Reads a key (single press, but repeating) and returns his ASCII value in reg X. 
; Z=1 if no keypress detected.
.proc _ReadKey
    ; Start by checking modifiers... because that modifies the keys...    
    ldx #1

    lda _KeyMatrix+(VKEY_LEFT_SHIFT/8)     ; Load the matrix row containing the Left Shift key
    and #1 << (VKEY_LEFT_SHIFT & 7)        ; Check the bit for the column used by that key
    bne shift_pressed
    lda _KeyMatrix+(VKEY_RIGHT_SHIFT/8)    ; Load the matrix row containing the Right Shift key
    and #1 << (VKEY_RIGHT_SHIFT & 7)       ; Check the bit for the column used by that key
    bne shift_pressed

    ldx #0

shift_pressed:
    txa
    sta __auto_shift_pressed

    ; Then we do the proper matrix scan
    ldx #7
loop_row:
    lda _KeyMatrix,x
    beq next_row

    sta tmprow

    txa
    asl
    asl
    asl
    tay 

    lda tmprow
loop_column:
    iny
    lsr tmprow
    bcc loop_column

    lda _KeyAsciiLower-1,y
    cmp #KEY_FIRST_ASCII
    bcs ascii_key

not_ascii_key:
    lda tmprow
    bne loop_column

next_row:
    dex
    bpl loop_row

    lda #0
    sta _ReadKeyNoBounce::__auto_raw_current_key_code
    tax
    rts

ascii_key:
    sta _ReadKeyNoBounce::__auto_raw_current_key_code
    cmp #97              ; 'a'
    bcc not_letter
    cmp #122+1           ; 'z'
    bcs not_letter
    ; For actual letters, we need to take into consideration both CAPS LOCK and SHIFT keys
    pha
__auto_shift_pressed = *+1     ; Status of any of the SHIFT keys
    lda #0
    eor _KeyCapsLock
    sta __auto_shift_pressed
    pla

not_letter:
    ; For non letter character, we only use the SHIFT Status
    asl __auto_shift_pressed
    beq not_shifted
    lda _KeyAsciiUpper-1,y
not_shifted:

    ldx #0
    rts
.endproc



