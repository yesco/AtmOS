#include "tui.h"
#include <string.h>


extern tui_widget *tui_org_list;
#pragma zpsym ("tui_org_list");

extern uint8_t tui_current;

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
    switch(tui_org_list[widget_idx].type){
        case TUI_BOX:
            tui_draw_box(widget_idx);
            break;
        case TUI_TXT:
        case TUI_SEL:
            tui_draw_txt(widget_idx);
            break;
        case TUI_INV:
        case TUI_BTN:
            tui_draw_txt(widget_idx);
            tui_toggle_highlight(widget_idx);
            break;
        case TUI_INP:
            tui_clear_txt(widget_idx);
            tui_draw_txt(widget_idx);
            break;
        default:
            break;
    }
}

