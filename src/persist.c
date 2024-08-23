#include <loci.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "persist.h"

#define XRAM_PERSIST_ADDR 0x2000
#define XRAM_PERSIST_PATH_SIZE 256
typedef struct persist_data {
    uint8_t magic[4];
    loci_cfg_t loci_cfg;
    char paths[6][XRAM_PERSIST_PATH_SIZE];
} persist_data_t;

#pragma optimize (push,off)
uint8_t const persist_magic[] = "LOCI";

bool persist_valid(void){
    //uint32_t tmp;
    uint8_t i;
    MIA.addr0 = XRAM_PERSIST_ADDR;
    MIA.step0 = 1;
    for(i=0; i<4; i++){
        if(MIA.rw0 != persist_magic[i])
            return false; 
    }
    return true;
}

void persist_set_magic(void){
    uint8_t i;
    MIA.addr0 = XRAM_PERSIST_ADDR;
    MIA.step0 = 1;
    for(i=0; i<4; i++){
        MIA.rw0 = persist_magic[i];
    }
}

void persist_set_loci_cfg(loci_cfg_t* cfg){
    uint16_t i;
    MIA.addr0 = XRAM_PERSIST_ADDR + offsetof(persist_data_t,loci_cfg);
    MIA.step0 = 1;
    for(i=0; i<sizeof(loci_cfg_t); i++){
        MIA.rw0 = ((uint8_t*)cfg)[i];
    }
}

bool persist_get_loci_cfg(loci_cfg_t* cfg){
    bool valid;
    uint16_t i;
    valid = persist_valid();
    MIA.addr0 = XRAM_PERSIST_ADDR + offsetof(persist_data_t,loci_cfg);
    MIA.step0 = 1;
    if(valid){
        for(i=0; i<sizeof(loci_cfg_t); i++){
            ((uint8_t*)cfg)[i] = MIA.rw0;
        }
    }
    return valid;
}


#pragma optimize (pop)
