;
; Ullrich von Bassewitz, 1998-12-07, 2004-12-01
;
; Copy the data segment from the LOAD to the RUN location
;

        .export         copyfont
        .import         __FONTSTD_LOAD__, __FONTSTD_RUN__, __FONTSTD_SIZE__
        .importzp       ptr1, ptr2, tmp1


copyfont:
        lda     #<__FONTSTD_LOAD__         ; Source pointer
        sta     ptr1
        lda     #>__FONTSTD_LOAD__
        sta     ptr1+1

        lda     #<__FONTSTD_RUN__          ; Target pointer
        sta     ptr2
        lda     #>__FONTSTD_RUN__
        sta     ptr2+1

        ldx     #<~__FONTSTD_SIZE__
        lda     #>~__FONTSTD_SIZE__        ; Use -(__DATASIZE__+1)
        sta     tmp1
        ldy     #$00

; Copy loop

@L1:    inx
        beq     @L3

@L2:    lda     (ptr1),y
        sta     (ptr2),y
        iny
        bne     @L1
        inc     ptr1+1
        inc     ptr2+1                  ; Bump pointers
        bne     @L1                     ; Branch always (hopefully)

; Bump the high counter byte

@L3:    inc     tmp1
        bne     @L2

; Done

        rts

