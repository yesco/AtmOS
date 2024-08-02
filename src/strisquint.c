#include <stdbool.h>

//Non-exact case-insensitive substring search
//Ignores bit 6 (0x20) when comparing characters

bool strisquint(char *haystack, char *needle){
    unsigned char i=0;
    unsigned char j=0;
    unsigned char mark=0;

    if( needle[0] == '\0')
        return true;

    for(; haystack[i] != '\0'; i++ ){
        if( (( haystack[i] ^ needle[j] ) & 0xdf ) == 0x00 ) //hit
        {
            if(j==0)                    //first hit?
                mark = i;               //mark start in haystack
            if( needle[++j] == '\0')    //end of needle after this?
                return true;            //return match
            continue;                   //check next pair
        }
        //miss
        if(j){          //partial match before miss?
            i = mark;   //return haystack to mark (+1 w/loop inc)
            j = 0;      //retrun needle to start
        }
    }
    return false;
}
