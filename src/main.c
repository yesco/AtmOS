#include <loci.h>
#include "tui.h"
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

char* dbg_status = TUI_SCREEN_XY_CONST(35,1);
#define DBG_STATUS(fourc) strcpy(dbg_status,fourc)

char tmp_str[256];

#define DIR_BUF_SIZE 3072
char dir_buf[DIR_BUF_SIZE];
char** dir_ptr_list = (char **)&dir_buf[DIR_BUF_SIZE];  //Reverse array
unsigned int dir_entries;
int dir_offset;
char dir_lpage[2];
char dir_rpage[2];
uint8_t dir_needs_refresh;
#define DIR_PAGE_SIZE 24

bool return_possible;

struct _loci_cfg loci_cfg;

extern void init_display(void);

uint8_t auto_tune_tior(void);
int dir_cmp(const void *lhsp,const void *rhsp);
uint8_t dir_fill(char* dname);
uint8_t tap_fill(void);
void parse_files_to_widget(void);
uint8_t update_dir_ui(void);
void boot(bool do_return);

unsigned char Mouse(unsigned char key);

// dir_cmp -- compare directory entries
int dir_cmp(const void *lhsp,const void *rhsp)
{
    const char *lhs = *((const char**)lhsp);
    const char *rhs = *((const char**)rhsp);
    int cmp;

    cmp = stricmp(lhs,rhs);

    //Sort dirs before files by inverting result
    if(lhs[0] != rhs[0]){
        return -cmp;
    }else{
        return cmp;
    }
}

/* Fill the directory buffer with filenames from the bottom
   and pointers from the top.
   Returns 0 if buffer becomes full before all dir entries are captured
*/
uint8_t dir_fill(char* dname){
    DIR* dir;
    struct dirent* fil;
    uint16_t tail;     //Filename buffer tail
    uint8_t ret;
    int len;

    if(!dir_needs_refresh){
    //    return 1;
    }
    //DBG_STATUS("odir");
    dir = opendir(dname);
    if(dname[0]==0x00){     //Root/device list
        tail = 0;
        dir_entries = 0;
    }else{                  //Non-root
        strcpy(dir_buf,"/..");
        dir_ptr_list[-1] = dir_buf;
        tail = 4; //strlen("/..")+1
        dir_entries = 1;
    }
    dir_offset = 0;
    ret = 1;
    //DBG_STATUS("rdir");
    while(tail < DIR_BUF_SIZE){             //Safeguard
        do {
            fil = readdir(dir);
        }while(fil->d_name[0]=='.');        //Skip hidden files
        if(fil->d_name[0] == 0){            //End of directory listing
            break;
        }
        dir_ptr_list[-(++dir_entries)] = &dir_buf[tail];  
        if(fil->d_attrib & DIR_ATTR_DIR){
            dir_buf[tail++] = '/';
        }else if(fil->d_attrib & DIR_ATTR_SYS){
            dir_buf[tail++] = '[';
        }else{
            if(filter[0]){
                if(!strcasestr(fil->d_name, filter)){
                    dir_entries--;  //roll-back
                    continue;       //next file
                }
            }
            dir_buf[tail++] = ' ';
        }
        len = strlen(fil->d_name);
        if(len > (DIR_BUF_SIZE-tail-(dir_entries*sizeof(char*)))){
            ret = 0;                     //Buffer is full
            dir_entries--;                //Rewind inclomplete entry
            break;
        }else{
            strcpy(&dir_buf[tail], fil->d_name);
            tail += len + 1;
        }
    }
    //DBG_STATUS("cdir");
    closedir(dir);
    //DBG_STATUS("    ");

    qsort(&dir_ptr_list[-(dir_entries)], dir_entries, sizeof(char*), dir_cmp);
    dir_needs_refresh = 0;
    return ret;
}

/* Reuse dir_buf for tape content listing */
uint8_t tap_fill(void){
    tap_header_t hdr;
    long counter;
    unsigned int start_addr, end_addr, size;
    dir_entries = 0;
    dir_offset = 0;
    
    TAP.cmd = TAP_CMD_REW;
                        
    //Using fixed 64 byte entries for tape content
    while((dir_entries < (DIR_BUF_SIZE/64))){
        counter = tap_read_header(&hdr);
        if(counter == -1)
            break;
        start_addr = (unsigned int)((hdr.start_addr_hi<<8) | hdr.start_addr_lo);
        end_addr =   (unsigned int)((hdr.end_addr_hi<<8)   | hdr.end_addr_lo);
        if(start_addr > end_addr)   //Bad/unsupported header
            size = 0;
        else
            size = end_addr - start_addr;
        *((long*)(&dir_buf[dir_entries*64])) = counter;
        sprintf(&dir_buf[(dir_entries*64) + 4]," %d %-12.12s %-3s $%04X %5db",
            dir_entries + 1,
            hdr.filename[0] ? (char*)hdr.filename : "<no name>",
            hdr.type == 0x80 ? "BIN" : "BAS",    //TODO Incomplete Type decode
            start_addr, 
            size
        );
        dir_ptr_list[-(dir_entries+1)] = &dir_buf[(dir_entries*64)+4];
        dir_entries++;
        //Seek over file on tape
        counter += sizeof(tap_header_t) + 4 + size;
        tap_seek(counter);
    }
    qsort(&dir_ptr_list[-(dir_entries)], dir_entries, sizeof(char*), dir_cmp);
    return dir_entries;
}

