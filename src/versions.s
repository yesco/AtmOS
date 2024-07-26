; ---------------------------------------------------------------------------
; versions.s
; ---------------------------------------------------------------------------
;
; Inserts the ROM version number in the file and makes space for the firmware's 

.export _locirom_version 
.export _locifw_version

.segment  "VERSIONS"
_locirom_version:
.byte   VERPATCH
.byte   VERMINOR
.byte   VERMAJOR
_locifw_version:
.byte   $F0
.byte   $F1
.byte   $F2    