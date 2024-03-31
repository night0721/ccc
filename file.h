#ifndef FILE_H_
#define FILE_H_

#include <stdio.h>

typedef struct file {
    char *path;
    char *filename;
    char *stats;
    char *type;
    wchar_t *icon;
    int color;
} file;

typedef struct ArrayList {
    size_t length;
    size_t capacity;
    file *items;
} ArrayList;

ArrayList *arraylist_init(size_t capacity);
void arraylist_free(ArrayList *list);
bool arraylist_includes(ArrayList *list, char* filename, char *path);
void arraylist_remove(ArrayList *list, long index);
void arraylist_add(ArrayList *list, char *filename, char *path, char *stats, char *type, wchar_t *icon, int color, bool marked, bool force);
char *get_line(ArrayList *list, long index, bool detail);

#endif
