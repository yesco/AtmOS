/*****************************************************************************/
/*                                                                           */
/*                                   loci.h                                  */
/*                                                                           */
/*                           LOCI by Sodiumlightbaby                         */
/*                    Derived from rp6502.h by Rumbledethumps                */
/*                                                                           */
/* This software is provided 'as-is', without any expressed or implied       */
/* warranty.  In no event will the authors be held liable for any damages    */
/* arising from the use of this software.                                    */
/*                                                                           */
/* Permission is granted to anyone to use this software for any purpose,     */
/* including commercial applications, and to alter it and redistribute it    */
/* freely, subject to the following restrictions:                            */
/*                                                                           */
/* 1. The origin of this software must not be misrepresented; you must not   */
/*    claim that you wrote the original software. If you use this software   */
/*    in a product, an acknowledgment in the product documentation would be  */
/*    appreciated but is not required.                                       */
/* 2. Altered source versions must be plainly marked as such, and must not   */
/*    be misrepresented as being the original software.                      */
/* 3. This notice may not be removed or altered from any source              */
/*    distribution.                                                          */
/*                                                                           */
/*****************************************************************************/

#ifndef _LOCI_H
#define _LOCI_H

/* Oric VIA $0300-$030F */

#include <_6522.h>
#define VIA (*(volatile struct __6522 *)0x0300)

/* LOCI MIA $03A0-$03BA */

struct __LOCI_MIA
{
    const unsigned char ready;
    unsigned char tx;
    const unsigned char rx;
    const unsigned char vsync;
    unsigned char rw0;
    unsigned char step0;
    unsigned int addr0;
    unsigned char rw1;
    unsigned char step1;
    unsigned int addr1;
    unsigned char xstack;
    unsigned char errno_lo;
    unsigned char errno_hi;
    unsigned char op;
    unsigned char irq;
    const unsigned char spin;
    const unsigned char busy;
    const unsigned char lda;
    unsigned char a;
    const unsigned char ldx;
    unsigned char x;
    const unsigned char rts;
    unsigned int sreg;
};
#define MIA (*(volatile struct __LOCI_MIA *)0x03A0)

#define MIA_READY_TX_BIT 0x80
#define MIA_READY_RX_BIT 0x40
#define MIA_BUSY_BIT 0x80

/* XSTACK helpers */

void __fastcall__ mia_push_long (unsigned long val);
void __fastcall__ mia_push_int (unsigned int val);
#define mia_push_char(v) MIA.xstack = v

long mia_pop_long (void);
int mia_pop_int (void);
#define mia_pop_char() MIA.xstack

/* Set the MIA fastcall register */

void __fastcall__ mia_set_axsreg (unsigned long axsreg);
void __fastcall__ mia_set_ax (unsigned int ax);
#define mia_set_a(v) MIA.a = v

/* Run an OS operation */

int __fastcall__ mia_call_int (unsigned char op);
long __fastcall__ mia_call_long (unsigned char op);

/* These run _mappederrno() on error */

int __fastcall__ mia_call_int_errno (unsigned char op);
long __fastcall__ mia_call_long_errno (unsigned char op);

/* OS operation numbers */

#define MIA_OP_EXIT 0xFF
#define MIA_OP_ZXSTACK 0x00
#define MIA_OP_XREG 0x01
#define MIA_OP_PHI2 0x02
#define MIA_OP_CODEPAGE 0x03
#define MIA_OP_LRAND 0x04
#define MIA_OP_STDIN_OPT 0x05
#define MIA_OP_CLOCK_GETRES 0x10
#define MIA_OP_CLOCK_GETTIME 0x11
#define MIA_OP_CLOCK_SETTIME 0x12
#define MIA_OP_CLOCK_GETTIMEZONE 0x13
#define MIA_OP_OPEN 0x14
#define MIA_OP_CLOSE 0x15
#define MIA_OP_READ_XSTACK 0x16
#define MIA_OP_READ_XRAM 0x17
#define MIA_OP_WRITE_XSTACK 0x18
#define MIA_OP_WRITE_XRAM 0x19
#define MIA_OP_LSEEK 0x1A
#define MIA_OP_UNLINK 0x1B
#define MIA_OP_RENAME 0x1C

