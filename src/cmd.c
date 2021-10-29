#include <unistd.h>

#include "cmd.h"

#define ESC '\033'

enum cmd getcmd(void) {
	char inbuf[8] = {0};
	ssize_t inlen, i;
	inlen = read(STDERR_FILENO, &inbuf, sizeof(inbuf));
	if (inlen < 0)
		return CMD_UNKNOWN;
	// ANSI escape sequence
	if (inbuf[0] == ESC && inlen >= 3) {
		if (inbuf[1] != '[')
			return CMD_UNKNOWN;
		switch (inbuf[2]) {
			// Up arrow: ^[[A
			case 'A':
				return CMD_UP;
			// Down arrow: ^[[B
			case 'B':
				return CMD_DOWN;
		}
		return CMD_UNKNOWN;
	}
	// Command
	for (i = 0; i < inlen; i++) {
		switch (inbuf[i]) {
			case 'q':
			case 'Q':
				return CMD_QUIT;
		}
	}
	return CMD_UNKNOWN;
}
