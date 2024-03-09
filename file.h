#ifndef FILE_H_
#define FILE_H_

typedef struct file {
    char *filename;
    // put some more useful stat here
    struct file *next;
} file;

long files_len();
long add_file(char *filename);
file *get_file(long index);
char *get_filename(long index);

#endif
