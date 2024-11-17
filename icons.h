#ifndef ICONS_H_
#define ICONS_H_

#define MAX_NAME 30
#define TABLE_SIZE 100

typedef struct {
    char name[MAX_NAME];
    char *icon;
} icon;

unsigned int hash(char *name);
void hashtable_init(void);
void hashtable_print(void);
int hashtable_add(icon *p);
icon *hashtable_search(char *name);

#endif
