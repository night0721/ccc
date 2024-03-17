#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <linux/limits.h>
#include <dirent.h>         /* directories etc. */
#include <sys/stat.h>
#include <errno.h>
#include <ftw.h>
#include <time.h>
#include <ncurses.h>

#include "file.h"
#include "util.h"
#include "config.h"

typedef struct {
    WINDOW *window;
    int location;
    int y;
    int x;
} WIN_STRUCT;

/* functions' definitions */
void change_dir(const char *buf, int selection);
int mkdir_p(const char *destdir);
void populate_files(const char *path, int ftype);
int get_directory_size(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
long add_file_stat(char *filepath, int ftype);
void highlight_current_line();
void show_file_content();
void edit_file();
void wpprintw(const char *line);
void init_windows();
void draw_border_title(WINDOW *window, bool active);

/* global variables */
unsigned int focus = 0;
long current_selection = 0;
bool dirs_size = DIRS_SIZE;
char *cwd;
int half_width;
WIN_STRUCT windows[5];
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

    /* check if it is interactive shell */
    if (!isatty(STDIN_FILENO)) 
        die("ccc: No tty detected. ccc requires an interactive shell to run.\n");

    /* initialize screen, don't print special chars,
     * make ctrl + c work, don't show cursor 
     * enable arrow keys */
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

    half_width = COLS / 2;
    init_windows();
    refresh();

    cwd = memalloc(PATH_MAX * sizeof(char));
    getcwd(cwd, PATH_MAX);

    populate_files(cwd, 0);
    highlight_current_line();

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
                endwin();
                return 0;

            /* reload using z */
            case 'z':
                change_dir(cwd, 0); 
                break;

            /* go back by backspace or h or left arrow */
            case BACKSPACE:
            case LEFT:
            case 'h':;
                /* get parent directory */
                char *last_slash = strrchr(cwd, '/');
                if (last_slash != NULL) {
                    *last_slash = '\0';
                    change_dir(cwd, 0);
                }
                break;

            /* enter directory/open a file using enter or l or right arrow */
            case ENTER:
            case RIGHT:
            case 'l':;
                file *file = get_file(current_selection);
                if (file != NULL) {
                    /* check if it is directory or a regular file */
                    if (strncmp(file->type, "DIR", 3) == 0) {
                        /* change cwd to directory */
                        change_dir(file->path, 0); 
                    } else if (strncmp(file->type, "REG", 3) == 0) {
                        edit_file();
                    }
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
                if ((current_selection + JUMP_NUM) < (files_len() - 1))
                    current_selection += JUMP_NUM;
                else
                    current_selection = (files_len() - 1);

                highlight_current_line();
                break;

            /* go down by j or down arrow */
            case DOWN:
            case 'j':
                if (current_selection < (files_len() - 1))
                    current_selection++;

                highlight_current_line();
                break;

            /* jump to the bottom */
            case 'G':
                current_selection = (files_len() - 1);
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
                    change_dir(home, 0);
                }
                break;

            /* go to the trash dir */
            case 't':;
                #ifdef TRASH_DIR
                    char *trash_dir = TRASH_DIR;
                #else
                    char *trash_dir = getenv("CCC_TRASH");
                #endif
                if (trash_dir == NULL) {
                    wpprintw("$CCC_TRASH is not defined");
                } else {
                    if (access(trash_dir, F_OK) != 0) {
                        /* create the directory with 755 perm if it doesn't exit */
                        if (mkdir_p(trash_dir) == -1) {
                            switch (errno) {
                                case EACCES:
                                    wpprintw("Parent directory does not allow write permission or one of directories does not allow search access");
                            }
                        }
                    }
                    change_dir(trash_dir, 0);
                }
                break;

            /* show directories' sizes */
            case 'A':
                dirs_size = !dirs_size;
                clear_files();
                populate_files(cwd, 0);
                highlight_current_line();
                break;

            /* mark one file */
            case SPACE:;
                add_file_stat(get_filepath(current_selection), 1);
                highlight_current_line();
                break;

            /* mark all files in directory */
            case 'a':;
                int save_current_sel = current_selection;
                populate_files(cwd, 1);
                change_dir(cwd, save_current_sel);      /* reload current dir */
                break;
            
            /* escape */
            case ESC:
                break;

            case KEY_RESIZE:
                for (int i = 0; i < 2; i++)
                    delwin(windows[i].window);

                endwin();
                init_windows();
                break;
            default:
                break;
        }
    }
    clear_files();
    clear_marked();
    endwin();
    return 0;
}

/*
 * Change directory in window with selection
 */
void change_dir(const char *buf, int selection)
{
    strcpy(cwd, buf);
    clear_files();
    populate_files(cwd, 0);
    current_selection = selection;
    highlight_current_line();
}

/*
 * Recursively create directory by creating each subdirectory
 */
