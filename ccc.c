#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <linux/limits.h>
#include <dirent.h>         /* directories etc. */
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <ncurses.h>

#include "file.h"
#include "util.h"

#define ESC 0x1B    /* \e or \033 */
#define PH 1        /* panel height */
#define JUMP_NUM 14 /* how long ctrl + u/d jump are */

typedef struct {
    WINDOW *window;
    int location;
    int y;
    int x;
} WIN_STRUCT;

/* functions' definitions */
void list_files(const char *path);
long long get_directory_size(const char *path);
long add_file_stat(char *filename);
void highlight_current_line();
void show_file_content();
void edit_file();
void init_windows();
void draw_border_title(WINDOW *window, bool active);

/* global variables */
unsigned int focus = 0;
long current_selection = 0;
char *cwd;
int half_width;
WIN_STRUCT windows[5];
WINDOW *directory_border;
WINDOW *directory_content;
WINDOW *preview_border;
WINDOW *preview_content;
WINDOW *panel;

int main(int argc, char** argv)
{
    if (argc > 1 && strcmp(argv[1], "-h") == 0)
        die("Usage: ccc filename");

    /* check if it is interactive shell */
    if (!isatty(STDIN_FILENO)) 
        die("ccc: No tty detected. ccc requires an interactive shell to run.\n");

    /* initialize screen, don't print special chars,
     * make ctrl + c work, don't show cursor */
    initscr();
    noecho();
    cbreak();
    curs_set(0);

    /* check terminal has colors */
    if (!has_colors()) {
        endwin();
        die("ccc: Color is not supported in your terminal.\n");
    } else {
        use_default_colors();
        start_color();
    }

    init_pair(1, COLOR_BLUE, -1);  /* foreground, background */
    init_pair(2, COLOR_CYAN, -1);  /* active color */

    half_width = COLS / 2;
    init_windows();
    refresh();

    cwd = memalloc(PATH_MAX * sizeof(char));
    getcwd(cwd, PATH_MAX);

    list_files(cwd);
    highlight_current_line();

    /* set window name */
    printf("%c]2;ccc: %s%c", ESC, cwd, ESC);

    int ch, second;
    while (1) {
        if (COLS < 80 || LINES < 24) {
            endwin();
            die("ccc: Terminal size needs to be at least 80x24\n");
        }
        ch = getch();
        switch (ch) {
            /* quit */
            case 'q':
                endwin();
                return 0;

            /* reload */
            case '.':
                clear_files();
                list_files(cwd);
                current_selection = 0;
                highlight_current_line();
                break;

            /* go back */
            case 'h':
                clear_files();
                /* get parent directory */
                char *last_slash = strrchr(cwd, '/');
                if (last_slash != NULL) {
                    *last_slash = '\0';
                    list_files(cwd);
                    current_selection = 0;
                    highlight_current_line();
                }
                break;

            /* enter directory/open a file */
            case 'l':;
                file *file = get_file(current_selection);
                if (file != NULL) {
                    /* check if it is directory or regular file */
                    if (strncmp(file->type, "DIR", 3) == 0) {
                        /* change cwd to directory */
                        strcpy(cwd, file->path);
                        clear_files();
                        list_files(cwd);
                        current_selection = 0;
                        highlight_current_line();
                    }
                }
                break;
                /*
                if (focus == 0) focus++;
                else if (focus == 1) focus--;
                break;
                */

            /* jump up */
            case '\x15':
                if ((current_selection - JUMP_NUM) > 0)
                    current_selection -= JUMP_NUM;
                else
                    current_selection = 0;

                highlight_current_line();
                break;

            /* go up */
            case 'k':
                if (current_selection > 0)
                    current_selection--;

                highlight_current_line();
                break;

            /* jump down */
            case '\x04':
                if ((current_selection + JUMP_NUM) < (files_len() - 1))
                    current_selection += JUMP_NUM;
                else
                    current_selection = (files_len() - 1);

                highlight_current_line();
                break;

            /* go down */
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
                second = getch();
                switch (second) {
                    case 'g':
                        current_selection = 0;
                        highlight_current_line();
                        break;
                    default:
                        break;
                }
                break;
            case 'e':
                edit_file();
                break;
            case 27: /* esc */
                break;
            case KEY_RESIZE:
                for (int i = 0; i < 2; i++) {
                    delwin(windows[i].window);
                }
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
 * Read the provided directory and list all files to window 0
 * ep->d_name -> filename
 */
void list_files(const char *path)
{
    DIR *dp;
    struct dirent *ep;

    draw_border_title(directory_border,  true);
    if ((dp = opendir(path)) != NULL) {
        int count = 0;
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
                strcat(filename, ep->d_name); /* add file name */

                long index = add_file_stat(filename);
                char *line = get_line(index);
               
                mvwprintw(directory_content, count, 0, "%s", line);
                free(line);
                count++;
            }
            free(filename);
        }
        closedir(dp);
        wrefresh(directory_content);
    } else {
        perror("ccc");
    }
}

long long get_directory_size(const char *path)
{
    DIR *dp;
    struct dirent *ep;
    struct stat statbuf;
    long long total_size = 0;

    if ((dp = opendir(path)) != NULL) {
        while ((ep = readdir(dp)) != NULL) {
            if (strcmp(ep->d_name, ".") == 0 || strcmp(ep->d_name, "..") == 0) {
                continue;
            }
            /* build full path of entry */
            char full_path[PATH_MAX];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, ep->d_name);

            if (lstat(full_path, &statbuf) == -1) {
                perror("lstat");
                closedir(dp);
                return -1;
            }
            /* recursively calculate its size if it is directory */
            if (S_ISDIR(statbuf.st_mode)) {
                total_size += get_directory_size(full_path);
            } else {
                /* else add the size of the file to the total */
                total_size += statbuf.st_size;
            }
        }
        closedir(dp);
    } else {
        perror("ccc");
        return -1;
    }

    return total_size;
}

