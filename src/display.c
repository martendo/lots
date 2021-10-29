#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <err.h>

#include "ctl.h"

#define BUFFER_SIZE 1024

void displayFile(struct lotsctl *const ctl, char *const filename) {
	// Get file information
	struct stat st;
	if (stat(filename, &st) < 0)
		err(1, "Could not stat file \"%s\"", filename);

	// Can't display directories
	if (S_ISDIR(st.st_mode)) {
		warnx("\"%s\": Is a directory", filename);
		return;
	}

	ctl->fileSize = st.st_size;

	// Open file
	ctl->file = fopen(filename, "r");
	if (!ctl->file)
		err(1, "Could not open \"%s\"", filename);

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
}
