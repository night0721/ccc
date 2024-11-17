#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <ftw.h>
#include <time.h>
#include <termios.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "file.h"
#include "icons.h"
#include "util.h"

/* Keybindings */
enum keys {
	CTRLD = 0x04,
	ENTER = 0xD,
	CTRLU = 0x15,
	SPACE = 0x20,
	TILDE = 0x7E,
	BACKSPACE = 127,
	ARROW_LEFT = 1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	DEL_KEY,
	HOME_KEY,
	END_KEY,
	PAGE_UP,
	PAGE_DOWN,
};

typedef struct {
	int key;
} key;

#define PATH_MAX 4096 /* Max length of path */

#include "config.h"

void handle_sigwinch(int ignore);
void cleanup(void);
void show_help(void);
char *check_trash_dir(void);
void change_dir(const char *buf, int selection, int ftype);
void mkdir_p(const char *destdir);
void populate_files(const char *path, int ftype, ArrayList **list);
int get_directory_size(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
void add_file_stat(char *filename, char *path, int ftype);
char *get_file_mode(mode_t mode);
void list_files(void);
void show_file_content(void);
void edit_file(void);
void toggle_executable(void);
void replace_home(char *str);
int write_last_d(void);
int sort_compare(const void *a, const void *b);
void sort_files(void);
char *get_panel_string(char *prompt);
void rename_file(void);
void goto_dir(void);
void create_dir(void);
void create_file(void);
void delete_files(void);
void start_shell(void);
void yank_clipboard(void);
void wpprintw(const char *fmt, ...);
void move_cursor(int row, int col);
int readch(void);
int get_window_size(int *row, int *col);
void bprintf(const char *fmt, ...);

/* global variables */
unsigned int focus = 0;
long sel_file = 0;
int file_picker = 1;
int to_open_file = 0;
char *argv_cp;
char *cwd;
char *p_cwd; /* previous cwd */
int half_width;
ArrayList *files;
ArrayList *marked;
ArrayList *tmp1; /* temp store of dirs */
ArrayList *tmp2; /* tmp store of files */
int rows, cols;
struct termios oldt, newt;

unsigned long total_dir_size = 0;

int main(int argc, char **argv)
{
	if (argc == 3) {
		if (strncmp(argv[2], "-p", 2) == 0)
			file_picker = 1;
	}
	if (argc <= 3 && argc != 1) {
		if (strncmp(argv[1], "-h", 2) == 0)
			die("Usage: ccc filename");

		struct stat st;
		if (lstat(argv[1], &st)) {
			perror("ccc");
			die("Error from lstat");
		}
		/* chdir to directory from argument */
		if (S_ISDIR(st.st_mode) && chdir(argv[1])) {
			perror("ccc");
			die("Error from chdir");
		} else if (S_ISREG(st.st_mode)) {
			argv_cp = estrdup(argv[1]);
			char *last_slash = strrchr(argv_cp, '/');
			if (last_slash) {
				*last_slash = '\0';
				if (chdir(argv[1])) {
					perror("ccc");
					die("Error from chdir");
				}
			}
			to_open_file = 1;
		}
	}

	/* check if it is interactive shell */
	if (!isatty(STDIN_FILENO)) 
		die("ccc: No tty detected. ccc requires an interactive shell to run.\n");

	struct sigaction sa;
	sa.sa_handler = handle_sigwinch;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sa.sa_mask);

	if (sigaction(SIGWINCH, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	/* initialize screen, don't print special chars,
	 * make ctrl + c work, don't show cursor 
	 * enable arrow keys */
	bprintf("\033[?1049h\033[2J\033[?25l");
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	/* Disable canonical mode and echo */
	newt.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	/* 	newt.c_oflag &= ~(OPOST); */
	newt.c_cflag |= (CS8);
	newt.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	newt.c_cc[VMIN] = 0;
	newt.c_cc[VTIME] = 1;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &newt);

	/* init files and marked arrays */
	marked = arraylist_init(100);
	hashtable_init();

	cwd = memalloc(PATH_MAX);
	getcwd(cwd, PATH_MAX);
	p_cwd = memalloc(PATH_MAX);
	p_cwd[0] = '\0';
	populate_files(cwd, 0, &files);
	handle_sigwinch(-1);

	if (to_open_file) {
		sel_file = arraylist_search(files, argv_cp, 1);
	}

	int ch, run = 1;
	while (run) {
		list_files();
		ch = readch();
		switch (ch) {
			/* quit */
			case 'q':
				if (write_last_d() == -1) {
					/* prompt user so error message can be shown to user */
					readch();
				}
				cleanup();
				run = 0;
				break;

			/* reload */
			case 'z':
				change_dir(cwd, 0, 0); 
				break;

			/* go back */
			case BACKSPACE:
			case ARROW_LEFT:
			case 'h':
				/* get parent directory */
				strcpy(p_cwd, cwd);
				char *last_slash = strrchr(cwd, '/');
				if (last_slash != NULL) {
					if (last_slash == cwd) {
						change_dir("/", 0, 0);
						break;
					}
					*last_slash = '\0';
					change_dir(cwd, 0, 0);
				}
				break;

			/* enter directory/open a file */
			case ENTER:
			case ARROW_RIGHT:
			case 'l':
				strcpy(p_cwd, cwd);
				file c_file = files->items[sel_file];

				/* Check if it is directory or a regular file */
				if (c_file.type == DRY) {
					/* Change cwd to directory */
					change_dir(c_file.path, 0, 0); 
				} else if (c_file.type == REG) {
					/* Write opened file to a file for file pickers */
					if (file_picker) {
						char opened_file_path[PATH_MAX];
						strcpy(opened_file_path, "~/.cache/ccc/opened_file");
						replace_home(opened_file_path);
						FILE *opened_file = fopen(opened_file_path, "w+");
						fprintf(opened_file, "%s\n", c_file.path);
						fclose(opened_file);
						cleanup();
						run = 0;
					} else {
						edit_file();
					}
				}
				break;

			/* jump up */
			case CTRLU:
				if ((sel_file - jump_num) > 0)
					sel_file -= jump_num;
				else
					sel_file = 0;
				break;

			/* go up */
			case ARROW_UP:
			case 'k':
				if (sel_file > 0)
					sel_file--;
				break;

			/* jump down */
			case CTRLD:
				if ((sel_file + jump_num) < (files->length - 1))
					sel_file += jump_num;
				else
					sel_file = (files->length - 1);
				break;

			/* go down */
			case ARROW_DOWN:
			case 'j':
				if (sel_file < (files->length - 1))
					sel_file++;
				break;

			/* jump to the bottom */
			case 'G':
				sel_file = (files->length - 1);
				break;

			/* jump to the top */
			case 'g':
				sel_file = 0;
				break;

			/* '~' go to $HOME */
			case TILDE:;
				char *home = getenv("HOME");
				if (home == NULL) {
					wpprintw("$HOME not defined");
				} else {
					strcpy(p_cwd, cwd);
					change_dir(home, 0, 0);
				}
				break;

			/* go to the trash dir */
			case 't':;
				char *trash_dir = check_trash_dir();
				if (trash_dir != NULL) {
					strcpy(p_cwd, cwd);
					change_dir(trash_dir, 0, 0);
				}
				break;

			/* sort files */
			case 'u':
				sort_files();
				break;

			/* show directories' sizes */
			case 'A':
				dirs_size = !dirs_size;
				change_dir(cwd, 0, 0);
				break;

			/* go to previous dir */
			case '-':
				if (strlen(p_cwd) != 0)
					change_dir(p_cwd, 0, 0);
				break;

			/* show help */
			case '?':
				show_help();
				break;

			/* toggle hidden files */
			case '.':
				show_hidden = !show_hidden;
				change_dir(cwd, 0, 0);
				break;

			/* toggle file details */
			case 'i':
				show_details = !show_details;
				change_dir(cwd, 0, 0);
				break;

			case 'w':
				show_icons = !show_icons;
				change_dir(cwd, 0, 0);
				break;

			case 'f':
				create_file();
				break;

			case 'n':
				create_dir();
				break;

			case 'r':
				rename_file();
				break;

			case ':':
				goto_dir();
				break;

			case 'X':
				toggle_executable();
				change_dir(cwd, 0, 0);
				break;

			case '!':
				start_shell();
				break;

			case 'y':
				yank_clipboard();
				break;

			/* mark one file */
			case SPACE:
				add_file_stat(files->items[sel_file].name, files->items[sel_file].path, 1);
				break;

			/* mark all files in directory */
			case 'a':
				change_dir(cwd, sel_file, 2); /* reload current dir */
				break;

			/* mark actions: */
			/* delete */
			case 'd':
				delete_files();
				break;

			/* move */
			case 'm':
				if (marked->length) {
					;
				}
				break;

			/* copy */
			case 'c':
				if (marked->length) {
					;
				}
				break;

			/* symbolic link */
			case 's':
				if (marked->length) {
					;
				}
				break;

			/* bulk rename */
			case 'b':
				if (marked->length) {
					;
				}
				break;

			default:
				break;
		}
	}

	return 0;
}

void handle_sigwinch(int ignore)
{
	get_window_size(&rows, &cols);
	if (cols < 80 || rows < 24) {
		cleanup();
		die("ccc: Terminal size needs to be at least 80x24");
	}
	half_width = cols / 2 + window_offset;
	list_files();
}

void cleanup(void)
{
	if (argv_cp != NULL)
		free(argv_cp);
	if (files->length != 0) {
		arraylist_free(files);
	}
	if (marked->length != 0) {
		arraylist_free(marked);
	}
	/* Restore old terminal settings */
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &oldt);
	bprintf("\033[2J\033[?1049l\033[?25h");
}

