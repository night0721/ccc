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

#include "file.h"
#include "util.h"
#include "config.h"

/* functions' definitions */
void show_help();
void start_ccc();
void change_dir(const char *buf, int selection);
int mkdir_p(const char *destdir);
void populate_files(const char *path, int ftype);
int get_directory_size(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
long add_file_stat(char *filepath, int ftype);
void highlight_current_line();
void show_file_content();
void edit_file();
int write_last_d();
void wpprintw(const char *line);
void init_windows();
void draw_border_title(WINDOW *window, bool active);

/* global variables */
unsigned int focus = 0;
long current_selection = 0;
bool dirs_size = DIRS_SIZE;
char *cwd;
char *p_cwd; /* previous cwd */
int half_width;
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

    cwd = memalloc(PATH_MAX * sizeof(char));
    p_cwd = memalloc(PATH_MAX * sizeof(char));
    getcwd(cwd, PATH_MAX);
    start_ccc();

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
                change_dir(cwd, 0); 
                break;

            /* go back by backspace or h or left arrow */
            case BACKSPACE:
            case LEFT:
            case 'h':;
                /* get parent directory */
                strcpy(p_cwd, cwd);
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
                strcpy(p_cwd, cwd);
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

            /* go to previous dir */
            case '-':
                change_dir(p_cwd, 0);
                break;

            case '?':
                show_help();
                break;

            /* mark one file */
            case SPACE:
                add_file_stat(get_filepath(current_selection), 1);
                highlight_current_line();
                break;

            /* mark all files in directory */
            case 'a':
                populate_files(cwd, 2);
                change_dir(cwd, current_selection); /* reload current dir */
                break;

            /* mark actions: */
            /* delete */
            case 'd':;
                if (marked_len()) {
                    ;
                }
                break;

            /* move */
            case 'm':;
                if (marked_len()) {
                    ;
                }
                break;

            /* copy */
            case 'c':;
                if (marked_len()) {
                    ;
                }
                break;

            /* symbolic link */
            case 's':;
                if (marked_len()) {
                    ;
                }
                break;

            /* bulk rename */
            case 'b':;
                if (marked_len()) {
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

void show_help() {
    wclear(directory_content);
    wclear(preview_content);
    wprintw(directory_content,"h: go to parent dir\nj: scroll down\nk: scroll up\nl: go to child dir\n\nleft:  go to parent dir\ndown:  scroll down\nup:    scroll up\nright: go to child dir\n\nenter: go to child dir/open file\nbackspace: go to parent dir\n\ngg: go to top\nG: go to bottom\n\nctrl+u: jump up\nctrl+d: jump down\n\nt: go to trash dir\n~: go to home dir\n-: go to previous dir\nz: refresh current dir\n\nA: show directory disk usage/block size\nspace: mark file\na: mark all files in directory\n\n?: show help\nq: exit");
    wpprintw("Visit https://github.com/piotr-marendowski/ccc or use 'man ccc' for help");
    wrefresh(directory_content);
    wrefresh(preview_content);
}

void start_ccc()
{
    half_width = COLS / 2;
    init_windows();
    refresh();
    populate_files(cwd, 0);
    highlight_current_line();
}

/*
 * Change directory in window with selection
 */
void change_dir(const char *buf, int selection)
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
    clear_files();
    populate_files(cwd, 0);
    current_selection = selection;
    highlight_current_line();
}

/*
 * Recursively create directory by creating each subdirectory
 * like mkdir -p
 */
int mkdir_p(const char *destdir)
{
    char *path = memalloc(PATH_MAX * sizeof(char));
    char dir_path[PATH_MAX] = "";

    if (destdir[0] == '~') {
        char *home = getenv("HOME");
        if (home == NULL) {
            wpprintw("$HOME is not defined");
            return -1;
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
 * ftype: normal files = 0, marked = 1, marking ALL = 2
 * ep->d_name -> filename
 */
void populate_files(const char *path, int ftype)
{
    DIR *dp;
    struct dirent *ep;

    #if DRAW_BORDERS
        draw_border_title(directory_border, true);
    #endif
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
 * ftype: normal file = 0, normal marked = 1, marking ALL = 2
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
    if (ftype == 1 || ftype == 2) {
        /* force if user is marking all files */
        bool force = ftype == 2 ? true : false;
        long index = add_marked(filepath, type, force);
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
    const char* units[] = {"B", "KiB", "MiB", "GiB", "TiB", "PiB"};
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
    #if DRAW_PREVIEW
        show_file_content();
    #endif
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
    #if DRAW_BORDERS
        draw_border_title(preview_border, true);
    #endif

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

int write_last_d() {
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
void wpprintw(const char *line)
{
    wclear(panel);
    wprintw(panel, "%s", line);
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
