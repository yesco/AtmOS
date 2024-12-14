;
; Ullrich von Bassewitz, 1998-12-07, 2004-12-01
;
; Copy the data segment from the LOAD to the RUN location
;

        .export         copyfont
        .import         __FONTSTD_LOAD__, __FONTSTD_RUN__, __FONTSTD_SIZE__
        .importzp       ptr1, ptr2, tmp1, tmp2

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

        inx

; Copy loop. Expands packed 6 bit wide ROM font to unpacked 8 bit RAM font
@cpyloop:
        jsr     exp0                  ;expand i0 -> o0
        jsr     incx
        beq     @done
        jsr     exp1                  ;expand i0,i1 -> o1
        jsr     incx
        beq     @done
        jsr     exp2                  ;expand i1,i2 -> o2,o3
        jsr     incx
        beq     @done
        jsr     incy
        bne     @cpyloop
@done:
        rts

incy:   iny
        bne     @ydone
        inc     ptr1+1
        inc     ptr2+1                  ; Bump pointers
@ydone: rts

incx:   inx
        bne     @xdone
        inc     tmp1
@xdone: rts

exp0:   lda     (ptr1),y                    ; 0 expand BBBBBB--
        lsr
        lsr
        sta     (ptr2),y                    ; 0 store
        rts

exp1:   lda     (ptr1),y                    ; 1 expand ------BB BBBB----
        sta     tmp2
        jsr     incy                        ;<<
        lda     (ptr1),y
        ror     tmp2
        ror     A
        ror     tmp2
        ror     A
        lsr
        lsr
        sta     (ptr2),y                    ; 1 store
        rts

exp2:   lda     (ptr1),y                    ; 2 expand ----BBBB BB------
        sta     tmp2
        jsr     incy                        ;<<
        lda     (ptr1),y
        rol     A 
        rol     tmp2
        rol     A 
        rol     tmp2 
        lda     #$3F
        and     tmp2
        sta     (ptr2),y                    ; 2 store

        lda     (ptr1),y                    ; 3 expand --BBBBBB
        and     #$3F
        inc     ptr2+0                      ;<< ptr2 skewed +1 per 3
        bne     @L4
        inc     ptr2+1
@L4:         
        sta     (ptr2),y                    ; 3 store
        rts
