#include <stdlib.h>

int main() {
    char* ptr = malloc(10);
    char out_of_bound = ptr[100];
    free(ptr);
    return 0;
}