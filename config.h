static int panel_height = 1; /* Panel height */
static int jump_num = 14; /* Length of ctrl + u/d jump */
static int decimal_place = 1; /* Number of decimal places size can be shown */

/* Colors for files */
enum files_colors {
	DIR_COLOR = 34, /* Directory */
	REG_COLOR = 37, /* Regular file */
	LNK_COLOR = 32, /* Symbolic link */
	CHR_COLOR = 33, /* Character device */
	SOC_COLOR = 35, /* Socket */
	BLK_COLOR = 33, /* Block device */
	FIF_COLOR = 35, /* FIFO */
	DEF_COLOR = 37, /* Default */
	EXE_COLOR = 32, /* Executable file */
	MAR_COLOR = 36, /* Marked files */
};

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

/* Default clipboard program */
static const char *clipboard = "wl-copy";

/* File location to write last directory */
static char last_d[PATH_MAX] = "~/.cache/ccc/.ccc_d";

/* Will create this directory if doesn't exist! */
static char trash_dir[PATH_MAX]  = "~/.cache/ccc/trash/";

static Key keybindings[] = {
	{'q', quit, {0}},
	{'z', reload, {0}},
	{BACKSPACE, nav_back, {0}},
	{ARROW_LEFT, nav_back, {0}},
	{'h', nav_back, {0}},
	{ENTER, nav_enter, {0}},
	{ARROW_RIGHT, nav_enter, {0}},
	{'l', nav_enter, {0}},
	{CTRLU, nav_jump_up, {0}},
	{CTRLD, nav_jump_down, {0}},
	{ARROW_UP, nav_up, {0}},
	{'k', nav_up, {0}},
	{ARROW_DOWN, nav_down, {0}},
	{'j', nav_down, {0}},
	{'G', nav_bottom, {0}},
	{'g', nav_top, {0}},
	{'~', goto_home_dir, {0}},
	{'t', goto_trash_dir, {0}},
	{'u', sort_files, {0}},
	{'A', show_dir_size, {0}},
	{'-', prev_dir, {0}},
	{'?', show_help, {0}},
	{'.', toggle_hidden_files, {0}},
	{'i', toggle_file_details, {0}},
	{'w', toggle_show_icons, {0}},
	{'f', create_file, {0}},
	{'n', create_dir, {0}},
	{'r', rename_file, {0}},
	{':', goto_dir, {0}},
	{'X', toggle_executable, {0}},
	{'!', start_shell, {0}},
	{'y', yank_clipboard, {0}},
	{'o', open_with, {0}},
	{'O', open_detached, {0}},
	{'x', view_file_attr, {0}},
	{'e', show_history, {0}},
	{'1', open_fav, {.i = 1}},
	{'2', open_fav, {.i = 2}},
	{'3', open_fav, {.i = 3}},
	{'4', open_fav, {.i = 4}},
	{'5', open_fav, {.i = 5}},
	{'6', open_fav, {.i = 6}},
	{'7', open_fav, {.i = 7}},
	{'8', open_fav, {.i = 8}},
	{'9', open_fav, {.i = 9}},
	{' ', mark_file, {0}},
	{'a', mark_all, {0}},
	{'d', delete_files, {0}},
	{'m', move_files, {0}},
	{'c', copy_files, {0}},
	{'s', symbolic_link, {0}},
	{'b', bulk_rename, {0}},
};