int mkdir_p(const char *destdir)
{
    char *path = memalloc(PATH_MAX * sizeof(char));
    char *token = strtok(path, "/");
    char dir_path[PATH_MAX] = "";

    strcpy(path, destdir);
    if (destdir[0] == '~') {
        char *home = getenv("HOME");
        if (home == NULL) {
            wpprintw("$HOME is not defined");
            return -1;
        }
        /* replace ~ with home */
        memmove(path + strlen(home), path + 1, strlen(path));
        memcpy(path, home, strlen(home));
    }
    /* fix first / not appearing in the string */
    if (path[0] == '/')
        strcat(dir_path, "/");

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
            
            perror("ccc");
            free(path);
            return -1;
        }
        token = strtok(NULL, "/");
    }

    free(path);
    return 0;
}

/*
 * Read the provided directory and add all files in directory to linked list
 * ftype: normal files = 0, marked = 1
 * ep->d_name -> filename
 */
void populate_files(const char *path, int ftype)
{
    DIR *dp;
    struct dirent *ep;

    draw_border_title(directory_border,  true);
    if ((dp = opendir(path)) != NULL) {
        /* clear directory window to ready for printing */
        wclear(directory_content);

        while ((ep = readdir(dp)) != NULL) {
            char *filename = memalloc(PATH_MAX * sizeof(char));
            /* make filename be basename of selected item just to pass check */
            filename[0] = '\0';
            strcat(filename, ep->d_name);

            /* can't be strncmp as that would filter out the dotfiles */
            if (strcmp(filename, ".") && strcmp(filename, "..")) {
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
    } else {
        perror("ccc");
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
 * ftype: normal file = 0, marked = 1
 */
long add_file_stat(char *filepath, int ftype)
{
    struct stat file_stat;
    if (stat(filepath, &file_stat) == -1) {
        /* can't be triggered? */
        if (errno == EACCES)
            return add_file(filepath, "", "", 8);
    }
    
    /* get file type and color, 4 chars for the type */
    char *type = memalloc(4 * sizeof(char));
    int color;
    if (S_ISDIR(file_stat.st_mode)) {
        strcpy(type, "DIR");            /* directory type */
        color = 5;                      /* blue color */
    } else if (S_ISREG(file_stat.st_mode)) {
        strcpy(type, "REG");            /* regular file */
        color = 8;                      /* white color */
    } else if (S_ISLNK(file_stat.st_mode)) {
        strcpy(type, "LNK");            /* symbolic link */
        color = 3;                      /* green color */
    } else if (S_ISCHR(file_stat.st_mode)) {
        strcpy(type, "CHR");            /* character device */
        color = 8;                      /* white color */
    } else if (S_ISSOCK(file_stat.st_mode)) {
        strcpy(type, "SOC");            /* socket */
        color = 8;                      /* white color */
    } else if (S_ISBLK(file_stat.st_mode)) {
        strcpy(type, "BLK");            /* block device */
        color = 4;                      /* yellow color */
    } else if (S_ISFIFO(file_stat.st_mode)) {
        strcpy(type, "FIF");            /* FIFO */
        color = 8;                      /* white color */
    } else {
        color = 8;                      /* white color */
    }

    /* if file is to be marked */
    if (ftype == 1) {
        long index = add_marked(filepath, type);
        /* free type and return without allocating more stuff */
        free(type);
        if (index != -1) {
            return index;   /* just marked */
        } else {
            return -1;      /* already marked */
        }
    }

    /* get last modified time */
    char *time = memalloc(20 * sizeof(char));
    /* format last modified time to a string */
    strftime(time, 20, "%Y-%m-%d %H:%M", localtime(&file_stat.st_mtime));
    
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
    /* max 25 chars due to long, space, suffix and null */
    char *size = memalloc(25 * sizeof(char));
    int unit = 0;
    const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    while (bytes > 1024) {
        bytes /= 1024;
        unit++;
    }
    /* display sizes */
    sprintf(size, "%.3g%s", bytes, units[unit]);

    char *total_stat = memalloc(45 * sizeof(char));
    snprintf(total_stat, 45, "%-18s %-8s", time, size);
    total_stat[strlen(total_stat)] = '\0';

    long index = add_file(filepath, total_stat, type, color);

    free(time);
    free(size);
    free(total_stat);
    free(type);
    return index;
}


/*
 * Highlight current line by reversing the color
 */
void highlight_current_line()
{
    long overflow = 0;
    if (current_selection > LINES - 4) {
        /* overflown */
        overflow = current_selection - (LINES - 4);
    }

    /* calculate range of files to show */
    long range = files_len();
    /* not highlight if no files in directory */
    if (range == 0) {
        wprintw(preview_content, "empty directory");
        wrefresh(preview_content);
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

            /* update the panel */
            wclear(panel);
            
            /* check for marked files */
            long num_marked = marked_len();
            if (num_marked > 0) {
                /* Determine length of formatted string */
                int m_len = snprintf(NULL, 0, "[%ld] selected", num_marked);
                char *selected = memalloc((m_len + 1) * sizeof(char));

                snprintf(selected, m_len + 1, "[%ld] selected", num_marked);
                wprintw(panel, "(%ld/%ld) %s %s", current_selection + 1, files_len(), selected, cwd);
            } else  {
                wprintw(panel, "(%ld/%ld) %s", current_selection + 1, files_len(), cwd);
            }
        }
        /* print the actual filename and stats */
        char *line = get_line(i);
        int color = get_color(i);
        /* check is file marked for action */
        bool marked = in_marked(get_filepath(i));
        if (marked) {
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

        if (marked) {
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
    show_file_content();
    wrefresh(preview_content);
}

/*
 * Get file content into buffer and show it to preview window
 */
void show_file_content()
{
    wclear(preview_content);
    file *current_file = get_file((long) current_selection);

    if (strncmp(current_file->type, "DIR", 3) == 0)
        return;

    FILE *file = fopen(current_file->path, "r");
    if (file == NULL) {
        mvwprintw(preview_content, 0, 0, "Unable to read %s", current_file->path);
        return;
    }
    draw_border_title(preview_border, true);

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

        char *filename = get_filepath(current_selection);
        int length = strlen(editor) + strlen(filename) + 2; /* one for space one for null */
        char command[length];

        snprintf(command, length, "%s %s", editor, filename);
        system(command);
        reset_prog_mode();  /* return to previous tty mode */
        refresh();          /* store the screen contents */
        free(filename);
    }
}

/*
 * Print line to the panel
 */
void wpprintw(const char *line)
{
    wclear(panel);
    wprintw(panel, "%s", line);
    wrefresh(panel);
}

void init_windows()
{
    /*------------------------------+
    |----border(0)--||--border(2)--||
    ||              ||             ||
    || content (1)  || content (3) ||
    ||              ||             ||
    ||              ||             ||
    |---------------||-------------||
    +==========panel (4)===========*/
    
    /*                         lines,          cols,           y,          x             */
    directory_border =  newwin(LINES - PH,     half_width,     0,          0             );
    directory_content = newwin(LINES - PH -2,  half_width - 2, 1,          1);
    preview_border =    newwin(LINES - PH,     half_width,     0,          half_width    );
    preview_content =   newwin(LINES - PH - 2, half_width - 2, 1,          half_width + 1);
    panel =             newwin(PH,             COLS,           LINES - PH, 0             );
    
    /* draw border around windows     */
    draw_border_title(directory_border,  true);
    draw_border_title(preview_border,   true);

    scrollok(directory_content, true);
    /*                          window             location  y,            x              */
    windows[0] = (WIN_STRUCT) { directory_border,  0,        0,            0              };
    windows[1] = (WIN_STRUCT) { directory_border,  0,        0,            0              };
    windows[2] = (WIN_STRUCT) { preview_border,    1,        0,            half_width     };
    windows[3] = (WIN_STRUCT) { preview_content,   1,        0,            half_width     };
    windows[4] = (WIN_STRUCT) { panel,             2,        LINES - PH,   0              };
}

/*
 * Draw the border of the window depending if it's active or not
 */
void draw_border_title(WINDOW *window, bool active)
{
    /* turn on color depends on active */
    if (active) {
        wattron(window, COLOR_PAIR(7));
    } else {
        wattron(window, COLOR_PAIR(5));
    }
    /* draw top border */
    mvwaddch(window, 0, 0, ACS_ULCORNER);  /* upper left corner */
    mvwhline(window, 0, 1, ACS_HLINE, half_width - 2); /* top horizontal line */
    mvwaddch(window, 0, half_width - 1, ACS_URCORNER); /* upper right corner */

    /* draw side border */
    mvwvline(window, 1, 0, ACS_VLINE, LINES - 2); /* left vertical line */
    mvwvline(window, 1, half_width - 1, ACS_VLINE, LINES - 2); /* right vertical line */

    /* draw bottom border
     * make space for the panel */
    mvwaddch(window, LINES - PH - 1, 0, ACS_LLCORNER); /* lower left corner */
    mvwhline(window, LINES - PH - 1, 1, ACS_HLINE, half_width - 2); /* bottom horizontal line */
    mvwaddch(window, LINES - PH - 1, half_width - 1, ACS_LRCORNER); /* lower right corner */

    /* turn color off after turning it on */
    if (active) {
        wattroff(window, COLOR_PAIR(7));
    } else {
        wattroff(window, COLOR_PAIR(5));
    }
    wrefresh(window);  /* Refresh the window to see the colored border and title */
}
