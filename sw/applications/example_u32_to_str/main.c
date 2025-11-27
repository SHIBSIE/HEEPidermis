#include "syscalls.h"
#include <stdlib.h>

void u32_to_str(int v, char *s) {
    char buf[11];
    int i = 0, j = 0;

    if (v == 0) { s[0] = '0'; s[1] = '\0'; return; }

    while (v > 0) {
        buf[i++] = '0' + (v % 10);
        v /= 10;
    }

    while (i--) s[j++] = buf[i];
    s[j] = '\0';
}

void main(){
    int value = 12345;
    char string[20];
    u32_to_str(value, &string);
    _writestr(string);
    return EXIT_SUCCESS;

}
