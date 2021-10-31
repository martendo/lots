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
	CMD_HELP,
	CMD_QUIT
};

enum cmd getcmd(const struct lotsctl *const);

#endif
