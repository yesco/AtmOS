; ---------------------------------------------------------------------------
; versions.s
; ---------------------------------------------------------------------------
;
; Inserts the ROM version number in the file and makes space for the firmware's 

.segment  "VERSIONS"

.byte   VERPATCH
.byte   VERMINOR
.byte   VERMAJOR
.byte   $F0
.byte   $F1
.byte   $F2    