#define MIA_OP_OPENDIR 0x80
#define MIA_OP_CLOSEDIR 0x81
#define MIA_OP_READDIR 0x82

#define MIA_OP_MOUNT 0x90
#define MIA_OP_UMOUNT 0x91

#define MIA_OP_BOOT 0xA0
#define MIA_OP_MAPTUNE 0xA1

/* C API for the operating system. */

int __cdecl__ xreg (char device, char channel, unsigned char address, ...);
int phi2 (void);
int codepage (void);
long lrand (void);
int __fastcall__ stdin_opt (unsigned long ctrl_bits, unsigned char str_length);
int __fastcall__ read_xstack (void* buf, unsigned count, int fildes);
int __fastcall__ read_xram (unsigned buf, unsigned count, int fildes);
int __fastcall__ write_xstack (const void* buf, unsigned count, int fildes);
int __fastcall__ write_xram (unsigned buf, unsigned count, int fildes);

int __fastcall__ mount (int drive, register const char* path,register const char* filename);
int __fastcall__ umount (int drive);

int __fastcall__ map_tune (unsigned char delay);

/* XREG location helpers */

#define xreg_mia_keyboard(...) xreg(0, 0, 0, __VA_ARGS__)
#define xreg_mia_mouse(...) xreg(0, 0, 1, __VA_ARGS__)
//#define xreg_vga_canvas(...) xreg(1, 0, 0, __VA_ARGS__)
//#define xreg_vga_mode(...) xreg(1, 0, 1, __VA_ARGS__)

/* XRAM structure helpers */

#define xram0_struct_set(addr, type, member, val)                  \
    MIA.addr0 = (unsigned)(&((type *)0)->member) + (unsigned)addr; \
    switch (sizeof(((type *)0)->member))                           \
    {                                                              \
    case 1:                                                        \
        MIA.rw0 = val;                                             \
        break;                                                     \
    case 2:                                                        \
        MIA.step0 = 1;                                             \
        MIA.rw0 = val & 0xff;                                      \
        MIA.rw0 = (val >> 8) & 0xff;                               \
        break;                                                     \
    case 4:                                                        \
        MIA.step0 = 1;                                             \
        MIA.rw0 = (unsigned long)val & 0xff;                       \
        MIA.rw0 = ((unsigned long)val >> 8) & 0xff;                \
        MIA.rw0 = ((unsigned long)val >> 16) & 0xff;               \
        MIA.rw0 = ((unsigned long)val >> 24) & 0xff;               \
        break;                                                     \
    }

#define xram1_struct_set(addr, type, member, val)                  \
    MIA.addr1 = (unsigned)(&((type *)0)->member) + (unsigned)addr; \
    switch (sizeof(((type *)0)->member))                           \
    {                                                              \
    case 1:                                                        \
        MIA.rw1 = val;                                             \
        break;                                                     \
    case 2:                                                        \
        MIA.step1 = 1;                                             \
        MIA.rw1 = val & 0xff;                                      \
        MIA.rw1 = (val >> 8) & 0xff;                               \
        break;                                                     \
    case 4:                                                        \
        MIA.step1 = 1;                                             \
        MIA.rw1 = (unsigned long)val & 0xff;                       \
        MIA.rw1 = ((unsigned long)val >> 8) & 0xff;                \
        MIA.rw1 = ((unsigned long)val >> 16) & 0xff;               \
        MIA.rw1 = ((unsigned long)val >> 24) & 0xff;               \
        break;                                                     \
    }

#define xram0_struct_get(addr, type, member, val)                  \
    MIA.addr0 = (unsigned)(&((type *)0)->member) + (unsigned)addr; \
    switch (sizeof(((type *)0)->member))                           \
    {                                                              \
    case 1:                                                        \
        val = MIA.rw0;                                             \
        break;                                                     \
    case 2:                                                        \
        MIA.step0 = 1;                                             \
        (unsigned char*)(&val)   = MIA.rw0;                        \
        (unsigned char*)(&val+1) = MIA.rw0;                        \
        break;                                                     \
    case 4:                                                        \
        MIA.step0 = 1;                                             \
        (unsigned char*)(&val)   = MIA.rw0;                        \
        (unsigned char*)(&val+1) = MIA.rw0;                        \
        (unsigned char*)(&val+2) = MIA.rw0;                        \
        (unsigned char*)(&val+3) = MIA.rw0;                        \
        break;                                                     \
    }

