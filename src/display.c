#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <err.h>

#include "ctl.h"

#define BUFFER_SIZE 1024

#define TTY_RESET "\033[0m"
#define TTY_INVERSE "\033[7m"

void printStatus(const struct lotsctl *const ctl) {
	unsigned int percent = ctl->filePos / ctl->fileSize * 100;
	printf(TTY_INVERSE "%s (%u%%)" TTY_RESET, ctl->filename, percent);
	fflush(stdout);
}

void displayFile(struct lotsctl *const ctl, const char *const filename) {
	ctl->filename = filename;

	// Open file
	ctl->file = fopen(ctl->filename, "r");
	if (!ctl->file) {
		warn("Could not open \"%s\"", ctl->filename);
		return;
	}

	// Get file information
	struct stat st;
	if (fstat(fileno(ctl->file), &st) < 0) {
		warn("Could not stat file \"%s\"", ctl->filename);
		return;
	}

	// Can't display directories
	if (S_ISDIR(st.st_mode)) {
		warnx("\"%s\" is a directory", ctl->filename);
		return;
	}

	ctl->fileSize = st.st_size;

	// Print screenful of content
	char buffer[BUFFER_SIZE];
	unsigned short lines = 0;
	while (fgets(buffer, sizeof(buffer), ctl->file)) {
		fputs(buffer, stdout);
		if (strcspn(buffer, "\n") < BUFFER_SIZE - 1) {
			lines++;
			if (lines >= ctl->lines - 1)
				break;
		}
	}
	ctl->filePos = ftello(ctl->file);

	printStatus(ctl);
}
