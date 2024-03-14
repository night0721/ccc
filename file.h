#ifndef FILE_H_
#define FILE_H_

typedef struct file {
    char *path;
    char *stats;
    char *type;
    int color;
    struct file *next;
} file;

long files_len();
long marked_len();
void clear_files();
void clear_marked();
long add_file(char *filepath, char *stats, char *type, int color);
file *get_file(long index);
char *get_filepath(long index);
int get_color(long index);
char *get_line(long index);

#endif
