#include <loci.h>
#include "keyboard.h"
#include "tui.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "libsrc/dir.h"
#include "libsrc/dirent.h"

const char txt_title[] = "Loci ROM dev " __DATE__;
const char txt_menu[] = "Select";
const char txt_mdisc[] = "Microdisc";
const char txt_df0[] = "  A:";
const char txt_df1[] = "  B:";
const char txt_df2[] = "  C:";
const char txt_df3[] = "  D:";
const char txt_tape[] = "Cassette";
const char txt_tap[] = "tap:";
const char txt_mouse[] = "Mouse";
const char txt_empty[] = " ";
const char txt_x[] = "[x]";
const char txt_on[] = "\x14on  \x10";
const char txt_off[] = "\x11off \x10";
const char txt_alt[] = "\011";
const char txt_usb[] = "\x09\x04#%&()";
const char txt_rew[] = "*";
const char txt_ffw[] = "+";
const char txt_cnt[] = "034";
const char txt_eject[] = "\011,";
const char txt_locked[] = "!";
const char txt_unlocked[] = "\"";
const char txt_warn_sign[] = "\x01!\x03";
const char txt_dir_warning[] = "Max files. Use filter";

#define DIR_BUF_SIZE 2048
char dir_buf[DIR_BUF_SIZE];
char** dir_ptr_list = (char **)&dir_buf[DIR_BUF_SIZE];  //Reverse array
unsigned int dir_entries;
int dir_offset;
char dir_lpage[2];
char dir_rpage[2];
uint8_t dir_needs_refresh;
#define DIR_PAGE_SIZE 24

#define PATH_SIZE 128
char path[PATH_SIZE];

tui_widget ui[] = {
    { TUI_START, 1, 0, 0, 0 },
    //{ TUI_BOX,  39,28, 0, 0 },
    { TUI_TXT,   1, 0, 40, txt_title },
    { TUI_TXT,   1, 2,10, txt_mdisc }, { TUI_SEL, 12, 2, 6, txt_off },
    { TUI_TXT,   3, 3, 4, txt_df0 }, { TUI_SEL,   8, 3,26, txt_empty },
    { TUI_TXT,   3, 4, 4, txt_df1 }, { TUI_SEL,   8, 4,26, txt_empty },
    { TUI_TXT,   3, 5, 4, txt_df2 }, { TUI_SEL,   8, 5,26, txt_empty },
    { TUI_TXT,   3, 6, 4, txt_df3 }, { TUI_SEL,   8, 6,26, txt_empty },
    { TUI_TXT,   1, 8,10, txt_tape },{ TUI_SEL, 12, 8, 6, txt_off },
    { TUI_TXT,   3, 9, 4, txt_tap }, { TUI_SEL,   8, 9,18, txt_empty },
    { TUI_TXT,   1,11,10, txt_mouse }, { TUI_SEL, 12,11, 6, txt_off },
    { TUI_TXT,  32, 0, 7, txt_usb},

    { TUI_TXT,  29,  9, 1, txt_alt},
    { TUI_SEL,  30,  9, 1, txt_rew},
    { TUI_TXT,  32,  9, 3, txt_cnt},
    { TUI_SEL,  36,  9, 1, txt_ffw},
    { TUI_SEL,  35,  3, 2, txt_eject},
    { TUI_SEL,  35,  4, 2, txt_eject},
    { TUI_SEL,  35,  5, 2, txt_eject},
    { TUI_SEL,  35,  6, 2, txt_eject},
    { TUI_SEL,  38,  3, 1, txt_unlocked},
    { TUI_SEL,  38,  4, 1, txt_locked},
    { TUI_SEL,  38,  5, 1, txt_locked},
    { TUI_SEL,  38,  6, 1, txt_unlocked},
    { TUI_TXT,  37,  9, 1, txt_alt},
    { TUI_SEL,  38,  9, 1, txt_locked},
    { TUI_END,   0, 0, 0, 0 }
};
#define IDX_FDC_ON 3
#define IDX_DF0 5
#define IDX_DF1 7
#define IDX_DF2 9
#define IDX_DF3 11
#define IDX_TAP_ON 13
#define IDX_TAP 15
#define IDX_MOU_ON 17

