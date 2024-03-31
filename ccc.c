#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>         /* directories etc. */
#include <sys/stat.h>
#include <errno.h>
#include <ftw.h>
#include <time.h>
#include <ncurses.h>
#include <locale.h>
#include <wchar.h>

#include "config.h"
#include "file.h"
#include "icons.h"
#include "util.h"

/* functions' definitions */
void show_help();
void start_ccc();
char *check_trash_dir();
void change_dir(const char *buf, int selection, int ftype);
void mkdir_p(const char *destdir);
void populate_files(const char *path, int ftype);
int get_directory_size(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
void add_file_stat(char *filepath, int ftype);
char *get_file_mode(mode_t mode);
void highlight_current_line();
void show_file_content();
void edit_file();
void toggle_executable();
int write_last_d();
void wpprintw(const char *fmt, ...);
void init_windows();
void draw_border_title(WINDOW *window, bool active);

/* global variables */
unsigned int focus = 0;
long current_selection = 0;
bool dirs_size = DIRS_SIZE;
bool show_hidden = SHOW_HIDDEN;
bool file_details = SHOW_DETAILS;
char *trash_dir;
char *cwd;
char *p_cwd; /* previous cwd */
char *trash_dir;
int half_width;
ArrayList *files;
ArrayList *marked;
WINDOW *directory_border;
WINDOW *directory_content;
WINDOW *preview_border;
WINDOW *preview_content;
WINDOW *panel;

unsigned long total_dir_size;

int main(int argc, char** argv)
{
    if (argc > 1 && strcmp(argv[1], "-h") == 0)
        die("Usage: ccc filename");
    if (argc == 2) {
        struct stat st;
        if (lstat(argv[1], &st)) {
            perror("ccc");
            die("Error from lstat");
        }
        /* chdir to directory from argument */
        if(S_ISDIR(st.st_mode) && chdir(argv[1])) {
            perror("ccc");
            die("Error from chdir");
        }
    }

    /* check if it is interactive shell */
    if (!isatty(STDIN_FILENO)) 
        die("ccc: No tty detected. ccc requires an interactive shell to run.\n");

    /* initialize screen, don't print special chars,
     * make ctrl + c work, don't show cursor 
     * enable arrow keys */
    setlocale(LC_ALL, "");
    initscr();
    noecho();
    cbreak();
    curs_set(0);
    keypad(stdscr, TRUE);

    /* check terminal has colors */
    if (!has_colors()) {
        endwin();
        die("ccc: Color is not supported in your terminal.\n");
    } else {
        use_default_colors();
        start_color();
    }

    /* colors */
    init_pair(1, COLOR_BLACK, -1);      /*  */
    init_pair(2, COLOR_RED, -1);        /*  */
    init_pair(3, COLOR_GREEN, -1);      /* LNK */
    init_pair(4, COLOR_YELLOW, -1);     /* BLK */
    init_pair(5, COLOR_BLUE, -1);       /* DIR */ 
    init_pair(6, COLOR_MAGENTA, -1);    /*  */
    init_pair(7, COLOR_CYAN, -1);       /* MARKED FILES */
    init_pair(8, COLOR_WHITE, -1);      /* REG */

    /* init files and marked arrays */
    files = arraylist_init(100);
    marked = arraylist_init(100);
    hashtable_init();

    cwd = memalloc(PATH_MAX * sizeof(char));
    getcwd(cwd, PATH_MAX);
    p_cwd = memalloc(PATH_MAX * sizeof(char));
    start_ccc();
    populate_files(cwd, 0);

    int ch, ch2;
    while (1) {
        if (COLS < 80 || LINES < 24) {
            endwin();
            die("ccc: Terminal size needs to be at least 80x24\n");
        }
        ch = getch();
        /* printf("%d ",ch); */
        switch (ch) {
            /* quit */
            case 'q':
                if (write_last_d() == -1) {
                    /* prompt user so error message can be shown to user */
                    getch();
                }
                endwin();
                return 0;

            /* reload using z */
            case 'z':
                change_dir(cwd, 0, 0); 
                break;

            /* go back by backspace or h or left arrow */
            case BACKSPACE:
            case LEFT:
            case 'h':;
                /* get parent directory */
                strcpy(p_cwd, cwd);
                char *last_slash = strrchr(cwd, '/');
                if (last_slash != NULL) {
                    if (last_slash == cwd) {
                        change_dir("/", 0, 0);
                        break;
                    }
                    *last_slash = '\0';
                    change_dir(cwd, 0, 0);
                }
                break;

            /* enter directory/open a file using enter or l or right arrow */
            case ENTER:
            case RIGHT:
            case 'l':;
                strcpy(p_cwd, cwd);
                file c_file = files->items[current_selection];
                /* check if it is directory or a regular file */
                if (strncmp(c_file.type, "DIR", 3) == 0) {
                    /* change cwd to directory */
                    change_dir(c_file.path, 0, 0); 
                } else if (strncmp(c_file.type, "REG", 3) == 0) {
                    edit_file();
                }
                break;

            /* jump up (ctrl u) */
            case CTRLU:
                if ((current_selection - JUMP_NUM) > 0)
                    current_selection -= JUMP_NUM;
                else
                    current_selection = 0;

                highlight_current_line();
                break;

            /* go up by k or up arrow */
            case UP:
            case 'k':
                if (current_selection > 0)
                    current_selection--;

                highlight_current_line();
                break;

            /* jump down (ctrl d) */
            case CTRLD:
                if ((current_selection + JUMP_NUM) < (files->length - 1))
                    current_selection += JUMP_NUM;
                else
                    current_selection = (files->length - 1);

                highlight_current_line();
                break;

            /* go down by j or down arrow */
            case DOWN:
            case 'j':
                if (current_selection < (files->length - 1))
                    current_selection++;

                highlight_current_line();
                break;

            /* jump to the bottom */
            case 'G':
                current_selection = (files->length - 1);
                highlight_current_line();
                break;

            /* jump to the top */
            case 'g':
                ch2 = getch();
                switch (ch2) {
                    case 'g':
                        current_selection = 0;
                        highlight_current_line();
                        break;
                    default:
                        break;
                }
                break;

            /* '~' go to $HOME */
            case TILDE:;
                char *home = getenv("HOME");
                if (home == NULL) {
                    wpprintw("$HOME is not defined");
                } else {
                    change_dir(home, 0, 0);
                }
                break;

            /* go to the trash dir */
            case 't':;
                char *trash_dir = check_trash_dir();
                if (trash_dir != NULL)
                    change_dir(trash_dir, 0, 0);
                break;

            /* show directories' sizes */
            case 'A':
                dirs_size = !dirs_size;
                change_dir(cwd, 0, 0);
                break;

            /* go to previous dir */
            case '-':
                change_dir(p_cwd, 0, 0);
                break;

            /* show help */
            case '?':
                show_help();
                break;
            
            /* toggle hidden files */
            case '.':
                show_hidden = !show_hidden;
                change_dir(cwd, 0, 0);
                break;

            /* toggle file details */
            case 'i':
                file_details = !file_details;
                change_dir(cwd, 0, 0);
                break;

            case 'X':
                toggle_executable();
                break;

            /* mark one file */
            case SPACE:
                add_file_stat(files->items[current_selection].path, 1);
                highlight_current_line();
                break;

            /* mark all files in directory */
            case 'a':
                change_dir(cwd, current_selection, 2); /* reload current dir */
                break;

            /* mark actions: */
            /* delete */
            case 'd':;
                if (marked->length) {
                    char *trash_dir = check_trash_dir();
                    if (trash_dir != NULL) {
                        ;
                        /* do your magic here */
                    }
                }
                break;

            /* move */
            case 'm':;
                if (marked->length) {
                    ;
                }
                break;

            /* copy */
            case 'c':;
                if (marked->length) {
                    ;
                }
                break;

            /* symbolic link */
            case 's':;
                if (marked->length) {
                    ;
                }
                break;

            /* bulk rename */
            case 'b':;
                if (marked->length) {
                    ;
                }
                break;
            
            /* escape */
            case ESC:
                break;

            case KEY_RESIZE:
                delwin(directory_border);
                delwin(directory_content);
                delwin(preview_border);
                delwin(preview_content);
                delwin(panel);
                endwin();
                start_ccc(); 
                highlight_current_line();
                break;
            default:
                break;
        }
    }
    arraylist_free(files);
    arraylist_free(marked);
    endwin();
    return 0;
}

void show_help()
{
    wclear(directory_content);
    wclear(preview_content);
    wprintw(directory_content,"h: go to parent dir\nj: scroll down\nk: scroll up\nl: go to child dir\n\nleft:  go to parent dir\ndown:  scroll down\nup:    scroll up\nright: go to child dir\n\nenter: go to child dir/open file\nbackspace: go to parent dir\n\ngg: go to top\nG: go to bottom\n\nctrl+u: jump up\nctrl+d: jump down\n\nt: go to trash dir\n~: go to home dir\n-: go to previous dir\nz: refresh current dir\n\ni: toggle file details\nX: toggle executable\n\nA: show directory disk usage/block size\nspace: mark file\na: mark all files in directory\n\n?: show help\nq: exit with last dir written to file\nctrl+c exit without writing last dir");
    wpprintw("Visit https://github.com/piotr-marendowski/ccc or use 'man ccc' for help");
    wrefresh(directory_content);
    wrefresh(preview_content);
}

void start_ccc()
{
    half_width = COLS / 2;
    init_windows();
}

/* Checks if trash directory is set and returns it */
char *check_trash_dir()
{
    char *path = memalloc(PATH_MAX * sizeof(char));

    #ifdef TRASH_DIR
        trash_dir = TRASH_DIR;
    #endif

    /* check if there is trash_dir */
    if (trash_dir == NULL) {
        wpprintw("No trash directory defined");
        return NULL;
    } else {
        /* if trash_dir has ~ then make it $HOME */
        /* use path as trash_dir */
        if (trash_dir[0] == '~') {
            char *home = getenv("HOME");
            if (home == NULL) {
                wpprintw("$HOME is not defined, can't read the trash directory");
                return NULL;
            }
            /* replace ~ with home */
            snprintf(path, PATH_MAX, "%s%s", home, trash_dir + 1);
        }
        else {
            strcpy(path, trash_dir);
        }

        /* if has access to trash_dir */
        if (access(path, F_OK) != 0) {
            /* create the directory with 755 perm if it doesn't exit */
            mkdir_p(path);        
        }
    }
    return path;
}

/*
 * Change directory in window with selection
 */
void change_dir(const char *buf, int selection, int ftype)
{
    char *buf_dup;
    if (buf == p_cwd) {
        buf_dup = strdup(p_cwd);
        if (buf_dup == NULL) {
            return;
        }
    } else {
        buf_dup = (char *) buf;
    }
    strcpy(cwd, buf_dup);
    arraylist_free(files);
    files = arraylist_init(100);
    current_selection = selection;
    populate_files(cwd, ftype);
}

/*
 * Recursively create directory by creating each subdirectory
 * like mkdir -p
 */
void mkdir_p(const char *destdir)
{
    char *path = memalloc(PATH_MAX * sizeof(char));
    char dir_path[PATH_MAX] = "";

    if (destdir[0] == '~') {
        char *home = getenv("HOME");
        if (home == NULL) {
            wpprintw("$HOME is not defined");
            return;
        }
        /* replace ~ with home */
        snprintf(path, PATH_MAX, "%s%s", home, destdir + 1);
    } else {
        strcpy(path, destdir);
     }

    /* fix first / not appearing in the string */
    if (path[0] == '/')
        strcat(dir_path, "/");

    char *token = strtok(path, "/");
    while (token != NULL) {
        strcat(dir_path, token);
        strcat(dir_path, "/");

        if (mkdir(dir_path, 0755) == -1) {
            struct stat st;
            if (stat(dir_path, &st) == 0 && S_ISDIR(st.st_mode)) {
                /* Directory already exists, continue to the next dir */
                token = strtok(NULL, "/");
                continue;
            }
            
            wpprintw("mkdir failed: %s\n", strerror(errno));
            free(path);
            return;
        }
        token = strtok(NULL, "/");
    }

    free(path);
    return;
}

/*
 * Read the provided directory and add all files in directory to linked list
 * ftype: normal files = 0, marked = 1, marking ALL = 2
 * ep->d_name -> filename
 */
void populate_files(const char *path, int ftype)
{
    DIR *dp;
    struct dirent *ep;

    if ((dp = opendir(path)) != NULL) {
        /* clear directory window to ready for printing */
        wclear(directory_content);

        while ((ep = readdir(dp)) != NULL) {
            char *filename = memalloc(PATH_MAX * sizeof(char));
            /* make filename be basename of selected item just to pass check */
            filename[0] = '\0';
            strcat(filename, ep->d_name);

            /* use strncmp to filter out dotfiles */
            if ((!show_hidden && strncmp(filename, ".", 1) && strncmp(filename, "..", 2)) || (show_hidden && strcmp(filename, ".") && strcmp(filename, ".."))) {
                /* construct full file path */
                filename[0] = '\0';
                strcat(filename, cwd);
                strcat(filename, "/");
                strcat(filename, ep->d_name);   /* add filename */

                add_file_stat(filename, ftype);
            }
            free(filename);
        }
        closedir(dp);
        wrefresh(directory_content);
        highlight_current_line();
    } else {
        wpprintw("stat failed: %s\n", strerror(errno));
    }
}

int get_directory_size(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    total_dir_size += sb->st_size;
    return 0;
}

/*
 * Get file's last modified time, size, type
 * Add that file into list
 * ftype: normal file = 0, normal marked = 1, marked ALL = 2
 */
void add_file_stat(char *filepath, int ftype)
{
    struct stat file_stat;
    if (stat(filepath, &file_stat) == -1) {
        /* can't be triggered? */
        if (errno == EACCES)
            arraylist_add(files, filepath, NULL, NULL, NULL, 8, false, false);
    }
    
    /* get file type and color, 4 chars for the type and icon */
    size_t type_size = 4 * sizeof(char);
    size_t icon_size = 2 * sizeof(wchar_t);

    char *type = memalloc(type_size);
    wchar_t *icon_str = memalloc(icon_size);

    filepath[strlen(filepath)] = '\0';
    /* find last / in path by finding basename */
    char *f_bname = strrchr(filepath, '/');
    char *ext = NULL;
    if (f_bname != NULL) {
        /* shift 1 to get basename */
        f_bname += 1;
        /* handle file without extension */
        ext = strrchr(f_bname, '.');
        if (ext != NULL) {
            ext += 1;
        } else {
            ext = f_bname;
        }
    }
    if (ext != NULL) {
        icon *ext_icon = hashtable_search(ext);
        if (ext_icon == NULL)
            wcsncpy(icon_str, L"", 2);
        else 
            wcsncpy(icon_str, ext_icon->icon, 2);
    } else {
        wcsncpy(icon_str, L"", 2);
    }

    int color;

    if (S_ISDIR(file_stat.st_mode)) {
        strncpy(type, "DIR", 4);        /* directory type */
        color = 5;                      /* blue color */
    } else if (S_ISREG(file_stat.st_mode)) {
        strncpy(type, "REG", 4);        /* regular file */
        color = 8;                      /* white color */
    } else if (S_ISLNK(file_stat.st_mode)) {
        strncpy(type, "LNK", 4);        /* symbolic link */
        color = 3;                      /* green color */
    } else if (S_ISCHR(file_stat.st_mode)) {
        strncpy(type, "CHR", 4);        /* character device */
        color = 8;                      /* white color */
    } else if (S_ISSOCK(file_stat.st_mode)) {
        strncpy(type, "SOC", 4);        /* socket */
        color = 8;                      /* white color */
    } else if (S_ISBLK(file_stat.st_mode)) {
        strncpy(type, "BLK", 4);        /* block device */
        color = 4;                      /* yellow color */
    } else if (S_ISFIFO(file_stat.st_mode)) {
        strncpy(type, "FIF", 4);        /* FIFO */
        color = 8;                      /* white color */
    } else {
        color = 8;                      /* white color */
    }

    /* if file is to be marked */
    if (ftype == 1 || ftype == 2) {
        /* force if user is marking all files */
        bool force = ftype == 2 ? true : false;
        arraylist_add(marked, filepath, NULL, type, icon_str, 8, true, force);
        /* free type and return without allocating more stuff */
        free(type);
        return;
    }

    /* get last modified time */
    size_t time_size = 17 * sizeof(char);
    char *time = memalloc(time_size);
    /* format last modified time to a string */
    strftime(time, time_size, "%Y-%m-%d %H:%M", localtime(&file_stat.st_mtime));
    
    /* get file size */
    double bytes = file_stat.st_size;
    
    if (dirs_size) {
        /* dirs_size is true, so calculate disk usage */
        if (S_ISDIR(file_stat.st_mode)) {
            /* at most 15 fd opened */
            total_dir_size = 0;
            nftw(filepath, &get_directory_size, 15, FTW_PHYS);
            bytes = total_dir_size;
        }
    }
    /* 4 before decimal + 1 dot + DECIMAL_PLACES (after decimal) + unit length(1 for K, 3 for KiB, taking units[1] as B never changes)+ 1 space + 1 null */
    int size_size = 4 + 1 + DECIMAL_PLACES + strlen(units[1]) + 1 + 1;
    char *size = memalloc(size_size * sizeof(char));
    int unit = 0;
    while (bytes > 1024) {
        bytes /= 1024;
        unit++;
    }
    /* display sizes and check if there are decimal places */
    if (bytes == (unsigned int) bytes) {
        sprintf(size, "%d%s", (unsigned int) bytes, units[unit]);
    } else {
        sprintf(size, "%.*f%s", DECIMAL_PLACES, bytes, units[unit]);
    }
    /* get file mode string */
    char *mode_str = get_file_mode(file_stat.st_mode);

    /* mode_str(11) + time(17) + size_size + 2 spaces + 1 null */
    size_t stat_size = 11 * sizeof(char) + time_size + size_size + 3 * sizeof(char);
    char *total_stat = memalloc(stat_size);
    snprintf(total_stat, stat_size, "%s %s %-*s", mode_str, time, size_size, size);

    arraylist_add(files, filepath, total_stat, type, icon_str, color, false, false);

    free(time);
    free(size);
    free(total_stat);
    free(type);
    free(mode_str);
    free(icon_str);
}

/*
 * get file mode string from stat mode
 * eg: drwxr-sr-x
 */
char *get_file_mode(mode_t mode)
{
    char *mode_str = memalloc(11 * sizeof(char));
    mode_str[0] = S_ISDIR(mode) ? 'd' : '-'; /* Check if it's a directory */
    mode_str[1] = (mode & S_IRUSR) ? 'r' : '-';
    mode_str[2] = (mode & S_IWUSR) ? 'w' : '-';
    mode_str[3] = (mode & S_IXUSR) ? 'x' : '-';
    mode_str[4] = (mode & S_IRGRP) ? 'r' : '-';
    mode_str[5] = (mode & S_IWGRP) ? 'w' : '-';
    mode_str[6] = (mode & S_IXGRP) ? 'x' : '-';
    mode_str[7] = (mode & S_IROTH) ? 'r' : '-';
    mode_str[8] = (mode & S_IWOTH) ? 'w' : '-';
    mode_str[9] = (mode & S_IXOTH) ? 'x' : '-';
    mode_str[10] = '\0';
    return mode_str;
}

/*
 * Highlight current line by reversing the color
 */
void highlight_current_line()
{
    #if DRAW_BORDERS
        draw_border_title(directory_border, true);
    #endif

    long overflow = 0;
    if (current_selection > LINES - 4) {
        /* overflown */
        overflow = current_selection - (LINES - 4);
    }

    /* calculate range of files to show */
    long range = files->length;
    /* not highlight if no files in directory */
    if (range == 0 && errno == 0) {
        #if DRAW_PREVIEW
            wprintw(preview_content, "empty directory");
            wrefresh(preview_content);
        #endif
        return;
    }

    if (range > LINES - 3) {
        /* if there are more files than lines available to display
         * shrink range to avaiable lines to display with
         * overflow to keep the number of iterations to be constant */
        range = LINES - 3 + overflow;
    }
    
    wclear(directory_content);
    long line_count = 0;
    for (long i = overflow; i < range; i++) {
        if ((overflow == 0 && i == current_selection) || (overflow != 0 && i == current_selection)) {
            wattron(directory_content, A_REVERSE);

            /* check for marked files */
            long num_marked = marked->length;
            if (num_marked > 0) {
                /* Determine length of formatted string */
                int m_len = snprintf(NULL, 0, "[%ld] selected", num_marked);
                char *selected = memalloc((m_len + 1) * sizeof(char));

                snprintf(selected, m_len + 1, "[%ld] selected", num_marked);
                wpprintw("(%ld/%ld) %s %s", current_selection + 1, files->length, selected, cwd);
            } else  {
                wpprintw("(%ld/%ld) %s", current_selection + 1, files->length, cwd);
            }
        }
        /* print the actual filename and stats */
        char *line = get_line(files, i, file_details);
        int color = files->items[i].color;
        /* check is file marked for action */
        bool is_marked = arraylist_includes(marked, files->items[i].path);
        if (is_marked) {
            /* show file is selected */
            wattron(directory_content, COLOR_PAIR(7));
        } else {
            /* print the whole directory with default colors */
            wattron(directory_content, COLOR_PAIR(color));
        }
        
        if (overflow > 0)
            mvwprintw(directory_content, line_count, 0, "%s", line);
        else
            mvwprintw(directory_content, i, 0, "%s", line);

        if (is_marked) {
            wattroff(directory_content, COLOR_PAIR(7));
        } else {
            wattroff(directory_content, COLOR_PAIR(color));
        }

        wattroff(directory_content, A_REVERSE);
        free(line);
        line_count++;
    }

    wrefresh(directory_content);
    wrefresh(panel);
    /* show file content every time cursor changes */
    #if DRAW_PREVIEW
        show_file_content();
    #endif
    #if DRAW_BORDERS
        draw_border_title(preview_border, true);
    #endif
    wrefresh(preview_content);
}

/*
 * Get file content into buffer and show it to preview window
 */
void show_file_content()
{
    wclear(preview_content);
    file current_file = files->items[current_selection];

    if (strncmp(current_file.type, "DIR", 3) == 0)
        return;

    FILE *file = fopen(current_file.path, "r");
    if (file == NULL) {
        mvwprintw(preview_content, 0, 0, "Unable to read %s", current_file.path);
        return;
    }
    
    int c;
    /* check if its binary */
    while ((c = fgetc(file)) != EOF) {
        if (c == '\0') {
            mvwprintw(preview_content, 0, 0, "binary");
            return;
        }
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    /* check if file isn't empty */
    if (length != 0) {
        fseek(file, 0, SEEK_SET); /* set cursor back to start of file */
        int max_length = (LINES - 3) * (COLS / 2 - 2);
        char *buffer = memalloc(max_length * sizeof(char));
        fread(buffer, 1, max_length, file);
        mvwprintw(preview_content, 0, 0, "%s", buffer);
        free(buffer);
    } else {
        wclear(preview_content);
    }
    fclose(file);
}

/*
 * Opens $EDITOR to edit the file
 */
void edit_file()
{
    #ifdef EDITOR
        char *editor = EDITOR;
    #else
        char *editor = getenv("EDITOR");
    #endif

    if (editor == NULL) {
        wpprintw("Cannot get EDITOR variable, is it defined?");
        return;
    } else {
        def_prog_mode();    /* save the tty modes */
        endwin();           /* end curses mode temporarily */

        char *filename = files->items[current_selection].path;
        int length = strlen(editor) + strlen(filename) + 2; /* one for space one for null */
        char command[length];

        snprintf(command, length, "%s %s", editor, filename);
        system(command);
        reset_prog_mode();  /* return to previous tty mode */
        refresh();          /* store the screen contents */
        free(filename);
    }
}

void toggle_executable()
{
    file current_file = files->items[current_selection];
    struct stat st;
    if (stat(current_file.path, &st) == -1) {
        wpprintw("stat failed: %s\n", strerror(errno));
    }
    if (strncmp(current_file.type, "DIR", 3) == 0)
        return;
    /* chmod by xor executable bits */
    if (chmod(current_file.path, st.st_mode ^ (S_IXUSR | S_IXGRP | S_IXOTH)) == -1) {
        wpprintw("Error toggling executable: %s", strerror(errno));
    }

}

int write_last_d()
{
    #ifdef LAST_D
        char *last_d = memalloc(PATH_MAX * sizeof(char)); 
        strcpy(last_d, LAST_D);
    #else
        char *last_d = getenv("CCC_LAST_D");
    #endif
    if (last_d == NULL) {
        wpprintw("Cannot get CCC_LAST_D variable, is it defined?");
        return -1;
    } else {
        char *last_ddup = memalloc(PATH_MAX * sizeof(char));
        char *home = getenv("HOME");
        if (home == NULL) {
            wpprintw("$HOME is not defined");
            return -1;
        }
        /* replace ~ with home */
        snprintf(last_ddup, PATH_MAX, "%s%s", home, last_d + 1);

        char *last_d_dir = strrchr(last_d, '/');
        if (last_d_dir != NULL) {
            *last_d_dir = '\0'; /* truncate string */
        }
        mkdir_p(last_d);
        FILE *last_d_file = fopen(last_ddup, "w");
        if (last_d_file == NULL) {
            wpprintw("Cannot open last directory file");
            return -1;
        }
        fwrite(cwd, strlen(cwd), sizeof(char), last_d_file);
        fclose(last_d_file);
    }
    return 0;
}

/*
 * Print line to the panel
 */
void wpprintw(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    wclear(panel);
    vw_printw(panel, fmt, args);
    va_end(args);
    wrefresh(panel);
}

void init_windows()
{
    /* offset for width of the content 1 and 2 */
    int width_left = half_width;
    int width_right = half_width;
    #ifdef WINDOW_OFFSET
        width_left += WINDOW_OFFSET;
        width_right -= WINDOW_OFFSET;
    #endif

    /*------------------------------+
    |----border(0)--||--border(2)--||
    ||              ||             ||
    || content (1)  || content (3) ||
    || (directory)  ||  (preview)  ||
    ||              ||             ||
    |---------------||-------------||
    +==========panel (4)===========*/
    
    /*                         lines,          cols,            y,          x             */
    panel =             newwin(PH,             COLS,            LINES - PH, 0             );
    /* draw border around windows */
    #if DRAW_BORDERS
    directory_border =  newwin(LINES - PH,     width_left,      0,          0             );
    directory_content = newwin(LINES - PH - 2, width_left - 2,  1,          1             );

    preview_border =    newwin(LINES - PH,     width_right,     0,          width_left    );
    preview_content =   newwin(LINES - PH - 2, width_right - 2, 1,          width_left + 1);
    
    draw_border_title(directory_border, true);
    draw_border_title(preview_border, true);
    #else
    /* if there are no borders, then draw content in their places */
    directory_border =  newwin(0,              0,               COLS,       LINES         );
    preview_border =    newwin(0,              0,               COLS,       LINES         );
    /* -1 for the one space to the left */
    directory_content = newwin(LINES - PH - 1, width_left,      0,          1             );
    preview_content =   newwin(LINES - PH,     width_right,     0,          width_left    );
    #endif

    scrollok(directory_content, true);
    refresh();
}

/*
 * Draw the border of the window depending if it's active or not,
 */
void draw_border_title(WINDOW *window, bool active)
{
    /* check if the window is directory of preview */
    int width = half_width;
    if (window == directory_border) {          /* left */
        width += WINDOW_OFFSET;    
    } else if (window == preview_border) {   /* right */
        width -= WINDOW_OFFSET;
    }

    /* turn on color depends on active */
    if (active) {
        wattron(window, COLOR_PAIR(7));
    } else {
        wattron(window, COLOR_PAIR(5));
    }
    /* draw top border */
    mvwaddch(window, 0, 0, ACS_ULCORNER);  /* upper left corner */
    mvwhline(window, 0, 1, ACS_HLINE, width - 2); /* top horizontal line */
    mvwaddch(window, 0, width - 1, ACS_URCORNER); /* upper right corner */

    /* draw side border */
    mvwvline(window, 1, 0, ACS_VLINE, LINES - 2); /* left vertical line */
    mvwvline(window, 1, width - 1, ACS_VLINE, LINES - 2); /* right vertical line */

    /* draw bottom border
     * make space for the panel */
    mvwaddch(window, LINES - PH - 1, 0, ACS_LLCORNER); /* lower left corner */
    mvwhline(window, LINES - PH - 1, 1, ACS_HLINE, width - 2); /* bottom horizontal line */
    mvwaddch(window, LINES - PH - 1, width - 1, ACS_LRCORNER); /* lower right corner */

    /* turn color off after turning it on */
    if (active) {
        wattroff(window, COLOR_PAIR(7));
    } else {
        wattroff(window, COLOR_PAIR(5));
    }
    wrefresh(window);  /* Refresh the window to see the colored border and title */
}
