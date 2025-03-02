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

#define TTY // minimal conio-raw
#include "../../simple6502js/65lisp/conio-raw.c"

extern void init_display();

extern uint8_t irq_ticks;
#pragma zpsym ("irq_ticks")

char filter[6] = ".dsk";

bool return_possible;

struct _loci_cfg loci_cfg;

int dir_cmp(const void *lhsp, const void *rhsp) {
  const char *lhs = *((const char**)lhsp);
  const char *rhs = *((const char**)rhsp);
  int cmp;

  cmp = stricmp(lhs, rhs);

  //Sort dirs before files by inverting result
  return (lhs[0] != rhs[0])? -cmp: +cmp;
}

int dir(char* dname){
  DIR* dir;
  struct dirent* fil;
  int ret= 0;

  dir = opendir(dname);
  if (dname[0]==0x00) return 0; // Root/device list

  // Non-root
  //strcpy(dir_buf, "/..");
  //tail = 4; //strlen("/..")+1; // jsk: hmmm?

  while(1) {

    // - Skip hidden files
    do {
      fil = readdir(dir);
    } while(fil->d_name[0]=='.');

    // End of directory listing
    if (fil->d_name[0] == 0) break;

    // dir? sys? file?
    if (fil->d_attrib & DIR_ATTR_DIR){
      printf(" DIR:");
    } else if (fil->d_attrib & DIR_ATTR_SYS){
      printf(" ["); // TODO(jsk): ??
    } else if (filter[0] && !strcasestr(fil->d_name, filter)) {
      continue;
    }

    printf("%s ", fil->d_name);
    ++ret;
  }
  closedir(dir);

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

  if(do_return)
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

// HMMM?
/*                 case(IDX_TAP): */
/*                     drive = 4; */
/*                     break; */
/*                 case(IDX_ROM_FILE): */
/*                     drive = 5; */
/*                     break; */
/*                 case(IDX_TAP_CNT): */
/*                     drive = 6;      //Pseudo drive */
/*             } */
/*             if(drive<6) */
/*                 if(mount(drive,loci_cfg.path,tmp_ptr)==0x00){ */
/*                     loci_cfg.mounts |= 1u << drive; */
/*                 }else{ */
/*                     loci_cfg.mounts &= ~1u << drive; */
/*                 } */


/*                 tap_seek(*((long*)(tmp_ptr-4-1)));  //Seek to start of header */

/*             if(drive<6){ */
/*                 strncpy(loci_cfg.drv_names[drive],tmp_ptr,32); */
/*                 tui_set_data(calling_widget,loci_cfg.drv_names[drive]); */

/*             if(drive < 4){ */
/*                 loci_cfg.fdc_on = 0x01; */
/*             if(drive == 4){ */
/*                 loci_cfg.tap_on = 0x01; */
/*                 loci_cfg.ald_on = 0x01; */
/*             if(drive == 6){ */
/*                 update_tap_counter(); */
//if(strisquint(screen,filter)){ /// ?
//    DBG_STATUS("HIT ");
//}else{
//    DBG_STATUS("MISS");
//}

// ESC
//              boot(false);

//                                  file_copy(tmp_str,loci_cfg.path);

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

void main(void){
  char key;

  init_display(); // inits loci font

  clrscr();
  printf("AtmOS v0.01 (c) Jonas S Karlsson, jsk@yesco.org");
    
  return_possible = mia_restore_buffer_ok();

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
    printf("\n\n");
    printf("Drive: a/b/c/d\n");
    printf("Tape: t/k/m\n");
    printf("ROM: o/r\n");
    printf("Filter: f/i(nstall)\n");
    printf("/ or ? unshifted...\n");
    putchar('\n');
    printf("> ");

    key = cgetc();
    key = Mouse(key);

    switch(key) {

      // - drive
    case 'a': case 'b': case 'c': case 'd':
      umount(key-'a'); // jsk: ???
      break;

      // - tape
    case 't':
      break;
    case 'k':
      tap_list();
      break;
    case 'm':
      break;

      // - drive
    case 'o':
      break;
    case 'r':
      break;

      // - etc
    case 'f':
      break;
    case 'i': // install
      break;
    case '/': case '?': // ? is unshifted /
      break;

    // - commands
    case 'l': // ls
      dir(loci_cfg.path);
      break;

    }
  }
}
