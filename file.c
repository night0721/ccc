#include <string.h>
#include <stdlib.h>
#include <libgen.h>

#include "util.h"

/* files in a link list data structure */
typedef struct file {
    char *path;
    char *stats;
    /* put some more useful stat here */
    struct file *next;
} file;

file *files = NULL;

/*
 * get length of files linked list
 */
long files_len()
{
    file *current = files;
    int count = 0;
    while (current != NULL) {
        count++;
        current = current->next;
    }
    return count;
}

void clear_files()
{
    file *tmp;
    while (files != NULL) {
        tmp = files;
        files = files->next;
        free(tmp);
    }
}

long add_file(char *filepath, char *stats)
{
    file *current = files;
    file *new_file = memalloc(sizeof(file));
    char *buf = strdup(filepath);
    char *buf2 = strdup(stats);
    if (buf == NULL || buf2 == NULL) {
        perror("ccc");
    }
    new_file->path = buf;
    new_file->stats = buf2; 
    new_file->next = NULL;
    if (current == NULL) {
        files = new_file;
        return 0;
    }
    long index = 1;
    while (current->next != NULL) {
        current = current->next;
        index++;
    }
    current->next = new_file;
    return index;
}

file *get_file(long index)
{
    file *current = files;
    if (index == 0) {
        return current;
    }
    if (index > files_len()) {
        return NULL;
    }
    for (long i = 0; i < index; i++) {
        current = current->next;
    }
    return current;
}

char *get_filepath(long index)
{
    file *file = get_file(index);
    if (file != NULL) {
        char *name = strdup(file->path);
        if (!name) {
            perror("ccc");
        }
        return name;
    } else {
        return NULL;
    }
}

char *get_line(long index)
{
    file *file = get_file(index);
    if (file != NULL) {
        char *name = strdup(file->path);
        name = basename(name);
        char *stats = strdup(file->stats);
        if (name == NULL || stats == NULL) {
            perror("ccc");
        }
        size_t length = strlen(name) + strlen(stats) + 2; /* one for space and one for nul */
        char *line = memalloc(length * sizeof(char));
        snprintf(line, length, "%s %s", stats, name);
        return line;
    } else {
        return NULL;
    }
}