void show_help(void)
{
	bprintf("\033[2J");
	move_cursor(1, 1);
	bprintf("h: go to parent dir\nj: scroll down\nk: scroll up\n"
			"l: go to child dir\n\nleft: go to parent dir\ndown: scroll down\n"
			"up: scroll up\nright: go to child dir\n\n"
			"enter: go to child dir/open file\nbackspace: go to parent dir\n\n"
			"gg: go to top\nG: go to bottom\n\nctrl+u: jump up\nctrl+d: jump down\n\n"
			"t: go to trash dir\n~: go to home dir\n-: go to previous dir\n"
			"z: refresh current dir\n:: go to a directory by typing\nu: sort files\n\n"
			".: toggle hidden files\ni: toggle file details\nX: toggle executable\n\n"
			"A: show directory disk usage/block size\n\nf: new file\nn: new dir\n"
			"r: rename\n\nspace: mark file\na: mark all files in directory\nd: trash"
			"\n\n?: show help\nq: exit with last dir written to file\n"
			"ctrl+c exit without writing last dir"
			"\nPress any key to continue"
		   );
	wpprintw("Visit https://github.com/night0721/ccc or use 'man ccc' for help");
	readch();
}

/*
 * Checks if the trash directory is set and returns it
 */
char *check_trash_dir(void)
{
	char *path = memalloc(PATH_MAX);

	/* check if there is trash_dir */
	if (!strcmp(trash_dir, "")) {
		wpprintw("Trash directory not defined");
		return NULL;
	} else {
		strcpy(path, trash_dir);
		/* if trash_dir has ~ then make it $HOME */
		/* use path as trash_dir */
		if (path[0] == '~')
			replace_home(path);

		/* if has access to trash_dir */
		if (access(path, F_OK) != 0) {
			/* create the directory with 755 permissions if it doesn't exist */
			mkdir_p(path);		
		}
	}
	return path;
}

