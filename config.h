/* Settings */
#define PH 1                /* panel height */
#define JUMP_NUM 14         /* how long ctrl + u/d jump are */
#define PATH_MAX 4096       /* max length of the path */
#define DECIMAL_PLACES 1    /* how many decimal places show size with */

/* Size units */
static const char* units[] = {"B", "K", "M", "G", "T", "P"};

/* Set width offset for windows:
+-------------%-------------+
|             %             |
|    files    %   preview   |
|             %             |
+=============%=============+
set where the % line between them resides

In COLS:
0 will make them equal (at the center),
15 will make files bigger
-15 will make preview bigger */
#define WINDOW_OFFSET 0

/* Options */
#define DRAW_BORDERS true   /* draw borders around windows */
#define DRAW_PREVIEW true   /* draw file preview */

#define SHOW_HIDDEN true    /* show hidden files/dotfiles at startup */
#define SHOW_DETAILS true   /* show file details at startup */
#define SHOW_ICONS true     /* show file icons at startup */

/* Calculate directories' sizes RECURSIVELY upon entering
   `A` keybind at the startup
 **VERY EXPENSIVE**, **CAN TAKE UP TO A MINUTE IN ROOT** */
#define DIRS_SIZE false

/* Default text editor */
#define EDITOR "nvim"

/* File location to write last directory */
#define LAST_D "~/.cache/ccc/.ccc_d"

/* Will create this directory if doesn't exist! */
#define TRASH_DIR "~/.cache/ccc/trash/"

/* Keybindings */
#define CTRLD 0x04
#define ENTER 0xA
#define CTRLU 0x15
#define ESC 0x1B
#define SPACE 0x20
#define TILDE 0x7E
#define DOWN 0x102
#define UP 0x103
#define LEFT 0x104
#define RIGHT 0x105
#define BACKSPACE 0x107
