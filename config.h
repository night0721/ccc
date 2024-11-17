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

/*
key keybindings[] = {

}
*/
