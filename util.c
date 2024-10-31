#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void die(char *reason)
{
    fprintf(stderr, "%s\n", reason);
    exit(EXIT_FAILURE);
}

void *memalloc(size_t size)
{
    void *ptr = malloc(size);
    if (!ptr) {
        perror("ccc");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void *estrdup(void *ptr)
{
    void *dup = strdup(ptr);
    if (!dup) {
        perror("ccc");
        exit(EXIT_FAILURE);
    }
    return dup;
}

void *rememalloc(void *ptr, size_t size)
{
    ptr = realloc(ptr, size);
    if (!ptr) {
        perror("ccc");
        exit(EXIT_FAILURE);
    }
    return ptr;
}
