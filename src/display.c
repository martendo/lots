#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <string.h>
#include <err.h>
#include <term.h>

#include "ctl.h"

#define BUFFER_SIZE 1024

void clear_status(void) {
	putchar('\r');
	putp(clr_eol);
}

static void status_printf(const char *const fmt, ...) {
	clear_status();
	putp(enter_reverse_mode);
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	putp(exit_attribute_mode);
	fflush(stdout);
}

void print_status(const struct lotsctl *const ctl) {
	unsigned int percent = ctl->file_pos * 100 / ctl->file_size;
	status_printf("%s (file %u of %u) line %u (%u%%)", ctl->filename, ctl->file_index + 1, ctl->file_count, ctl->line, percent);
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

void display_file(struct lotsctl *const ctl, const int inc) {
	clear_status();
	fflush(stdout);

	do {
		ctl->filename = ctl->files[ctl->file_index];

		// Open file
		ctl->file = fopen(ctl->filename, "r");
		if (!ctl->file) {
			warn("Could not open \"%s\"", ctl->filename);
			continue;
		}

		// Get file information
		struct stat st;
		if (fstat(fileno(ctl->file), &st) < 0) {
			warn("Could not stat file \"%s\"", ctl->filename);
			continue;
		}

		// Can't display directories
		if (S_ISDIR(st.st_mode)) {
			warnx("\"%s\" is a directory", ctl->filename);
			continue;
		}

		ctl->line = 0;
		ctl->file_size = st.st_size;

		// Print screenful of content
		move_forwards(ctl, lines - 1);
		return;
	} while ((ctl->file_index += inc) < ctl->file_count);
}

void switch_file(struct lotsctl *const ctl, const int offset) {
	const unsigned int new_index = ctl->file_index + offset;
	// Underflow also makes the following condition true
	if (new_index >= ctl->file_count) {
		status_printf("No %s file", offset > 0 ? "next" : "previous");
		return;
	}
	ctl->file_index = new_index;
	// Keep walking the file list in the direction if some files need to
	// be skipped
	display_file(ctl, (offset > 0) - (offset < 0));
}
