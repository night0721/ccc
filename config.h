#define PATH_MAX 4096 /* Max length of path */
/* Settings */
static int panel_height = 1; /* panel height */
static int jump_num = 14; /* Length of ctrl + u/d jump */
static int decimal_place = 1; /* Number of decimal places size can be shown */

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
static int window_offset = -30;

/* Options */
static int show_hidden = 1; /* Show hidden files/dotfiles at startup */
static int show_details = 0; /* Show file details at startup */
static int show_icons = 1; /* Show file icons at startup */

/* Calculate directories' sizes RECURSIVELY upon entering
   `A` keybind at the startup
 **VERY EXPENSIVE**, **CAN TAKE UP TO A MINUTE IN ROOT** */
static int dirs_size = 0;

/* Default text editor */
static const char *editor = "nvim";

/* File location to write last directory */
static char last_d[PATH_MAX] = "~/.cache/ccc/.ccc_d";

/* Will create this directory if doesn't exist! */
static char trash_dir[PATH_MAX]  = "~/.cache/ccc/trash/";

/* Keybindings */
#define CTRLD 0x04
#define ENTER 0xD
#define CTRLU 0x15
#define SPACE 0x20
#define TILDE 0x7E

enum keys {
	BACKSPACE = 127,
	ARROW_LEFT = 1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	DEL_KEY,
	HOME_KEY,
	END_KEY,
	PAGE_UP,
	PAGE_DOWN
};
