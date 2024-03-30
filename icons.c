#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <wchar.h>

#include "icons.h"
#include "util.h"

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

    printf("Name: %s | Hash Value: %d\n", name, hash_value);
    return hash_value;
}

void hashtable_init()
{
    for (int i = 0; i < TABLE_SIZE; i++)
        hash_table[i] = NULL;
    
    icon *c = memalloc(sizeof(icon));
    strcpy(c->name, "c");
    c->icon = L"";

    icon *h = memalloc(sizeof(icon));
    strcpy(h->name, "h");
    h->icon = L"";

    icon *cpp = memalloc(sizeof(icon));
    strcpy(cpp->name, "cpp");
    cpp->icon = L"";

    icon *hpp = memalloc(sizeof(icon));
    strcpy(hpp->name, "hpp");
    hpp->icon = L"󰰀";

    icon *md = memalloc(sizeof(icon));
    strcpy(md->name, "md");
    md->icon = L"";

    hashtable_add(c);
    hashtable_add(h);
    hashtable_add(cpp);
    hashtable_add(hpp);
    hashtable_add(md);
}

void hashtable_print()
{
    int i = 0;

    for (; i < TABLE_SIZE; i++) {
        if (hash_table[i] == NULL) {
            printf("%i. ---\n", i);
        } else {
            printf("%i. | Name %s | Icon %ls\n", i, hash_table[i]->name, hash_table[i]->icon);
        }
    }
}

/* Gets hashed name and tries to store the icon struct in that place */
bool hashtable_add(icon *p)
{
    if (p == NULL) return false;

    int index = hash(p->name);
    int initial_index = index;
     /* linear probing until an empty slot is found */
    while (hash_table[index] != NULL) {
        index = (index + 1) % TABLE_SIZE; /* move to next item */
        /* the hash table is full as no available index back to initial index, cannot fit new item */
        if (index == initial_index) return false;
    }

    hash_table[index] = p;
    return true;
}

/* Rehashes the name and then looks in this spot, if found returns icon */
icon *hashtable_search(char *name)
{
    int index = hash(name);
    int initial_index = index;
    
    /* Linear probing until an empty slot or the desired item is found */
    while (hash_table[index] != NULL) {
        if (strncmp(hash_table[index]->name, name, MAX_NAME) == 0)
            return hash_table[index];
        
        index = (index + 1) % TABLE_SIZE; /* Move to the next slot */
        /* back to same item */
        if (index == initial_index) break;
    }
    
    return NULL;
}