#define POPUP_FILE_START 6
tui_widget popup[32] = {
    { TUI_START, 2, 2, 0, 0 },
    { TUI_BOX,  35,26, 0, 0 },
    { TUI_TXT,   1, 0,30, path},
    { TUI_SEL,  31, 0, 3, txt_x},
    { TUI_TXT,  30,25, 1, dir_lpage},
    { TUI_TXT,  31,25, 1, dir_rpage},
    { TUI_END,   0, 0, 0, 0 }
};
#define IDX_XPAGE 3
#define IDX_LPAGE 4
#define IDX_RPAGE 5

tui_widget warning[] = {
    { TUI_START, 4,10, 0, 0},
    { TUI_BOX,  32, 3, 0, 0},
    { TUI_INV,   1, 1,  3, txt_warn_sign},
    { TUI_TXT,   5, 1, 25, txt_dir_warning},
    { TUI_END,   0, 0, 0, 0}
};

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
        return 1;
    }

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
    closedir(dir);
    qsort(&dir_ptr_list[-(dir_entries)], dir_entries, sizeof(char*), dir_cmp);
    dir_needs_refresh = 0;
    return ret;
}

void parse_files_to_widget(void){
    uint8_t i;
    char** dir_idx;
    tui_widget* popupf;

    //Directory page out-of-bounds checks
    if(dir_offset >= dir_entries){
        dir_offset -= DIR_PAGE_SIZE;
    }
    if(dir_offset < 0){
        dir_offset = 0;
    }
    dir_idx = &dir_ptr_list[-(dir_entries-dir_offset)]; //(char**)(dir_ptr_list - dir_entries + offset);
    popupf = &popup[POPUP_FILE_START]; //(tui_widget*)(popup + POPUP_FILE_START);

    for(i=0; (i < DIR_PAGE_SIZE) && ((i+dir_offset) < dir_entries); i++){
        popupf[i].type = TUI_SEL;
        popupf[i].x = 1;
        popupf[i].y = i+1;
        popupf[i].len = 31;
        popupf[i].data = dir_idx[i]; //dir_ptr_list[-(dir_entries-offset-i)];
    }

    popupf[i].type = TUI_END;
    popupf[i].x = 0;
    popupf[i].y = 0;
    popupf[i].len = 0;
    popupf[i].data = 0;

    dir_lpage[0] = '-';
    dir_rpage[0] = '-';
    popup[IDX_LPAGE].type = TUI_TXT;
    popup[IDX_RPAGE].type = TUI_TXT;
    if(dir_offset > 0){
        dir_lpage[0] = '<';
        popup[IDX_LPAGE].type = TUI_SEL;
    }
    if(dir_offset+DIR_PAGE_SIZE < dir_entries){
        dir_rpage[0] = '>';
        popup[IDX_RPAGE].type = TUI_SEL;
    }
    
}

extern void init_display(void);

int8_t calling_widget = -1;
struct _loci_cfg {
    uint8_t fdc_on;
    uint8_t tap_on;
    uint8_t mou_on;
    uint8_t b11_on;
} loci_cfg = { 0x00, 0x00, 0x00, 0x01 };

