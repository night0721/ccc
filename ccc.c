#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <linux/limits.h>
#include <dirent.h>         /* directories etc. */
#include <ncurses.h>

#include "file.h"
#include "util.h"

#define ESC 0x1B    /* \e or \033 */
#define PH 1        /* panel height */

typedef struct {
    WINDOW *window;
    int location;
    int y;
    int x;
} WIN_STRUCT;

/* functions' definitions */
void list_files(char *path);
void highlight_current_line();
void show_file_content();
void init_windows();
void draw_border_title(WINDOW *window, bool active);

/* global variables */
unsigned int focus = 0;
long current_selection = 0;
int half_width;
WIN_STRUCT windows[3];

int main(int argc, char** argv)
{
    char cwd[PATH_MAX];

    if (argc > 1 && strcmp(argv[2], "-h"))
        die("Usage: ccc filename");

    /* check if it is interactive shell */
    if (!isatty(STDIN_FILENO)) 
        die("ccc: No tty detected. ccc requires an interactive shell to run.\n");

    /* set window name */
    printf("%c]2;ccc: %s%c", ESC, getcwd(cwd, sizeof(cwd)), ESC);

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
        start_color();
    }

    init_pair(1, COLOR_BLUE, COLOR_BLACK);  /* foreground, background */
    init_pair(2, COLOR_CYAN, COLOR_BLACK);  /* active color */

    refresh();
    half_width = COLS / 2;
    init_windows();
    list_files(getcwd(cwd, sizeof(cwd)));
    highlight_current_line();

    int ch, second;
    while (1) {
        if (COLS < 80 || LINES < 24) {
            endwin();
            die("ccc: Terminal size needs to be at least 80x24\n");
        }
        ch = getch();
        switch (ch) {
            case 'q':
                endwin();
                return 0;
            case '.':
                list_files(getcwd(cwd, sizeof(cwd)));
                break;
            /* go back */
            case 'h':
                break;
            /* enter directory/open a file */
            case 'l':
                if (focus == 0) focus++;
                else if (focus == 1) focus--;
                break;
            /* go up */
            case 'k':
                if (current_selection > 0)
                    current_selection--;

                highlight_current_line();
                break;
            /* go down */
            case 'j':
                if (current_selection < files_len() - 1)
                    current_selection++;

                highlight_current_line();
                break;
            /* jump to the bottom */
            case 'G':
                current_selection = files_len() - 1;
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
            case 27: /* esc */
                break;
            case KEY_RESIZE:
                for (int i = 0; i < 2; i++) {
                    delwin(windows[i].window);
                }
                init_windows();
                break;
            default:
                break;
        }
    }
    endwin();
    return 0;
}

/*
 * Read the provided directory and list all files to window 0
 * ep->d_name -> filename
 */
void list_files(char *path)
{
    DIR *dp;
    struct dirent *ep;

    dp = opendir(path);
    if (dp != NULL) {
        int count = 0;

        while ((ep = readdir(dp)) != NULL) {
            char *filename = strdup(ep->d_name);
            if (filename == NULL) {
                /* memory allocation failed */
                perror("ccc");
                fprintf(stderr, "ccc: Cannot read filename %s.", ep->d_name);
                exit(EXIT_FAILURE); 
            }
            /* can't be strncmp as that would filter out the dotfiles */
            if (strcmp(filename, ".") && strcmp(filename, "..")) {
                add_file(filename);
                mvwprintw(windows[0].window, count + 1, 1, "%s", filename);
                count++;
            }
        }
        closedir(dp);
        wrefresh(windows[0].window);
    } else {
        perror("ccc");
    }
}

/*
 * Highlight current line by reversing the color
 */
void highlight_current_line()
{
    char cwd[PATH_MAX];
    char *filename;

    for (long i = 0; i < files_len(); i++) {
        if (i == current_selection) {
            wattron(windows[0].window, A_REVERSE);
            wattron(windows[0].window, COLOR_PAIR(1));

            /* update the panel */
            wclear(windows[2].window);
            wprintw(windows[2].window, "(%ld/%ld) %s", i + 1, files_len(),
                    getcwd(cwd, sizeof(cwd)));
        }
        /* print the actual filename */
        filename = get_filename(i);
        mvwprintw(windows[0].window, i + 1, 1, "%s", filename);
        wattroff(windows[0].window, A_REVERSE);
        wattroff(windows[0].window, COLOR_PAIR(1));
    }

    wrefresh(windows[0].window);
    wrefresh(windows[2].window);
    /* show file content every time cursor changes */
    show_file_content();
}

/*
 * Get file content into buffer and show it to preview window
 */
void show_file_content()
{
    FILE *file = fopen(get_filename((long) current_selection), "rb");
    if (file) {
        wclear(windows[1].window);
        draw_border_title(windows[1].window, true);

        fseek(file, 0, SEEK_END);
        /* check if file isn't empty */
        long length = ftell(file);
        if (length != 0) {
            fseek(file, 0, SEEK_SET);

            char *buffer = memalloc(length * sizeof(char));
            fread(buffer, 1, length, file);

            mvwprintw(windows[1].window, 1, 1, "%s", buffer);
            wrefresh(windows[1].window);
        } else {
            wclear(windows[1].window);
        }
        fclose(file);
    }
}

void init_windows()
{
    /*-----------------------------+
    |               |              |
    |               |              |
    | directory (0) |  preview (1) |
    |               |              |
    |               |              |
    |               |              |
    +==========panel (2)==========*/
    
    /*                         lines,  cols,        y,          x         */
    WINDOW *directory = newwin(LINES,  half_width,  0,          0         );
    WINDOW *preview =   newwin(LINES,  half_width,  0,          half_width);
    WINDOW *panel =     newwin(PH,     COLS,        LINES - PH, 0         );
    
    /* draw border around windows     */
    draw_border_title(directory, true);
    draw_border_title(preview,   false);

    /*                          window      location  y,            x           */
    windows[0] = (WIN_STRUCT) { directory,  0,        0,            0           };
    windows[1] = (WIN_STRUCT) { preview,    1,        0,            half_width  };
    windows[2] = (WIN_STRUCT) { panel,      2,        LINES - PH,   0           };
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