/*
 * Get file's last modified time, size, type
 * Add that file into list
 */
long add_file_stat(char *filepath)
{
    struct stat file_stat;
    if (stat(filepath, &file_stat) == -1) {
        /* can't be triggered ? */
        if (errno == EACCES) {
            return add_file(filepath, "", "");
        }
    }

    /* get last modified time */
    char *time = memalloc(20 * sizeof(char));
    /* format last modified time to a string */
    strftime(time, 20, "%Y-%m-%d %H:%M", localtime(&file_stat.st_mtime));
    
    
    /* get file size */
    double bytes = file_stat.st_size;
    if (S_ISDIR(file_stat.st_mode)) {
        bytes = (double) get_directory_size(filepath);
    }
    /* max 25 chars due to long, space, suffix and nul */
    char *size = memalloc(25 * sizeof(char));
    int unit = 0;
    const char* units[] = {"B", "KiB", "MiB", "GiB", "TiB", "PiB"};
    while (bytes > 1024) {
        bytes /= 1024;
        unit++;
    }
    sprintf(size, "%.*f%s", unit, bytes, units[unit]);
    
    /* get file type */
    char *type = memalloc(4 * sizeof(char)); /* 3 chars for type */
    if (S_ISDIR(file_stat.st_mode)) {
        strcpy(type, "DIR"); /* directory */
    } else if (S_ISREG(file_stat.st_mode)) {
        strcpy(type, "REG"); /* regular file */
    } else if (S_ISLNK(file_stat.st_mode)) {
        strcpy(type, "LNK"); /* symbolic link */
    } else if (S_ISCHR(file_stat.st_mode)) {
        strcpy(type, "CHR"); /* character device */
    } else if (S_ISSOCK(file_stat.st_mode)) {
        strcpy(type, "SOC"); /* socket */
    } else if (S_ISBLK(file_stat.st_mode)) {
        strcpy(type, "BLK"); /* block device */
    } else if (S_ISFIFO(file_stat.st_mode)) {
        strcpy(type, "FIF"); /* FIFO */
    }
    /* don't know how to handle socket, block device, character device and FIFO */

    char *total_stat = memalloc(45 * sizeof(char));
    snprintf(total_stat, 45, "%-18s %-10s", time, size);
    total_stat[strlen(total_stat)] = '\0';

    free(time);
    free(size);
    
    long index = add_file(filepath, total_stat, type);

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
    if (range > LINES - 3) {
        /* if there is more files than lines available to display*/
        /* shrink range to avaiable lines to display */
        /* with overflow to keep number of iterations to be constant */
        range = LINES - 3 + overflow;
    }
    
    wclear(directory_content);
    long line_count = 0;
    for (long i = overflow; i < range; i++) {
        if ((overflow == 0 && i == current_selection) || (overflow != 0 && i == current_selection)) {
            wattron(directory_content, A_REVERSE);
            wattron(directory_content, COLOR_PAIR(1));

            /* update the panel */
            wclear(panel);
            wprintw(panel, "(%ld/%ld) %s", current_selection + 1, files_len(), cwd);
        }
        /* print the actual filename and stats */
        char *line = get_line(i);
        if (overflow > 0)
            mvwprintw(directory_content, line_count, 0, "%s", line);
        else
            mvwprintw(directory_content, i, 0, "%s", line);
        wattroff(directory_content, A_REVERSE);
        wattroff(directory_content, COLOR_PAIR(1));
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
    if (strncmp(current_file->type, "DIR", 3) == 0) return;
    FILE *file = fopen(current_file->path, "rb");
    if (file) {
        draw_border_title(preview_border, true);
        fseek(file, 0, SEEK_END);
        long length = ftell(file);
        /* check if file isn't empty */
        if (length != 0 && length < 4096) {
            fseek(file, 0, SEEK_SET); /* set cursor back to start of file */
            char *buffer = memalloc(length * sizeof(char));
            fread(buffer, 1, length, file);
            mvwprintw(preview_content, 0, 0, "%s", buffer);
            free(buffer);
        } else {
            wclear(preview_content);
        }
        fclose(file);
    }
}

/*
 * Opens $EDITOR to edit the file
 */
void edit_file()
{
    char *editor = getenv("EDITOR");
    if (editor == NULL) {
        wclear(panel);
        wprintw(panel, "Cannot get EDITOR variable, is it defined?");
        wrefresh(panel);
        return;
    } else {
        def_prog_mode(); /* save the tty modes */
        endwin(); /* end curses mode temporarily */
        char *filename = get_filepath(current_selection);
        int length = strlen(editor) + strlen(filename) + 2; /* one for space one for nul */
        char command[length];
        snprintf(command, length, "%s %s", editor, filename);
        system(command);
        reset_prog_mode(); /* return to previous tty mode */
        refresh(); /* store the screen contents */
        free(filename);
    }
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
    draw_border_title(preview_border,   false);

    scrollok(directory_content, true);
    /*                          window             location  y,            x           */
    windows[0] = (WIN_STRUCT) { directory_border,         0,        0,            0              };
    windows[1] = (WIN_STRUCT) { directory_border,         0,        0,            0              };
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
        wattron(window, COLOR_PAIR(2));
    } else {
        wattron(window, COLOR_PAIR(1));
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
        wattroff(window, COLOR_PAIR(2));
    } else {
        wattroff(window, COLOR_PAIR(1));
    }
    wrefresh(window);  /* Refresh the window to see the colored border and title */
}
