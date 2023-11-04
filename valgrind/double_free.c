#include <stdlib.h>

int main() {
    char* ptr = malloc(100);
    free(ptr);
    free(ptr);
    return 0;
}