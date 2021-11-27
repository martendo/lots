#include <unistd.h>
#include <string.h>
#include <term.h>
#include <sys/signalfd.h>
#include <poll.h>
#include <stdlib.h>
#include <err.h>

#include "ctl.h"
#include "cmd.h"
#include "display.h"

void __attribute__ ((noreturn)) lots_exit(const struct lotsctl *const ctl, const int status) {
	// Clear status line
	putchar('\r');
	putp(clr_eol);

	fflush(NULL);
	if (ctl->file)
		fclose(ctl->file);
	del_curterm(cur_term);
	if (tcsetattr(STDOUT_FILENO, TCSAFLUSH, &ctl->oldattr) < 0)
		err(1, "Failed to reset terminal attributes");
	exit(status);
}

int lots_poll(struct lotsctl *const ctl) {
	// Wait for signals and/or user input
	struct pollfd pfd[2];
	pfd[0].fd = ctl->sigfd;
	pfd[0].events = POLLIN | POLLERR | POLLHUP;
	pfd[1].fd = STDOUT_FILENO;
	pfd[1].events = POLLIN;
	if (poll(pfd, sizeof(pfd), -1) < 0)
		return 1;
	// Handle input
	if (pfd[0].revents) {
		struct signalfd_siginfo siginfo;
		size_t len = read(pfd[0].fd, &siginfo, sizeof(struct signalfd_siginfo));
		if (len != sizeof(struct signalfd_siginfo))
			return 1;
		switch (siginfo.ssi_signo) {
			case SIGINT:
			case SIGQUIT:
				lots_exit(ctl, 0);
				// Not reached
			case SIGWINCH:
				// Move to position in file at top of terminal
				const int ret = seek_line(ctl, ctl->line - (lines - 1));
				// Get terminfo to update the terminal size
				setupterm(NULL, STDOUT_FILENO, NULL);
				// Redraw screen
				if (ret == 0)
					redraw(ctl);
				// If the number of lines per page wasn't set by the
				// user, update it with the new terminal height
				if (!ctl->set_lines)
					ctl->page_lines = lines - 1;
				break;
			default:
				abort();
				// Not reached
		}
	}
	// Don't try to read commands if no user input was received
	if (!pfd[1].revents)
		return 1;
	// There was user input -> read commands
	return 0;
}

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
