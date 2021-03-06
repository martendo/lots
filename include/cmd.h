#ifndef LOTS_CMD_H
#define LOTS_CMD_H

#include "ctl.h"

enum cmd {
	CMD_UNKNOWN,
	CMD_UP,
	CMD_DOWN,
	CMD_UP_PAGE,
	CMD_DOWN_PAGE,
	CMD_HOME,
	CMD_END,
	CMD_NEXT_FILE,
	CMD_PREV_FILE,
	CMD_HELP,
	CMD_QUIT
};

void __attribute__ ((noreturn)) lots_exit(const struct lotsctl *const, const int);

int lots_poll(struct lotsctl *const);

enum cmd getcmd(const struct lotsctl *const);

#endif
