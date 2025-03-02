#include <loci.h>
//#include "tui.h"
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

extern uint8_t irq_ticks;
#pragma zpsym ("irq_ticks")

char filter[6] = ".dsk";

uint8_t rv1;
uint8_t spin_cnt;

//#define DBG_STATUS(fourc) strcpy(dbg_status,fourc)

char tmp_str[256];

#define DIR_BUF_SIZE 3072
char dir_buf[DIR_BUF_SIZE];

char** dir_ptr_list = (char **)&dir_buf[DIR_BUF_SIZE];  //Reverse array
unsigned int dir_entries;
int dir_offset;
char dir_lpage[2];
char dir_rpage[2];
#define DIR_PAGE_SIZE 24

bool return_possible;

struct _loci_cfg loci_cfg;

int dir_cmp(const void *lhsp,const void *rhsp);
uint8_t dir(char* dname);
uint8_t tap_list(void);
void boot(bool do_return);

unsigned char Mouse(unsigned char key);

// dir_cmp -- compare directory entries
int dir_cmp(const void *lhsp,const void *rhsp)
{
  const char *lhs = *((const char**)lhsp);
  const char *rhs = *((const char**)rhsp);
  int cmp;

  cmp = stricmp(lhs, rhs);

  //Sort dirs before files by inverting result
  return (lhs[0] != rhs[0])? -cmp: +cmp;
}

uint8_t dir(char* dname){
  DIR* dir;
  struct dirent* fil;
  uint8_t ret;
  int len;

  dir = opendir(dname);
  if (dname[0]==0x00){     // Root/device list
    dir_entries = 0;
  } else {                  // Non-root
    strcpy(dir_buf,"/..");
    dir_ptr_list[-1] = dir_buf;
    //tail = 4; //strlen("/..")+1
    dir_entries = 1;
  }
  dir_offset = 0;
  ret = 1;
  //DBG_STATUS("rdir");
  while(1) { // tail < DIR_BUF_SIZE) { // Safeguard
    // Skip hidden files (jsk:why?)
    //do {
    //fil = readdir(dir);
    //} while(fil->d_name[0]=='.');

    if (fil->d_name[0] == 0) break; // End of directory listing

    //dir_ptr_list[-(++dir_entries)] = &dir_buf[tail];  

    if (fil->d_attrib & DIR_ATTR_DIR){
      //dir_buf[tail++] = '/';
      printf(" DIR:");
    } else if (fil->d_attrib & DIR_ATTR_SYS){
      //dir_buf[tail++] = '[';
      printf(" [");
    } else {
      if (filter[0]){
        if (!strcasestr(fil->d_name, filter)) {
          dir_entries--;
          printf("NOT:");
          continue; 
        }
      }
      //dir_buf[tail++] = ' ';
    }

    len = strlen(fil->d_name);
    printf("%s ", fil->d_name);
  }
  closedir(dir);
  //qsort(&dir_ptr_list[-(dir_entries)], dir_entries, sizeof(char*), dir_cmp);
  return ret;
}

uint8_t tap_list(void){
  tap_header_t hdr;
  long counter;
  unsigned int start_addr, end_addr, size;
    
  //Using fixed 64 byte entries for tape content
  while(1) { // (dir_entries < (DIR_BUF_SIZE/64))){
    counter = tap_read_header(&hdr);
    if (counter == -1) break;
    start_addr = (unsigned int)((hdr.start_addr_hi<<8) | hdr.start_addr_lo);
    end_addr =   (unsigned int)((hdr.end_addr_hi<<8)   | hdr.end_addr_lo);
    if (start_addr > end_addr) // Bad/unsupported header
      size = 0;
    else
      size = end_addr - start_addr;

    //*((long*)(&dir_buf[dir_entries*64])) = counter;

    printf(//&dir_buf[(dir_entries*64) + 4],
           " %-12.12s %-3s $%04X %5db",
           hdr.filename[0] ? (char*)hdr.filename : "<no name>",
           hdr.type == 0x80 ? "BIN" : "BAS",    //TODO Incomplete Type decode
           start_addr,
           size
           );
    //dir_ptr_list[-(dir_entries+1)] = &dir_buf[(dir_entries*64)+4];
    //dir_entries++;
    //Seek over file on tape
    counter += sizeof(tap_header_t) + 4 + size;
    tap_seek(counter);
  }
  //qsort(&dir_ptr_list[-(dir_entries)], dir_entries, sizeof(char*), dir_cmp);
  //return dir_entries;
  return 1;
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

  //tui_cls(3);
  //clrscr();
  printf("\n%%DEBUG: !ROM\n");
}

