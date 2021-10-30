#ifndef LOTS_CTL_H
#define LOTS_CTL_H

#include <stdio.h>

struct lotsctl {
	const char *filename; // Path to current file
	FILE *file; // Pointer to the current file stream
	off_t file_pos; // Current position in current file
	unsigned int line; // Current line number in current file
	off_t file_size; // Size of current file

	size_t key_up_len; // strlen(key_up)
	size_t key_down_len; // strlen(key_down)
	size_t key_ppage_len; // strlen(key_ppage)
	size_t key_npage_len; // strlen(key_npage)
	size_t key_home_len; // strlen(key_home)
};

#endif
