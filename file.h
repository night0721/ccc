#ifndef FILE_H_
#define FILE_H_

#include <stdio.h>

typedef struct file {
    char *path;
    char *stats;
    char *type;
    int color;
} file;

typedef struct ArrayList {
    size_t length;
    size_t capacity;
    file *items;
} ArrayList;

ArrayList *arraylist_init(size_t capacity);
void arraylist_free(ArrayList *list);
bool arraylist_includes(ArrayList *list, char *path);
void arraylist_remove(ArrayList *list, long index);
void arraylist_add(ArrayList *list, char *filepath, char *stats, char *type, int color, bool marked, bool force);
char *get_line(ArrayList *list, long index, bool detail);

#endif
