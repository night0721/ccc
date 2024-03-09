#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <linux/limits.h>
#include <dirent.h>
#include <ncurses.h>

#include "util.h"

typedef struct {
    WINDOW *window;
    int location;
    int y;
    int x;
} WIN_STRUCT;

void list_cwd_files();
void highlight_current_line();
void show_file_content();
long files_len();
void init_windows();
void draw_border_title(WINDOW *window, bool active);

unsigned int focus = 0;
int current_selection = 0;
char **files;
int halfx;
WIN_STRUCT windows[2];

int main(int argc, char** argv)
{
    
    // check if it is interactive shell
    if (!isatty(STDIN_FILENO)) 
    {
        die("ccc: No tty detected. ccc requires an interactive shell to run.\n");
    }
    initscr();
    noecho();
    curs_set(0);

    // check terminal have colors
    if (!has_colors()) 
    {
        endwin();
        die("ccc: Color is not supported in your terminal.\n");
    }
    start_color();
    init_pair(1, COLOR_BLUE, COLOR_BLACK);  // foreground, background
    init_pair(2, COLOR_CYAN, COLOR_BLACK); // active color
    refresh();
    halfx = COLS / 2;
    init_windows();
    list_cwd_files();
    highlight_current_line();
    int ch;
    while (1)
    {
        if (COLS < 80 || LINES < 24)
        {
            endwin();
            die("ccc: Terminal size needs to be at least 80x24\n");
        }
        ch = getch();
        if (ch == 'q')
            break;
        switch (ch)
        {
            case '.':
                list_cwd_files();
                break;
            case 'h':
            case 'l':
                if (focus == 0) focus++;
                else if (focus == 1) focus--;
                break;
            case 'k':
                if (current_selection > 0) {
                    current_selection--;
                }
                highlight_current_line();
                break;
            case 'j':
                if (current_selection < files_len() - 1) {
                    current_selection++;
                }
                highlight_current_line();
                break;
            case 27: // esc
                break;
            case KEY_RESIZE:
                for (int i = 0; i < 2; i++)
                {
                    delwin(windows[i].window);
                }
                init_windows();
                break;
        }
    }
    endwin(); // End curses
    return EXIT_SUCCESS;
    return 0;
}

void list_cwd_files()
{
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    DIR *dp;
    struct dirent *ep;
    dp = opendir(cwd);
    if (dp != NULL)
    {
        int count = 0;
        while ((ep = readdir(dp)) != NULL) {
            char *filename = strdup(ep->d_name);
            if (!filename)
            {
                perror("ccc");
                exit(EXIT_FAILURE);
            }
            // can't be strncmp as that filter out dotfiles
            if (strcmp(filename, ".") && strcmp(ep->d_name, ".."))
            {
                files = rememalloc(files, (count + 1) * sizeof(char *));
                files[count] = filename;
                mvwprintw(windows[0].window, count + 1, 1, "%s", filename);
                count++;
            }
        }
        closedir(dp);
        files[count] = NULL;
        wrefresh(windows[0].window);
    }
    else
    {
        perror("ccc");
    }
}

void highlight_current_line()
{
    for (int i = 0; i < files_len(); i++)
    {
        if (i == current_selection)
        {
            wattron(windows[0].window, A_REVERSE);
            wattron(windows[0].window, COLOR_PAIR(1));
        }
        mvwprintw(windows[0].window, i + 1, 1, "%s", files[i]);
        wattroff(windows[0].window, A_REVERSE);
        wattroff(windows[0].window, COLOR_PAIR(1));
    }
    wrefresh(windows[0].window);
    show_file_content();
}

void show_file_content()
{
    FILE *file = fopen(files[current_selection], "rb");
    if (file)
    {
        fseek(file, 0, SEEK_END);
        long length = ftell(file);
        fseek(file, 0, SEEK_SET);
        char *buffer = memalloc(length * sizeof(char));
        fread(buffer, 1, length, file);
        fclose(file);
        mvwprintw(windows[1].window, 1, 1, "%s", buffer);
        wrefresh(windows[1].window);
    }
}

long files_len() {
    long i = 0;
    while (files[i] != NULL) {
        i++;
    }
    return i;
}

void init_windows()
{
    /*
     * ------------------------------/
     * |  directory(0)/ preview(1)   /
     * |              /              /
     * |              /              /
     * |              /              /
     * |              /              /
     * |-----------------------------/
     */
    
    // create windows
    WINDOW *directory = newwin(LINES, halfx, 0, 0);
    WINDOW *preview = newwin(LINES, halfx, 0, halfx);
    
    // draw border around windows
    draw_border_title(directory,    true);
    draw_border_title(preview,      false);

    /*                          window      location  y,     x    */
    windows[0] = (WIN_STRUCT) { directory,  0,        0,     0     };
    windows[1] = (WIN_STRUCT) { preview,    1,        0,     halfx };
}

/*
 * draw the border of the window depends on it is active or not
 */
void draw_border_title(WINDOW *window, bool active)
{
    if (active) {
        wattron(window, COLOR_PAIR(2));
    } else {
        wattron(window, COLOR_PAIR(1));
    }
    // draw top border
    mvwaddch(window, 0, 0, ACS_ULCORNER);  // upper left corner
    mvwhline(window, 0, 1, ACS_HLINE, halfx - 2); // top horizontal line
    mvwaddch(window, 0, halfx - 1, ACS_URCORNER); // upper right corner

    // draw side border
    mvwvline(window, 1, 0, ACS_VLINE, LINES - 2); // left vertical line
    mvwvline(window, 1, halfx - 1, ACS_VLINE, LINES - 2); // right vertical line

    // draw bottom border
    mvwaddch(window, LINES - 1, 0, ACS_LLCORNER); // lower left corner
    mvwhline(window, LINES - 1, 1, ACS_HLINE, halfx - 2); // bottom horizontal line
    mvwaddch(window, LINES - 1, halfx - 1, ACS_LRCORNER); // lower right corner
    if (active) {
        wattroff(window, COLOR_PAIR(2));
    } else {
        wattroff(window, COLOR_PAIR(1));
    }
    wrefresh(window);  // Refresh the window to see the colored border and title
}
