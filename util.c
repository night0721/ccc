#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

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
    void *duped = strdup(ptr);
    if (!duped) {
        perror("ccc");
        exit(EXIT_FAILURE);
    }
    return duped;
}

void *ewcsdup(void *ptr)
{
    void *duped = wcsdup(ptr);
    if (!duped) {
        perror("ccc");
        exit(EXIT_FAILURE);
    }
    return duped;
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
