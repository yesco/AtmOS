#include "tui.h"
#include <string.h>


static tui_widget *tui_org_list;
static uint8_t tui_current;

void tui_fill(unsigned char len, unsigned char ch, char* buf);

void tui_draw(tui_widget* list){
    unsigned char i;
    for(i=0; list[i].type != TUI_END; i++){
        switch(list[i].type){
            case TUI_START:
                tui_org_list = list;
                tui_current = i;
                break;
            case TUI_NOP:
                break;
            default:
                tui_draw_widget(i);
                break;
        }
    }
}

void tui_draw_widget(uint8_t widget_idx){
    tui_widget* widget = &tui_org_list[widget_idx];
    unsigned char local_x = widget->x;
    unsigned char local_y = widget->y;
    unsigned char local_len = widget->len;
    switch(widget->type){
        case TUI_BOX:
            tui_draw_box(local_x, local_y);
            break;
        case TUI_TXT:
        case TUI_SEL:
            tui_draw_txt(local_x, local_y, (char *)(widget->data), local_len);
            break;
        case TUI_INV:
        case TUI_BTN:
            tui_draw_txt(local_x, local_y, (char *)(widget->data), local_len);
            tui_toggle_highlight(widget_idx);
            break;
        case TUI_INP:
            tui_clear_txt(widget_idx);
            tui_draw_txt(local_x, local_y, (char *)(widget->data), local_len);
            break;
        default:
            break;
    }
}

uint8_t tui_hit(uint8_t x, uint8_t y){
    uint8_t i;
    int8_t local_x = x - tui_org_list[0].x;
    int8_t local_y = y - tui_org_list[0].y;
    for(i=1; tui_org_list[i].type != TUI_END; i++){
        if( tui_org_list[i].type >= TUI_ACTIVE &&
            local_y == tui_org_list[i].y && 
            local_x >= tui_org_list[i].x && 
            local_x < (tui_org_list[i].len + tui_org_list[i].x)
        ){
            return i;
        }
    }
    return 0;
}


void tui_draw_clr(uint8_t w, uint8_t h){
    tui_widget* org = tui_org_list;
    uint8_t j;
    for(j = org->y; j < (org->y+h); j++){
        tui_fill(w,' ',TUI_SCREEN_XY(org->x,j));
    } 
}

void tui_clear_box(uint8_t widget_idx){
    tui_draw_clr(tui_org_list[widget_idx].x,tui_org_list[widget_idx].y);
}

void tui_draw_box(unsigned char w, unsigned char h){
    uint8_t org_x = tui_org_list->x;
    uint8_t org_y = tui_org_list->y;
    uint8_t j;
    TUI_PUTC(org_x,     org_y,     TUI_BOX_UL);
    TUI_PUTC(org_x+w-1, org_y,     TUI_BOX_UR);
    TUI_PUTC(org_x,     org_y+h-1, TUI_BOX_LL);
    TUI_PUTC(org_x+w-1, org_y+h-1, TUI_BOX_LR);
    tui_fill(w-2,TUI_BOX_H,TUI_SCREEN_XY(org_x+1,org_y  ));
    tui_fill(w-2,TUI_BOX_H,TUI_SCREEN_XY(org_x+1,org_y+h-1));
    for(j = org_y+1; j < (org_y+h-1); j++){
        TUI_PUTC(org_x    ,j,TUI_BOX_V);
        TUI_PUTC(org_x+w-1,j,TUI_BOX_V);
        tui_fill(w-2,' ',TUI_SCREEN_XY(org_x+1,j));
    } 
}

void tui_draw_txt(unsigned char x, unsigned char y, char* str, unsigned char len){
    uint8_t i;
    for(i=0; i<len && str[i]; i++){
        TUI_PUTC(tui_org_list->x + x + i, tui_org_list->y + y, str[i]);
    }
}

void tui_clear_txt(uint8_t widget_idx){
    char* txt = TUI_SCREEN_XY(tui_org_list->x + tui_org_list[widget_idx].x, tui_org_list->y + tui_org_list[widget_idx].y);
    uint8_t len = tui_org_list[widget_idx].len;
    do{
        txt[--len] = ' ';
    }while(len);
}

void tui_toggle_highlight(uint8_t widget_idx){
    char *scr_widget;
    tui_widget *widget = &tui_org_list[widget_idx];
    unsigned char i;
    if(widget->type < TUI_ACTIVE) return; //Ignore passive widgets
    scr_widget = TUI_SCREEN_XY(tui_org_list->x + widget->x, tui_org_list->y + widget->y);
    if(widget->type == TUI_INP){
        scr_widget += strlen(widget->data);
        if(scr_widget[0] == ' '){
            scr_widget[0] ^= 0x80;
        }
        else{
            scr_widget[0] = ' ';
        } 
    }else{
        for(i=0;i<widget->len;i++){
            scr_widget[i] ^= 0x80;  //Toggle invert bit
        }
    }
}

void tui_set_current(uint8_t widget_idx){
    tui_toggle_highlight(tui_current);      //Un-highlight previous
    tui_current = widget_idx;
    tui_toggle_highlight(tui_current);      //Highlight new
}

uint8_t tui_get_current(void){
    return tui_current;
}

void tui_set_data(uint8_t widget_idx, const char* data){
    tui_org_list[widget_idx].data = data;
}
const char* tui_get_data(uint8_t widget_idx){
    return tui_org_list[widget_idx].data;
}
enum tui_type tui_get_type(uint8_t widget_idx){
    return tui_org_list[widget_idx].type;
}
unsigned char tui_get_len(uint8_t widget_idx){
    return tui_org_list[widget_idx].len;
}

void tui_next_active(void){
    int8_t i;
    tui_widget* widget = &tui_org_list[tui_current];
    if(widget->type == TUI_END) return;
    for(i=1; widget[i].type != TUI_END; i++){
        if(widget[i].type >= TUI_ACTIVE){       //Active (selecable) widget
            tui_set_current(tui_current + i);
            return;
        }
    }

}

void tui_prev_active(void){
    int8_t i;
    tui_widget* widget = &tui_org_list[tui_current];
    if(widget->type == TUI_START) return;
    for(i=-1; widget[i].type != TUI_START; i--){
        if(widget[i].type >= TUI_ACTIVE){       //Active (selecable) widget
            tui_set_current(tui_current + i);
            return;
        }
    }

}