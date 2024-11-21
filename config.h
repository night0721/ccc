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
	{'q', ACT_QUIT},
	{'z', ACT_RELOAD},
	{BACKSPACE, ACT_BACK},
	{ARROW_LEFT, ACT_BACK},
	{'h', ACT_BACK},
	{ENTER, ACT_ENTER},
	{ARROW_RIGHT, ACT_ENTER},
	{'l', ACT_ENTER},
	{CTRLU, ACT_JUMP_UP},
	{CTRLD, ACT_JUMP_DOWN},
	{ARROW_DOWN, ACT_DOWN},
	{ARROW_UP, ACT_UP},
	{'k', ACT_UP},
	{'j', ACT_DOWN},
	{'G', ACT_BOTTOM},
	{'g', ACT_TOP},
	{'~', ACT_HOME},
	{'t', ACT_TRASH_DIR},
	{'u', ACT_SORT},
	{'A', ACT_SHOW_DIR_SIZE},
	{'-', ACT_PREV_DIR},
	{'?', ACT_SHOW_HELP},
	{'.', ACT_HIDDEN_FILES},
	{'i', ACT_FILE_DETAILS},
	{'w', ACT_SHOW_ICONS},
	{'f', ACT_CREATE_FILE},
	{'n', ACT_CREATE_DIR},
	{'r', ACT_RENAME_FILE},
	{':', ACT_GOTO_DIR},
	{'X', ACT_TOGGLE_EXE},
	{'!', ACT_START_SHELL},
	{'y', ACT_COPY_FILENAME},
	{'o', ACT_OPEN_FILE},
	{'O', ACT_OPEN_FILE_DETACHED},
	{'x', ACT_VIEW_FILE_ATTR},
	{'e', ACT_SHOW_HIST},
	{'1', ACT_FAV1},
	{'2', ACT_FAV2},
	{'3', ACT_FAV3},
	{'4', ACT_FAV4},
	{'5', ACT_FAV5},
	{'6', ACT_FAV6},
	{'7', ACT_FAV7},
	{'8', ACT_FAV8},
	{'9', ACT_FAV9},
	{' ', ACT_MARK_FILE},
	{'a', ACT_MARK_ALL},
	{'d', ACT_DELETE},
	{'m', ACT_MOVE},
	{'c', ACT_COPY},
	{'s', ACT_SYM_LINK},
	{'b', ACT_BULK_RENAME},
};

