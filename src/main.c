#include <stdio.h>
#include <unistd.h>
#include <err.h>
#include <getopt.h>

#define LOTS_VERSION "0.0.0"
#define LOTS_HOME_PAGE "lots home page: <https://github.com/martendo/lots>"

#define BUFFER_SIZE 1024

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
	return 0;
}
