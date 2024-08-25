#include "loci.h"

//Tape emulation counter. Seek == 0 returns current counter/file pos
//To seek to zero, use TAP command register (tap cmd = rewind)
unsigned long tap_counter(unsigned long seek){
    mia_set_axsreg(seek);
    return mia_call_long(MIA_OP_TAP_CNT);
}