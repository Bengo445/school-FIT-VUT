#include <stdio.h>
#include <ctype.h>
#include <string.h>

int test(void) {

    char lol[95];
    for(int i = 0; i<95; i++){
        lol[i] = 1;
    }
    lol[94] = '\0';


    lol['A'-32] = 'A';

    for(int i = 0; i<94; i++){
        if (lol[i] != 1){
            printf("%c ", lol[i]);
        }
    }


    return 0;
}
