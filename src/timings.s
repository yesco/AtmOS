; ---------------------------------------------------------------------------
; timings.s
; ---------------------------------------------------------------------------
;
; Location for firmware to insert LOCI interface timings  

.export _loci_tmap, _loci_tadr, _loci_tior, _loci_tiow, _loci_tiod

.segment  "TIMINGS"
_loci_tmap:
.byte   $FA
_loci_tior:
.byte   $FB
_loci_tiow:
.byte   $FC
_loci_tiod:
.byte   $FD
_loci_tadr:
.byte   $FE
