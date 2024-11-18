#include <string.h>
#include <stdlib.h>

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
	/*
	for (size_t i = 0; i < list->length; i++) {
		if (list->items[i].name != NULL)
			free(list->items[i].name);
		if (list->items[i].path != NULL)
			free(list->items[i].path);
		if (list->items[i].stats != NULL)
			free(list->items[i].stats);
	}
	*/
	free(list->items);
	free(list);
}

/*
 * Check if the file is in the arraylist
 * Treat filepath as base name if bname is 1
 */
long arraylist_search(ArrayList *list, char *filepath, int bname)
{
	for (long i = 0; i < list->length; i++) {
		if (!bname && strcmp(list->items[i].path, filepath) == 0) {
			return i;
		}
		if (bname) {
			if (strcmp(list->items[i].name, filepath) == 0) {
				return i;
			}
		}
	}
	return -1;
}

void arraylist_remove(ArrayList *list, long index)
{
	if (index >= list->length)
		return;

	/* marked stuff doesn't work with this
	free(list->items[index].name);
	free(list->items[index].path);
	free(list->items[index].stats);
	*/
	for (long i = index; i < list->length - 1; i++)
		list->items[i] = list->items[i + 1];

	list->length--;
}

/*
 * Force will not remove duplicate marked files, instead it just skip adding
 */
void arraylist_add(ArrayList *list, char *name, char *path, char *stats, int type, char *icon, int color, int marked, int force)
{
	file new_file = { name, path, type, stats, color };
	strcpy(new_file.icon, icon);

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
char *get_line(ArrayList *list, long index, int detail, int icons)
{
	file f = list->items[index];

	size_t length = strlen(f.name) + 1;
	if (detail) {
		length += strlen(f.stats) + 1; /* 1 for space */
	}
	if (icons) {
		length += 5; /* 4 for icon, 1 for space */
	}

	char *line = memalloc(length);
	line[0] = '\0';
	if (detail) {
		strcat(line, f.stats);
		strcat(line, " ");
	}
	if (icons) {
		strcat(line, f.icon);
		strcat(line, " ");
	}
	strcat(line, f.name);
	line[length] = '\0';
	return line;
}
