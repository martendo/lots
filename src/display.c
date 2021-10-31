#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <err.h>
#include <term.h>

#include "ctl.h"

#define BUFFER_SIZE 1024

void print_status(const struct lotsctl *const ctl) {
	unsigned int percent = ctl->file_pos * 100 / ctl->file_size;
	putp(enter_reverse_mode);
	printf("%s line %u (%u%%)", ctl->filename, ctl->line, percent);
	putp(exit_attribute_mode);
	fflush(stdout);
}

void clear_status(void) {
	putchar('\r');
	putp(clr_eol);
}

void move_forwards(struct lotsctl *const ctl, unsigned long nlines) {
	clear_status();

	// Print nlines lines from file
	char buffer[BUFFER_SIZE];
	while (fgets(buffer, sizeof(buffer), ctl->file)) {
		fputs(buffer, stdout);
		putp(clr_eol);
		if (strcspn(buffer, "\n") < BUFFER_SIZE - 1) {
			ctl->line++;
			if (!--nlines)
				break;
		}
	}
	ctl->file_pos = ftello(ctl->file);

	// Reprint status
	print_status(ctl);
}

void move_backwards(struct lotsctl *const ctl, unsigned long nlines) {
	// Seek to new position
	fseeko(ctl->file, 0, SEEK_SET);
	char buffer[BUFFER_SIZE];
	nlines = ctl->line - (lines - 1) - nlines;
	// If nlines becomes greater than ctl->line, underflow occurred
	if (nlines > 0 && nlines < ctl->line) {
		ctl->line = nlines;
		while (fgets(buffer, sizeof(buffer), ctl->file)) {
			if (strcspn(buffer, "\n") < BUFFER_SIZE - 1) {
				if (!--nlines)
					break;
			}
		}
	} else {
		ctl->line = 0;
	}
	// Print screenful of content at new position
	putp(cursor_home);
	move_forwards(ctl, lines - 1);
}

void display_file(struct lotsctl *const ctl, const char *const filename) {
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

	ctl->line = 0;
	ctl->file_size = st.st_size;

	// Print screenful of content
	move_forwards(ctl, lines - 1);
}
