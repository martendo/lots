#include <unistd.h>
#include <string.h>
#include <term.h>

#include "ctl.h"
#include "cmd.h"

enum cmd getcmd(const struct lotsctl *const ctl) {
	char inbuf[8] = {0};
	if (read(STDOUT_FILENO, &inbuf, sizeof(inbuf)) <= 0)
		return CMD_UNKNOWN;

	// Special key escape sequence
	if (!memcmp(inbuf, key_down, ctl->key_down_len))
		return CMD_DOWN;
	else if (!memcmp(inbuf, key_up, ctl->key_up_len))
		return CMD_UP;
	else if (!memcmp(inbuf, key_npage, ctl->key_npage_len))
		return CMD_DOWN_PAGE;
	else if (!memcmp(inbuf, key_ppage, ctl->key_ppage_len))
		return CMD_UP_PAGE;
	else if (!memcmp(inbuf, key_home, ctl->key_home_len))
		return CMD_HOME;
	else if (!memcmp(inbuf, key_end, ctl->key_end_len))
		return CMD_END;

	// Command character
	switch (inbuf[0]) {
		case 'j':
		case '\n':
		case '\r':
			return CMD_DOWN;
		case 'k':
			return CMD_UP;
		case ' ':
		case 'f':
			return CMD_DOWN_PAGE;
		case 'b':
			return CMD_UP_PAGE;
		case 'g':
		case '<':
			return CMD_HOME;
		case 'G':
		case '>':
			return CMD_END;
		case 'n':
			return CMD_NEXT_FILE;
		case 'p':
			return CMD_PREV_FILE;
		case 'h':
		case 'H':
			return CMD_HELP;
		case 'q':
		case 'Q':
			return CMD_QUIT;
		default:
			return CMD_UNKNOWN;
	}
}
