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


