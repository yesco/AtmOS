#include <loci.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "libsrc/dir.h"
#include "libsrc/dirent.h"
#include "persist.h"
#include "filemanager.h"
#include "keyboard.h"

#undef KEY_FUNCT
#undef KEY_LEFT
#undef KEY_RIGHT
#undef KEY_UP
#undef KEY_DOWN

#undef KEY_RCTRL
#undef KEY_LCTRL
#undef KEY_LSHIFT
#undef KEY_RSHIFT

#define TTY // minimal conio-raw
#include "../../simple6502js/65lisp/conio-raw.c"

// debug: write a char at last pos of screen
//   this is to determine current/last state
//   before crash?
#define DID(c) *(TEXTLAST)=(c)

extern void init_display();

extern uint8_t irq_ticks;
#pragma zpsym ("irq_ticks")

char filter[6] = ".dsk";

bool return_possible;

struct _loci_cfg loci_cfg;

char drive;

int dir_cmp(const void *lhsp, const void *rhsp) {
  const char *lhs = *((const char**)lhsp);
  const char *rhs = *((const char**)rhsp);
  int cmp;

  cmp = stricmp(lhs, rhs);

  //Sort dirs before files by inverting result
  return (lhs[0] != rhs[0])? -cmp: +cmp;
}

void putbytes(unsigned long n) {
  //putchar('('); putint((int)n); putchar(')');

  if (n<1024) { putint(n); return; }
  n/= 1024;
  //putchar('['); putint(n); putchar(']');
  if (n<1024) { putint(n); putchar('K'); return; }
  n/= 1024;
  //putchar('{'); putint(n); putchar('}');
  if (n<1024) { putint(n); putchar('M'); return; }
  n/= 1024;
  putint(n); putchar('G');
}

// TODO: add paging/return/space
int dir(char* dname, char format) {
  DIR* dir;
  struct dirent* fil;
  int ret= 0;
  char color= *WHITE;
  char * name, attr, exec;
  unsigned long bytes, sbytes= 0;

  dname= "1:";  // just showns directories?
  dname= "1:/";  // just shows directories?
  //dname= "/"; // TODO: 

  dir = opendir(dname);
  if (dname[0]==0x00) return 0; // Root/device list

  // Non-root
  // jsk: first entry added is ".." to go up? lol
  //strcpy(dir_buf, "/..");
  //tail = 4; //strlen("/..")+1; // jsk: hmmm?

  while(1) {

    // - Skip hidden files (TODO: enable with -a)
    do {
      fil = readdir(dir);
      name= fil->d_name;
    } while(name[0]=='.');

    // End of directory listing
    if (!name[0]) break;

    // color type of file
    color= *WHITE; // normal file

    attr= fil->d_attrib;
    exec= '-';
    if (attr & DIR_ATTR_DIR) color= *CYAN;
    else if (attr & DIR_ATTR_SYS) color= *YELLOW;

    //if (filter[0] && !strcasestr(fil->d_name, filter)) {
    ////continue;

    if (strcasestr(name, ".doc") ||
        strcasestr(name, ".txt") ||
        strcasestr(name, ".md") ||
        strcasestr(name, "read")) {
      color= *GREEN;
    } else if (strcasestr(name, ".rom") ||
               strcasestr(name, "locirom") ||
               strcasestr(name, ".com") ||
               strcasestr(name, ".tap") ||
               strcasestr(name, ".dsk") ||
               strcasestr(name, ".oxe") ||
               strcasestr(name, ".exe") ||
               strcasestr(name, ".bat") ||
               strcasestr(name, ".sh") ||
               strcasestr(name, ".osh") ||
               strcasestr(name, "rp6502")) {
      color= *RED; exec= 'x';
    }

    sbytes+= (bytes= fil->d_size & 0x00ffffffL);

    // print file entry
    switch(format) {

    case 'd': // dos style
      //  README  .TXT 
      // 0123456789012
      {
        char* ext= strchr(name, '.');
        if (curx>=3*12) putchar('\n');
        if (ext) *ext= 0;
        putchar(color);
        puts(name);
        // "tab"
        while(curx<40 && (curx%12)<12-4)
          putchar(' ');
        putchar('.');
        puts(ext);
        // "tab"
        while(curx<40 && (curx%12))
          putchar(' ');
      } break;
        
    case 'l': // ls -l
      // drwx- 1234567 <date> <time> <filename>
      // lrwx-                       link
      putchar((attr & DIR_ATTR_DIR)? 'd':
              (attr & DIR_ATTR_SYS)? 's':
              '-');
      putchar('r');
      putchar((attr & DIR_ATTR_RDO)? '-': 'w');
      putchar(exec);
      putchar('-');
      // LOL
      { char n= sprintf(curp, "%8lu", bytes);
        curp+= n;
        curx+= n;
      }
      putchar(color); puts(name); putchar('\n');
      break;

    case 0:
    default:
      if (curx && ((curx+7)/8)*8+1+strlen(name)+1+3+1>39) putchar('\n');
      else putchar('\t');

      putchar(color); puts(name);
      putchar(*WHITE); putbytes(bytes);
      break;
    }

    ++ret;
  }
  closedir(dir);
  puts("\n    "); putint(ret); puts(" files, ");
  putbytes(sbytes); puts(" bytes\n");

  return ret;
}

