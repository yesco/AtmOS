#include "loci.h"
#include <fcntl.h>
//#include <stdlib.h>
//#include <unistd.h>

#define FM_XRAM_ADDR 0x8000
#define FM_XRAM_SIZE 0x2000

int file_copy(const char *dst, const char *src){
    int fd_dst;
    int fd_src;
    int len;

    fd_src = open(src, O_RDONLY | O_EXCL );
    if(fd_src < 0)
        return fd_src;
    fd_dst = open(dst, O_WRONLY | O_CREAT );
    
    if(fd_dst < 0){
        close(fd_src);
        return fd_dst;
    }
    
    do {
        len = read_xram(FM_XRAM_ADDR,FM_XRAM_SIZE,fd_src);
        write_xram(FM_XRAM_ADDR,len,fd_dst);
    }while(len==FM_XRAM_SIZE);
    close(fd_src);
    close(fd_dst);
    return 0;
}