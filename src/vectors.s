; ---------------------------------------------------------------------------
; vectors.s
; ---------------------------------------------------------------------------
;
; Defines the interrupt vector table.

.import    _init,  mia_save_state
.import    _nmi_int, _irq_int

.segment  "VECTORS"

;.addr      _nmi_int    ; NMI vector
.addr      mia_save_state; NMI vector
.addr      _init       ; Reset vector
.addr      _irq_int    ; IRQ/BRK vector