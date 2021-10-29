#ifndef LOTS_CTL_H
#define LOTS_CTL_H

struct lotsctl {
	unsigned short lines; // Number of lines in the terminal window
	const char *filename; // Path to current file
	FILE *file; // Pointer to the current file stream
	off_t filePos; // Current position in current file
	off_t fileSize; // Size of current file
};

#endif
