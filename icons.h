#ifndef ICONS_H_
#define ICONS_H_

#include <stdbool.h>

#define MAX_NAME 30
#define TABLE_SIZE 100

typedef struct {
    char name[MAX_NAME];
    char *icon;
} icon;

unsigned int hash(char *name);
void hashtable_init();
void hashtable_print();
bool hashtable_add(icon *p);
icon *hashtable_search(char *name);

#endif
