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
void free_file(file *toremove);
void clear_files();
void clear_marked();
long add_file(char *filepath, char *stats, char *type, int color);
void remove_marked(file *marked_file);
long add_marked(char *filepath, char *type, bool force);
file *get_marked(long index);
bool in_marked(char *path);
file *get_file(long index);
char *get_filepath(long index);
int get_color(long index);
char *get_line(long index);

#endif
