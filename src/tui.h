#include <stdint.h>
#include <loci.h>


#define TUI_BOX_UL '^'
#define TUI_BOX_UR '`'
#define TUI_BOX_LL '~'
#define TUI_BOX_LR 0x7f
#define TUI_BOX_H '-'
#define TUI_BOX_V '|'

#define TUI_SCREEN_W 40
#define TUI_SCREEN_H 28

unsigned int tui_screen_xy(uint8_t x, uint8_t y);

#define TUI_SCREEN_XY_CONST(x,y) (char*)(0xbb80 + ((y) * TUI_SCREEN_W) + (x)) 
#define TUI_SCREEN_XY(x,y) (char*)tui_screen_xy((x),(y))
#define TUI_SCREEN (char*)(0xbb80);

//#define TUI_PUTC(x,y,ch) (TUI_SCREEN[(TUI_SCREEN_W * (y)) + (x)] = (ch))
#define TUI_PUTC(x,y,ch) (*(TUI_SCREEN_XY(x,y)) = (ch))
#define TUI_PUTC_CONST(x,y,ch) (*(TUI_SCREEN_XY_CONST(x,y)) = (ch))
//First active widget
#define TUI_ACTIVE 128

enum tui_type {
    //Passive widgets and tokens
    TUI_END = 0,
    TUI_START,
    TUI_BOX,
    TUI_TXT,
    TUI_INV,
    TUI_NOP,
    //Active widgets
    TUI_SEL = TUI_ACTIVE,       //TXT but selectable
    TUI_BTN,                    //SEL but reversed paper/ink
    TUI_INP,                    //INPut field
};

struct _tui_widget {
    enum tui_type   type;
    unsigned char   x;
    unsigned char   y;
    unsigned char   len;
    const char      *data;
};

typedef struct _tui_widget tui_widget;
void tui_cls(unsigned char ink);
void tui_draw(tui_widget* list);
void tui_draw_widget(uint8_t widget_idx);
uint8_t tui_hit(uint8_t x, uint8_t y);

void tui_set_current(uint8_t widget_idx);
uint8_t tui_get_current(void);
void tui_set_data(uint8_t widget_idx, const char* data);
const char* tui_get_data(uint8_t widget_idx);
void tui_set_type(uint8_t widget_idx, enum tui_type);
enum tui_type tui_get_type(uint8_t widget_idx);
unsigned char tui_get_len(uint8_t widget_idx);

void tui_draw_box(uint8_t widget_idx);
void tui_clear_box(uint8_t widget_idx);
void tui_draw_txt(uint8_t widget_idx);
void tui_clear_txt(uint8_t widget_idx);
void tui_toggle_highlight(uint8_t widget_idx);

void tui_next_active(void);
void tui_prev_active(void);

