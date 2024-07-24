
#include <loci.h>

int __fastcall__ map_tune (unsigned char delay)
{
    mia_set_ax (delay);
    return mia_call_int_errno (MIA_OP_MAPTUNE);
}
