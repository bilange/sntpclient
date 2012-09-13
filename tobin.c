#include <stdio.h>      // printf
#include <string.h>     // strcat
#include <stdlib.h>     // strtol

const char *byte_to_binary(int x) {
    static char b[9];
    b[0] = '\0';

    int z;
    for (z = 256; z > 0; z >>= 1)
    {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }

    return b;
}

const char *ulong_to_binary(int x) {
    static char b[17];
    b[0] = '\0';

    int z;
    for (z = 65536; z > 0; z >>= 1)
    {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }

    return b;
}

const char *ulong_to_hex(unsigned char *x) {
    //int vals[5] = {250, 25, 8, 16, 0};
    static char rtn[17];
    sprintf(rtn, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",(unsigned int)*x,(unsigned int)*(x+1),(unsigned int)*(x+2),(unsigned int)*(x+3),(unsigned int)*(x+4),(unsigned int)*(x+5),(unsigned int)*(x+6),(unsigned int)*(x+7),(unsigned int)*(x+8),(unsigned int)*(x+9),(unsigned int)*(x+10),(unsigned int)*(x+11),(unsigned int)*(x+12),(unsigned int)*(x+13),(unsigned int)*(x+14),(unsigned int)*(x+15));
    return rtn;
}