void parse_files_to_widget(void){
    uint8_t i;
    char** dir_idx;

    //Directory page out-of-bounds checks
    if(dir_offset >= dir_entries){
        dir_offset -= DIR_PAGE_SIZE;
    }
    if(dir_offset < 0){
        dir_offset = 0;
    }
    dir_idx = &dir_ptr_list[-(dir_entries-dir_offset)]; //(char**)(dir_ptr_list - dir_entries + offset);
    for(i=0; (i < DIR_PAGE_SIZE) && ((i+dir_offset) < dir_entries); i++){
      //widget->len = 34;
      //widget->data = dir_idx[i]; //dir_ptr_list[-(dir_entries-offset-i)];
      printf("\n%s\n", dir_idx[i]); // 34?
    }

    dir_lpage[0] = '-';
    dir_rpage[0] = '-';

    if(dir_offset > 0){
        dir_lpage[0] = '<';
    }
    if(dir_offset+DIR_PAGE_SIZE < dir_entries){
        dir_rpage[0] = '>';
    }
}

uint8_t update_dir_ui(void){
    uint8_t dir_ok;
    //dir_needs_refresh = true;
    dir_ok = dir_fill(loci_cfg.path);
    parse_files_to_widget();    
    return dir_ok;
}

int8_t calling_widget = -1;

void boot(bool do_return){
  //char* boot_text;
    if(do_return && !return_possible) return;
    printf("!boot: %s\n", do_return? "returning": "booting");
    //clrscr();

    persist_set_loci_cfg(&loci_cfg);
    persist_set_magic();
    mia_set_ax(0x80 | (loci_cfg.ald_on <<4) | (loci_cfg.bit_on <<3) | (loci_cfg.b11_on <<2) | (loci_cfg.tap_on <<1) | loci_cfg.fdc_on);
    //mia_set_ax(0x00 | (loci_cfg.b11_on <<2) | (loci_cfg.tap_on <<1) | loci_cfg.fdc_on);
    VIA.ier = 0x7F;         //Disable VIA interrupts

    if(do_return)
        mia_restore_state();
    else{
        mia_clear_restore_buffer();
        mia_call_int_errno(MIA_OP_BOOT);    //Only returns if boot fails
    }

    VIA.ier = 0xC0;

    //tui_cls(3);
    //clrscr();
    printf("\n%%DEBUG: !ROM\n");
}

void do_return() {
  //dir_ok = dir_fill(loci_cfg.path);
  parse_files_to_widget();
  boot(true);
}


void do_tap() {
  //if(calling_widget <= IDX_DF3)
  //strcpy(filter,".dsk");
  //else if(calling_widget == IDX_TAP)
  //strcpy(filter,".tap");
  //else
  //strcpy(filter,".rom");
  //popup[IDX_PATH].data = (char*)&loci_cfg.path;

  //dir_ok = update_dir_ui();

  // TODO: do_eject(4,IDX_TAP);
  // TODO: update_tap_counter();
  //case(IDX_ROM_FILE):
  // TODO:(jsk): do_eject(5,IDX_ROM_FILE);
  //if(dir_ok && idx > POPUP_FILE_START && loci_cfg.path[0]=='0'){
    //tmp_ptr = (char*)tui_get_data(idx);
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
/*         dir_ok = dir_fill(loci_cfg.path); */
/*         parse_files_to_widget(); */
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
                    /* case(IDX_TAP_CNT): */
                    /*     filter[0] = '\0'; */
                    /*     dir_ok = tap_fill(); */
                    /*     parse_files_to_widget(); */
                    /*     popup[IDX_PATH].data = (char*)&loci_cfg.drv_names[4]; */
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
                  //parse_files_to_widget();
//                  if(dir_entries)
//                            tui_set_current(POPUP_FILE_START);
                  //case(IDX_RPAGE):
//                        dir_offset += DIR_PAGE_SIZE;
                  //parse_files_to_widget();
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
//dir_ok = dir_fill(loci_cfg.path);
///parse_files_to_widget();
//tui_draw(popup);
//if(!dir_ok){
//                    tui_draw(warning);
//}
//}
//if(calling_widget == -1){
//                boot(true);
//            }
            //screen[y++] = 0x00;
            //dir_fill(screen);
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
    
    uint16_t pos;
    uint8_t x,y,btn;
    //char *screen;
    uint8_t widget;

    if(!loci_cfg.mou_on)
        return key;
    
    //screen = TUI_SCREEN;
    MIA.addr0 = 0x7000;
    MIA.step0 = 1;
    btn = MIA.rw0;
    x = MIA.rw0;
    y = MIA.rw0;
    sx = sx + (((int8_t)(x - prev_x))>>3);
    sy = sy + (((int8_t)(y - prev_y))>>3);
    if(sx >= TUI_SCREEN_W)
        sx = TUI_SCREEN_W-1;
    if(sx < 0)
        sx = 0;
    if(sy >= TUI_SCREEN_H)
        sy = TUI_SCREEN_H-1;
    if(sy < 0)
        sy = 0;
    pos = (TUI_SCREEN_W * sy) + sx;
    if(pos != prev_pos){
      //if(cursor) screen[prev_pos] ^= 0x80;
        cursor = 1;
        //screen[pos] ^= 0x80;
        prev_pos = pos;
        prev_x = x;
        prev_y = y;
    }
    
    if(((btn ^ prev_btn) & btn & 0x01)){  //Left mouse button release
        widget = tui_hit(sx,sy);
        if(widget){
            cursor = 0;
            tui_set_current(widget);
            key = ' ';//KEY_SPACE;
        }
    } 
    prev_btn = btn;
    return key;
}

void main(void){
    uint8_t i;

    //tui_cls(3);
    clrscr();
    //init_display();
    
    return_possible = mia_restore_buffer_ok();

    if(!persist_get_loci_cfg(&loci_cfg)){
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
        char key;
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
