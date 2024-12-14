#include <stdbool.h>

typedef struct _loci_cfg {
    uint8_t fdc_on;
    uint8_t tap_on;
    uint8_t bit_on;
    uint8_t mou_on;
    uint8_t b11_on;
    uint8_t ser_on;
    uint8_t ald_on;
    uint8_t mounts; //0-3 fdc, 4 tap, 5 rom
    uint8_t tui_pos;
    char path[256];
    char drv_names[6][35];
} loci_cfg_t;

bool persist_valid(void);
void persist_set_magic(void);

void persist_set_loci_cfg(loci_cfg_t* cfg);
bool persist_get_loci_cfg(loci_cfg_t* cfg);