/*
 * Change directory in window with selection
 */
void change_dir(const char *buf, int selection, int ftype)
{
	if (cwd != buf)
		strcpy(cwd, buf);
	if (ftype == 0)
		arraylist_free(files);
	chdir(cwd);
	sel_file = selection;
	populate_files(cwd, ftype, &files);
}

/*
 * Recursively create directory by creating each subdirectory
 * like mkdir -p
 */
void mkdir_p(const char *destdir)
{
	char *path = memalloc(PATH_MAX);
	char dir_path[PATH_MAX] = "";

	if (destdir[0] == '~') {
		char *home = getenv("HOME");
		if (home == NULL) {
			wpprintw("$HOME not defined");
			return;
		}
		/* replace ~ with home */
		snprintf(path, PATH_MAX, "%s%s", home, destdir + 1);
	} else {
		strcpy(path, destdir);
	}

	/* fix first / not appearing in the string */
	if (path[0] == '/')
		dir_path[0] = '/';

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

			wpprintw("mkdir failed: %s", strerror(errno));
			free(path);
			return;
		}
		token = strtok(NULL, "/");
	}

	free(path);
	return;
}

/*
 * Read the provided directory and add all files in directory to an Arraylist
 * ftype: normal files = 0, marked = 1, marking ALL = 2
 */