int tap_list(void){
  tap_header_t hdr;
  unsigned int start_addr, end_addr, size;
  int res= 0;

  //Using fixed 64 byte entries for tape content
  while(1) { // (dir_entries < (DIR_BUF_SIZE/64))){
    long counter = tap_read_header(&hdr);
    if (counter == -1) break;
    start_addr = (unsigned int)((hdr.start_addr_hi<<8) | hdr.start_addr_lo);
    end_addr =   (unsigned int)((hdr.end_addr_hi  <<8) | hdr.end_addr_lo);
    if (start_addr > end_addr) // Bad/unsupported header
      size = 0;
    else
      size = end_addr - start_addr;

    printf(" %-12.12s %-3s $%04X %5db",
           hdr.filename[0] ? (char*)hdr.filename : "<no name>",
           hdr.type == 0x80 ? "BIN" : "BAS",    //TODO Incomplete Type decode
           start_addr,
           size
           );
    ++res;

    // Seek over file on tape
    counter += sizeof(tap_header_t) + 4 + size; // TODO(jsk): 4?
    tap_seek(counter);
  }
  //qsort(&dir_ptr_list[-(dir_entries)], dir_entries, sizeof(char*), dir_cmp);
  return res;
}

int8_t calling_widget = -1;

void boot(bool do_return){
  //char* boot_text;
  if(do_return && !return_possible) return;
  printf("!boot: %s\n", do_return? "returning": "booting");

  persist_set_loci_cfg(&loci_cfg);
  persist_set_magic();
  mia_set_ax(0x80 | (loci_cfg.ald_on <<4) | (loci_cfg.bit_on <<3) | (loci_cfg.b11_on <<2) | (loci_cfg.tap_on <<1) | loci_cfg.fdc_on);
  //mia_set_ax(0x00 | (loci_cfg.b11_on <<2) | (loci_cfg.tap_on <<1) | loci_cfg.fdc_on);
  VIA.ier = 0x7F;         //Disable VIA interrupts

  if (do_return)
    mia_restore_state();
  else {
    mia_clear_restore_buffer();
    mia_call_int_errno(MIA_OP_BOOT);    //Only returns if boot fails
  }

  VIA.ier = 0xC0;

  // clrscr();
  printf("\n%%DEBUG: !ROM\n");
}

void do_return() { boot(true); }

// FILENAME: loci_cfg.path "/" tmp_ptr

// jsk: names are pushed backwards, putting / last
/*         tmp_ptr[0]='/'; */
/*         len = strlen(tmp_ptr); */
/*         do{ */
/*             mia_push_char(tmp_ptr[--len]); */
/*         }while(len); */
/*         tmp_ptr[0]=' '; */
/*         len = strlen(loci_cfg.path); */
/*         do{ */
/*             mia_push_char(loci_cfg.path[--len]); */
/*         }while(len); */
// ERROR
/*         if(mia_call_int_errno(MIA_OP_UNLINK)<0) */
/*             sprintf(TUI_SCREEN_XY_CONST(37,1),"%02x",errno); */
/*         dir_ok = dir(loci_cfg.path); */
/*         parse_files(); */

