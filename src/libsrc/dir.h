/*
** Internal include file, do not use directly.
** Written by Ullrich von Bassewitz. Based on code by Groepaz.
*/



#ifndef _DIR_H
#define _DIR_H



#include "dirent.h"



/*****************************************************************************/
/*                                   Data                                    */
/*****************************************************************************/



struct DIR {
    int         fd;             /* File descriptor for directory */
    unsigned    off;            /* Current byte offset in directory */
    char        name[256];      /* Name passed to opendir */
};

/* End of dir.h */
#endif



