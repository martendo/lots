#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <err.h>
#include <getopt.h>

#define LOTS_VERSION "0.0.0"
#define LOTS_HOME_PAGE "lots home page: <https://github.com/martendo/lots>"

#define BUFFER_SIZE 1024

#define ESC '\033'

static const char *const optstring = "hv";

static const struct option longopts[] = {
	{"help",    no_argument, NULL, 'h'},
	{"version", no_argument, NULL, 'v'},
	{NULL, 0, NULL, 0}
};

int main(int argc, char *const argv[]) {
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
			char *const filename = argv[optind];
			FILE *file = fopen(filename, "r");
			if (!file)
				err(1, "Could not open \"%s\"", filename);

			char buffer[BUFFER_SIZE];
			size_t size;
			while ((size = fread(&buffer, sizeof(char), sizeof(buffer), file)) > 0)
				fwrite(&buffer, sizeof(char), size, stdout);
			fclose(file);
		} while (++optind < argc);
		return 0;
	}

	// stdout is a terminal -> modify terminal attributes
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

	char inbuf[8] = {0};
	ssize_t inlen, i;
	while (1) {
		inlen = read(STDERR_FILENO, &inbuf, sizeof(inbuf));
		if (inlen < 0)
			continue;
		// ANSI escape sequence
		if (inbuf[0] == ESC && inlen >= 3) {
			if (inbuf[1] != '[')
				continue;
			switch (inbuf[2]) {
				// Up arrow: ^[[A
				case 'A':
					puts("up");
					break;
				// Down arrow: ^[[B
				case 'B':
					puts("down");
					break;
				// Right arrow: ^[[C
				case 'C':
					puts("right");
					break;
				// Left arrow: ^[[D
				case 'D':
					puts("left");
					break;
			}
			continue;
		}
		// Command
		for (i = 0; i < inlen; i++) {
			switch (inbuf[i]) {
				case 'q':
					goto quit;
			}
		}
	}
quit:
	if (tcsetattr(STDERR_FILENO, TCSAFLUSH, &oldattr) < 0)
		err(1, "Failed to set terminal attributes");

	return 0;
}