// space

//
/// what is this?
/*                 if(tmp_ptr[0]=='['){ */
/*                     loci_cfg.path[0] = tmp_ptr[1]; */
/*                     loci_cfg.path[1] = tmp_ptr[2]; */
/*                     loci_cfg.path[2] = 0x00; */

// jsk: generalize?
unsigned char Mouse(unsigned char key){
  static uint16_t prev_pos = 0;
  static int8_t sx = 0, sy = 0;
  static uint8_t  prev_x = 0, prev_y = 0, prev_btn = 0, cursor = 0; 
  uint8_t x,y,btn;

  if (!loci_cfg.mou_on) return key;
    
  // read mouse
  MIA.addr0 = 0x7000;
  MIA.step0 = 1;
  btn = MIA.rw0;
  x = MIA.rw0;
  y = MIA.rw0;

  // undraw move draw
  if (cursor) *SCREENXY(sx, sy) ^= 0x80;
  sx = sx + (((int8_t)(x - prev_x))>>3);
  sy = sy + (((int8_t)(y - prev_y))>>3);
  if (sx >= 40) sx= 39;
  if (sx < 0)   sx= 0;
  if (sy >= 25) sy= 25;
  if (sy < 0)   sy= 0;
  cursor= 1;
  *SCREENXY(sx, sy) ^= 0x80;
    
  if (((btn ^ prev_btn) & btn & 0x01)){  //Left mouse button release
//    if(widget){
//      cursor = 0;
//      tui_set_current(widget);
//      key = ' ';//KEY_SPACE;
//    }
  } 

  prev_btn= btn;
  return key;
}

void cd(char* path, char* cd) {
  while(*cd) {

    switch(*cd) {

    case '.':
      // up
      if (cd[1]=='.') {
        while(*path && path[strlen(path)-1]!='/') {
          path[strlen(path)-1]= 0;
        }
        cd+= 2;
        if (*cd=='/') continue;
      }
      break; // break; wft? ..foo?

    case '/': // root
      strcpy(path, cd);
      break;

    // case '[': ???
      break;

    default:
      // drive letter?
      if (cd[1]==':') {
        // TODO: drive
        break;
      }

      // TODO: directory (match?)
      break;
    }
  }
}

#ifdef FOO
// Returns: an mallocated string
//          or NULL on end of file
char* readline (const char *prompt) {
  return NULL;
}

char* fgets(char* s, int size, FILE *stream) {
}

ssize_t getline(char** lineptr, size_t* n, FILE* stream) {
}

ssize_t getdelim(char** lineptr, size_t* n, int delim, FILE* stream) {
}
#endif // FOO

// __LOCI_MIA:
// xstack
// errno_lo, errno_hi
// op
// irq
//    loci_cfg.drv_names[0][0] = 0x00;
//    loci_cfg.drv_names[1][0] = 0x00;
//    loci_cfg.drv_names[2][0] = 0x00;
//    loci_cfg.drv_names[3][0] = 0x00;
//    loci_cfg.drv_names[4][0] = 0x00;
//    loci_cfg.drv_names[5][0] = 0x00;

// getcwd_xram(unisnged buf, unsigned len)
// mount(drive, char* path, filename)
// umount(drive)

// long tap_seek(long pos)
// long tap_tell()
// long tap_read_header(tap_header* h)

// mia_restore_state()
// uchar mia_restore_buffer_ok()
// mia_clear_restore_bufffer()
// char mia_get_vmode()