void populate_files(const char *path, int ftype, ArrayList **list)
{
	DIR *dp;
	struct dirent *ep;

	if ((dp = opendir(path)) != NULL) {
		if (ftype == 0) {
			tmp1 = arraylist_init(10);
			tmp2 = arraylist_init(10);
		}

		while ((ep = readdir(dp)) != NULL) {
			char *filename = estrdup(ep->d_name);

			/* Filter out dotfiles */
			if ((!show_hidden && strncmp(filename, ".", 1) && strncmp(filename, "..", 2))
					|| (show_hidden && strcmp(filename, ".") && strcmp(filename, ".."))) {
				/* Construct full file path */
				int fpath_len = strlen(path) + strlen(filename) + 2;
				char *fpath = memalloc(fpath_len);
				snprintf(fpath, fpath_len, "%s/%s", path, filename);
				add_file_stat(filename, fpath, ftype);
			}
			else free(filename);
		}
		if (ftype == 0) {
			*list = arraylist_init(tmp1->length + tmp2->length);
			(*list)->length = tmp1->length + tmp2->length;
			qsort(tmp1->items, tmp1->length, sizeof(file), sort_compare);
			qsort(tmp2->items, tmp2->length, sizeof(file), sort_compare);
			memcpy((*list)->items, tmp1->items, tmp1->length * sizeof(file));
			memcpy((*list)->items + tmp1->length, tmp2->items, tmp2->length * sizeof(file));
			free(tmp1->items);
			free(tmp2->items);
			free(tmp1);
			free(tmp2);
		}
		closedir(dp);
	} else {
		wpprintw("stat failed: %s", strerror(errno));
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
 * ftype: normal file = 0, normal marked = 1, marked ALL = 2
 */
void add_file_stat(char *filename, char *path, int ftype)
{
	struct stat file_stat;
	if (stat(path, &file_stat) == -1) {
		/* can't be triggered? */
		if (errno == EACCES)
			arraylist_add(files, filename, path, NULL, REG, NULL, DEF_COLOR, 0, 0);
	}

	int type;
	char *icon_str = memalloc(8);

	filename[strlen(filename)] = '\0';
	/* handle file without extension
	 * ext is the extension if . exist in filename
	 * otherwise is nothing and handled through tenery operator */
	char *ext = strrchr(filename, '.');
	if (ext != NULL) {
		ext += 1;
	}
	/* add file extension */
	icon *ext_icon = hashtable_search(ext != NULL ? ext : filename);
	if (ext_icon == NULL)
		memcpy(icon_str, "", 4);
	else 
		memcpy(icon_str, ext_icon->icon, 5);

	int color = DEF_COLOR;

	if (S_ISDIR(file_stat.st_mode)) {
		type = DRY; /* dir */
		color = DIR_COLOR;
		memcpy(icon_str, "󰉋", 5);
	} else if (S_ISREG(file_stat.st_mode)) {
		type = REG; /* regular file */
		color = REG_COLOR;
	} else if (S_ISLNK(file_stat.st_mode)) {
		type = LNK; /* symbolic link */
		color = LNK_COLOR;
	} else if (S_ISCHR(file_stat.st_mode)) {
		type = CHR; /* character device */
		color = CHR_COLOR;
	} else if (S_ISSOCK(file_stat.st_mode)) {
		type = SOC; /* socket */
		color = SOC_COLOR;
	} else if (S_ISBLK(file_stat.st_mode)) {
		type = BLK; /* block device */
		color = BLK_COLOR;
	} else if (S_ISFIFO(file_stat.st_mode)) {
		type = FIF; /* FIFO */
		color = FIF_COLOR;
	}

	/* If file is to be marked */
	if (ftype == 1 || ftype == 2) {
		/* Force if user is marking all files */
		int force = ftype == 2 ? 1 : 0;
		arraylist_add(marked, filename, path, NULL, type, icon_str, DEF_COLOR, 1,
				force);
		/* free type and return without allocating more stuff */
		return;
	}

	/* get last modified time */
	size_t time_size = 17;
	char *time = memalloc(time_size);
	/* Format last modified time to a string */
	strftime(time, time_size, "%Y-%m-%d %H:%M", localtime(&file_stat.st_mtime));

	/* get file size */
	double bytes = file_stat.st_size;

	if (dirs_size) {
		/* dirs_size is 1, so calculate disk usage */
		if (S_ISDIR(file_stat.st_mode)) {
			/* at most 15 fd opened */
			total_dir_size = 0;
			nftw(path, &get_directory_size, 15, FTW_PHYS);
			bytes = total_dir_size;
		}
	}
	/* 4 before decimal + 1 dot + decimal_place (after decimal) +
	   unit length (1 for K, 3 for KiB, taking units[1] as B never changes) + 1 space + 1 null */
	static const char* units[] = {"B", "K", "M", "G", "T", "P"};
	int size_size = 4 + 1 + decimal_place + strlen(units[1]) + 1 + 1;
	char *size = memalloc(size_size);
	int unit = 0;
	while (bytes > 1024) {
		bytes /= 1024;
		unit++;
	}
	/* display sizes and check if there are decimal places */
	if (bytes == (unsigned int) bytes) {
		sprintf(size, "%d%s", (unsigned int) bytes, units[unit]);
	} else {
		sprintf(size, "%.*f%s", decimal_place, bytes, units[unit]);
	}
	/* get file mode string */
	char *mode_str = get_file_mode(file_stat.st_mode);
	if (mode_str[0] == '-' && (mode_str[3] == 'x' || mode_str[6] == 'x' || mode_str[9] == 'x')) {
		color = EXE_COLOR;
	}

	/* mode_str(11) + time(17) + size_size + 2 spaces + 1 null */
	size_t stat_size = 11 + time_size + size_size + 3;
	char *total_stat = memalloc(stat_size);
	snprintf(total_stat, stat_size, "%s %s %-*s", mode_str, time, size_size, size);

	/* DIR if color is blue */
	if (color == 34)
		arraylist_add(tmp1, filename, path, total_stat, type, icon_str, color, 0, 0);
	else
		arraylist_add(tmp2, filename, path, total_stat, type, icon_str, color, 0, 0);

	free(time);
	free(size);
	free(mode_str);
}

/*
 * get file mode string from stat mode
 * eg: drwxr-sr-x
 */
char *get_file_mode(mode_t mode)
{
	char *mode_str = memalloc(11);
	mode_str[0] = S_ISDIR(mode) ? 'd' : '-';
	mode_str[1] = (mode & S_IRUSR) ? 'r' : '-';
	mode_str[2] = (mode & S_IWUSR) ? 'w' : '-';
	mode_str[3] = (mode & S_IXUSR) ? 'x' : '-';
	mode_str[4] = (mode & S_IRGRP) ? 'r' : '-';
	mode_str[5] = (mode & S_IWGRP) ? 'w' : '-';
	mode_str[6] = (mode & S_IXGRP) ? 'x' : '-';
	mode_str[7] = (mode & S_IROTH) ? 'r' : '-';
	mode_str[8] = (mode & S_IWOTH) ? 'w' : '-';
	mode_str[9] = (mode & S_IXOTH) ? 'x' : '-';
	mode_str[10] = '\0';
	return mode_str;
}

/*
 * Print files in directory window 
 */
void list_files(void)
{
	long overflow = 0;
	if (sel_file > rows - 4) {
		/* overflown */
		overflow = sel_file - (rows - 4);
	}

	/* calculate range of files to show */
	long range = files->length;
	/* not highlight if no files in directory */
	if (range == 0 && errno == 0) {
		for (int i = 0; i < rows; i++) {
			move_cursor(i + 1, 1);
			bprintf("\033[K");
		}
		move_cursor(1, half_width);
		bprintf("empty directory");
		return;
	}

	if (range > rows - 3) {
		/* if there are more files than rows available to display
		 * shrink range to avaiable rows to display with
		 * overflow to keep the number of iterations to be constant */
		range = rows - 3 + overflow;
	}

	move_cursor(1, 1);
	bprintf("\033[2J");

	for (long i = overflow; i < range; i++) {
		int is_selected = 0;
		if ((overflow == 0 && i == sel_file) ||
				(overflow != 0 && i == sel_file)) {
			is_selected = 1;
			/* check for marked files */
			long num_marked = marked->length;
			if (num_marked > 0) {
				/* Determine length of formatted string */
				int m_len = snprintf(NULL, 0, "[%ld] selected", num_marked);
				char *selected = memalloc(m_len + 1);

				snprintf(selected, m_len + 1, "[%ld] selected", num_marked);
				wpprintw("(%ld/%ld) %s %s", sel_file + 1, files->length, selected, cwd);
			} else  {
				wpprintw("(%ld/%ld) %s", sel_file + 1, files->length, cwd);
			}
		}
		/* print the actual filename and stats */
		char *line = get_line(files, i, show_details, show_icons);
		int color = files->items[i].color;
		/* check is file marked for action */
		int is_marked = arraylist_search(marked, files->items[i].path, 0) != -1;
		move_cursor(i + 1, 1);
		if (is_marked) color = 32;
		bprintf("\033[30m\033[%dm%s\033[0m\n",
				is_selected ? color + 10 : color, line);

		free(line);
	}

	/* show file content every time cursor changes */
	show_file_content();
}

size_t get_effective_length(const char *str)
{
	size_t length = 0;
	int in_ansi_sequence = 0;

	while (*str) {
		if (*str == '\033') {
			in_ansi_sequence = 1;
		} else if (in_ansi_sequence && *str == 'm') {
			in_ansi_sequence = 0;
		} else if (!in_ansi_sequence) {
			length++;
		}
		str++;
	}

	return length;
}

void print_chunk(const char *str, size_t start, size_t end)
{
	int in_ansi_sequence = 0;
	size_t printed_length = 0;

	for (size_t i = start; i < end; i++) {
		if (str[i] == '\033') {
			in_ansi_sequence = 1;
		}
		if (in_ansi_sequence) {
			putchar(str[i]);
			if (str[i] == 'm') {
				in_ansi_sequence = 0;
			}
		} else {
			putchar(str[i]);
			printed_length++;
			if (printed_length >= (end - start)) {
				break;
			}
		}
	}
}

/*
 * Get file content into buffer and show it to preview window
 */
void show_file_content(void)
{
	file current_file = files->items[sel_file];

	move_cursor(1, half_width);
	if (current_file.type == DRY) {
		ArrayList *files_visit;
		populate_files(current_file.name, 0, &files_visit);
		for (long i = 0; i < files_visit->length; i++) {
			char *line = get_line(files_visit, i, 0, show_icons);
			int color = files_visit->items[i].color;
			move_cursor(i + 1, half_width);
			bprintf("\033[K\033[%dm%s\033[m\n", color, line);
		}
		arraylist_free(files_visit);
		return;
	}
	FILE *file = fopen(current_file.path, "r");
	if (file == NULL) {
		bprintf("Unable to read %s", current_file.name);
		return;
	}

	int c;
	/* Check if its binary */
	while ((c = fgetc(file)) != EOF) {
		if (c == '\0') {
			bprintf("binary");
			return;
		}
	}
	int pipe_fd[2];
	if (pipe(pipe_fd) == -1) {
		perror("pipe");
		return;
	}
	int pid = fork();
	if (pid == 0) {
		/* Child */
		move_cursor(1, half_width);
		close(pipe_fd[0]);
		dup2(pipe_fd[1], STDOUT_FILENO);
		dup2(pipe_fd[1], STDERR_FILENO);
		close(pipe_fd[1]);
		execlp("vip", "vip", "-c", current_file.name, NULL);
		_exit(1);
	} else if (pid > 0) {
		/* Parent */
		close(pipe_fd[1]);
		char buffer[4096];
		int row = 1;
		FILE *stream = fdopen(pipe_fd[0], "r");
		while (fgets(buffer, sizeof(buffer), stream) != NULL && row <= rows - 1) {
			buffer[strcspn(buffer, "\n")] = 0;

			if (buffer[0] == '\0' || strspn(buffer, " \t") == strlen(buffer)) {
				move_cursor(row++, half_width);
				putchar('\n');
				continue;
			}
			/* Length without ANSI codes */
			size_t effective_len = get_effective_length(buffer);
			size_t offset = 0;
			while (effective_len > 0 && row <= rows - 1) {
				move_cursor(row++, half_width);

				/* Calculate the chunk size based on effective length */
				size_t chunk_size = (effective_len > (cols - half_width)) ? (cols - half_width) : effective_len;

				/* Find the actual end position in the string to avoid cutting ANSI sequences */
				size_t actual_end = offset;
				size_t visible_count = 0;
				int in_ansi_sequence = 0;

				while (visible_count < chunk_size && buffer[actual_end] != '\0') {
					if (buffer[actual_end] == '\033') {
						in_ansi_sequence = 1;
					}
					if (!in_ansi_sequence) {
						visible_count++;
					}
					if (in_ansi_sequence && buffer[actual_end] == 'm') {
						in_ansi_sequence = 0;
					}
					actual_end++;
				}

				/* Print the chunk from `offset` to `actual_end` */
				print_chunk(buffer, offset, actual_end);

				/* Update offsets based on the effective length and ANSI-aware chunk */
				offset = actual_end;
				effective_len -= chunk_size;

				putchar('\n');
			}
			/* Check if the line ends with the ANSI reset sequence `\033[0m` */
			size_t len = strlen(buffer);
			if (len >= 4 && strcmp(&buffer[len - 4], "\033[0m") == 0) {
				/* Line ends with ANSI reset, print it */
				printf("\033[0m");
			}
		}
		fclose(stream);
		waitpid(pid, NULL, 0);
	} else {
		wpprintw("fork failed: %s", strerror(errno));
	}
}

/*
 * Opens $EDITOR to edit the file
 */
void edit_file(void)
{
	if (editor == NULL) {
		editor = getenv("EDITOR");
		if (editor == NULL) {
			wpprintw("$EDITOR not defined");
			return;
		}
	} else {
		char *filename = files->items[sel_file].path;

		pid_t pid = fork();
		if (pid == 0) {
			/* Child process */
			execlp(editor, editor, filename, NULL);
			_exit(1); /* Exit if exec fails */
		} else if (pid > 0) {
			/* Parent process */
			waitpid(pid, NULL, 0);
		} else {
			/* Fork failed */
			wpprintw("fork failed: %s", strerror(errno));
		}
	}
}

void toggle_executable(void)
{
	file f = files->items[sel_file];
	struct stat st;
	if (stat(f.path, &st) == -1) {
		wpprintw("stat failed: %s", strerror(errno));
	}
	if (f.type == DRY)
		return;
	/* chmod by xor executable bits */
	if (chmod(f.path, st.st_mode ^ (S_IXUSR | S_IXGRP | S_IXOTH)) == -1) {
		wpprintw("Error toggling executable: %s", strerror(errno));
	}

}

void replace_home(char *str)
{
	char *home = getenv("HOME");
	if (home == NULL) {
		wpprintw("$HOME not defined");
		return;
	}
	int len = strlen(str) + strlen(home) + 1;
	char newstr[len];
	/* replace ~ with home */
	snprintf(newstr, len, "%s%s", home, str + 1);
	strcpy(str, newstr);
}

int write_last_d(void)
{
	if (!strcmp(last_d, "")) {
		strcpy(last_d, getenv("CCC_LAST_D"));
		if (!strcmp(last_d, "")) {
			wpprintw("$CCC_LAST_D not defined (Press any key to continue)");
			return -1;
		}
	} else {
		if (last_d[0] == '~')
			replace_home(last_d);

		char last_ddup[PATH_MAX];
		strcpy(last_ddup, last_d);

		char *last_d_dir = strrchr(last_ddup, '/');
		if (last_d_dir != NULL) {
			*last_d_dir = '\0'; /* truncate string */
		}
		mkdir_p(last_ddup);
		FILE *last_d_file = fopen(last_d, "w");
		if (last_d_file == NULL) {
			wpprintw("Cannot open last directory file (Press any key to continue)");
			return -1;
		}
		fwrite(cwd, strlen(cwd), sizeof(char), last_d_file);
		fclose(last_d_file);
	}
	return 0;
}

int sort_compare(const void *a, const void *b)
{
	return strcmp(((file *) a)->name, ((file *) b)->name);
}

void sort_files(void)
{
	qsort(files->items, files->length, sizeof(file), sort_compare);
}

char *get_panel_string(char *prompt)
{
	size_t bufsize = 128;
	char *buf = memalloc(bufsize);
	size_t buflen = 0;
	buf[0] = '\0';
	bprintf("\033[?25h");
	while (1) {
		wpprintw("%s%s", prompt, buf);
		int c = readch();
		if (c == BACKSPACE) {
			if (buflen != 0) {
				buf[--buflen] = '\0';
			}
		} else if (c == '\033') {
			wpprintw("");
			free(buf);
			bprintf("\033[?25l");
			return NULL;
		} else if (c == '\r') {
			wpprintw("");
			if (buflen != 0) {
				bprintf("\033[?25l");
				return buf;
			}
		} else if (!iscntrl(c) && c < 128) {
			if (buflen == bufsize - 1) {
				bufsize *= 2;
				buf = realloc(buf, bufsize);
			}
			buf[buflen++] = c;
			buf[buflen] = '\0';
		}
	}

	if (buf[0] == '~')
		replace_home(buf);

	bprintf("\033[?25l");
	return buf;
}

void rename_file(void)
{
	char *filename = files->items[sel_file].path;
	char *input = get_panel_string("Rename file: ");
	if (!input) {
		return;
	}
	char *newfilename = estrdup(filename);
	/* remove basename of newfilename */
	char *last_slash = strrchr(newfilename, '/');
	*last_slash = '\0';
	/* add the slash back to newfilename */
	strcat(newfilename, "/");
	strcat(newfilename, input);
	if (rename(filename, newfilename)) {
		wpprintw("rename failed: %s (Press any key to continue)", strerror(errno));
		readch();
	} else {
		change_dir(cwd, 0, 0);
		wpprintw("Renamed %s to %s", filename, newfilename);
	}
	free(input);
	free(newfilename);
}

void goto_dir(void)
{
	char *input = get_panel_string("Goto dir: ");
	struct stat st;
	if (lstat(input, &st)) {
		wpprintw("lstat failed: %s (Press any key to continue)", strerror(errno));
		readch();
	}
	/* chdir to directory from argument */
	if (S_ISDIR(st.st_mode) && chdir(input)) {
		wpprintw("chdir failed: %s (Press any key to continue)", strerror(errno));
		readch();
	}
	getcwd(cwd, PATH_MAX);
	change_dir(cwd, 0, 0);
	free(input);
}

void create_dir(void)
{
	char *input = get_panel_string("New dir: ");
	char *newfilename = memalloc(PATH_MAX);
	snprintf(newfilename, PATH_MAX, "%s/%s", cwd, input);
	if (access(newfilename, F_OK) != 0) {
		mkdir_p(newfilename);
		change_dir(cwd, 0, 0);
		wpprintw("Created %s", input);
	} else {
		wpprintw("Directory already exist");
	}
	free(input);
	free(newfilename);
}

void create_file(void)
{
	char *input = get_panel_string("New file: ");
	FILE *f = fopen(input, "w+");
	fclose(f);
	change_dir(cwd, 0, 0);
	wpprintw("Created %s", input);
	free(input);
}

void delete_files(void)
{
	if (marked->length) {
		char *trash_dir = check_trash_dir();
		if (trash_dir != NULL) {
			for (int i = 0; i < marked->length; i++) {
				char *new_path = memalloc(PATH_MAX);
				snprintf(new_path, PATH_MAX, "%s/%s", trash_dir, marked->items[i].name);
				if (rename(marked->items[i].path, new_path)) {
					wpprintw("delete failed: %s", strerror(errno));
				} else {
					change_dir(cwd, 0, 0);
				}
			}
			for (int i = 0; i < marked->length; i++) {
				arraylist_remove(marked, 0);
			}
		} else {
			wpprintw("TODO: implement hard delete");
		}
	}
}

void start_shell(void)
{
	bprintf("\033[2J\033[?25h");
	move_cursor(1, 1);
	char shell[PATH_MAX];
	strcpy(shell, getenv("SHELL"));
	if (strlen(shell) == 0) {
		strcpy(shell, "sh");
	} else {
		pid_t pid = fork();
		if (pid == 0) {
			/* Child process */
			execlp(shell, shell, NULL);
			_exit(1); /* Exit if exec fails */
		} else if (pid > 0) {
			/* Parent process */
			waitpid(pid, NULL, 0);
			bprintf("\033[?25l");
		} else {
			/* Fork failed */
			wpprintw("fork failed: %s", strerror(errno));
		}
	}
}

void yank_clipboard(void)
{
	pid_t pid = fork();
	if (pid == 0) {
		/* Child process */
		execlp(clipboard, clipboard, files->items[sel_file].name, NULL);
		_exit(1); /* Exit if exec fails */
	} else if (pid > 0) {
		/* Parent process */
		waitpid(pid, NULL, 0);
		bprintf("\033[?25l");
	} else {
		/* Fork failed */
		wpprintw("fork failed: %s", strerror(errno));
	}
}

/*
 * Print line to the panel
 */
void wpprintw(const char *fmt, ...)
{
	char buffer[256];
	va_list args;

	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	move_cursor(rows, 1);
	/* Clear line and print formatted string */
	bprintf("\033[K%s", buffer);
}

void move_cursor(int row, int col)
{
	bprintf("\033[%d;%dH", row, col);
}

int readch(void)
{
	int nread;
	char c;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN) {
			die("read");
		}
	}

	if (c == '\033') {
		char seq[3];

		if (read(STDIN_FILENO, &seq[0], 1) != 1)
			return '\033';
		if (read(STDIN_FILENO, &seq[1], 1) != 1)
			return '\033';

		if (seq[0] == '[') {
			if (seq[1] >= '0' && seq[1] <= '9') {
				if (read(STDIN_FILENO, &seq[2], 1) != 1)
					return '\033';
				if (seq[2] == '~') {
					switch (seq[1]) {
						case '1': return HOME_KEY;
						case '3': return DEL_KEY;
						case '4': return END_KEY;
						case '5': return PAGE_UP;
						case '6': return PAGE_DOWN;
						case '7': return HOME_KEY;
						case '8': return END_KEY;
					}
				}
			} else {
				switch (seq[1]) {
					case 'A': return ARROW_UP;
					case 'B': return ARROW_DOWN;
					case 'C': return ARROW_RIGHT;
					case 'D': return ARROW_LEFT;
					case 'F': return END_KEY;
					case 'H': return HOME_KEY;
				}
			}
		} else if (seq[0] == 'O') {
			switch (seq[1]) {
				case 'F': return END_KEY;
				case 'H': return HOME_KEY;
			}
		}
		return '\033';
	} else {
		return c;
	}
}

int get_cursor_position(int *rows, int *cols)
{
	char buf[32];
	unsigned int i = 0;
	bprintf("\033[6n");
	while (i < sizeof(buf) - 1) {
		if (read(STDIN_FILENO, &buf[i], 1) != 1) {
			break;
		}
		if (buf[i] == 'R') {
			break;
		}
		i++;
	}
	buf[i] = '\0';
	if (buf[0] != '\033' || buf[1] != '[') {
		return -1;
	}
	if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) {
		return -1;
	}
	return 0;
}

int get_window_size(int *row, int *col)
{
	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		/* Can't get window size */
		bprintf("\033[999C\033[999B");
		return get_cursor_position(row, col);
	} else {
		*col = ws.ws_col;
		*row = ws.ws_row;
		return 0;
	}
}

/*
 * printf, but write to STDOUT_FILENO
 */
void bprintf(const char *fmt, ...)
{
	char buffer[512];
	va_list args;

	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	write(STDOUT_FILENO, buffer, strlen(buffer));
}
