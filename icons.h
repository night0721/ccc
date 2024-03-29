#ifndef FILE_H_
#define FILE_H_

#define MAX_NAME 30
#define TABLE_SIZE 20

typedef struct {
    char name[MAX_NAME];
    char icon[4];
} icon;

unsigned int hash(char *name);
void init_hash_table();
void print_table();
bool add_icon(icon *p);
icon *icon_search(char *name);

#endif