void do_return() {
  boot(true);
}

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
/*         //tui_draw(popup); */
/*     } */
/* }                           */
//}
//break;
//        case 0x82:
// space
//            if(calling_widget == -1){
/*     if(calling_widget <= IDX_DF3) */
/*         strcpy(filter,".dsk"); */
/*     else if(calling_widget == IDX_TAP) */
/*         strcpy(filter,".tap"); */
/*     else */
/*         strcpy(filter,".rom"); */
/*     popup[IDX_PATH].data = (char*)&loci_cfg.path; */
/*     dir_ok = update_dir_ui(); */
//case(IDX_EJECT_ROM):
//do_eject(5,IDX_ROM_FILE);
//case(IDX_TAP_REW):
//TAP.cmd = TAP_CMD_REW;
//update_tap_counter();
//case(IDX_TAP_FFW):
//TAP.cmd = TAP_CMD_FFW;
//nupdate_tap_counter();
//            }else{
//dir_offset -= DIR_PAGE_SIZE;
//parse_files();
//                  if(dir_entries)
//                            tui_set_current(POPUP_FILE_START);
//case(IDX_RPAGE):
//                        dir_offset += DIR_PAGE_SIZE;
//parse_files();
//                  case(IDX_FILTER):
//                        tmp_ptr = (char*)tui_get_data(idx);
//                        len = strlen(tmp_ptr);
//                        if(len < (tui_get_len(idx)-1)){
//                            tmp_ptr[len] = key;
//                            tmp_ptr[len+1] = '\0';
/*       default: */
/*            //Selection from the list */
/*             tmp_ptr = (char*)tui_get_data(tui_get_current()); */
/*             if(tmp_ptr[0]=='/' || tmp_ptr[0]=='['){    //Directory or device selection */

// HMMM?
/*                 if(tmp_ptr[0]=='['){ */
/*                     loci_cfg.path[0] = tmp_ptr[1]; */
/*                     loci_cfg.path[1] = tmp_ptr[2]; */
/*                     loci_cfg.path[2] = 0x00; */

// HMMM?
/*                 }else if(tmp_ptr[1]=='.'){              //Go back down (/..) */
/*                     if((ret = strrchr(loci_cfg.path,'/')) != NULL){ */
/*                         ret[0] = 0x00; */
/*                     }else{ */
/*                         loci_cfg.path[0] = 0x00; */
/*                     } */
/*                 }else{ */
/*                     strncat(loci_cfg.path,tmp_ptr,256-strlen(loci_cfg.path)); */
/*                 } */
/*                 dir_ok = update_dir_ui(); */
/*                 break; */
/*             } */

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
/*             else */
/*                 tap_seek(*((long*)(tmp_ptr-4-1)));  //Seek to start of header */

/*             if(drive<6){ */
/*                 strncpy(loci_cfg.drv_names[drive],tmp_ptr,32); */
/*                 tui_set_data(calling_widget,loci_cfg.drv_names[drive]); */
/*             if(drive < 4){ */
/*                 loci_cfg.fdc_on = 0x01; */
/*             if(drive == 4){ */
/*                 loci_cfg.tap_on = 0x01; */
/*                 loci_cfg.ald_on = 0x01; */
/*                 update_load_btn(); */
/*                 update_tap_counter(); */
/*             if(drive == 6){ */
/*                 update_tap_counter(); */
//        case(KEY_RETURN):
//if(tui_get_type(tui_get_current()) == TUI_INP){
//dir_ok = dir(loci_cfg.path);
///parse_files();
//tui_draw(popup);
//if(!dir_ok){
//                    tui_draw(warning);
//}
//}
//if(calling_widget == -1){
//                boot(true);
//            }
//screen[y++] = 0x00;
//dir(screen);
//if(strisquint(screen,filter)){
//    DBG_STATUS("HIT ");
//}else{
//    DBG_STATUS("MISS");
//}
//write(STDOUT_FILENO, screen, y);
//write(STDOUT_FILENO, '\n', 1);
//read(STDIN_FILENO, oscreen, 280);
//break;
//case 0x83:
//case(KEY_ESCAPE):
//            if(!dir_ok){
//                dir_ok = 1;
//                tui_clear_box(1);
//                tui_draw(popup);
//                break;
//            }
//            if(calling_widget == -1){   //Return to Oric
//              boot(false);

//        default:
//                len = strlen(tmp_ptr);
//                if(len < (tui_get_len(idx)-1)){
//                    tmp_ptr[len] = key;
///                    tmp_ptr[len+1] = '\0';
//                    tui_draw_widget(idx);
//                    tui_toggle_highlight(idx);
//                }

//---Main menu keyboard shortcuts

// ???

//                                    tmp_ptr[0] = '/';
//                                    len = strlen(loci_cfg.path);
//                                   strncat(loci_cfg.path,tmp_ptr,256-len);
//                                    tmp_str[0] = '0';
//                                    tmp_str[1] = ':';
//                                    strcpy(&tmp_str[2],tmp_ptr);
//                                    strcpy(dbg_status,"COPY");
//                                  file_copy(tmp_str,loci_cfg.path);
//                                    strcpy(dbg_status,"    ");
//                                    tmp_ptr[0] = ' ';
//                                    loci_cfg.path[len] = '\0';
//                                }
//                                break;

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

    }
  }
}
