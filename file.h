#ifndef FILE_H_
#define FILE_H_

#include <stdio.h>
#include <stdbool.h>

enum ftypes {
	REG,
	DRY, /* DIR is taken */
	LNK,
	CHR,
	SOC,
	BLK,
	FIF
};

typedef struct file {
	char *name; /* basename */
	char *path; /* absolute path */
	int type;
	char *stats;
	char *icon;
	char color[12];
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
void arraylist_add(ArrayList *list, char *name, char *path, char *stats, int type, char *icon, char color[12], bool marked, bool force);
char *get_line(ArrayList *list, long index, bool detail, bool icons);

#endif
