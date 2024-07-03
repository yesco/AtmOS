/*
** Ullrich von Bassewitz, 2012-05-30. Based on code by Groepaz.
** Sodiumlightbaby 2024 CUmini version
*/

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "dir.h"
#include <loci.h>


DIR* __fastcall__ opendir (register const char* name)
{
    unsigned i;
    int ret;
    static DIR d;
    for (i = strlen(name); i;) {
        mia_push_char (((char*)name)[--i]);
    }
    ret = mia_call_int_errno (MIA_OP_OPENDIR);
    d.fd = ret;
    strcpy(d.name, name);
    d.off = 0;
    return &d;   
}