void DisplayKey(unsigned char key)
{
    static unsigned char y = 0;
    char* screen, oscreen;
    const char* tmp_ptr;
    char* ret;
    int drive;
    static uint8_t dir_ok = 1;
    screen = (char*)(0xbb80+40*20+1);
    oscreen = (char*)(0xbb80+40*21);
    //screen[0] = 16+7;
    //screen[1] = 0+4;
    switch(key){
        case(KEY_DELETE):
            screen[0 + --y] = ' ';
            break;
        case(KEY_UP):
            tui_prev_active();
            break;
        case(KEY_LEFT):
            if(calling_widget != -1 && dir_ok){
                dir_offset -= DIR_PAGE_SIZE;
                parse_files_to_widget();
                tui_draw(popup);
            }
            break;
        case(KEY_DOWN):
            tui_next_active();
            break;
        case(KEY_RIGHT):
            if(calling_widget != -1 && dir_ok){
                dir_offset += DIR_PAGE_SIZE;
                parse_files_to_widget();
                tui_draw(popup);
            }
            break;
        case(KEY_SPACE):
            if(!dir_ok){
                dir_ok = 1;
                tui_clear_box(1);
                tui_draw(popup);
                break;
            }
            if(calling_widget == -1){
                switch(tui_get_current()){
                    case(IDX_FDC_ON):
                        if(loci_cfg.fdc_on){
                            loci_cfg.fdc_on = 0x00;
                            tui_set_data(IDX_FDC_ON,txt_off);
                        }else{
                            loci_cfg.fdc_on = 0x01;
                            tui_set_data(IDX_FDC_ON,txt_on);
                        }
                        tui_draw_widget(IDX_FDC_ON);
                        tui_toggle_highlight(IDX_FDC_ON);
                        break;
                    case(IDX_MOU_ON):
                        if(loci_cfg.mou_on){
                            loci_cfg.mou_on = 0x00;
                            tui_set_data(IDX_MOU_ON,txt_off);
                        }else{
                            loci_cfg.mou_on = 0x01;
                            tui_set_data(IDX_MOU_ON,txt_on);
                        }
                        tui_draw_widget(IDX_MOU_ON);
                        tui_toggle_highlight(IDX_MOU_ON);
                        break;
                    case(IDX_TAP_ON):
                        if(loci_cfg.tap_on){
                            loci_cfg.tap_on = 0x00;
                            tui_set_data(IDX_TAP_ON,txt_off);
                        }else{
                            loci_cfg.tap_on = 0x01;
                            tui_set_data(IDX_TAP_ON,txt_on);
                        }
                        tui_draw_widget(IDX_TAP_ON);
                        tui_toggle_highlight(IDX_TAP_ON);
                        break;
                    case(IDX_DF0):
                    case(IDX_DF1):
                    case(IDX_DF2):
                    case(IDX_DF3):
                    case(IDX_TAP):
                        calling_widget = tui_get_current();
                        tui_toggle_highlight(calling_widget);
                        dir_ok = dir_fill(path);
                        parse_files_to_widget();
                        tui_draw(popup);
                        if(!dir_ok){
                            tui_draw(warning);
                        }
                        break;
                }
            }else{
                switch(tui_get_current()){
                    //Control elements selected
                    case(IDX_XPAGE):
                        tui_clear_box(1);
                        tui_draw(ui);
                        tui_set_current(calling_widget);
                        calling_widget = -1;
                        break;
                    case(IDX_LPAGE):
                        dir_offset -= DIR_PAGE_SIZE;
                        parse_files_to_widget();
                        tui_draw(popup);
                        break;
                    case(IDX_RPAGE):
                        dir_offset += DIR_PAGE_SIZE;
                        parse_files_to_widget();
                        tui_draw(popup);
                        break;
                    default:
                        //Selection from the list
                        tmp_ptr = tui_get_data(tui_get_current());
                        if(tmp_ptr[0]=='/' || tmp_ptr[0]=='['){    //Directory or device selection
                            if(tmp_ptr[0]=='['){
                                path[0] = tmp_ptr[1];
                                path[1] = tmp_ptr[2];
                                path[2] = 0x00;
                            }else if(tmp_ptr[1]=='.'){              //Go back down (/..)
                                if((ret = strrchr(path,'/')) != NULL){
                                    ret[0] = 0x00;
                                }else{
                                    path[0] = 0x00;
                                }
                            }else{
                                strncat(path,tmp_ptr,128-strlen(path));
                            }
                            dir_needs_refresh = 1;
                            dir_ok = dir_fill(path);
                            parse_files_to_widget();
                            tui_draw(popup);
                            if(!dir_ok){
                                tui_draw(warning);
                            }
                            break;
                        }
                        //File selection
                        tmp_ptr = tmp_ptr + 1;      //adjust for leading space
                        switch(calling_widget){
                            case(IDX_DF0):
                                drive = 0;
                                break;
                            case(IDX_DF1):
                                drive = 1;
                                break;
                            case(IDX_DF2):
                                drive = 2;
                                break;
                            case(IDX_DF3):
                                drive = 3;
                                break;
                            case(IDX_TAP):
                                drive = 4;
                                break;
                        }
                        mount(drive,path,tmp_ptr);
                        tui_clear_box(1);
                        tui_draw(ui);
                        tui_clear_txt(calling_widget);
                        tui_set_data(calling_widget,tmp_ptr);
                        tui_draw_widget(calling_widget);
                        tui_set_current(calling_widget);
                        calling_widget = -1;
                        if(drive < 4){
                            loci_cfg.fdc_on = 0x01;
                            tui_set_data(IDX_FDC_ON,txt_on);
                            tui_draw_widget(IDX_FDC_ON);
                        }else{
                            loci_cfg.tap_on = 0x01;
                            tui_set_data(IDX_TAP_ON,txt_on);
                            tui_draw_widget(IDX_TAP_ON);
                        }
                }
            }
            break;
        case(KEY_RETURN):
            screen[y++] = 0x00;
            dir_fill(screen);
            //write(STDOUT_FILENO, screen, y);
            //write(STDOUT_FILENO, '\n', 1);
            //read(STDIN_FILENO, oscreen, 280);
            break;
        case(KEY_ESCAPE):
            if(!dir_ok){
                dir_ok = 1;
                tui_clear_box(1);
                tui_draw(popup);
                break;
            }
            if(calling_widget == -1){   //Return to Oric
                mia_set_ax(0x80 | (loci_cfg.b11_on <<2) | (loci_cfg.tap_on <<1) | loci_cfg.fdc_on);
                //mia_set_ax(0x00 | (loci_cfg.b11_on <<2) | (loci_cfg.tap_on <<1) | loci_cfg.fdc_on);
                VIA.ier = 0x7F;         //Disable VIA interrupts
                mia_call_int_errno(MIA_OP_BOOT);
            }else{                      //Escape from popup
                tui_clear_box(1);
                tui_draw(ui);
                tui_set_current(calling_widget);
                calling_widget = -1;
            }
            break;

        default:
            screen[ 0 + y++] = key;
    }

    if(y>35) 
        y = 0;
}

