#ifndef FILE_H_
#define FILE_H_

typedef struct file {
    char *path;
    char *stats;
    char *type;
    // put some more useful stat here
    struct file *next;
} file;

long files_len();
long marked_len();
void clear_files();
void clear_marked();
long add_file(char *filename, char *time, char *type);
long add_marked(char *filepath, char *type);
file *get_file(long index);
char *get_filepath(long index);
char *get_line(long index);

#endif
