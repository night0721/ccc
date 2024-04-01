#ifndef UTIL_H_
#define UTIL_H_

#include <stdio.h>

void die(char *reason);
void *memalloc(size_t size);
void *estrdup(void *ptr);
void *ewcsdup(void *ptr);
void *rememalloc(void *ptr, size_t size);

#endif
