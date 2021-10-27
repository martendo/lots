#include <stdio.h>
#include <getopt.h>

#define LOTS_VERSION "0.0.0"
#define LOTS_HOME_PAGE "lots home page: <https://github.com/martendo/lots>"

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
	return 0;
}
