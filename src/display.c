#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <term.h>

#include "ctl.h"

#define BUFFER_SIZE 1024

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
			printf(" line %lu (%u%%)", ctl->line, percent);
		else
			fputs(" (END)", stdout);
	} else {
		printf(" line %lu", ctl->line);
	}
	putp(exit_attribute_mode);
	putp(clr_eol);
	fflush(stdout);
}

static inline void wait_key(void) {
	// Wait for any input
	char c;
	read(STDOUT_FILENO, &c, 1);
}

void status_printf(const struct lotsctl *const ctl, const char *const fmt, ...) {
	va_list args;
	va_start(args, fmt);
	if (!ctl->file) {
		vprintf(fmt, args);
		putchar('\n');
		return;
	}
	putchar('\r');
	putp(enter_reverse_mode);
	vprintf(fmt, args);
	va_end(args);
	fputs(" (press a key)", stdout);
	putp(exit_attribute_mode);
	putp(clr_eol);
	fflush(stdout);

	wait_key();
	print_status(ctl);
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

void redraw(struct lotsctl *const ctl) {
	// Print screenful of content starting at home
	putp(cursor_home);
	move_forwards(ctl, lines - 1);
	// Clear any leftover text after moving cursor position up (home)
	putp(clr_eos);
	fflush(stdout);
}

// This function is only used to seek to a line that appears before the
// current file position, seeking backwards
int seek_line(struct lotsctl *const ctl, unsigned long target) {
	// Seek to beginning of file
	if (fseeko(ctl->file, 0, SEEK_SET) == -1)
		return -1;
	ctl->line = target;
	if (target == 0)
		return 0;
	char buffer[BUFFER_SIZE];
	while (fgets(buffer, sizeof(buffer), ctl->file)) {
		if (strcspn(buffer, "\n") < BUFFER_SIZE - 1) {
			if (!--target)
				break;
		}
	}
	return 0;
}

void move_backwards(struct lotsctl *const ctl, const unsigned long nlines) {
	// Find line before first line to print after moving backwards
	unsigned long target = ctl->line - (lines - 1) - nlines;
	// If target line becomes greater than or equal to current line,
	// underflow occurred
	if (target >= ctl->line)
		target = 0;
	if (seek_line(ctl, target) != 0) {
		status_printf(ctl, "Can't seek file");
		return;
	}
	redraw(ctl);
}

static void remove_file(struct lotsctl *const ctl) {
	// Shift all file pointers following the current one down one spot
	for (int i = ctl->file_index; i < ctl->file_count - 1; i++)
		ctl->files[i] = ctl->files[i + 1];
	// Adjust file count and current index
	ctl->file_count--;
	ctl->file_index--;
	// Update displayed file index if in file-viewing mode
	if (ctl->file)
		print_status(ctl);
}

int display_file(struct lotsctl *const ctl, const int inc) {
	// Store the old file count to check if some errors were printed
	// (files removed from the file array due to being undisplayable)
	const int old_file_count = ctl->file_count;

	do {
		const char *const filename = ctl->files[ctl->file_index];

		// Open file
		FILE *const file = fopen(filename, "r");
		if (!file) {
			status_printf(ctl, "Could not open \"%s\": %s", filename, strerror(errno));
			remove_file(ctl);
			continue;
		}

		// Get file information
		struct stat st;
		if (fstat(fileno(file), &st) == -1) {
			status_printf(ctl, "Could not stat file \"%s\": %s", filename, strerror(errno));
			fclose(file);
			remove_file(ctl);
			continue;
		}

		// Can't display directories
		if (S_ISDIR(st.st_mode)) {
			status_printf(ctl, "\"%s\" is a directory", filename);
			fclose(file);
			remove_file(ctl);
			continue;
		}

		// File can be displayed, but first wait for key if not yet
		// entered "interactive mode" and errors were printed
		if (!ctl->file && old_file_count != ctl->file_count) {
			puts("\nPress any key to continue");
			wait_key();
		}

		ctl->filename = filename;
		ctl->file = file;
		ctl->line = 0;
		ctl->file_size = st.st_size;

		redraw(ctl);
		return 0;
	} while ((ctl->file_index += inc) < ctl->file_count);
	return -1;
}

void switch_file(struct lotsctl *const ctl, const int offset) {
	const int new_index = ctl->file_index + offset;
	// New index is out of range of the file array
	if (new_index >= ctl->file_count || new_index < 0) {
		status_printf(ctl, "No %s file", offset > 0 ? "next" : "previous");
		return;
	}
	ctl->file_index = new_index;

	FILE *const prev_file = ctl->file;
	// Keep walking the file list in the direction of offset if some
	// files need to be skipped
	if (display_file(ctl, (offset > 0) - (offset < 0)) == 0)
		// File has been successfully switched -> close previous file
		fclose(prev_file);
}
