#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <term.h>

#include "ctl.h"

#define BUFFER_SIZE 1024

void status_printf(const char *const fmt, ...) {
	putchar('\r');
	putp(enter_reverse_mode);
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	putp(exit_attribute_mode);
	putp(clr_eol);
	fflush(stdout);
}

void print_status(const struct lotsctl *const ctl) {
	putchar('\r');
	putp(enter_reverse_mode);
	fputs(ctl->filename, stdout);
	if (ctl->file_count > 1 && ctl->file_index >= 0)
		printf(" (file %d of %d)", ctl->file_index + 1, ctl->file_count);
	else if (ctl->file_count > 0 && ctl->file_index == -1)
		printf(" (%d file%s next)", ctl->file_count, ctl->file_count - 1 ? "s" : "");
	if (ctl->file_size) {
		unsigned int percent = ctl->file_pos * 100 / ctl->file_size;
		if (percent < 100)
			printf(" line %u (%u%%)", ctl->line, percent);
		else
			fputs(" (END)", stdout);
	} else {
		printf(" line %u", ctl->line);
	}
	putp(exit_attribute_mode);
	putp(clr_eol);
	fflush(stdout);
}

void move_forwards(struct lotsctl *const ctl, unsigned long nlines) {
	putchar('\r');
	// Clear the line for e.g. TAB
	putp(clr_eol);

	// Print nlines lines from file
	char buffer[BUFFER_SIZE];
	while (fgets(buffer, sizeof(buffer), ctl->file)) {
		// Remove trailing newline, if any, so that the rest of the line
		// may be cleared before moving on to the next
		size_t len = strcspn(buffer, "\n");
		buffer[len] = '\0';
		fputs(buffer, stdout);
		// If there was a newline, clear the rest of the line, then print
		// the newline, clear the next line (for e.g. TAB) and count the
		// line
		if (len < BUFFER_SIZE - 1) {
			putp(clr_eol);
			putchar('\n');
			putp(clr_eol);
			ctl->line++;
			if (!--nlines)
				break;
		}
	}
	ctl->file_pos = ftello(ctl->file);
	// File size was previously unknown and reached EOF
	if (!ctl->file_size && nlines)
		ctl->file_size = ctl->file_pos;

	// Reprint status
	print_status(ctl);
}

void move_backwards(struct lotsctl *const ctl, unsigned long nlines) {
	// Seek to new position
	if (fseeko(ctl->file, 0, SEEK_SET) < 0) {
		status_printf("Can't seek file");
		return;
	}
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
	// Clear any leftover text after moving cursor position up (home)
	putp(clr_eos);
	fflush(stdout);
}

static void remove_file(struct lotsctl *const ctl) {
	for (int i = ctl->file_index; i < ctl->file_count - 1; i++)
		ctl->files[i] = ctl->files[i + 1];
	ctl->file_count--;
	ctl->file_index--;
}

int display_file(struct lotsctl *const ctl, const int inc) {
	do {
		const char *const filename = ctl->files[ctl->file_index];

		// Open file
		FILE *const file = fopen(filename, "r");
		if (!file) {
			status_printf("Could not open \"%s\": %s", filename, strerror(errno));
			remove_file(ctl);
			continue;
		}

		// Get file information
		struct stat st;
		if (fstat(fileno(file), &st) < 0) {
			status_printf("Could not stat file \"%s\": %s", filename, strerror(errno));
			fclose(file);
			remove_file(ctl);
			continue;
		}

		// Can't display directories
		if (S_ISDIR(st.st_mode)) {
			status_printf("\"%s\" is a directory", filename);
			fclose(file);
			remove_file(ctl);
			continue;
		}

		// File can be displayed
		ctl->filename = filename;
		ctl->file = file;
		ctl->line = 0;
		ctl->file_size = st.st_size;

		// Print screenful of content
		move_forwards(ctl, lines - 1);
		return 1;
	} while ((ctl->file_index += inc) < ctl->file_count);
	return 0;
}

void switch_file(struct lotsctl *const ctl, const int offset) {
	const int new_index = ctl->file_index + offset;
	if (new_index >= ctl->file_count || new_index < 0) {
		status_printf("No %s file", offset > 0 ? "next" : "previous");
		return;
	}
	ctl->file_index = new_index;

	FILE *const prev_file = ctl->file;
	// Keep walking the file list in the direction of offset if some
	// files need to be skipped
	if (display_file(ctl, (offset > 0) - (offset < 0)))
		// File has been successfully switched -> close previous file
		fclose(prev_file);
}