unsigned char Mouse(unsigned char key){
    static uint16_t prev_pos = 0;
    static int8_t sx = 0, sy = 0;
    static uint8_t  prev_x = 0, prev_y = 0, prev_btn = 0, cursor = 0; 
    
    uint16_t pos;
    uint8_t x,y,btn;
    char *screen;
    uint8_t widget;
    
    screen = TUI_SCREEN;
    MIA.addr0 = 0x8000;
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
        if(cursor)
            screen[prev_pos] ^= 0x80;
        cursor = 1;
        screen[pos] ^= 0x80;
        prev_pos = pos;
    }
    prev_x = x;
    prev_y = y;
    
    if(((btn ^ prev_btn) & btn & 0x01)){  //Left mouse button release
        widget = tui_hit(sx,sy);
        if(widget){
            cursor = 0;
            tui_set_current(widget);
            key = KEY_SPACE;
        }
    } 
    prev_btn = btn;
    return key;
}

void main(void){
    xreg_mia_mouse(0x8000);
    init_display();
    tui_cls(3);
    tui_draw(ui);
    strncpy(path,"",128);
    dir_needs_refresh = 1;
    //dir_fill(path);
    //parse_files_to_widget();
    //tui_draw(popup);
    //tui_draw_box(10,28);
    while(1){
        unsigned char key = ReadKeyNoBounce();
        key = Mouse(key);
        if(key)
            DisplayKey(key);
        while(VIA.t1_hi);
        while(!VIA.t1_hi);
        while(!VIA.t1_hi);
    }
}
