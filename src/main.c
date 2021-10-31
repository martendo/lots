#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <term.h>
#include <termios.h>
#include <err.h>
#include <getopt.h>

#include "ctl.h"
#include "cmd.h"
#include "display.h"

#define LOTS_VERSION "0.0.0"
#define LOTS_HOME_PAGE "lots home page: <https://github.com/martendo/lots>"

#define BUFFER_SIZE 1024

static const char *const optstring = "hv";

static const struct option longopts[] = {
	{"help",    no_argument, NULL, 'h'},
	{"version", no_argument, NULL, 'v'},
	{NULL, 0, NULL, 0}
};

int main(const int argc, char *const argv[]) {
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
					"  -h, --help     Print this message and exit\n"
					"  -v, --version  Print the version number and exit\n"
					"\n"
					LOTS_HOME_PAGE);
				return 0;
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
	// Quit if there are no input files
	if (optind == argc)
		errx(1, "No input files given");

	// stdout is not a terminal -> simply copy all files
	if (!isatty(STDOUT_FILENO)) {
		do {
			const char *const filename = argv[optind];
			FILE *file = fopen(filename, "r");
			if (!file) {
				warn("Could not open \"%s\"", filename);
				continue;
			}

			char buffer[BUFFER_SIZE];
			size_t size;
			while ((size = fread(&buffer, sizeof(char), sizeof(buffer), file)) > 0)
				fwrite(&buffer, sizeof(char), size, stdout);
			if (ferror(file))
				warn("Could not read \"%s\"", filename);
			fclose(file);
		} while (++optind < argc);
		return 0;
	}

	// stdout is a terminal -> get terminal capabilities
	struct lotsctl ctl;
	setupterm(NULL, STDOUT_FILENO, NULL);
	ctl.key_up_len = strlen(key_up);
	ctl.key_down_len = strlen(key_down);
	ctl.key_ppage_len = strlen(key_ppage);
	ctl.key_npage_len = strlen(key_npage);
	ctl.key_home_len = strlen(key_home);

	// Display first file
	display_file(&ctl, argv[optind++]);

	// Modify terminal attributes
	struct termios oldattr, attr;
	if (tcgetattr(STDIN_FILENO, &oldattr) < 0 && tcgetattr(STDERR_FILENO, &oldattr) < 0)
		err(1, "Failed to get terminal attributes");
	attr = oldattr;
	// Enter noncanonical mode to recieve character inputs immediately
	// and disable echoing of input characters
	attr.c_lflag &= ~(ICANON | ECHO);
	// Process key after at least 1 character of input with no timer
	attr.c_cc[VMIN] = 1;
	attr.c_cc[VTIME] = 0;
	tcsetattr(STDERR_FILENO, TCSANOW, &attr);

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
			// Jump to beginning of file
			case CMD_HOME:
				fseeko(ctl.file, 0, SEEK_SET);
				ctl.line = 0;
				// Fall-through
			// Move forwards 1 page
			case CMD_DOWN_PAGE:
				move_forwards(&ctl, lines - 1);
				break;
			// Move backwards 1 page
			case CMD_UP_PAGE:
				move_backwards(&ctl, lines - 1);
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
					"------------------------------------------------------------------------");
				print_status(&ctl);
				break;
			// Exit lots
			case CMD_QUIT:
				goto quit;
		}
	}
quit:
	if (tcsetattr(STDERR_FILENO, TCSAFLUSH, &oldattr) < 0)
		err(1, "Failed to reset terminal attributes");

	return 0;
}