#define xram1_struct_get(addr, type, member, val)                  \
    MIA.addr1 = (unsigned)(&((type *)0)->member) + (unsigned)addr; \
    switch (sizeof(((type *)0)->member))                           \
    {                                                              \
    case 1:                                                        \
        val = MIA.rw1;                                             \
        break;                                                     \
    case 2:                                                        \
        MIA.step1 = 1;                                             \
        (unsigned char*)(&val)   = MIA.rw1;                        \
        (unsigned char*)(&val+1) = MIA.rw1;                        \
        break;                                                     \
    case 4:                                                        \
        MIA.step1 = 1;                                             \
        (unsigned char*)(&val)   = MIA.rw1;                        \
        (unsigned char*)(&val+1) = MIA.rw1;                        \
        (unsigned char*)(&val+2) = MIA.rw1;                        \
        (unsigned char*)(&val+3) = MIA.rw1;                        \
        break;                                                     \
    }

/* Values in __oserror are the union of these FatFs errors and errno.h */

typedef enum
{
    FR_OK = 32,             /* Succeeded */
    FR_DISK_ERR,            /* A hard error occurred in the low level disk I/O layer */
    FR_INT_ERR,             /* Assertion failed */
    FR_NOT_READY,           /* The physical drive cannot work */
    FR_NO_FILE,             /* Could not find the file */
    FR_NO_PATH,             /* Could not find the path */
    FR_INVALID_NAME,        /* The path name format is invalid */
    FR_DENIED,              /* Access denied due to prohibited access or directory full */
    FR_EXIST,               /* Access denied due to prohibited access */
    FR_INVALID_OBJECT,      /* The file/directory object is invalid */
    FR_WRITE_PROTECTED,     /* The physical drive is write protected */
    FR_INVALID_DRIVE,       /* The logical drive number is invalid */
    FR_NOT_ENABLED,         /* The volume has no work area */
    FR_NO_FILESYSTEM,       /* There is no valid FAT volume */
    FR_MKFS_ABORTED,        /* The f_mkfs() aborted due to any problem */
    FR_TIMEOUT,             /* Could not get a grant to access the volume within defined period */
    FR_LOCKED,              /* The operation is rejected according to the file sharing policy */
    FR_NOT_ENOUGH_CORE,     /* LFN working buffer could not be allocated */
    FR_TOO_MANY_OPEN_FILES, /* Number of open files > FF_FS_LOCK */
    FR_INVALID_PARAMETER    /* Given parameter is invalid */
} FRESULT;

typedef enum
{
    LFS_ERR_OK          = 128 + 0,    // No error
    LFS_ERR_IO          = 128 + 5,   // Error during device operation
    LFS_ERR_CORRUPT     = 128 + 84,  // Corrupted
    LFS_ERR_NOENT       = 128 + 2,   // No directory entry
    LFS_ERR_EXIST       = 128 + 17,  // Entry already exists
    LFS_ERR_NOTDIR      = 128 + 20,  // Entry is not a dir
    LFS_ERR_ISDIR       = 128 + 21,  // Entry is a dir
    LFS_ERR_NOTEMPTY    = 128 + 39,  // Dir is not empty
    LFS_ERR_BADF        = 128 + 9,   // Bad file number
    LFS_ERR_FBIG        = 128 + 27,  // File too large
    LFS_ERR_INVAL       = 128 + 22,  // Invalid parameter
    LFS_ERR_NOSPC       = 128 + 28,  // No space left on device
    LFS_ERR_NOMEM       = 128 + 12,  // No more memory available
    LFS_ERR_NOATTR      = 128 + 61,  // No data/attr available
    LFS_ERR_NAMETOOLONG = 128 + 36   // File name too long
} LFS_RESULT;

#endif /* _LOCI_H */
