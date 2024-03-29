#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_NAME 30
#define TABLE_SIZE 20

typedef struct {
    char name[MAX_NAME];
    char *icon;
} icon;

icon *hash_table[TABLE_SIZE];

/* Hashes every name with: name and TABLE_SIZE */
unsigned int hash(char *name)
{
    int length = strnlen(name, MAX_NAME), i = 0;
    unsigned int hash_value = 0;
    
    for (; i < length; i++) {
        hash_value += name[i];
        hash_value = (hash_value * name[i]) % TABLE_SIZE;
    }

    return hash_value;
}

void init_hash_table()
{
    int i = 0;

    for (; i < TABLE_SIZE; i++)
        hash_table[i] = NULL;
}

void print_table()
{
    int i = 0;

    for (; i < TABLE_SIZE; i++) {
        if (hash_table[i] == NULL) {
            printf("%i. ---\n", i);
        } else {
            printf("%i. %s %s\n", i, hash_table[i]->name, hash_table[i]->icon);
        }
    }
}

/* Gets hashed name and tries to store the icon struct in that place */
bool add_icon(icon *p)
{
    if (p == NULL) return false;

    int index = hash(p->name);
    int i = 0, try;

    try = (i + index) % TABLE_SIZE;
    if (hash_table[try] == NULL) {
        hash_table[try] = p;
        return true;
    }

    return false;
}

/* Rehashes the name and then looks in this spot, if found returns icon */
icon *icon_search(char *name)
{
    int index = hash(name), i = 0;

    // this handles icons with the same hash values
    // or use linked lists in structs
    // for (; i < TABLE_SIZE; i++) {
    //     int try = (index + i) % TABLE_SIZE;
    //
    //     for (; i < TABLE_SIZE; i++) {
    //         try = (i + index) % TABLE_SIZE;
    //         
    //         if (hash_table[try] == NULL)
    //             return false;
    //
    //         if (strncmp(hash_table[try]->name, name, TABLE_SIZE) == 0)
    //             return hash_table[try];
    //     }
    //
    //     return false;
    // }
    //
    // return NULL;

    if (hash_table[index] == NULL)
        return false;

    if (strncmp(hash_table[index]->name, name, MAX_NAME) == 0)
        return hash_table[index];

    return NULL;
}

int main(void)
{
    init_hash_table();

    /*   name           reference name      icon     */
    icon c =          { .name="c",          .icon="" };
    icon h =          { .name="h",          .icon="" };
    icon cpp =        { .name="cpp",        .icon="" };
    icon hpp =        { .name="hpp",        .icon="󰰀" };
    icon md =         { .name="md",         .icon="" };

    add_icon(&c);
    add_icon(&h);
    add_icon(&cpp);
    add_icon(&hpp);
    add_icon(&md);

    print_table();

    icon *tmp = icon_search("hpp");

    if (tmp == NULL)
        printf("null\n");

    printf("%s", tmp->icon);

    return 0;
}

