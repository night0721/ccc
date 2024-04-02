#ifndef FILE_H_
#define FILE_H_

#include <stdio.h>

typedef struct file {
    char *name; /* basename */
    char *path; /* absolute path */
    char *type;
    char *stats;
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
long arraylist_search(ArrayList *list, char *filepath, bool bname);
void arraylist_remove(ArrayList *list, long index);
void arraylist_add(ArrayList *list, char *filename, char *path, char *stats, char *type, wchar_t *icon, int color, bool marked, bool force);
char *get_line(ArrayList *list, long index, bool detail, bool icons);

#endif