void main(void){
  char key;

  DID('A');
  init_display(); // inits loci font

  DID('B');
  clrscr();
  printf("AtmOS v0.01 (c) Jonas S Karlsson, jsk@yesco.org\n\n");
  DID('C');
    
  InitKeyboard();
  DID('D');

  return_possible = mia_restore_buffer_ok();
  DID('E');

  if (!persist_get_loci_cfg(&loci_cfg)){
    loci_cfg.fdc_on = 0x00;
    loci_cfg.tap_on = 0x00;
    loci_cfg.bit_on = 0x00;
    loci_cfg.mou_on = 0x00;
    loci_cfg.b11_on = 0x01;
    loci_cfg.ser_on = 0x01;
    loci_cfg.ald_on = 0x00;
    loci_cfg.mounts = 0x00;
    loci_cfg.path[0] = 0x00;
    loci_cfg.drv_names[0][0] = 0x00;
    loci_cfg.drv_names[1][0] = 0x00;
    loci_cfg.drv_names[2][0] = 0x00;
    loci_cfg.drv_names[3][0] = 0x00;
    loci_cfg.drv_names[4][0] = 0x00;
    loci_cfg.drv_names[5][0] = 0x00;
    //loci_cfg.tui_pos = IDX_DF0;
  }
   
  while(1){
    DID('F');
    puts("\n\n");
    puts("Drive: a/b/c/d\n");
    puts("Tape: t/k/m\n");
    puts("ROM: o/r\n");
    puts("Filter: f/i(nstall)\n");
    puts("/ or ? unshifted...\n");

    putchar('\n');

    // TODO: read errno and print? (if not 0)

    puts("LOCI_CFG.path: ");
    puts(loci_cfg.path);
    putchar('>'); putchar(' ');

    DID('G');
    //key = cgetc(); // TODO: not working? reboots?
    do { key= ReadKeyNoBounce(); } while(!key);
    DID('H');
    //key = Mouse(key);
    clrscr();

    // TODO: scroll deletes first char on last line??
    putchar(key);
    putchar('['); putint(key); putchar(']');
    putchar('\n');
    //DID('W');
    //waitms(1000);

    switch(key) {

    // - drive
    //case 'a': case 'b': case 'c': case 'd':
      //umount(key-'a'); // jsk: ???

    // - tape
    case 't':
      drive= 4;
      //tap_seek(*((long*)(tmp_ptr-4-1))); // Seek to star t
      break;
    case 'k':
      tap_list();
      break;

    // - drive
    case 'o':
      drive= 5;
      break;
    case 'r':
      drive= 5;
      break;

    // - etc
    case '/': case '?': // ? is unshifted /
      break;

    // - commands
    case 'b': // boot
      boot(false);
      break;
    case 'e': // exit
    case CTRL+'C':
      do_return(); // lol
      break;

    case 'f': // "file" (set selected file)
      // TODO: read line and extract filename
      if (drive<6) {
        //strncpy(loci_cfg.drv_names[drive], tmp_ptr, 32);
        char tmp_ptr[32]= {0};
        if (mount(drive, loci_cfg.path, tmp_ptr)==0x00){
          loci_cfg.mounts |= 1u << drive;
        } else {
          loci_cfg.mounts &= ~1u << drive;
        }
      }

      if (drive<4) {
        loci_cfg.tap_on = 0;
        loci_cfg.ald_on = 0;
        loci_cfg.fdc_on = 0x01;
      } else if (drive == 4) {
        loci_cfg.tap_on = 0x01;
        loci_cfg.ald_on = 0x01;
        loci_cfg.fdc_on = 0;
      } else if (drive == 6) { // virtual
        //update_tap_counter();
      }

      break;

    // - unixy
    case 's': // clear Screen
      clrscr();
      break;

    case 'm': // mkdir
//#define MIA_OP_MKDIR 0x83
//#define MIA_OP_GETCWD 0x88
      break;

    case 'u': // umount / mount
//#define MIA_OP_MOUNT 0x90
//#define MIA_OP_UMOUNT 0x91
      break;

//    case 'd': // date
//#define MIA_OP_CLOCK_GETRES 0x10
//#define MIA_OP_CLOCK_GETTIME 0x11
//#define MIA_OP_CLOCK_SETTIME 0x12
//#define MIA_OP_CLOCK_GETTIMEZONE 0x13
//      break;

    case 'c': // cp
      //file_copy(dst, src);
      break;

    case 'd': // dir
      dir(loci_cfg.path, 'd');
      break;

    case 'l': // ls -l
      dir(loci_cfg.path, 'l');
      break;

    case 'p': // pwd
      { char i;
        for(i=0; i<=6; ++i) {
          putint(i); putchar(':'); putchar(' '); puts(loci_cfg.drv_names[i]); putchar('\n');
        }
        puts("Path: "); puts(loci_cfg.path); putchar('\n');
      } break;
    }

    DID('I');
  }
}
