#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <term.h>
#include <termios.h>
#include <err.h>
#include <errno.h>
#include <getopt.h>

#include "ctl.h"
#include "cmd.h"
#include "display.h"

#define LOTS_VERSION "0.0.0"
#define LOTS_HOME_PAGE "lots home page: <https://github.com/martendo/lots>"

#define BUFFER_SIZE 1024

static void print_file(FILE *const file, const char *const filename) {
	char buffer[BUFFER_SIZE];
	size_t size;
	while ((size = fread(&buffer, sizeof(char), sizeof(buffer), file)) > 0)
		fwrite(&buffer, sizeof(char), size, stdout);
	if (ferror(file))
		warn("Could not read \"%s\"", filename);
}

static const char *const optstring = "hl:v";

static const struct option longopts[] = {
	{"help",    no_argument,       NULL, 'h'},
	{"lines",   required_argument, NULL, 'l'},
	{"version", no_argument,       NULL, 'v'},
	{NULL, 0, NULL, 0}
};

int main(const int argc, char *const argv[]) {
	struct lotsctl ctl = {
		.page_lines = 0
	};

	// Get options
	int option;
	while ((option = getopt_long(argc, argv, optstring, longopts, NULL)) != -1) {
		switch (option) {
			case 'h':
				puts(
					"Usage: lots [options] <file>...\n"
					"\n"
					"A dead simple terminal pager.\n"
					"\n"
					"Options:\n"
					"  -l N, --lines=N  Set number of lines per page to N\n"
					"  -h, --help       Print this message and exit\n"
					"  -v, --version    Print the version number and exit\n"
					"\n"
					LOTS_HOME_PAGE);
				return 0;
			case 'l':
				char *endstr;
				errno = 0;
				const unsigned long num = strtoul(optarg, &endstr, 0);
				if (errno)
					err(1, "Not a valid integer \"%s\"", optarg);
				// Don't accept any non-digit characters in number string
				if (endstr == optarg || *endstr != '\0')
					errx(1, "Not a valid integer \"%s\"", optarg);
				// Don't accept zero
				if (!num)
					errx(1, "Lines per page must not be zero");
				ctl.page_lines = num;
				break;
			case 'v':
				puts(
					"lots " LOTS_VERSION "\n"
					"Copyright (C) 2021 martendo\n"
					"Licensed under the MIT License.\n"
					"\n"
					LOTS_HOME_PAGE);
				return 0;
			default:
				fputs("Try 'lots --help' for more information.\n", stderr);
				return 1;
		}
	}
	int stdintty = isatty(STDIN_FILENO);

	// Quit if there are no input files
	if (optind == argc && stdintty)
		errx(1, "No input files given");

	// stdout is not a terminal -> simply copy all files
	if (!isatty(STDOUT_FILENO)) {
		// Copy stdin if it is not a terminal (e.g. pipe, file)
		if (!stdintty)
			print_file(stdin, "stdin");
		while (optind < argc) {
			const char *const filename = argv[optind++];
			FILE *file = fopen(filename, "r");
			if (!file) {
				warn("Could not open \"%s\"", filename);
				continue;
			}
			print_file(file, filename);
			fclose(file);
		}
		return 0;
	}

	// stdout is a terminal -> get terminal capabilities
	setupterm(NULL, STDOUT_FILENO, NULL);
	if (!ctl.page_lines)
		ctl.page_lines = lines - 1;
	ctl.key_up_len = strlen(key_up);
	ctl.key_down_len = strlen(key_down);
	ctl.key_ppage_len = strlen(key_ppage);
	ctl.key_npage_len = strlen(key_npage);
	ctl.key_home_len = strlen(key_home);

	// Get file list
	ctl.file_count = argc - optind;
	ctl.files = argv + optind;
	ctl.file_index = 0;

	// Display stdin or first file
	if (!stdintty) {
		ctl.filename = "stdin";
		ctl.file = stdin;
		ctl.line = 0;
		struct stat st;
		fstat(fileno(ctl.file), &st);
		ctl.file_size = st.st_size;
		// Print screenful of content
		move_forwards(&ctl, lines - 1);
	} else {
		if (!display_file(&ctl, 1))
			// No files could be displayed
			return 1;
	}

	// Modify terminal attributes
	struct termios oldattr, attr;
	if (tcgetattr(STDIN_FILENO, &oldattr) < 0 && tcgetattr(STDOUT_FILENO, &oldattr) < 0)
		err(1, "Failed to get terminal attributes");
	attr = oldattr;
	// Enter noncanonical mode to recieve character inputs immediately
	// and disable echoing of input characters
	attr.c_lflag &= ~(ICANON | ECHO);
	// Process key after at least 1 character of input with no timer
	attr.c_cc[VMIN] = 1;
	attr.c_cc[VTIME] = 0;
	tcsetattr(STDOUT_FILENO, TCSANOW, &attr);

	enum cmd command = CMD_UNKNOWN;
	while (1) {
		command = getcmd(&ctl);
		switch (command) {
			case CMD_UNKNOWN:
				break;
			// Move forwards 1 line
			case CMD_DOWN:
				move_forwards(&ctl, 1);
				break;
			// Move backwards 1 line
			case CMD_UP:
				move_backwards(&ctl, 1);
				break;
			// Move forwards 1 page
			case CMD_DOWN_PAGE:
				move_forwards(&ctl, ctl.page_lines);
				break;
			// Move backwards 1 page
			case CMD_UP_PAGE:
				move_backwards(&ctl, ctl.page_lines);
				break;
			// Jump to beginning of file
			case CMD_HOME:
				if (fseeko(ctl.file, 0, SEEK_SET) < 0) {
					status_printf("Can't seek file");
					break;
				}
				ctl.line = 0;
				move_forwards(&ctl, lines - 1);
				break;
			// Switch to next file
			case CMD_NEXT_FILE:
				switch_file(&ctl, 1);
				break;
			// Switch to previous file
			case CMD_PREV_FILE:
				switch_file(&ctl, -1);
				break;
			// Display command help
			case CMD_HELP:
				clear_status();
				puts(
					"------------------------------------------------------------------------\n"
					"lots Commands\n"
					"\n"
					"  h  H                  Display this help\n"
					"  q  Q                  Quit lots\n"
					"\n"
					"  j  DownArrow  Return  Move forwards one line\n"
					"  k  UpArrow            Move backwards one line\n"
					"  f  PageDown  Space    Move forwards one page\n"
					"  b  PageUp             Move backwards one page\n"
					"\n"
					"  g  Home               Jump to beginning of file\n"
					"\n"
					"  n                     Switch to next file\n"
					"  p                     Switch to previous file\n"
					"------------------------------------------------------------------------");
				print_status(&ctl);
				break;
			// Exit lots
			case CMD_QUIT:
				goto quit;
		}
	}
quit:
	clear_status();
	if (tcsetattr(STDOUT_FILENO, TCSAFLUSH, &oldattr) < 0)
		err(1, "Failed to reset terminal attributes");

	return 0;
}
