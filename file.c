#include <string.h>

#include "util.h"

// files in a link list data structure
typedef struct file {
    char *filename;
    // put some more useful stat here
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
    while (current != NULL)
    {
        count++;
        current = current->next;
    }
    return count;
}

long add_file(char *filename)
{
    file *current = files;
    file *new_file = memalloc(sizeof(file));
    char *buf = strdup(filename);
    if (!buf)
    {
        perror("ccc");
    }
    new_file->filename = buf;
    new_file->next = NULL;
    if (current == NULL)
    {
        files = new_file;
        return 0;
    }
    long index = 1;
    while (current->next != NULL)
    {
        current = current->next;
        index++;
    }
    current->next = new_file;
    return index;
}

file *get_file(long index)
{
    file *current = files;
    if (index == 0)
    {
        return current;
    }
    if (index > files_len())
    {
        return NULL;
    }
    for (long i = 0; i < index; i++)
    {
        current = current->next;
    }
    return current;
}

char *get_filename(long index)
{
    file *file = get_file(index);
    if (!file)
    {
        return file->filename;
    }
    else
    {
        return NULL;
    }
}
