#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <libgen.h>
#include <wchar.h>

#include "util.h"
#include "file.h"

ArrayList *arraylist_init(size_t capacity)
{
    ArrayList *list = memalloc(sizeof(ArrayList));
    list->length = 0;
    list->capacity = capacity;
    list->items = memalloc(capacity * sizeof(file));

    return list;
}

void arraylist_free(ArrayList *list)
{
    for (size_t i = 0; i < list->length; i++) {
        if (list->items[i].type != NULL)
            free(list->items[i].type);
        if (list->items[i].path != NULL)
            free(list->items[i].path);
        if (list->items[i].stats != NULL)
            free(list->items[i].stats);
        if (list->items[i].icon != NULL)
            free(list->items[i].icon);
    }

    free(list->items);
    list->length = 0;
}

bool arraylist_includes(ArrayList *list, char *path)
{
    for (int i = 0; i < list->length; i++) {
        if (strcmp(list->items[i].path, path) == 0) {
            return true;
        }
    }

    return false;
}

void arraylist_remove(ArrayList *list, long index)
{
    if (index >= list->length)
        return;

    free(list->items[index].path);
    free(list->items[index].stats);
    free(list->items[index].type);
    free(list->items[index].icon);

    for (long i = index; i < list->length - 1; i++)
        list->items[i] = list->items[i + 1];

    list->length--;
}

/*
 * Force will not remove duplicate marked files, instead it just skip adding
 */
void arraylist_add(ArrayList *list, char *filepath, char *stats, char *type, wchar_t *icon, int color, bool marked, bool force)
{
    char *filepath_cp = NULL;
    char *stats_cp = NULL;
    char *type_cp = NULL;
    wchar_t *icon_cp = NULL;

    if (filepath != NULL) {
        filepath_cp = strdup(filepath);
        if (filepath_cp == NULL)
            perror("ccc");
    }
    if (stats != NULL) {
        stats_cp = strdup(stats);
        if (stats_cp == NULL)
            perror("ccc");
    }
    if (type != NULL) {
        type_cp = strdup(type);
        if (type_cp == NULL)
            perror("ccc");
    }
    if (icon != NULL) {
        icon_cp = wcsdup(icon);
        if (icon_cp == NULL)
            perror("ccc");
    }

    /* path, stats, type, icon, color */
    file new_file = { filepath_cp, stats_cp, type_cp, icon_cp, color };

    if (list->capacity != list->length) {
        if (marked) {
            for (int i = 0; i < list->length; i++) {
                if (strcmp(list->items[i].path, new_file.path) == 0) {
                    if (!force)
                        arraylist_remove(list, i);
                    return;
                }
            }
        }
        list->items[list->length] = new_file;
    } else {
        int new_cap = list->capacity * 2;
        file *new_items = memalloc(new_cap * sizeof(file));
        file *old_items = list->items;
        list->capacity = new_cap;
        list->items = new_items;

        for (int i = 0; i < list->length; i++)
            new_items[i] = old_items[i];

        free(old_items);
        list->items[list->length] = new_file;
    }
    list->length++;
}

/*
 * Construct a formatted line for display
 */
char *get_line(ArrayList *list, long index, bool detail)
{
    file file = list->items[index];
    char *name = strdup(file.path);
    wchar_t *icon = wcsdup(file.icon);
    char *stats = NULL;
    size_t length;
    if (detail) {
        stats = strdup(file.stats);
        length = strlen(name) + strlen(stats) + 2;   /* one for space and one for null */
        if (stats == NULL) {
            perror("ccc");
        }
    } else {
        length = strlen(name) + 2;   /* one for space and one for null */
    }
    char *line = memalloc(length * sizeof(char));

    name = basename(name);
    if (name == NULL)
        perror("ccc");

    if (detail) {
        snprintf(line, length, "%s %ls %s", stats, icon, name);
    } else {
        snprintf(line, length, "%ls %s", icon, name);
    }

    return line;
}
