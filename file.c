#include <string.h>
#include <stdlib.h>
#include <libgen.h>

#include "util.h"

/* files in a link list data structure */
typedef struct file {
    char *path;
    char *stats;
    char *type;
    int color;
    struct file *next;
} file;

file *files = NULL;
file *marked = NULL;

/*
 * Get length of files linked list
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

/*
 * Get length of marked files
 */
long marked_len()
{
    file *current = marked;
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

void clear_marked()
{
    file *tmp;
    while (marked != NULL) {
        tmp = marked;
        files = marked->next;
        free(tmp);
    }
}

long add_file(char *filepath, char *stats, char *type, int color)
{
    file *current = files;
    file *new_file = memalloc(sizeof(file));
    char *buf = strdup(filepath);
    char *buf2 = strdup(stats);
    char *buf3 = strdup(type);
    int buf4 = color;

    if (buf == NULL || buf2 == NULL || buf3 == NULL)
        perror("ccc");

    new_file->path = buf;
    new_file->stats = buf2; 
    new_file->type = buf3;
    new_file->color = buf4;
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

long add_marked(char *filepath, char *type)
{
    file *current = marked;
    file *new_file = memalloc(sizeof(file));
    char *buf = strdup(filepath);
    char *buf2 = strdup(type);
    if (buf == NULL || buf2 == NULL) {
        perror("ccc");
    }
    new_file->path = buf;
    new_file->type = buf2;
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

int get_color(long index)
{
    file *file = get_file(index);
    if (file != NULL) {
        int color = file->color;
        if (!color) {
            perror("ccc");
        }
        return color;
    } else {
        return 8;       /* white */
    }
}

/*
 * Construct a formatted line for display
 */
char *get_line(long index)
{
    file *file = get_file(index);
    if (file != NULL) {
        char *name = strdup(file->path);
        char *stats = strdup(file->stats);
        size_t length = strlen(name) + strlen(stats) + 2;   /* one for space and one for null */
        char *line = memalloc(length * sizeof(char));

        name = basename(name);
        if (name == NULL || stats == NULL)
            perror("ccc");

        snprintf(line, length, "%s %s", stats, name);

        return line;
    } else {
        return NULL;
    }
}
