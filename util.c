#include <stdio.h>
#include <stdlib.h>

void die(char *reason)
{
    fprintf(stderr, reason);
    exit(EXIT_FAILURE);
}

void *memalloc(size_t size)
{
    void *ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "ccc: Error allocating memory\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void *rememalloc(void *ptr, size_t size)
{
    ptr = realloc(ptr, size);
    if (!ptr) {
        perror("ccc");
        fprintf(stderr, "ccc: Error allocating memory\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}
