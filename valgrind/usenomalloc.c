#include <stdio.h>
#include <stdlib.h>

int main() {
    char* ptr = malloc(100);
    printf("%c\n", ptr[5]);
    free(ptr);
    return 0;
}