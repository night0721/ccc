#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

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

    return hash_value;
}

void hashtable_init()
{
    for (int i = 0; i < TABLE_SIZE; i++)
        hash_table[i] = NULL;
    
    icon *c = memalloc(sizeof(icon));
    strcpy(c->name, "c");
    c->icon = "";

    icon *h = memalloc(sizeof(icon));
    strcpy(h->name, "h");
    h->icon = "";

    icon *cpp = memalloc(sizeof(icon));
    strcpy(cpp->name, "cpp");
    cpp->icon = "";

    icon *hpp = memalloc(sizeof(icon));
    strcpy(hpp->name, "hpp");
    hpp->icon = "󰰀";

    icon *md = memalloc(sizeof(icon));
    strcpy(md->name, "md");
    md->icon = "";

    icon *py = memalloc(sizeof(icon));
    strcpy(py->name, "py");
    py->icon = "";

    icon *java = memalloc(sizeof(icon));
    strcpy(java->name, "java");
    java->icon = "";

    icon *json = memalloc(sizeof(icon));
    strcpy(json->name, "json");
    json->icon = "";

    icon *js = memalloc(sizeof(icon));
    strcpy(js->name, "js");
    js->icon = "";

    icon *html = memalloc(sizeof(icon));
    strcpy(html->name, "html");
    html->icon = "";

    icon *rs = memalloc(sizeof(icon));
    strcpy(rs->name, "rs");
    rs->icon = "";

    icon *sh = memalloc(sizeof(icon));
    strcpy(sh->name, "sh");
    sh->icon = "";

    icon *go = memalloc(sizeof(icon));
    strcpy(go->name, "go");
    go->icon = "";

    icon *r = memalloc(sizeof(icon));
    strcpy(r->name, "r");
    r->icon = "";

    icon *diff = memalloc(sizeof(icon));
    strcpy(diff->name, "diff");
    diff->icon = "";

    icon *hs = memalloc(sizeof(icon));
    strcpy(hs->name, "hs");
    hs->icon = "";

    icon *log = memalloc(sizeof(icon));
    strcpy(log->name, "log");
    log->icon = "󱀂";

    icon *rb = memalloc(sizeof(icon));
    strcpy(rb->name, "rb");
    rb->icon = "";

    icon *iso = memalloc(sizeof(icon));
    strcpy(iso->name, "iso");
    iso->icon = "󰻂";

    icon *lua = memalloc(sizeof(icon));
    strcpy(lua->name, "lua");
    lua->icon = "";
    
    icon *license = memalloc(sizeof(icon));
    strcpy(license->name, "LICENSE");
    license->icon = "";

    icon *gitignore = memalloc(sizeof(icon));
    strcpy(gitignore->name, "gitignore");
    gitignore->icon = "";

    hashtable_add(c);
    hashtable_add(h);
    hashtable_add(cpp);
    hashtable_add(hpp);
    hashtable_add(md);
    hashtable_add(py);
    hashtable_add(java);
    hashtable_add(json);
    hashtable_add(js);
    hashtable_add(html);
    hashtable_add(rs);
    hashtable_add(sh);
    hashtable_add(go);
    hashtable_add(r);
    hashtable_add(diff);
    hashtable_add(hs);
    hashtable_add(log);
    hashtable_add(rb);
    hashtable_add(iso);
    hashtable_add(lua);
    hashtable_add(license);
    hashtable_add(gitignore);
}

void hashtable_print()
{
    int i = 0;

    for (; i < TABLE_SIZE; i++) {
        if (hash_table[i] == NULL) {
            printf("%i. ---\n", i);
        } else {
            printf("%i. | Name %s | Icon %s\n", i, hash_table[i]->name, hash_table[i]->icon);
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
