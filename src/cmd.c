#include <unistd.h>
#include <string.h>
#include <term.h>

#include "ctl.h"
#include "cmd.h"

enum cmd getcmd(const struct lotsctl *const ctl) {
	char inbuf[8] = {0};
	const ssize_t inlen = read(STDERR_FILENO, &inbuf, sizeof(inbuf));
	if (inlen < 0)
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

	// Command
	for (ssize_t i = 0; i < inlen; i++) {
		switch (inbuf[i]) {
			case 'j':
			case '\n':
				return CMD_DOWN;
			case 'k':
				return CMD_UP;
			case ' ':
			case 'f':
				return CMD_DOWN_PAGE;
			case 'b':
				return CMD_UP_PAGE;
			case 'g':
				return CMD_HOME;
			case 'q':
			case 'Q':
				return CMD_QUIT;
		}
	}
	return CMD_UNKNOWN;
}
