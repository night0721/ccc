#ifndef UTIL_H_
#define UTIL_H_

#include <stdio.h>

void die(char *reason);
void *memalloc(size_t size);
void *rememalloc(void *ptr, size_t size);

#endif
